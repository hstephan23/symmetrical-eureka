cmake_minimum_required(VERSION 3.20)
project(mysh C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

add_subdirectory(src)

add_executable(${PROJECT_NAME} main.c)

target_include_directories(mysh PRIVATE inc)

target_link_libraries(${PROJECT_NAME} mysh_core)

target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Werror -g -O0)
target_compile_options(mysh_core PRIVATE -Wall -Wextra -Werror -g -O0)

enable_testing()
add_subdirectory(tests)

add_custom_target(run COMMAND ${PROJECT_NAME} DEPENDS ${PROJECT_NAME} USES_TERMINAL)