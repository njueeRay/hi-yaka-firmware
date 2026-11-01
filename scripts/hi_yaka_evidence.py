#!/usr/bin/env python3
"""Create and validate traceable HiYaKa first-board evidence packages."""

from __future__ import annotations

import argparse
import csv
import json
import re
from datetime import datetime, timezone
from pathlib import Path
from typing import Any


SCHEMA_VERSION = 1
SHA_PATTERN = re.compile(r"^[0-9a-f]{40}$")
SHA256_PATTERN = re.compile(r"^[0-9a-f]{64}$")
DIGEST_PATTERN = re.compile(r"^sha256:[0-9a-f]{64}$")
IDENTIFIER_PATTERN = re.compile(r"^[A-Za-z0-9][A-Za-z0-9._-]{0,63}$")
RESULTS = {"NOT_RUN", "PASS", "FAIL", "BLOCKED"}

MODULES: dict[str, dict[str, Any]] = {
    "power": {
        "title": "限流上电与关键电源轨",
        "target": "L1",
        "milestones": ["M1", "M7"],
        "steps": ["记录获批的限流值和静态电阻", "分级上电并记录电流", "测量关键电源轨与复位行为"],
        "expected": "无异常发热、异味或持续限流；电源轨与复位行为符合硬件事实源。",
    },
    "flash": {
        "title": "下载链、Flash 与启动日志",
        "target": "L1",
        "milestones": ["M1"],
        "steps": ["记录串口枚举", "读取芯片和 Flash 信息", "烧录已追溯候选产物", "保存冷启动与复位日志"],
        "expected": "芯片、Flash 容量、分区和固件 SHA 可追溯，重复冷启动无重启循环。",
    },
    "i2c": {
        "title": "共享 I²C 总线与固定地址",
        "target": "L1",
        "milestones": ["M1", "M2", "M3", "M6"],
        "steps": ["以 100 kHz 初始化 GPIO10/11", "保存五个固定地址的探测结果", "记录超时和恢复行为"],
        "expected": "装配器件响应可解释；缺失器件进入可诊断降级态而非重启循环。",
    },
    "pca9557": {
        "title": "PCA9557 安全态与回读",
        "target": "L1",
        "milestones": ["M1", "M2"],
        "steps": ["先写 Output=0x40", "再写 Configuration=0x38", "回读两个寄存器", "确认背光和马达默认关闭"],
        "expected": "写入顺序正确，回读一致；背光、马达和 LCD 片选保持安全态。",
    },
    "display": {
        "title": "ST7789 显示链",
        "target": "L2",
        "milestones": ["M2"],
        "steps": ["验证复位与首帧后开背光", "显示方向/色阶/边界测试图", "记录刷新和低电量亮度"],
        "expected": "200×320 方向、颜色、边界正确；无明显撕裂、花屏或异常背光时序。",
    },
    "input": {
        "title": "DPAD、KEYA 与 KEYB",
        "target": "L2",
        "milestones": ["M2"],
        "steps": ["采集 DPAD 空闲和各方向 ADC 分布", "验证独立键极性和去抖", "验证长按、连按和异常组合"],
        "expected": "输入映射稳定、无明显误触；原始 ADC 数据足以冻结窗口和迟滞。",
    },
    "audio": {
        "title": "双麦、播放参考与扬声器",
        "target": "L2",
        "milestones": ["M3", "M5"],
        "steps": ["保存 codec 寄存器和 I²S 参数", "采集原始多通道 PCM", "验证播放、录音和 PA 开关时序"],
        "expected": "通道映射可解释；采播稳定，无不可接受爆音、削波或持续丢帧。",
    },
    "storage": {
        "title": "TF 挂载、录音与恢复",
        "target": "L2",
        "milestones": ["M4", "M7"],
        "steps": ["覆盖无卡/慢卡/满卡/坏卡", "执行长录音", "在定义检查点执行硬切电并恢复扫描"],
        "expected": "写卡不阻塞实时音频；文件可恢复或产生明确、可追溯的错误。",
    },
    "nfc": {
        "title": "PN532 只读业务链",
        "target": "L2",
        "milestones": ["M6"],
        "steps": ["确认 0x24 与 I²C 模式", "验证多类标签", "记录轮询超时、读距和金属环境"],
        "expected": "只读识别可重复，超时可退出，不阻塞主交互。",
    },
    "imu": {
        "title": "QMI8658A 采样与轴向",
        "target": "L2",
        "milestones": ["M6"],
        "steps": ["确认芯片 ID", "记录静止零偏和六面数据", "验证装配轴向、量程和采样率"],
        "expected": "轴向与结构定义一致，零偏和噪声可解释，轮询有界。",
    },
    "feedback": {
        "title": "RGB 与振动反馈",
        "target": "L2",
        "milestones": ["M2", "M6"],
        "steps": ["验证单灯和五灯链默认关闭", "限制亮度执行颜色测试", "以短脉冲验证 ERM 马达并监测电源"],
        "expected": "反馈映射正确，不引发掉压或异常复位；马达动作受最大持续时间约束。",
    },
    "battery": {
        "title": "电池 ADC、充电与低电行为",
        "target": "L2",
        "milestones": ["M2", "M7"],
        "steps": ["用外部仪表同步记录电池和 ADC", "建立多点标定", "验证低电和充电期间显示/行为"],
        "expected": "电量估算误差可量化；不把电压单点误报为充满状态。",
    },
    "network": {
        "title": "配网、弱网与对话恢复",
        "target": "L2",
        "milestones": ["M5", "M7"],
        "steps": ["验证首次配网和凭据恢复", "执行断网、弱网和服务超时", "验证录音/对话互斥与恢复"],
        "expected": "网络失败可见、可恢复，不泄漏资源或破坏本地录音。",
    },
    "ota": {
        "title": "OTA 身份校验、首启确认与回滚",
        "target": "L3",
        "milestones": ["M7"],
        "steps": ["验证候选包身份和兼容门禁", "执行正常升级", "注入坏包/断网/未确认首启", "保存回滚和数据完整性证据"],
        "expected": "错误候选被拒绝；未确认新固件可回滚；已提交录音和配置保持一致。",
    },
    "long_run": {
        "title": "并发、功耗、温升与长稳",
        "target": "L3",
        "milestones": ["M7"],
        "steps": ["定义负载和时长", "并发运行 Wi-Fi/显示/音频/TF", "记录堆、队列、功耗、温升和复位"],
        "expected": "在批准的时间和环境范围内无不可恢复错误，资源趋势和热行为可解释。",
    },
}


