cmake_minimum_required(VERSION 3.20)
project(tide C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

option(TIDE_ENABLE_SANITIZERS "Build with AddressSanitizer and UndefinedBehaviorSanitizer" OFF)

add_library(tide_core
    src/status.c
    src/rect.c
    src/input.c
    src/screen.c
    src/string_builder.c
    src/syntax.c
    src/command_palette.c
    src/project_files.c
    src/ansi.c
    src/terminal.c
    src/app.c
    src/buffer.c
    src/editor.c
    src/editor_render.c
    src/workspace.c
)

target_include_directories(tide_core PUBLIC include)
target_compile_options(tide_core PRIVATE -Wall -Wextra -Werror -g -O0)

if(TIDE_ENABLE_SANITIZERS)
    target_compile_options(tide_core PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(tide_core PUBLIC -fsanitize=address,undefined)
endif()

add_executable(tide main.c)
target_link_libraries(tide PRIVATE tide_core)
target_compile_options(tide PRIVATE -Wall -Wextra -Werror -g -O0)

include(CTest)
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
