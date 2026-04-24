cmake_minimum_required(VERSION 3.20)
project(mysh C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# Sanitizers are opt-in so the default build stays close to a normal release
# build, while local debugging can enable runtime memory/UB checks.
option(MYSH_ENABLE_SANITIZERS "Build mysh with AddressSanitizer and UndefinedBehaviorSanitizer" OFF)

if(MYSH_ENABLE_SANITIZERS)
    add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address,undefined)
endif()

add_subdirectory(src)

add_executable(${PROJECT_NAME} main.c)

target_include_directories(mysh PRIVATE inc)

target_link_libraries(${PROJECT_NAME} mysh_core)

target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Werror -g -O0)
target_compile_options(mysh_core PRIVATE -Wall -Wextra -Werror -g -O0)

# If Valgrind is installed, configure CTest's memcheck mode and expose it
# through `cmake --build build --target memcheck`.
find_program(VALGRIND_EXECUTABLE valgrind)
if(VALGRIND_EXECUTABLE)
    set(MEMORYCHECK_COMMAND "${VALGRIND_EXECUTABLE}" CACHE FILEPATH "Valgrind executable" FORCE)
    set(MEMORYCHECK_COMMAND_OPTIONS
        "--leak-check=full --show-leak-kinds=all --track-origins=yes --error-exitcode=1"
        CACHE STRING "Valgrind command line options" FORCE
    )
endif()

include(CTest)

# CTest defines BUILD_TESTING; disabling it skips all test targets.
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

if(VALGRIND_EXECUTABLE)
    add_custom_target(memcheck
        COMMAND ${CMAKE_CTEST_COMMAND}
            --test-dir ${CMAKE_BINARY_DIR}
            --output-on-failure
            -T memcheck
        DEPENDS test_util
        USES_TERMINAL
    )
else()
    add_custom_target(memcheck
        COMMAND ${CMAKE_COMMAND} -E echo "Valgrind was not found. Install valgrind to run memory checks."
        COMMAND ${CMAKE_COMMAND} -E false
        USES_TERMINAL
    )
endif()

add_custom_target(run COMMAND ${PROJECT_NAME} DEPENDS ${PROJECT_NAME} USES_TERMINAL)