def write_json(path: Path, data: dict[str, Any]) -> None:
    path.write_text(json.dumps(data, ensure_ascii=False, indent=2, sort_keys=True) + "\n", encoding="utf-8")


def read_json(path: Path) -> dict[str, Any]:
    data = json.loads(path.read_text(encoding="utf-8"))
    if not isinstance(data, dict):
        raise ValueError(f"{path} must contain a JSON object")
    return data


def checked_identifier(name: str, value: str) -> str:
    if not IDENTIFIER_PATTERN.fullmatch(value):
        raise ValueError(f"invalid {name}: use 1-64 letters, digits, dot, underscore or hyphen")
    return value


def checked_hash(name: str, value: str, pattern: re.Pattern[str]) -> str:
    normalized = value.lower()
    if not pattern.fullmatch(normalized):
        raise ValueError(f"invalid {name}")
    return normalized


def parse_timestamp(value: str | None) -> datetime:
    if value is None:
        return datetime.now(timezone.utc).replace(microsecond=0)
    normalized = value[:-1] + "+00:00" if value.endswith("Z") else value
    parsed = datetime.fromisoformat(normalized)
    if parsed.tzinfo is None:
        raise ValueError("timestamp must include a timezone")
    return parsed.astimezone(timezone.utc).replace(microsecond=0)


