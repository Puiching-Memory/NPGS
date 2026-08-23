# Contributing

## Build and test

1. Run the platform bootstrap script in `scripts/`.
2. Configure and build with a preset documented in `README.md`.
3. Run CTest with `--output-on-failure` before opening a pull request.

The default build enables `/W4` on MSVC or `-Wall -Wextra -Wpedantic` on
GCC/Clang. Existing warnings are being paid down incrementally. New or modified
code should introduce no additional warnings; use
`-DNPGS_WARNINGS_AS_ERRORS=ON` when the touched targets are warning-clean.

Run `clang-format` using the repository `.clang-format` file for modified C++
sources. Large assets must follow `NPGS/Assets/README.md`.

## Changes

Keep changes focused and explain user-visible behavior, platforms tested, and
any performance or numerical impact. Changes to generators or shader math should
include deterministic tests or a documented reference image/data set.
