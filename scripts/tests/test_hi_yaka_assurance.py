import json
import tempfile
import unittest
from pathlib import Path

from scripts import hi_yaka_assurance as assurance


ROOT = Path(__file__).resolve().parents[2]
POLICY = ROOT / "config" / "hi-yaka-assurance.json"
PARTITION = ROOT / "partitions" / "hi-yaka-16m.csv"


class HiYaKaAssuranceTest(unittest.TestCase):
    def test_license_boundary_is_explicit(self):
        notice = (ROOT / "LICENSE").read_text(encoding="utf-8")
        scope = (ROOT / "LICENSE_SCOPE.md").read_text(encoding="utf-8")
        polyform = ROOT / "LICENSES" / "PolyForm-Noncommercial-1.0.0.md"
        upstream_mit = ROOT / "LICENSES" / "MIT-xiaozhi-esp32.txt"
        self.assertIn("Commercial use is not granted", notice)
        self.assertIn("Required Notice: Copyright 2026 ChatChatTech.", notice)
        self.assertIn("main/boards/chatchat/hi-yaka/", scope)
        self.assertIn("main/yaka/", scope)
        self.assertIn("不覆盖小智", scope)
        self.assertTrue(polyform.is_file())
        self.assertTrue(upstream_mit.is_file())
        self.assertIn("PolyForm Noncommercial License 1.0.0",
                      polyform.read_text(encoding="utf-8"))
        self.assertIn("MIT License", upstream_mit.read_text(encoding="utf-8"))

    def test_github_jobs_pin_ubuntu_24_04(self):
        workflows = sorted((ROOT / ".github" / "workflows").glob("*.yml"))
        self.assertTrue(workflows)
        job_count = 0
        for workflow in workflows:
            content = workflow.read_text(encoding="utf-8")
            self.assertNotIn("runs-on: ubuntu-latest", content, workflow.name)
            job_count += content.count("runs-on: ubuntu-24.04")
        self.assertEqual(job_count, 8)

    def test_frozen_partition_and_source_budgets(self):
        policy = assurance.load_policy(POLICY)
        result = assurance.validate_partition(policy, PARTITION)
        self.assertEqual(result["partitions"][3]["name"], "ota_0")
        self.assertEqual(result["partitions"][4]["name"], "ota_1")
        self.assertEqual(result["partitions"][3]["size"], 0x3F0000)
        self.assertEqual(result["partitions"][4]["offset"], 0x410000)
        self.assertGreater(result["unallocated_bytes"], 0)
        self.assertEqual(assurance.validate_source_budgets(policy)["additional_product_tasks"], 0)

    def test_overlap_is_rejected(self):
        policy = assurance.load_policy(POLICY)
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / "bad.csv"
            path.write_text(
                "nvs,data,nvs,0x9000,0x4000\n"
                "otadata,data,ota,0x9000,0x2000\n",
                encoding="utf-8",
            )
            with self.assertRaisesRegex(ValueError, "overlaps"):
                assurance.validate_partition(policy, path)

    def test_size_report_gates_each_internal_ram_region(self):
        policy = assurance.load_policy(POLICY)
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / "size.json"
            path.write_text(json.dumps({
                "total_size": 123,
                "layout": [
                    {"name": "DIRAM", "total": 1000, "used": 800},
                    {"name": "Flash Code", "total": 10000, "used": 9000},
                ],
            }), encoding="utf-8")
            result = assurance.validate_size_report(policy, path)
            self.assertEqual(result["internal_ram"][0]["used_percent"], 80.0)
            report = json.loads(path.read_text(encoding="utf-8"))
            report["layout"][0]["used"] = 901
            path.write_text(json.dumps(report), encoding="utf-8")
            with self.assertRaisesRegex(ValueError, "exceeds policy"):
                assurance.validate_size_report(policy, path)

    def test_idf_action_log_can_wrap_size_json(self):
        report = assurance.parse_idf_size_output(
            "Executing action: size\n"
            "Running ninja in directory /tmp/build\n"
            '{"version":"1.2","layout":[{"name":"DIRAM","total":100,"used":10}]}\n'
            "Done\n"
        )
        self.assertEqual(report["layout"][0]["used"], 10)

    def test_provenance_hashes_every_input(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            app = root / "app.bin"
            merged = root / "merged.bin"
            sdkconfig = root / "sdkconfig"
            report = root / "resources.json"
            for path, content in ((app, b"app"), (merged, b"merged"),
                                  (sdkconfig, b"config"), (report, b"{}")):
                path.write_bytes(content)
            args = type("Args", (), {
                "policy": POLICY, "partition": PARTITION, "app_bin": app,
                "merged_bin": merged, "sdkconfig": sdkconfig,
                "resource_report": report, "source_sha": "a" * 40,
                "idf_version": "v6.1", "ci_repository": "owner/repo",
                "ci_run_id": "42",
            })()
            result = assurance.provenance(args)
            self.assertEqual(result["board"], "hi-yaka")
            self.assertEqual(result["upstream"]["commit"], "49b4380b3faa0ceadd28a3e78aaf0f311c46c57e")
            self.assertEqual(len(result["artifacts"]["app"]["sha256"]), 64)


if __name__ == "__main__":
    unittest.main()