def resolve_evidence(run_dir: Path, relative: str, require_content: bool = True) -> Path:
    candidate = Path(relative)
    if candidate.is_absolute() or ".." in candidate.parts:
        raise ValueError(f"evidence path must stay inside the run directory: {relative}")
    resolved_run = run_dir.resolve()
    resolved = (run_dir / candidate).resolve()
    if resolved != resolved_run and resolved_run not in resolved.parents:
        raise ValueError(f"evidence path escapes the run directory: {relative}")
    if not resolved.is_file():
        raise ValueError(f"evidence file does not exist: {relative}")
    if require_content and resolved.stat().st_size == 0:
        raise ValueError(f"evidence file is empty: {relative}")
    return resolved


def module_template(key: str, definition: dict[str, Any]) -> dict[str, Any]:
    return {
        "schema": SCHEMA_VERSION,
        "module": key,
        "title": definition["title"],
        "target_evidence_level": definition["target"],
        "milestones": definition["milestones"],
        "steps": definition["steps"],
        "expected": definition["expected"],
        "actual": "",
        "result": "NOT_RUN",
        "raw_evidence": [],
        "defect_ids": [],
        "tester": "",
        "tested_at": "",
    }


def initialise(args: argparse.Namespace) -> Path:
    sample_id = checked_identifier("sample ID", args.sample_id)
    hardware_revision = checked_identifier("hardware revision", args.hardware_revision)
    source_sha = checked_hash("firmware SHA", args.firmware_sha, SHA_PATTERN)
    upstream_sha = checked_hash("upstream SHA", args.upstream_sha, SHA_PATTERN)
    artifact_sha = checked_hash("firmware artifact SHA-256", args.firmware_artifact_sha256, SHA256_PATTERN)
    container_digest = checked_hash("container digest", args.container_digest, DIGEST_PATTERN)
    created = parse_timestamp(args.timestamp)
    run_id = f"{created.strftime('%Y%m%dT%H%M%SZ')}_{sample_id}_{source_sha[:12]}"
    run_dir = args.output_root / run_id
    if run_dir.exists():
        raise ValueError(f"run already exists: {run_dir}")
    for path in (run_dir / "modules", run_dir / "logs", run_dir / "measurements", run_dir / "photos"):
        path.mkdir(parents=True, exist_ok=False)
    manifest = {
        "schema": SCHEMA_VERSION,
        "product": "ChatChat HiYaKa",
        "board": "hi-yaka",
        "run_id": run_id,
        "state": "OPEN",
        "evidence_level": "UNVERIFIED",
        "created_at": created.isoformat(),
        "operator": args.operator,
        "environment": args.environment,
        "sample_id": sample_id,
        "hardware_revision": hardware_revision,
        "firmware": {
            "source_sha": source_sha,
            "upstream_sha": upstream_sha,
            "artifact_name": args.firmware_artifact_name,
            "artifact_sha256": artifact_sha,
            "configuration": args.configuration,
            "esp_idf": args.idf_version,
            "container_digest": container_digest,
        },
    }
    write_json(run_dir / "run.json", manifest)
    for key, definition in MODULES.items():
        write_json(run_dir / "modules" / f"{key}.json", module_template(key, definition))
    (run_dir / "logs" / "serial.log").write_text("", encoding="utf-8")
    with (run_dir / "measurements" / "power.csv").open("w", encoding="utf-8", newline="") as output:
        csv.writer(output).writerow(["timestamp", "stage", "supply_v", "current_limit_a", "current_a", "rail", "rail_v", "note"])
    (run_dir / "photos" / "README.md").write_text(
        "# 照片与视频证据\n\n文件名使用 `模块_时间_说明`；原始媒体只追加，不覆盖。\n",
        encoding="utf-8",
    )
    (run_dir / "defects.md").write_text("# 缺陷索引\n\n| ID | 模块 | 现象 | 状态 | 复测条件 |\n|---|---|---|---|---|\n", encoding="utf-8")
    (run_dir / "README.md").write_text(
        f"# HiYaKa 首板证据包 {run_id}\n\n"
        "本目录初始状态为 `UNVERIFIED`。`NOT_RUN` 不是通过；只有附带非空原始证据的记录才允许标记 `PASS`。\n\n"
        f"- 样机：`{sample_id}`\n- 硬件 revision：`{hardware_revision}`\n- 固件 SHA：`{source_sha}`\n",
        encoding="utf-8",
    )
    return run_dir


