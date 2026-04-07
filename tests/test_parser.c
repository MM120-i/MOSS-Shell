#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>

#include "../include/parser.h"

private void freeStrArray(char **arr)
{
    if (!arr)
        return;

    for (size_t i = 0; arr[i]; i++)
        free(arr[i]);

    free(arr);
}

// ======================= expandEnvVar Tests =======================

// test 1: NULL input returns NULL
private void test_expandEnvVar_null(void **state)
{
    (void)state;
    char *result = expandEnvVar(NULL);
    assert_null(result);
}

// test 2: No env vars - returns a copy of the input
private void test_expandEnvVar_noVar(void **state)
{
    (void)state;
    setenv("TEST_VAR", "dummy", 1);
    char *result = expandEnvVar("hello world");
    assert_non_null(result);
    assert_string_equal(result, "hello world");
    free(result);
}

// test 3: Undefined env var returns empty string
private void test_expandEnvVar_undefined(void **state)
{
    (void)state;
    unsetenv("UNDEFINED_VAR_12345");
    char *result = expandEnvVar("$UNDEFINED_VAR_12345");
    assert_non_null(result);
    assert_string_equal(result, "");
    free(result);
}

// test 4: Defined env var expands correctly
private void test_expandEnvVar_defined(void **state)
{
    (void)state;
    setenv("MY_TEST_VAR", "my_value", 1);
    char *result = expandEnvVar("$MY_TEST_VAR");
    assert_non_null(result);
    assert_string_equal(result, "my_value");
    free(result);
}

// test 5: Prefix + env var
private void test_expandEnvVar_prefix(void **state)
{
    (void)state;
    setenv("PATH", "/usr/bin", 1);
    char *result = expandEnvVar("prefix$PATH");
    assert_non_null(result);
    assert_string_equal(result, "prefix/usr/bin");
    free(result);
}

// test 6: env var + suffix
private void test_expandEnvVar_suffix(void **state)
{
    (void)state;
    setenv("HOME", "/home/user", 1);
    char *result = expandEnvVar("$HOME suffix");
    assert_non_null(result);
    assert_string_equal(result, "/home/user suffix");
    free(result);
}

// test 7: ${VAR} syntax
private void test_expandEnvVar_braces(void **state)
{
    (void)state;
    setenv("MY_VAR", "braces_value", 1);
    char *result = expandEnvVar("${MY_VAR}");
    assert_non_null(result);
    assert_string_equal(result, "braces_value");
    free(result);
}

// test 8: $$ expands to PID
private void test_expandEnvVar_dollar_dollar(void **state)
{
    (void)state;
    pid_t my_pid = getpid();
    char expected[32];
    snprintf(expected, sizeof(expected), "%d", my_pid);
    char *result = expandEnvVar("$$");
    assert_non_null(result);
    assert_string_equal(result, expected);
    free(result);
}

// test 9: Multiple env vars in one string
private void test_expandEnvVar_multiple(void **state)
{
    (void)state;
    setenv("VAR1", "a", 1);
    setenv("VAR2", "b", 1);
    char *result = expandEnvVar("$VAR1 middle $VAR2");
    assert_non_null(result);
    assert_string_equal(result, "a middle b");
    free(result);
}

// ======================= strip_quotes Tests =======================

// test 1: NULL input returns NULL
private void test_strip_quotes_null(void **state)
{
    (void)state;
    char *result = strip_quotes(NULL);
    assert_null(result);
}

// test 2: No quotes - returns original
private void test_strip_quotes_noQuotes(void **state)
{
    (void)state;
    char input[] = "hello";
    char *result = strip_quotes(input);
    assert_string_equal(result, "hello");
}

// test 3: Double quotes removed
private void test_strip_quotes_double(void **state)
{
    (void)state;
    char input[] = "\"hello\"";
    char *result = strip_quotes(input);
    assert_string_equal(result, "hello");
}

// test 4: Single quotes removed
private void test_strip_quotes_single(void **state)
{
    (void)state;
    char input[] = "'hello'";
    char *result = strip_quotes(input);
    assert_string_equal(result, "hello");
}

// test 5: Mismatched quotes - no change
private void test_strip_quotes_mismatched(void **state)
{
    (void)state;
    char input[] = "\"hello";
    char *result = strip_quotes(input);
    assert_string_equal(result, "\"hello");
}

// ======================= isSafe Tests =======================

// test 1: NULL is not safe
private void test_isSafe_null(void **state)
{
    (void)state;
    bool result = isSafe(NULL);
    assert_false(result);
}

// test 2: Empty string is not safe
private void test_isSafe_empty(void **state)
{
    (void)state;
    bool result = isSafe("");
    assert_false(result);
}

// test 3: Normal string is safe
private void test_isSafe_normal(void **state)
{
    (void)state;
    bool result = isSafe("hello");
    assert_true(result);
}

// test 4: String with semicolon is unsafe
private void test_isSafe_semicolon(void **state)
{
    (void)state;
    bool result = isSafe("hello;rm -rf");
    assert_false(result);
}

// test 5: String with semicolon is unsafe (pipe && is NOT in DANGEROUS_CHARS)
private void test_isSafe_pipe(void **state)
{
    (void)state;
    bool result = isSafe("hello;ls");
    assert_false(result);
}

