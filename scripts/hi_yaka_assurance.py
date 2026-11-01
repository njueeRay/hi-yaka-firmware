#!/usr/bin/env python3
"""HiYaKa build assurance gates and provenance manifest generator."""

from __future__ import annotations

import argparse
import csv
import hashlib
import json
import re
from datetime import datetime, timezone
from pathlib import Path
from typing import Any


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_POLICY = ROOT / "config" / "hi-yaka-assurance.json"


def parse_number(value: str) -> int:
    text = value.strip().lower()
    if text.endswith("k"):
        return int(text[:-1], 0) * 1024
    if text.endswith("m"):
        return int(text[:-1], 0) * 1024 * 1024
    return int(text, 0)


def load_policy(path: Path) -> dict[str, Any]:
    policy = json.loads(path.read_text(encoding="utf-8"))
    if policy.get("schema") != 1 or policy.get("board") != "hi-yaka":
        raise ValueError("unsupported HiYaKa assurance policy")
    return policy


def read_partition_table(path: Path) -> list[dict[str, Any]]:
    rows: list[dict[str, Any]] = []
    cursor = 0
    with path.open(encoding="utf-8", newline="") as source:
        for raw in csv.reader(source):
            if not raw or raw[0].strip().startswith("#"):
                continue
            if len(raw) < 5:
                raise ValueError(f"invalid partition row: {raw}")
            name, kind, subtype, offset_text, size_text = (item.strip() for item in raw[:5])
            offset = parse_number(offset_text) if offset_text else cursor
            size = parse_number(size_text)
            if offset < cursor:
                raise ValueError(f"partition {name} overlaps its predecessor")
            rows.append({"name": name, "type": kind, "subtype": subtype,
                         "offset": offset, "size": size, "end": offset + size})
            cursor = offset + size
    return rows


def validate_partition(policy: dict[str, Any], path: Path) -> dict[str, Any]:
    rows = read_partition_table(path)
    by_name = {row["name"]: row for row in rows}
    required = {"nvs", "otadata", "phy_init", "ota_0", "ota_1", "assets"}
    missing = required - by_name.keys()
    if missing:
        raise ValueError(f"missing partitions: {', '.join(sorted(missing))}")
    slot_size = int(policy["ota_slot_bytes"])
    if by_name["ota_0"]["size"] != slot_size or by_name["ota_1"]["size"] != slot_size:
        raise ValueError("OTA slots must match the frozen symmetric slot size")
    if by_name["ota_0"]["end"] != by_name["ota_1"]["offset"]:
        raise ValueError("OTA slots must be contiguous")
    if by_name["ota_1"]["end"] != by_name["assets"]["offset"]:
        raise ValueError("assets must start immediately after ota_1")
    if rows[-1]["end"] > int(policy["flash_bytes"]):
        raise ValueError("partition table exceeds physical flash capacity")
    return {"partitions": rows, "used_bytes": rows[-1]["end"],
            "unallocated_bytes": int(policy["flash_bytes"]) - rows[-1]["end"]}


def validate_source_budgets(policy: dict[str, Any], root: Path = ROOT) -> dict[str, int]:
    types = (root / "main" / "yaka" / "product_types.h").read_text(encoding="utf-8")
    capacities = {
        "product_event_queue_capacity": "kProductEventQueueCapacity",
        "audio_descriptor_queue_capacity": "kAudioBlockQueueCapacity",
        "storage_write_queue_capacity": "kStorageWriteQueueCapacity",
    }
    actual: dict[str, int] = {}
    for key, symbol in capacities.items():
        match = re.search(rf"{symbol}\s*=\s*(\d+)\s*;", types)
        if not match:
            raise ValueError(f"unable to find {symbol}")
        actual[key] = int(match.group(1))
        if actual[key] > int(policy[key]):
            raise ValueError(f"{symbol}={actual[key]} exceeds policy {policy[key]}")
    source_text = "\n".join(
        path.read_text(encoding="utf-8", errors="replace")
        for path in (root / "main" / "yaka").glob("*.cc")
    )
    product_tasks = len(re.findall(r"\bxTaskCreate(?:PinnedToCore)?\s*\(", source_text))
    actual["additional_product_tasks"] = product_tasks
    if product_tasks > int(policy["additional_product_tasks"]):
        raise ValueError("HiYaKa product layer created tasks outside its frozen budget")
    return actual


def parse_idf_size_output(text: str) -> dict[str, Any]:
    """Extract the JSON report from idf.py's mixed action-log/stdout stream."""
    decoder = json.JSONDecoder()
    for match in re.finditer(r"\{", text):
        try:
            candidate, _ = decoder.raw_decode(text, match.start())
        except json.JSONDecodeError:
            continue
        if isinstance(candidate, dict) and isinstance(candidate.get("layout"), list):
            return candidate
    raise ValueError("unable to find an ESP-IDF size JSON object in command output")


