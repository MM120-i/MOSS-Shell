#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdio.h>
#include <pwd.h>

#include "../src/modules/builtins/builtin.h"

private char saved_cwd[4096];

private int setup(void **state)
{
    (void)state;

    if (getcwd(saved_cwd, sizeof(saved_cwd)) == NULL)
        return -1;

    return 0;
}

/* Teardown: restore cwd after each test */
private int teardown(void **state)
{
    (void)state;
    chdir(saved_cwd);
    return 0;
}

/* Helper to capture stdout from a builtin */
private int capture_stdout(int (*func)(char **), char **args, char *buffer, size_t bufsize)
{
    int pipefd[2];

    if (pipe(pipefd) == -1)
        return -1;

    pid_t pid = fork();

    if (pid == 0)
    {
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[1]);
        int result = func(args);
        fflush(stdout);
        _exit(result);
    }
    else
    {
        close(pipefd[1]);
        size_t total = 0;
        ssize_t n;

        while ((n = read(pipefd[0], buffer + total, bufsize - total - 1)) > 0)
        {
            total += n;
            if (total >= bufsize - 1)
                break;
        }

        buffer[total] = '\0';
        close(pipefd[0]);
        int status;
        waitpid(pid, &status, 0);

        return WEXITSTATUS(status);
    }
}

/* Helper to trim trailing newline */
private void trim_newline(char *str)
{
    if (!str)
        return;

    size_t len = strlen(str);

    while (len > 0 && (str[len - 1] == '\n' || str[len - 1] == '\r'))
        str[--len] = '\0';
}

/* ==================== moss_echo Tests ==================== */

/* Test: echo with no arguments */
private void test_echo_noArgs(void **state)
{
    (void)state;
    char *args[] = {"echo", NULL};
    char output[256] = {0};
    int result = capture_stdout(moss_echo, args, output, sizeof(output));
    assert_int_equal(result, 1);
    assert_string_equal(output, "\n");
}

/* Test: echo with one argument */
private void test_echo_oneArg(void **state)
{
    (void)state;
    char *args[] = {"echo", "hello", NULL};
    char output[256] = {0};
    int result = capture_stdout(moss_echo, args, output, sizeof(output));
    assert_int_equal(result, 1);
    assert_string_equal(output, "hello\n");
}

/* Test: echo with multiple arguments */
private void test_echo_multipleArgs(void **state)
{
    (void)state;
    char *args[] = {"echo", "hello", "world", NULL};
    char output[256] = {0};
    int result = capture_stdout(moss_echo, args, output, sizeof(output));
    assert_int_equal(result, 1);
    assert_string_equal(output, "hello world\n");
}

/* Test: echo with env var */
private void test_echo_envVar(void **state)
{
    (void)state;
    setenv("TEST_ECHO_VAR", "testvalue", 1);
    char *args[] = {"echo", "$TEST_ECHO_VAR", NULL};
    char output[256] = {0};
    int result = capture_stdout(moss_echo, args, output, sizeof(output));
    assert_int_equal(result, 1);
    trim_newline(output);
    assert_string_equal(output, "$TEST_ECHO_VAR");
}

/* ==================== moss_pwd Tests ==================== */

/* Test: pwd returns current directory */
private void test_pwd_basic(void **state)
{
    (void)state;
    char *args[] = {"pwd", NULL};
    char output[256] = {0};
    int result = capture_stdout(moss_pwd, args, output, sizeof(output));
    assert_int_equal(result, 1);
    trim_newline(output);
    assert_string_equal(output, saved_cwd);
}

/* Test: pwd with argument should still work */
private void test_pwd_withArg(void **state)
{
    (void)state;
    char *args[] = {"pwd", "ignored", NULL};
    char output[256] = {0};
    int result = capture_stdout(moss_pwd, args, output, sizeof(output));
    assert_int_equal(result, 1);
    trim_newline(output);
    assert_string_equal(output, saved_cwd);
}

/* ==================== moss_help Tests ==================== */

/* Test: help prints builtin names */
private void test_help_basic(void **state)
{
    (void)state;
    char *args[] = {"help", NULL};
    char output[1024] = {0};
    int result = capture_stdout(moss_help, args, output, sizeof(output));
    assert_int_equal(result, 1);
    assert_non_null(strstr(output, "cd"));
    assert_non_null(strstr(output, "echo"));
    assert_non_null(strstr(output, "pwd"));
}

/* ==================== moss_whoami Tests ==================== */