// test 6: String with brackets is unsafe
private void test_isSafe_brackets(void **state)
{
    (void)state;
    bool result = isSafe("hello[]");
    assert_false(result);
}

// ======================= moss_split_line Tests =======================

// test 1: NULL input returns NULL
private void test_split_line_null(void **state)
{
    (void)state;
    char **result = moss_split_line(NULL);
    assert_null(result);
}

// test 2: Empty input returns array with NULL
private void test_split_line_empty(void **state)
{
    (void)state;
    char **result = moss_split_line("");
    assert_non_null(result);
    assert_null(result[0]);
    freeStrArray(result);
}

// test 3: Single word
private void test_split_line_single(void **state)
{
    (void)state;
    char input[] = "hello";
    char **result = moss_split_line(input);
    assert_non_null(result);
    assert_string_equal(result[0], "hello");
    assert_null(result[1]);
    freeStrArray(result);
}

// test 4: Multiple words
private void test_split_line_multiple(void **state)
{
    (void)state;
    char input[] = "hello world";
    char **result = moss_split_line(input);
    assert_non_null(result);
    assert_string_equal(result[0], "hello");
    assert_string_equal(result[1], "world");
    assert_null(result[2]);
    freeStrArray(result);
}

// test 5: Multiple spaces trimmed
private void test_split_line_spaces(void **state)
{
    (void)state;
    char input[] = "  hello   world  ";
    char **result = moss_split_line(input);
    assert_non_null(result);
    assert_string_equal(result[0], "hello");
    assert_string_equal(result[1], "world");
    assert_null(result[2]);
    freeStrArray(result);
}

// test 6: Tab delimiter
private void test_split_line_tab(void **state)
{
    (void)state;
    char input[] = "hello\tworld";
    char **result = moss_split_line(input);
    assert_non_null(result);
    assert_string_equal(result[0], "hello");
    assert_string_equal(result[1], "world");
    assert_null(result[2]);
    freeStrArray(result);
}

// ======================= strip_comment Tests =======================

// test 1: NULL input returns NULL
private void test_strip_comment_null(void **state)
{
    (void)state;
    char *result = strip_comment(NULL);
    assert_null(result);
}

// test 2: Comment removed
private void test_strip_comment_simple(void **state)
{
    (void)state;
    char input[] = "hello # comment";
    char *result = strip_comment(input);
    assert_string_equal(result, "hello ");
}

// test 3: Comment without space
private void test_strip_comment_noSpace(void **state)
{
    (void)state;
    char input[] = "hello#comment";
    char *result = strip_comment(input);
    assert_string_equal(result, "hello");
}

// test 4: Hash in double quotes is NOT a comment
private void test_strip_comment_doubleQuote(void **state)
{
    (void)state;
    char input[] = "hello \"# not comment\"";
    char *result = strip_comment(input);
    assert_string_equal(result, "hello \"# not comment\"");
}

// test 5: Hash in single quotes is NOT a comment
private void test_strip_comment_singleQuote(void **state)
{
    (void)state;
    char input[] = "hello '# comment'";
    char *result = strip_comment(input);
    assert_string_equal(result, "hello '# comment'");
}

// ======================= Main =======================

int main()
{
    const struct CMUnitTest tests[] = {
        /* expandEnvVar tests */
        cmocka_unit_test(test_expandEnvVar_null),
        cmocka_unit_test(test_expandEnvVar_noVar),
        cmocka_unit_test(test_expandEnvVar_undefined),
        cmocka_unit_test(test_expandEnvVar_defined),
        cmocka_unit_test(test_expandEnvVar_prefix),
        cmocka_unit_test(test_expandEnvVar_suffix),
        cmocka_unit_test(test_expandEnvVar_braces),
        cmocka_unit_test(test_expandEnvVar_dollar_dollar),
        cmocka_unit_test(test_expandEnvVar_multiple),

        /* strip_quotes tests */
        cmocka_unit_test(test_strip_quotes_null),
        cmocka_unit_test(test_strip_quotes_noQuotes),
        cmocka_unit_test(test_strip_quotes_double),
        cmocka_unit_test(test_strip_quotes_single),
        cmocka_unit_test(test_strip_quotes_mismatched),

        /* isSafe tests */
        cmocka_unit_test(test_isSafe_null),
        cmocka_unit_test(test_isSafe_empty),
        cmocka_unit_test(test_isSafe_normal),
        cmocka_unit_test(test_isSafe_semicolon),
        cmocka_unit_test(test_isSafe_pipe),
        cmocka_unit_test(test_isSafe_brackets),

        /* moss_split_line tests */
        cmocka_unit_test(test_split_line_null),
        cmocka_unit_test(test_split_line_empty),
        cmocka_unit_test(test_split_line_single),
        cmocka_unit_test(test_split_line_multiple),
        cmocka_unit_test(test_split_line_spaces),
        cmocka_unit_test(test_split_line_tab),

        /* strip_comment tests */
        cmocka_unit_test(test_strip_comment_null),
        cmocka_unit_test(test_strip_comment_simple),
        cmocka_unit_test(test_strip_comment_noSpace),
        cmocka_unit_test(test_strip_comment_doubleQuote),
        cmocka_unit_test(test_strip_comment_singleQuote),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