def validate_size_report(policy: dict[str, Any], report_path: Path) -> dict[str, Any]:
    report = parse_idf_size_output(report_path.read_text(encoding="utf-8-sig"))
    layout = report.get("layout")
    if not isinstance(layout, list) or not layout:
        raise ValueError("ESP-IDF size report has no layout array")
    matched: list[dict[str, Any]] = []
    for item in layout:
        name = str(item.get("name", ""))
        total = int(item.get("total", 0))
        used = int(item.get("used", 0))
        if "ram" not in name.lower() or total <= 0:
            continue
        percent = used * 100.0 / total
        if percent > float(policy["internal_ram_max_percent"]):
            raise ValueError(f"{name} usage {percent:.2f}% exceeds policy")
        matched.append({"name": name, "total": total, "used": used,
                        "used_percent": round(percent, 2)})
    if not matched:
        raise ValueError("ESP-IDF size report contains no internal RAM region")
    return {"total_size": report.get("total_size"), "internal_ram": matched}


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as source:
        for block in iter(lambda: source.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def resource_check(args: argparse.Namespace) -> dict[str, Any]:
    policy = load_policy(args.policy)
    partition = validate_partition(policy, args.partition)
    source = validate_source_budgets(policy)
    app_size = args.app_bin.stat().st_size
    if app_size > int(policy["app_image_max_bytes"]):
        raise ValueError(f"app image {app_size} exceeds {policy['app_image_max_bytes']} bytes")
    size = validate_size_report(policy, args.size_json)
    return {"schema": 1, "board": "hi-yaka", "app_image_bytes": app_size,
            "app_image_limit_bytes": int(policy["app_image_max_bytes"]),
            "partition": partition, "product_layer": source, "idf_size": size}


def provenance(args: argparse.Namespace) -> dict[str, Any]:
    policy = load_policy(args.policy)
    partition = validate_partition(policy, args.partition)
    artifacts = {}
    for name, path in (("app", args.app_bin), ("merged", args.merged_bin),
                       ("sdkconfig", args.sdkconfig), ("partition_table", args.partition),
                       ("resource_report", args.resource_report)):
        artifacts[name] = {"path": path.name, "bytes": path.stat().st_size,
                           "sha256": sha256_file(path)}
    return {
        "schema": 1,
        "product": "ChatChat HiYaKa",
        "board": policy["board"],
        "generated_at": datetime.now(timezone.utc).isoformat(),
        "source_sha": args.source_sha,
        "upstream": policy["upstream"],
        "esp_idf": args.idf_version,
        "ci": {"repository": args.ci_repository, "run_id": args.ci_run_id},
        "partition": partition,
        "artifacts": artifacts,
    }


def write_json(data: dict[str, Any], output: Path | None) -> None:
    encoded = json.dumps(data, ensure_ascii=False, indent=2, sort_keys=True) + "\n"
    if output:
        output.parent.mkdir(parents=True, exist_ok=True)
        output.write_text(encoded, encoding="utf-8")
    else:
        print(encoded, end="")


def parser() -> argparse.ArgumentParser:
    result = argparse.ArgumentParser(description=__doc__)
    result.add_argument("--policy", type=Path, default=DEFAULT_POLICY)
    commands = result.add_subparsers(dest="command", required=True)
    partition = commands.add_parser("partition")
    partition.add_argument("--partition", type=Path, required=True)
    partition.add_argument("--output", type=Path)
    resources = commands.add_parser("resources")
    resources.add_argument("--partition", type=Path, required=True)
    resources.add_argument("--app-bin", type=Path, required=True)
    resources.add_argument("--size-json", type=Path, required=True)
    resources.add_argument("--output", type=Path)
    manifest = commands.add_parser("provenance")
    manifest.add_argument("--partition", type=Path, required=True)
    manifest.add_argument("--app-bin", type=Path, required=True)
    manifest.add_argument("--merged-bin", type=Path, required=True)
    manifest.add_argument("--sdkconfig", type=Path, required=True)
    manifest.add_argument("--resource-report", type=Path, required=True)
    manifest.add_argument("--source-sha", required=True)
    manifest.add_argument("--idf-version", required=True)
    manifest.add_argument("--ci-repository", default="local")
    manifest.add_argument("--ci-run-id", default="local")
    manifest.add_argument("--output", type=Path)
    return result


def main() -> int:
    args = parser().parse_args()
    if args.command == "partition":
        data = validate_partition(load_policy(args.policy), args.partition)
    elif args.command == "resources":
        data = resource_check(args)
    else:
        data = provenance(args)
    write_json(data, args.output)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