/* Test: whoami with USER set */
private void test_whoami_withUser(void **state)
{
    (void)state;
    setenv("USER", "testuser", 1);
    char *args[] = {"whoami", NULL};
    char output[256] = {0};
    int result = capture_stdout(moss_whoami, args, output, sizeof(output));
    assert_int_equal(result, 1);
    trim_newline(output);
    assert_string_equal(output, "testuser");
}

/* Test: whoami without USER set */
private void test_whoami_noUser(void **state)
{
    (void)state;
    unsetenv("USER");
    unsetenv("USERNAME");
    char *args[] = {"whoami", NULL};
    char output[256] = {0};
    int result = capture_stdout(moss_whoami, args, output, sizeof(output));
    assert_int_equal(result, 1);
}

/* ==================== moss_env Tests ==================== */

/* Test: env prints environment */
private void test_env_basic(void **state)
{
    (void)state;
    setenv("TEST_VAR", "testvalue", 1);
    int result = moss_env((char *[]){"env", NULL});
    assert_int_equal(result, 1);
}

/* ==================== moss_export Tests ==================== */

/* Test: export with no args prints env */
private void test_export_noArgs(void **state)
{
    (void)state;
    setenv("TEST_EXPORT", "original", 1);
    int result = moss_export((char *[]){"export", NULL});
    assert_int_equal(result, 1);
}

/* Test: export NAME=value sets env var */
private void test_export_setVar(void **state)
{
    (void)state;
    char *arg = (char *)malloc(20);

    if (!arg)
    {
        perror("Memory allocation failed");
        exit(EXIT_FAILURE);
    }

    strcpy(arg, "FOO=bar");
    char **args = (char **)malloc(2 * sizeof(char *));

    if (!args)
    {
        perror("Memory allocation failed");
        exit(EXIT_FAILURE);
    }

    args[0] = arg;
    args[1] = NULL;

    int result = moss_export(args);
    assert_int_equal(result, 1);

    free(arg);
    free(args);
}

/* Test: export without = returns error */
private void test_export_noEqual(void **state)
{
    (void)state;
    char *args[] = {"export", "INVALID", NULL};
    int result = moss_export(args);
    assert_int_equal(result, 1);
}

/* ==================== moss_unset Tests ==================== */

/* Test: unset removes env var */
private void test_unset_basic(void **state)
{
    (void)state;
    setenv("TEST_UNSET", "value", 1);
    char *args[] = {"unset", "TEST_UNSET", NULL};
    int result = moss_unset(args);
    assert_int_equal(result, 1);
    char *value = getenv("TEST_UNSET");
    assert_null(value);
}

/* Test: unset with no arg returns error */
private void test_unset_noArg(void **state)
{
    (void)state;
    char *args[] = {"unset", NULL};
    int result = moss_unset(args);
    assert_int_equal(result, 1);
}

/* ==================== moss_type Tests ==================== */

/* Test: type with builtin */
private void test_type_builtin(void **state)
{
    (void)state;
    char *args[] = {"type", "cd", NULL};
    char output[256] = {0};
    int result = capture_stdout(moss_type, args, output, sizeof(output));
    assert_int_equal(result, 1);
    assert_non_null(strstr(output, "cd is a shell builtin"));
}

/* Test: type with echo */
private void test_type_echo(void **state)
{
    (void)state;
    char *args[] = {"type", "echo", NULL};
    char output[256] = {0};
    int result = capture_stdout(moss_type, args, output, sizeof(output));
    assert_int_equal(result, 1);
    assert_non_null(strstr(output, "echo is a shell builtin"));
}

/* Test: type with unknown command */
private void test_type_unknown(void **state)
{
    (void)state;
    char *args[] = {"type", "nonexistent123", NULL};
    char output[256] = {0};
    int result = capture_stdout(moss_type, args, output, sizeof(output));
    assert_int_equal(result, 1);
    assert_non_null(strstr(output, "nonexistent123 is an external command"));
}

/* ==================== moss_clear Tests ==================== */

/* Test: clear sends ANSI escape codes */
private void test_clear_basic(void **state)
{
    (void)state;
    char *args[] = {"clear", NULL};
    char output[256] = {0};
    int result = capture_stdout(moss_clear, args, output, sizeof(output));
    assert_int_equal(result, 1);
    assert_string_equal(output, "\033[H\033[J");
}

/* ==================== moss_cd Tests ==================== */

