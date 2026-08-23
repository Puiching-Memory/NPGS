import subprocess
import sys
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[1]
MAX_TRACKED_FILE_SIZE = 20 * 1024 * 1024
FORBIDDEN_SUFFIXES = {".bak", ".dll", ".exe", ".obj", ".orig", ".pdb"}
FORBIDDEN_PARTS = {"Win64", "vcpkg_installed"}


def tracked_files() -> list[Path]:
    result = subprocess.run(
        ["git", "ls-files", "-z"],
        cwd=REPO_ROOT,
        check=True,
        capture_output=True,
    )
    return [REPO_ROOT / item.decode("utf-8") for item in result.stdout.split(b"\0") if item]


def main() -> int:
    violations: list[str] = []
    for path in tracked_files():
        # A deleted path is expected while this hygiene change is still uncommitted.
        if not path.is_file():
            continue

        relative_path = path.relative_to(REPO_ROOT)
        if path.suffix.lower() in FORBIDDEN_SUFFIXES:
            violations.append(f"generated/backup file is tracked: {relative_path}")
        if any(part in FORBIDDEN_PARTS for part in relative_path.parts):
            violations.append(f"generated directory is tracked: {relative_path}")
        if path.stat().st_size > MAX_TRACKED_FILE_SIZE:
            size_mib = path.stat().st_size / (1024 * 1024)
            violations.append(
                f"tracked file exceeds 20 MiB ({size_mib:.1f} MiB): {relative_path}"
            )

    if violations:
        print("Repository hygiene check failed:", file=sys.stderr)
        for violation in violations:
            print(f"- {violation}", file=sys.stderr)
        return 1

    print("Repository hygiene check passed.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
