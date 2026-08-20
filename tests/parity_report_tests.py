from pathlib import Path
import sys


def parse_rows(text: str):
    for raw in text.splitlines():
        line = raw.strip()
        if not line.startswith("|"):
            continue
        columns = [part.strip() for part in line.strip("|").split("|")]
        if len(columns) != 5 or not columns[0].isdigit():
            continue
        yield columns


def main() -> int:
    if len(sys.argv) != 2:
        print("usage: parity_report_tests.py <parity-status.md>", file=sys.stderr)
        return 2

    path = Path(sys.argv[1])
    rows = list(parse_rows(path.read_text(encoding="utf-8")))
    if len(rows) < 20:
        raise AssertionError("parity status table must keep all 20 tracked subsystems")

    by_id = {int(row[0]): row for row in rows}
    for row in rows:
        number, subsystem, status, evidence, acceptance = row
        if status == "matched":
            if not evidence or evidence in {"-", "n/a"}:
                raise AssertionError(f"matched row {number} ({subsystem}) has no evidence path")
            if not acceptance or acceptance in {"-", "n/a"} or "planned" in acceptance.lower():
                raise AssertionError(f"matched row {number} ({subsystem}) has no concrete acceptance test")

    completed_test_rows = {
        8: "network_parity",
        10: "asset_source",
        11: "ui_audio",
        13: "camera",
        14: "api_transcript_parity",
        15: "java_api_surface",
        16: "python_api_compatibility",
        20: "platform_contract",
    }
    for number, test_name in completed_test_rows.items():
        row = by_id[number]
        if "planned" in row[4].lower():
            raise AssertionError(f"row {number} still marks {test_name} as planned")
        if test_name not in row[4]:
            raise AssertionError(f"row {number} must name acceptance test {test_name}")

    print("parity report contract passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
