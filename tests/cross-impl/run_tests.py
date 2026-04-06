#!/usr/bin/env python3
"""Main runner for cross-implementation tests."""

import sys
import os
import json
from datetime import datetime
from pathlib import Path

sys.path.insert(0, os.path.dirname(__file__))
from compare import run_all, compare_results, format_diff, BINARIES
from test_cases import get_all_test_cases


def run_test_cases(output_dir=None):
    cases = get_all_test_cases()
    print(f"=== Running {len(cases)} test cases ===")
    print(f"Implementations: {', '.join(BINARIES.keys())}")
    print()

    for name, binary in BINARIES.items():
        status = "OK" if binary.exists() else f"MISSING: {binary}"
        print(f"  {name}: {binary} [{status}]")

    missing = [n for n, b in BINARIES.items() if not b.exists()]
    if missing:
        print(f"\nERROR: Missing binaries: {', '.join(missing)}")
        print("Run ./build.sh first")
        return False

    print()

    total = len(cases)
    passed = 0
    failed = 0
    errors = 0
    all_diffs = []

    for i, case in enumerate(cases):
        name = case["name"]
        input_fn = case["input_fn"]
        args = case["args"]
        mode = case["mode"]

        try:
            input_data = input_fn()
            results = run_all(input_data, args=args, timeout=15.0)
            diffs = compare_results(results, name, mode=mode)

            if diffs:
                failed += 1
                all_diffs.extend(diffs)
                print(f"  [{i + 1}/{total}] DIFF: {name}")
                for d in diffs:
                    print(f"         {d.impl_a} vs {d.impl_b}: {d.description}")
            else:
                passed += 1
                print(f"  [{i + 1}/{total}] PASS: {name}")

        except Exception as e:
            errors += 1
            print(f"  [{i + 1}/{total}] ERROR: {name} - {e}")

    print()
    print("=" * 50)
    print(f"RESULTS: {passed}/{total} passed, {failed} differences, {errors} errors")
    print("=" * 50)

    if output_dir:
        ts = datetime.now().strftime("%Y%m%d_%H%M%S")
        report_path = Path(output_dir) / f"test_report_{ts}.txt"

        with open(report_path, "w") as f:
            f.write(f"Cross-Implementation Test Report: {datetime.now().isoformat()}\n")
            f.write(
                f"Test cases: {total}, Passed: {passed}, Differences: {failed}, Errors: {errors}\n"
            )
            f.write("=" * 60 + "\n\n")

            if all_diffs:
                f.write("DIFFERENCES FOUND:\n\n")
                for d in all_diffs:
                    f.write(format_diff(d) + "\n")
                    f.write("-" * 40 + "\n")
            else:
                f.write("All implementations produced identical output.\n")

        print(f"Report saved to: {report_path}")

        json_path = Path(output_dir) / f"test_report_{ts}.json"
        json.dump(
            {
                "timestamp": datetime.now().isoformat(),
                "total": total,
                "passed": passed,
                "differences": failed,
                "errors": errors,
                "diffs": [
                    {
                        "test": d.test_name,
                        "impl_a": d.impl_a,
                        "impl_b": d.impl_b,
                        "description": d.description,
                    }
                    for d in all_diffs
                ],
            },
            open(json_path, "w"),
            indent=2,
        )

    return failed == 0 and errors == 0


if __name__ == "__main__":
    results_dir = Path(__file__).parent / "results"
    results_dir.mkdir(exist_ok=True)
    ok = run_test_cases(output_dir=results_dir)
    sys.exit(0 if ok else 1)
