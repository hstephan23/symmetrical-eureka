add_library(mysh_core STATIC
        builtins.c
        env.c
        executor.c
        jobs.c
        lexer.c
        parser.c
        signals.c
        util.c
)

target_include_directories(mysh_core PUBLIC ${PROJECT_SOURCE_DIR}/inc)