def validate_module(run_dir: Path, key: str, data: dict[str, Any]) -> list[str]:
    errors: list[str] = []
    prefix = f"modules/{key}.json"
    if data.get("schema") != SCHEMA_VERSION or data.get("module") != key:
        errors.append(f"{prefix}: schema or module identity mismatch")
    result = data.get("result")
    if result not in RESULTS:
        errors.append(f"{prefix}: invalid result {result!r}")
        return errors
    actual = str(data.get("actual", "")).strip()
    evidence = data.get("raw_evidence")
    defects = data.get("defect_ids")
    if not isinstance(evidence, list) or not all(isinstance(item, str) for item in evidence):
        errors.append(f"{prefix}: raw_evidence must be a string list")
        evidence = []
    if not isinstance(defects, list) or not all(isinstance(item, str) and item.strip() for item in defects):
        errors.append(f"{prefix}: defect_ids must be a non-empty string list")
        defects = []
    for defect in defects:
        if not IDENTIFIER_PATTERN.fullmatch(defect):
            errors.append(f"{prefix}: invalid defect or blocker ID {defect!r}")
    if result in {"PASS", "FAIL", "BLOCKED"}:
        if not actual:
            errors.append(f"{prefix}: {result} requires an actual observation")
        if not str(data.get("tester", "")).strip() or not str(data.get("tested_at", "")).strip():
            errors.append(f"{prefix}: {result} requires tester and tested_at")
        else:
            try:
                parse_timestamp(str(data["tested_at"]))
            except ValueError as error:
                errors.append(f"{prefix}: invalid tested_at: {error}")
    if result in {"PASS", "FAIL"} and not evidence:
        errors.append(f"{prefix}: {result} requires raw evidence")
    if result in {"FAIL", "BLOCKED"} and not defects:
        errors.append(f"{prefix}: {result} requires at least one defect or blocker ID")
    for relative in evidence:
        try:
            resolve_evidence(run_dir, relative)
        except ValueError as error:
            errors.append(f"{prefix}: {error}")
    return errors


def validate_run(run_dir: Path, required_modules: list[str] | None = None) -> dict[str, Any]:
    errors: list[str] = []
    manifest_path = run_dir / "run.json"
    if not manifest_path.is_file():
        return {"valid": False, "errors": ["run.json is missing"], "counts": {}}
    manifest = read_json(manifest_path)
    if manifest.get("schema") != SCHEMA_VERSION or manifest.get("product") != "ChatChat HiYaKa" or manifest.get("board") != "hi-yaka":
        errors.append("run.json: schema or HiYaKa identity mismatch")
    if manifest.get("state") != "OPEN" or manifest.get("evidence_level") != "UNVERIFIED":
        errors.append("run.json: initial state must remain OPEN/UNVERIFIED; release conclusions are external decisions")
    for key in ("run_id", "operator", "environment", "sample_id", "hardware_revision", "created_at"):
        if not str(manifest.get(key, "")).strip():
            errors.append(f"run.json: missing {key}")
    try:
        parse_timestamp(str(manifest.get("created_at", "")))
    except ValueError as error:
        errors.append(f"run.json: invalid created_at: {error}")
    firmware = manifest.get("firmware", {})
    if not isinstance(firmware, dict):
        errors.append("run.json: firmware must be an object")
        firmware = {}
    for key, pattern in (("source_sha", SHA_PATTERN), ("upstream_sha", SHA_PATTERN), ("artifact_sha256", SHA256_PATTERN)):
        if not pattern.fullmatch(str(firmware.get(key, ""))):
            errors.append(f"run.json: invalid firmware.{key}")
    if not DIGEST_PATTERN.fullmatch(str(firmware.get("container_digest", ""))):
        errors.append("run.json: invalid firmware.container_digest")
    for key in ("artifact_name", "configuration", "esp_idf"):
        if not str(firmware.get(key, "")).strip():
            errors.append(f"run.json: missing firmware.{key}")
    counts = {result: 0 for result in sorted(RESULTS)}
    for key in MODULES:
        path = run_dir / "modules" / f"{key}.json"
        if not path.is_file():
            errors.append(f"modules/{key}.json is missing")
            continue
        data = read_json(path)
        result = str(data.get("result", ""))
        if result in counts:
            counts[result] += 1
        errors.extend(validate_module(run_dir, key, data))
    for key in required_modules or []:
        if key not in MODULES:
            errors.append(f"unknown required module: {key}")
            continue
        path = run_dir / "modules" / f"{key}.json"
        if path.is_file() and read_json(path).get("result") == "NOT_RUN":
            errors.append(f"modules/{key}.json: required module is NOT_RUN")
    return {"valid": not errors, "errors": errors, "counts": counts, "run_id": manifest.get("run_id")}