/* Test: cd with no args goes to HOME */
private void test_cd_noArgs(void **state)
{
    (void)state;
    char *home = getenv("HOME");

    if (!home)
    {
        skip();
        return;
    }

    char *args[] = {"cd", NULL};
    int result = moss_cd(args);
    char cwd[4096] = {0};
    getcwd(cwd, sizeof(cwd));
    chdir(saved_cwd);
    assert_int_equal(result, 1);
    assert_string_equal(cwd, home);
}

/* Test: cd to . stays in current dir */
private void test_cd_dot(void **state)
{
    (void)state;
    char *args[] = {"cd", ".", NULL};
    int result = moss_cd(args);
    char cwd[4096];
    getcwd(cwd, sizeof(cwd));
    chdir(saved_cwd);
    assert_int_equal(result, 1);
    assert_string_equal(cwd, saved_cwd);
}

/* Test: cd to .. goes to parent */
private void test_cd_dotDot(void **state)
{
    (void)state;
    char *args[] = {"cd", "..", NULL};
    int result = moss_cd(args);
    char cwd[4096];
    getcwd(cwd, sizeof(cwd));
    chdir(saved_cwd);
    assert_int_equal(result, 1);
    assert_true(strlen(cwd) <= strlen(saved_cwd));
}

/* Test: cd to ~ goes to HOME */
private void test_cd_tilde(void **state)
{
    (void)state;
    char *home = getenv("HOME");

    if (!home)
    {
        skip();
        return;
    }

    char *args[] = {"cd", "~", NULL};
    int result = moss_cd(args);
    char cwd[4096];
    getcwd(cwd, sizeof(cwd));
    chdir(saved_cwd);
    assert_int_equal(result, 1);
    assert_string_equal(cwd, home);
}

/* Test: cd to invalid path returns error */
private void test_cd_invalid(void **state)
{
    (void)state;
    char cwd_before[4096];
    getcwd(cwd_before, sizeof(cwd_before));

    char *args[] = {"cd", "/nonexistent/path/12345", NULL};
    moss_cd(args);

    char cwd_after[4096];
    getcwd(cwd_after, sizeof(cwd_after));
    chdir(saved_cwd);
    assert_string_equal(cwd_before, cwd_after);
}

/* Test: cd - goes to previous directory */
private void test_cd_dash(void **state)
{
    (void)state;
    chdir("/");
    char *args1[] = {"cd", saved_cwd, NULL};
    moss_cd(args1);
    char *args2[] = {"cd", "-", NULL};
    int result = moss_cd(args2);
    char cwd[4096];
    getcwd(cwd, sizeof(cwd));
    chdir(saved_cwd);
    assert_int_equal(result, 1);
    assert_string_equal(cwd, "/");
}

/* ==================== Main ==================== */

int main(void)
{
    const struct CMUnitTest tests[] = {
        /* moss_echo tests */
        cmocka_unit_test(test_echo_noArgs),
        cmocka_unit_test(test_echo_oneArg),
        cmocka_unit_test(test_echo_multipleArgs),
        cmocka_unit_test(test_echo_envVar),

        /* moss_pwd tests */
        cmocka_unit_test(test_pwd_basic),
        cmocka_unit_test(test_pwd_withArg),

        /* moss_help tests */
        cmocka_unit_test(test_help_basic),

        /* moss_whoami tests */
        cmocka_unit_test(test_whoami_withUser),
        cmocka_unit_test(test_whoami_noUser),

        /* moss_env tests */
        cmocka_unit_test(test_env_basic),

        /* moss_export tests */
        cmocka_unit_test(test_export_noArgs),
        cmocka_unit_test(test_export_setVar),
        cmocka_unit_test(test_export_noEqual),

        /* moss_unset tests */
        cmocka_unit_test(test_unset_basic),
        cmocka_unit_test(test_unset_noArg),

        /* moss_type tests */
        cmocka_unit_test(test_type_builtin),
        cmocka_unit_test(test_type_echo),
        cmocka_unit_test(test_type_unknown),

        /* moss_clear tests */
        cmocka_unit_test(test_clear_basic),

        /* moss_cd tests */
        cmocka_unit_test(test_cd_noArgs),
        cmocka_unit_test(test_cd_dot),
        cmocka_unit_test(test_cd_dotDot),
        cmocka_unit_test(test_cd_tilde),
        cmocka_unit_test(test_cd_invalid),
        cmocka_unit_test(test_cd_dash),
    };

    return cmocka_run_group_tests(tests, setup, teardown);
}
