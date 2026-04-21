# mysh

A simple Unix shell written in C. Built step by step as a learning project — each milestone adds one system-programming concept (processes, file descriptors, pipes, signals, job control).

This is the repository for the 10-step plan in `mysh-plan/`. At this point the shell only has a REPL that echoes input back, but it's fully buildable, testable, and ready to grow.

## Status

| Step | Feature | Status |
|------|---------|--------|
| 01 | REPL skeleton | Done |
| 02 | Lexer | Not started |
| 03 | External commands (`fork`/`exec`) | Not started |
| 04 | Built-ins (`cd`, `pwd`, `exit`) | Not started |
| 05 | I/O redirection | Not started |
| 06 | Pipelines | Not started |
| 07 | Signal handling | Not started |
| 08 | Background jobs | Not started |
| 09 | Variables & expansion | Not started |
| 10 | Polish | Not started |

## Requirements

- A C11 compiler (`clang` on macOS, `gcc` on Linux).
- CMake 3.20 or newer.
- POSIX environment (macOS or Linux). Windows is not supported — this project uses POSIX system calls like `fork`, `execvp`, `pipe`, and `dup2`.
- Optional: Ninja (`brew install ninja`) — faster builds than Unix Makefiles.

## Build

From the project root:

```bash
cmake -S . -B build          # configure (run once, or after editing CMakeLists.txt)
cmake --build build          # build (run after editing .c/.h files)
```

The binary ends up at `./build/mysh`.

## Run

```bash
./build/mysh
```

Or via the custom CMake target:

```bash
cmake --build build --target run
```

Exit with `exit` or press Ctrl-D on an empty line.

## Test

```bash
ctest --test-dir build --output-on-failure
```

Or run an individual test binary directly (faster during iteration):

```bash
./build/tests/test_util
```

## Project layout

```
mysh/
├── CMakeLists.txt          # root build config
├── main.c                  # entry point + REPL loop
├── inc/                    # public headers for mysh_core
│   ├── util.h
│   └── ...                 # (stubs for future steps)
├── src/
│   ├── CMakeLists.txt      # builds the mysh_core static library
│   ├── util.c
│   └── ...                 # (stubs for future steps)
└── tests/
    ├── CMakeLists.txt
    ├── test_util.c         # unit tests for util
    └── integration/        # (shell-script integration tests, added in later steps)
```

`main.c` is a thin entry point that links against `mysh_core`, a static library holding all the shell's internals. This keeps tests simple — they link against the same library the main binary uses.

## Clean

```bash
rm -rf build
```

## License

Personal learning project — do what you want with it.
