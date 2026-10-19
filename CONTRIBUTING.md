# Working on 3dWhiz

Start with the [build instructions](README.md#build-and-run). The CMake build
produces the framework library, demo editor and display-independent tests.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

For viewport, input or layout changes, enable `WHIZ_UI_TESTS=ON` and run CTest
in a desktop session. Check the editor by hand too: the automated suite injects
GTK events and cannot replace real pointer interaction. Sanitizers are available
with `WHIZ_SANITIZERS=ON`; see [Tests and CI](README.md#tests-and-ci).

Keep changes focused and follow the surrounding indentation and line endings.
Avoid formatting whole legacy files while changing a small part of them. The
notes in `src/txt` are an unchanged record of the original project.

Bug reports are most useful with the OS, GTK version, viewport, active tool and
steps to reproduce. Include a small redistributable OBJ when a model is needed.
For a pull request, describe the behaviour changed and how you checked it.
