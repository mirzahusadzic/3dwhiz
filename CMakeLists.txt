cmake_minimum_required(VERSION 3.20)
project(3dwhiz VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK3 REQUIRED IMPORTED_TARGET gtk+-3.0>=3.24)
find_package(OpenGL REQUIRED)

file(GLOB WHIZ_SOURCES CONFIGURE_DEPENDS src/*.cpp)
add_library(3dwhiz STATIC ${WHIZ_SOURCES} src/marshalers.c)
target_include_directories(3dwhiz PUBLIC include PRIVATE src)
target_link_libraries(3dwhiz PUBLIC PkgConfig::GTK3 OpenGL::GL)
if(APPLE)
  target_compile_definitions(3dwhiz PUBLIC GL_SILENCE_DEPRECATION)
  # Terminal launchers may omit Homebrew's shared data from XDG_DATA_DIRS.
  find_path(WHIZ_GTK_DATA_DIR NAMES glib-2.0/schemas/gschemas.compiled
    HINTS "${GTK3_PREFIX}/share" PATHS /opt/homebrew/share /usr/local/share
    DOC "GTK runtime data directory (compiled schemas and icon themes)")
  if(WHIZ_GTK_DATA_DIR)
    target_compile_definitions(3dwhiz PRIVATE WHIZ_GTK_DATA_DIR="${WHIZ_GTK_DATA_DIR}")
  endif()
else()
  pkg_check_modules(EPOXY REQUIRED IMPORTED_TARGET epoxy)
  target_link_libraries(3dwhiz PUBLIC OpenGL::GLU PkgConfig::EPOXY)
endif()
target_compile_options(3dwhiz PRIVATE -Wall -Wextra -Wno-deprecated-declarations
  -Wno-unused-parameter -Wno-deprecated-dynamic-exception-spec -Wno-return-type-c-linkage)

option(WHIZ_SANITIZERS "Enable AddressSanitizer and UndefinedBehaviorSanitizer" OFF)
if(WHIZ_SANITIZERS AND CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
  target_compile_options(3dwhiz PUBLIC -fsanitize=address,undefined -fno-omit-frame-pointer)
  target_link_options(3dwhiz PUBLIC -fsanitize=address,undefined)
endif()

add_executable(3dwhiz-demo examples/editor.cpp)
target_link_libraries(3dwhiz-demo PRIVATE 3dwhiz)
target_compile_options(3dwhiz-demo PRIVATE -Wno-deprecated-declarations -Wno-return-type-c-linkage)
include(CTest)
option(WHIZ_UI_TESTS "Register editor interaction tests (requires a display and compatibility OpenGL)" OFF)
if(BUILD_TESTING)
  add_executable(3dwhiz-obj-tests tests/obj.cpp)
  target_link_libraries(3dwhiz-obj-tests PRIVATE 3dwhiz)
  target_compile_options(3dwhiz-obj-tests PRIVATE -Wno-deprecated-declarations -Wno-return-type-c-linkage)
  add_test(NAME obj-import COMMAND 3dwhiz-obj-tests)
  set_tests_properties(obj-import PROPERTIES LABELS unit TIMEOUT 60)
  if(WHIZ_UI_TESTS)
    add_test(NAME editor-smoke COMMAND ${CMAKE_COMMAND} -E env
      G_DEBUG=fatal-warnings GSETTINGS_BACKEND=memory
      $<TARGET_FILE:3dwhiz-demo> --smoke-test
      --screenshot ${CMAKE_CURRENT_BINARY_DIR}/editor.png)
    set_tests_properties(editor-smoke PROPERTIES LABELS ui TIMEOUT 180 RUN_SERIAL TRUE)
  endif()
endif()
