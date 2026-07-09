from __future__ import annotations

import json
import tempfile
import unittest
from pathlib import Path

from bench.artifact_store import ArtifactStore
from bench.nightly import NightlyHistory, NightlyPhase
from bench.report import ReportGenerator


def _write_json(path: Path, data: dict) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(data, indent=2) + "\n")


class CiInterfaceTests(unittest.TestCase):
    def test_append_experiment_reads_artifact_manifest_configs(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            root = Path(temp_dir)
            experiment_dir = root / "experiment-output"
            history_file = root / "nightly-history.json"
            machine_specs = root / "machine-specs.json"

            config_450 = {
                "full_ibd": False,
                "start_height": 840000,
                "runs": 2,
                "instrumentation": "uninstrumented",
                "bitcoind": {"dbcache": 450, "stopatheight": 900000},
            }
            config_32000 = {
                "full_ibd": False,
                "start_height": 840000,
                "runs": 2,
                "instrumentation": "uninstrumented",
                "bitcoind": {"dbcache": 32000, "stopatheight": 900000},
            }
            _write_json(
                experiment_dir / "artifacts.json",
                {
                    "runs": [
                        {
                            "subject": "master",
                            "profile": "small-cache",
                            "config": config_450,
                            "output_dir": "runs/small-cache/master",
                            "results_file": "runs/small-cache/master/results.json",
                        },
                        {
                            "subject": "master",
                            "profile": "large-cache",
                            "config": config_32000,
                            "output_dir": "runs/large-cache/master",
                            "results_file": "runs/large-cache/master/results.json",
                        },
                    ],
                    "comparisons": [],
                },
            )
            _write_json(
                experiment_dir / "runs/small-cache/master/results.json",
                {"results": [{"mean": 120, "stddev": 4, "times": [118, 122]}]},
            )
            _write_json(
                experiment_dir / "runs/large-cache/master/results.json",
                {"results": [{"mean": 90, "stddev": 3, "times": [89, 91]}]},
            )
            _write_json(
                machine_specs,
                {
                    "cpu_model": "Test CPU",
                    "architecture": "x86_64",
                    "cpu_cores": 4,
                    "disk_type": "NVMe SSD",
                    "os_kernel": "6.6.0",
                    "total_ram_gb": 32,
                },
            )

            count = NightlyPhase(history_file).append_experiment(
                experiment_dir=experiment_dir,
                commit="abc123",
                date_str="2026-06-04",
                machine_specs_file=machine_specs,
                run_date="2026-06-05",
                trigger="manual",
            )

            self.assertEqual(count, 2)
            runs = ArtifactStore(experiment_dir).load_runs()
            self.assertEqual(
                [run.results_file for run in runs],
                [
                    experiment_dir / "runs/small-cache/master/results.json",
                    experiment_dir / "runs/large-cache/master/results.json",
                ],
            )
            history = json.loads(history_file.read_text())
            self.assertEqual(
                [r["config"]["bitcoind"]["dbcache"] for r in history["results"]],
                [450, 32000],
            )
            self.assertEqual(
                [r["config"]["instrumentation"] for r in history["results"]],
                ["uninstrumented", "uninstrumented"],
            )

    def test_report_uses_config_snapshot_and_writes_summary(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            root = Path(temp_dir)
            experiment_dir = root / "experiment-output"
            output_dir = root / "report"
            history_file = root / "nightly-history.json"
            config = {
                "full_ibd": False,
                "start_height": 840000,
                "runs": 2,
                "instrumentation": "uninstrumented",
                "bitcoind": {"dbcache": 450, "stopatheight": 900000},
            }

            _write_json(
                experiment_dir / "artifacts.json",
                {
                    "runs": [
                        {
                            "subject": "head",
                            "profile": "renamed-profile",
                            "config": config,
                            "output_dir": "runs/renamed-profile/head",
                            "results_file": "runs/renamed-profile/head/results.json",
                        }
                    ],
                    "comparisons": [],
                },
            )
            _write_json(
                history_file,
                {
                    "results": [
                        {
                            "date": "2026-06-01",
                            "commit": "base123",
                            "mean": 180,
                            "stddev": 6,
                            "runs": 2,
                            "config": config,
                            "machine": {},
                        }
                    ]
                },
            )
            _write_json(
                experiment_dir / "runs/renamed-profile/head/results.json",
                {
                    "results": [
                        {
                            "command": "head-renamed-profile",
                            "mean": 120,
                            "stddev": 4,
                            "user": 100,
                            "system": 20,
                        }
                    ]
                },
            )

            result = ReportGenerator(
                nightly_history=NightlyHistory(history_file)
            ).generate_experiment(
                experiment_dir=experiment_dir,
                output_dir=output_dir,
                pr_number="123",
                run_id="456",
            )

            self.assertEqual(result.summary_file, output_dir / "summary.txt")
            self.assertIn("dbcache=450MB", result.index_file.read_text())
            self.assertNotIn("dbcache=0MB", result.index_file.read_text())
            self.assertEqual(
                (output_dir / "summary.txt").read_text().strip(),
                "dbcache=450MiB: 2 min (nightly median of 1: 3 min, "
                "2026-06-01 to 2026-06-01) -> +33.3% faster",
            )
            combined = json.loads((output_dir / "results.json").read_text())
            self.assertEqual(
                combined["results"][0]["config"]["bitcoind"]["dbcache"],
                450,
            )


if __name__ == "__main__":
    unittest.main()