def record_result(args: argparse.Namespace) -> Path:
    run_dir = args.run
    if args.module not in MODULES:
        raise ValueError(f"unknown module: {args.module}")
    path = run_dir / "modules" / f"{args.module}.json"
    if not path.is_file():
        raise ValueError(f"module record is missing: {path}")
    data = read_json(path)
    if data.get("result") != "NOT_RUN":
        raise ValueError(f"module {args.module} is already recorded; create a new run for retest")
    tested_at = parse_timestamp(args.tested_at).isoformat()
    evidence = []
    for relative in args.raw_evidence:
        resolved = resolve_evidence(run_dir, relative)
        evidence.append(resolved.relative_to(run_dir.resolve()).as_posix())
    data.update({
        "result": args.result,
        "actual": args.actual.strip(),
        "raw_evidence": evidence,
        "defect_ids": args.defect_id,
        "tester": args.tester,
        "tested_at": tested_at,
    })
    errors = validate_module(run_dir, args.module, data)
    if errors:
        raise ValueError("; ".join(errors))
    write_json(path, data)
    return path


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    commands = parser.add_subparsers(dest="command", required=True)
    init = commands.add_parser("init", help="create an immutable-identity evidence package")
    init.add_argument("--output-root", type=Path, required=True)
    init.add_argument("--sample-id", required=True)
    init.add_argument("--hardware-revision", required=True)
    init.add_argument("--firmware-sha", required=True)
    init.add_argument("--upstream-sha", required=True)
    init.add_argument("--firmware-artifact-name", required=True)
    init.add_argument("--firmware-artifact-sha256", required=True)
    init.add_argument("--configuration", default="hi-yaka")
    init.add_argument("--idf-version", required=True)
    init.add_argument("--container-digest", required=True)
    init.add_argument("--operator", required=True)
    init.add_argument("--environment", required=True)
    init.add_argument("--timestamp", help="ISO-8601 timestamp with timezone; defaults to current UTC")
    record = commands.add_parser("record", help="write one validated module result")
    record.add_argument("--run", type=Path, required=True)
    record.add_argument("--module", choices=sorted(MODULES), required=True)
    record.add_argument("--result", choices=sorted(RESULTS - {"NOT_RUN"}), required=True)
    record.add_argument("--actual", required=True)
    record.add_argument("--raw-evidence", action="append", default=[])
    record.add_argument("--defect-id", action="append", default=[])
    record.add_argument("--tester", required=True)
    record.add_argument("--tested-at", help="ISO-8601 timestamp with timezone; defaults to current UTC")
    validate = commands.add_parser("validate", help="validate structure, evidence files and requested gates")
    validate.add_argument("--run", type=Path, required=True)
    validate.add_argument("--require-module", action="append", default=[])
    return parser


def main() -> int:
    parser = build_parser()
    args = parser.parse_args()
    try:
        if args.command == "init":
            print(initialise(args))
            return 0
        if args.command == "record":
            print(record_result(args))
            return 0
        result = validate_run(args.run, args.require_module)
        print(json.dumps(result, ensure_ascii=False, indent=2, sort_keys=True))
        return 0 if result["valid"] else 2
    except (OSError, ValueError, json.JSONDecodeError) as error:
        parser.error(str(error))
    return 2


if __name__ == "__main__":
    raise SystemExit(main())
