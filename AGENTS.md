# Repository Guidelines

## Project Structure & Module Organization

CHARX Integration for KDE adds Character Card V3 thumbnails to KDE applications.
Keep framework integration, archive handling, and image decoding separate:

- `src/thumbnail.cpp`: KIO plugin adapter and request validation.
- `src/assetprocess.cpp`: Python subprocess transport and deadline enforcement.
- `src/imagedecoder.cpp`: Qt raster decoding and image size checks.
- `reader/charx_reader.py`: standard-library ZIP parsing and asset resolution.
- `data/`: MIME registration; plugin metadata lives in `src/charxthumbnail.json`.
- `tests/`: Python reader tests, C++ decoder tests, and plugin smoke executable.
- `character-card-v3/SPEC_V3.md`: bundled format reference.

## Build, Test, and Development Commands

Install Fedora dependencies listed in `README.md`, then build without installing:

```sh
cmake -S . -B build-dev -G Ninja -DPython3_EXECUTABLE=/usr/bin/python3
cmake --build build-dev
ctest --test-dir build-dev --output-on-failure
```

CTest runs reader and decoder checks. To test the reader independently:

```sh
/usr/bin/python3 -m unittest discover -s tests -v
```

Use the README's explicit user-local configuration before installing. The default
prefix may target system directories. Run `charx-plugin-smoke` against an installed
plugin and a card to verify the complete thumbnail pipeline.

## Coding Style & Naming Conventions

Use C++17 with Qt/KDE APIs and four-space indentation. Prefer camelCase functions
and PascalCase classes. Python uses four-space indentation, snake_case functions,
and uppercase constants. Keep Python runtime dependencies within the standard
library. Match nearby CMake formatting. No formatter or linter is configured.

## Testing Guidelines

Python uses `unittest`; name files `test_*.py` and methods `test_*`. C++ checks are
standalone executables registered with CTest. Add behavioral regression coverage
for parsing, asset selection, resource limits, or decoder changes. No numeric
coverage threshold is configured. Generate temporary archives within tests rather
than depending on local character cards.

## Commit & Pull Request Guidelines

Git history is unavailable in this checkout, so no historical commit convention
can be established. Use concise, imperative commit subjects. PRs should explain
the problem, behavior changes, validation commands, and installation implications.
Link relevant issues and include before/after screenshots for visible changes.

## Security & Configuration

Preserve archive, image, memory, and time limits. Never fetch remote assets,
extract archive contents, execute card-provided code, or modify input cards.
Respect the spec's exact `embeded://` spelling. Avoid global environment changes,
Fedora-managed file replacements, and broad cache deletion. Document installation
and removal changes together.
