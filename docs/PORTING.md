# GTK 3 port

The port retains the GObject API, custom `GtkRtgFourPaned` layout, fixed-function
OpenGL renderer, camera controls, colour-buffer picking and transform gizmos.
The original [development notes](../src/txt) remain unchanged. Build the port
with CMake; the old Makefiles document the original build.

## Rendering and UI

The canvas is a GTK 3 drawing area backed by a compatibility OpenGL context.
Each viewport renders into a framebuffer, reads its pixels back and presents
them through Cairo. Pango/Cairo provide the viewport captions and text used by
the renderer. Toolbar buttons use GTK theme icons and text labels.

On macOS, shared legacy CGL contexts support the original fixed-function
renderer. Rendering uses logical pixels, so it does not yet provide full Retina
resolution. Readback also adds overhead compared with direct GPU presentation.

The Linux backend uses GDK and libepoxy with desktop compatibility OpenGL.
Startup defaults `GDK_GL` to `legacy` unless it is already set. For headless
checks, CI uses an X11 display under Xvfb with Mesa software rendering.

CMake discovers GTK's runtime data directory on macOS. Startup adds it to the
process's `XDG_DATA_DIRS` when needed so the file chooser can find its schemas
and icon themes, including when launched from a terminal.

## Embedding the framework

`gtk_rtg_canvas_new()` no longer takes a GtkGLExt configuration object.
`gtk_rtg_canvas_make_current(canvas)` selects a realized canvas's context before
client OpenGL calls. The destroy callback releases client GL resources on
unrealize or destruction; the init callback runs when a new context is created.

`gtk_rtg_primitive_get_transformation_matrix()` returns the rigid matrix.
Translation and scale are applied separately by the renderer.

## Validation

The local validation platform is Apple Silicon macOS with GTK 3.24. Both ordinary
and AddressSanitizer/UndefinedBehaviorSanitizer builds have been exercised.
System GTK/OpenGL libraries are not sanitizer-instrumented, and this macOS run
does not include LeakSanitizer.

CTest registers the parser, geometry and math suite as `obj-import` (label
`unit`). With `WHIZ_UI_TESTS=ON`, it also registers `editor-smoke` (label `ui`),
with a three-minute timeout. The UI test needs a display and compatibility
OpenGL; it saves `editor.png` and `editor.png.drag.png` in the build directory.

The UI checks inject GTK events into live widgets and inspect rendered
transforms. They cover selection, transforms, camera navigation, both up-axis
conventions, viewport layout, OBJ import and context lifecycle. They do not
automate the operating system's mouse or validate its event delivery.

The [CI workflow](../.github/workflows/ci.yml) builds on Ubuntu and macOS. Linux
UI checks use Xvfb; hosted macOS jobs run the display-independent unit suite.
See the [README](../README.md#tests-and-ci) for local commands.

## Current scope

This is a restored framework and demonstration editor. Legacy texture,
sub-object editing, clone/instance, serialization and other application-facing
APIs have not received a comprehensive audit. Some original compiler warnings
remain. OBJ import supports convex faces or pre-triangulated input, without
materials, textures or smoothing groups.

Full Retina rendering, a shader-based renderer, and Windows verification are
future work. Linux desktop behaviour beyond CI's Xvfb/Mesa environment also
needs hands-on validation.
