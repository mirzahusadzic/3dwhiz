# Agent guide for 3dWhiz

3dWhiz is a C++14 editor framework and demo using GTK 3.24+, GObject and
compatibility OpenGL. Read [README.md](README.md) for setup and controls,
and [docs/PORTING.md](docs/PORTING.md) for the rendering and API conventions.

## Code map

- `include/gtkrtg/`: public framework headers and math types.
- `src/`: GTK widgets, scene objects, rendering, cameras and gizmos.
- `examples/editor.cpp`: demo application and OBJ import UI.
- `examples/*_checks.h`: live GTK interaction checks used by the demo.
- `tests/obj.cpp`: display-independent parser, geometry and math checks.
- `src/txt/`: original development notes; preserve their archival contents.
- `CMakeLists.txt`: current build. The old Makefiles are historical.

Use **3dWhiz** in prose and UI labels. Repository paths, build targets and
commands use lowercase `3dwhiz`.

## Editing conventions

- Follow the surrounding indentation and preserve each file's line endings,
  including CRLF. Avoid whole-file formatting during focused changes.
- Use `.clang-format` as a guide for new code and edited regions: four-space
  indentation, Allman braces, and the existing pointer style. Preserve include
  order and historical comments.
- Keep C++14 compatibility and follow the existing GObject ownership patterns.
- Preserve third-party attribution and license headers. Update
  `THIRD_PARTY_NOTICES.md` when introducing third-party code or assets.
- Put temporary models, logs and scratch work in ignored `.local/`, and build
  outputs in `build*/`. Documentation screenshots belong in `docs/images/`.

## Build and checks

Install dependencies using the README, then build and run the unit suite:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON
cmake --build build --parallel
ctest --test-dir build -L unit --output-on-failure
```

For rendering, input, layout or GUI lifecycle changes, enable and run the
editor checks in a session with a display and compatibility OpenGL:

```sh
cmake -S . -B build -DWHIZ_UI_TESTS=ON
cmake --build build --parallel
ctest --test-dir build -L ui --output-on-failure
```

CTest supplies `G_DEBUG=fatal-warnings` and `GSETTINGS_BACKEND=memory` for the
UI test. Screenshots are written to the build directory. These checks inject
GTK events; report them as such, separately from manual pointer testing.
The Linux Xvfb/Mesa setup is in [.github/workflows/ci.yml](.github/workflows/ci.yml).

Use a separate `build-sanitize` directory with `WHIZ_SANITIZERS=ON` for memory
or undefined-behaviour changes. Run its checks with
`UBSAN_OPTIONS=halt_on_error=1`; full commands are in the README.

Run checks appropriate to the change and add regression coverage for
substantive behaviour changes. Documentation-only edits need link and diff
checks, without rebuilding the application. Run `git diff --check` before
committing. Distinguish actual test results from configured CI coverage.

## Viewport conventions

- Preserve compatibility OpenGL support, including the macOS CGL backend.
  Make the receiving canvas's context current before issuing OpenGL calls.
- Keep GTK pointer coordinates, framebuffer coordinates and world coordinates
  explicit. Use the receiving canvas's dimensions for conversions.
- Exercise all four viewports and both Y-up and Z-up conventions when changing
  picking, camera navigation or transform gizmos.
- Match CPU transforms to the renderer's transform order. The primitive's
  transformation-matrix accessor returns the rigid matrix; translation and
  scale are applied separately.
- Account for widget realization, resizing and destruction when managing
  OpenGL resources, callbacks and timers.

## Commit messages

Use a concise conventional subject and wrap body paragraphs at 72–80 columns.
Use one blank line between the subject and body, and between paragraphs.
Describe the resulting change and relevant validation.
