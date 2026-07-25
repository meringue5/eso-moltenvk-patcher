from __future__ import annotations

from pathlib import Path
import tempfile
import unittest

from analyze_descriptor_benchmark import classify, parse_run


def benchmark_text(submit_values: list[int], draws: int = 100) -> str:
    lines = [
        (
            f"descriptor benchmark sample={index} draws={draws} "
            f"submit_ns={submit} wait_ns=200"
        )
        for index, submit in enumerate(submit_values)
    ]
    lines.extend(
        [
            "descriptor benchmark pixel=1,2,3,4 expected-red=3 match=yes",
            (
                f"descriptor benchmark: PASS draws={draws} "
                f"samples={len(submit_values)} alternating_resources=yes"
            ),
        ]
    )
    return "\n".join(lines)


class DescriptorBenchmarkTests(unittest.TestCase):
    def test_parses_complete_run(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "run.txt"
            path.write_text(benchmark_text([1000, 1100, 900]))
            run = parse_run(path)
        self.assertEqual(run.draws, 100)
        self.assertEqual(run.submit_ns, (1000, 1100, 900))
        self.assertEqual(run.median_submit_ns_per_draw, 10)

    def test_rejects_failed_pixel(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "run.txt"
            path.write_text(
                benchmark_text([1000, 1100]).replace(
                    "match=yes", "match=NO"
                )
            )
            with self.assertRaisesRegex(ValueError, "pixel"):
                parse_run(path)

    def test_rejects_missing_sample(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "run.txt"
            path.write_text(
                benchmark_text([1000, 1100, 900]).replace(
                    "descriptor benchmark sample=1 draws=100 "
                    "submit_ns=1100 wait_ns=200\n",
                    "",
                )
            )
            with self.assertRaisesRegex(ValueError, "sample count"):
                parse_run(path)

    def test_classifies_measured_gain(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            on_path = Path(directory) / "on.txt"
            off_path = Path(directory) / "off.txt"
            on_path.write_text(benchmark_text([1000, 1010, 990]))
            off_path.write_text(benchmark_text([900, 910, 890]))
            classification, reduction = classify(
                [parse_run(on_path)], [parse_run(off_path)]
            )
        self.assertEqual(classification, "MEASURED_GAIN")
        self.assertAlmostEqual(reduction, 10)

    def test_classifies_negligible_gain(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            on_path = Path(directory) / "on.txt"
            off_path = Path(directory) / "off.txt"
            on_path.write_text(benchmark_text([1000, 1010, 990]))
            off_path.write_text(benchmark_text([980, 990, 970]))
            classification, reduction = classify(
                [parse_run(on_path)], [parse_run(off_path)]
            )
        self.assertEqual(classification, "NEGLIGIBLE_GAIN")
        self.assertAlmostEqual(reduction, 2)


if __name__ == "__main__":
    unittest.main()
