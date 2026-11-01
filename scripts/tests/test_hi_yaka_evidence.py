import tempfile
import unittest
from argparse import Namespace
from pathlib import Path

from scripts import hi_yaka_evidence as evidence


SOURCE_SHA = "a" * 40
UPSTREAM_SHA = "b" * 40
ARTIFACT_SHA = "c" * 64
CONTAINER_DIGEST = "sha256:" + "d" * 64


def init_args(root: Path) -> Namespace:
    return Namespace(
        output_root=root,
        sample_id="HYK-001",
        hardware_revision="R1.6",
        firmware_sha=SOURCE_SHA,
        upstream_sha=UPSTREAM_SHA,
        firmware_artifact_name="merged-binary.bin",
        firmware_artifact_sha256=ARTIFACT_SHA,
        configuration="hi-yaka",
        idf_version="v6.1",
        container_digest=CONTAINER_DIGEST,
        operator="lab-user",
        environment="bench-a / 25C",
        timestamp="2026-11-02T12:34:56+08:00",
    )


class HiYaKaEvidenceTest(unittest.TestCase):
    def test_init_creates_complete_unverified_package(self):
        with tempfile.TemporaryDirectory() as temporary:
            run = evidence.initialise(init_args(Path(temporary)))
            result = evidence.validate_run(run)
            self.assertTrue(result["valid"], result["errors"])
            self.assertEqual(result["counts"]["NOT_RUN"], len(evidence.MODULES))
            self.assertEqual(run.name, "20261102T043456Z_HYK-001_aaaaaaaaaaaa")
            self.assertEqual((run / "logs" / "serial.log").stat().st_size, 0)

    def test_pass_requires_nonempty_raw_evidence(self):
        with tempfile.TemporaryDirectory() as temporary:
            run = evidence.initialise(init_args(Path(temporary)))
            args = Namespace(
                run=run,
                module="power",
                result="PASS",
                actual="3.3 V rail stable",
                raw_evidence=[],
                defect_id=[],
                tester="lab-user",
                tested_at="2026-11-02T13:00:00+08:00",
            )
            with self.assertRaisesRegex(ValueError, "PASS requires raw evidence"):
                evidence.record_result(args)
            (run / "measurements" / "power.csv").write_text("timestamp,rail_v\nnow,3.30\n", encoding="utf-8")
            args.raw_evidence = ["measurements/power.csv"]
            evidence.record_result(args)
            result = evidence.validate_run(run, ["power"])
            self.assertTrue(result["valid"], result["errors"])
            self.assertEqual(result["counts"]["PASS"], 1)

    def test_fail_requires_defect_id(self):
        with tempfile.TemporaryDirectory() as temporary:
            run = evidence.initialise(init_args(Path(temporary)))
            log = run / "logs" / "serial.log"
            log.write_text("reset loop\n", encoding="utf-8")
            args = Namespace(
                run=run,
                module="flash",
                result="FAIL",
                actual="device resets after boot",
                raw_evidence=["logs/serial.log"],
                defect_id=[],
                tester="lab-user",
                tested_at="2026-11-02T13:00:00+08:00",
            )
            with self.assertRaisesRegex(ValueError, "requires at least one defect"):
                evidence.record_result(args)
            args.defect_id = ["HYK-DEF-001"]
            evidence.record_result(args)
            self.assertTrue(evidence.validate_run(run)["valid"])

    def test_evidence_path_cannot_escape_run(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            run = evidence.initialise(init_args(root))
            outside = root / "outside.log"
            outside.write_text("not owned by this run", encoding="utf-8")
            with self.assertRaisesRegex(ValueError, "stay inside"):
                evidence.resolve_evidence(run, "../outside.log")

    def test_required_module_cannot_remain_not_run(self):
        with tempfile.TemporaryDirectory() as temporary:
            run = evidence.initialise(init_args(Path(temporary)))
            result = evidence.validate_run(run, ["power", "flash", "i2c", "pca9557"])
            self.assertFalse(result["valid"])
            self.assertEqual(sum("required module is NOT_RUN" in item for item in result["errors"]), 4)

    def test_identity_inputs_are_strict(self):
        with tempfile.TemporaryDirectory() as temporary:
            args = init_args(Path(temporary))
            args.firmware_sha = "main"
            with self.assertRaisesRegex(ValueError, "firmware SHA"):
                evidence.initialise(args)

    def test_recorded_result_cannot_be_overwritten(self):
        with tempfile.TemporaryDirectory() as temporary:
            run = evidence.initialise(init_args(Path(temporary)))
            log = run / "logs" / "serial.log"
            log.write_text("boot complete\n", encoding="utf-8")
            args = Namespace(
                run=run,
                module="flash",
                result="PASS",
                actual="three cold boots completed",
                raw_evidence=["logs/serial.log"],
                defect_id=[],
                tester="lab-user",
                tested_at="2026-11-02T13:00:00+08:00",
            )
            evidence.record_result(args)
            with self.assertRaisesRegex(ValueError, "already recorded"):
                evidence.record_result(args)


if __name__ == "__main__":
    unittest.main()
