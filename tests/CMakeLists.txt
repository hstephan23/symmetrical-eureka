# Each test binary links against mysh_core so unit tests exercise the same
# implementation objects as the interactive shell.
add_executable(test_util test_util.c)
target_link_libraries(test_util PRIVATE mysh_core)

add_test(NAME util COMMAND test_util)

add_executable(test_lexer test_lexer.c)
target_link_libraries(test_lexer PRIVATE mysh_core)

add_test(NAME lexer COMMAND test_lexer)
