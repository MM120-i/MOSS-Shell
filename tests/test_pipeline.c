/* This is shell Integration test (not unit test) cuz unit testing sys calls r complex 🥀🥀 */
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdio.h>
#include <fcntl.h>

#include "include/pipeline.h"

// helper to trim trailing whitespace (newlines)
private void trim_newline(char *str)
{
    if (!str)
        return;

    size_t len = strlen(str);

    while (len > 0 && (str[len - 1] == '\n' || str[len - 1] == '\r'))
        str[--len] = '\0';
}

// helper function
private int capture_stdout(char **args, char *buffer, size_t bufSize)
{
    int pipeFd[2];

    if (pipe(pipeFd) == -1)
        return -1;

    pid_t pid = fork();

    if (pid == 0)
    {
        close(pipeFd[0]);
        dup2(pipeFd[1], STDOUT_FILENO);
        close(pipeFd[1]);
        int result = moss_execute_pipeline(args);
        fflush(stdout);
        _exit(result);
    }
    else
    {
        close(pipeFd[1]);
        size_t total = 0;
        size_t n;

        while ((n = read(pipeFd[0], buffer + total, bufSize - total - 1)) > 0)
        {
            total += n;

            if (total >= bufSize - 1)
                break;
        }

        buffer[total] = '\0';
        close(pipeFd[0]);
        int status;
        waitpid(pid, &status, 0);

        return WEXITSTATUS(status);
    }
}

// test 1: simple pipe ls | wc -l
private void test_pipeSimple(void **state)
{
    (void)state;

    // creating a temp file with known content
    char fileName[] = "/tmp/testfileXXXXXX";
    int fd = mkstemp(fileName);
    write(fd, "line1\nline2\nline3\n", 18);
    close(fd);

    // builing the args: cat filename | wc -l
    char *args[] = {
        "cat",
        fileName,
        "|",
        "wc",
        "-l",
        NULL,
    };

    char output[256] = {0};
    int result = capture_stdout(args, output, sizeof(output));

    // moss_execute_pipeline returns 1 on success
    assert_int_equal(result, 1);
    trim_newline(output);
    assert_string_equal(output, "3");
    unlink(fileName);
}

// test 2: Multiple pipes echo | cat | wc -c
private void test_pipeMultiple(void **state)
{
    (void)state;
    char *args[] = {
        "echo",
        "hello",
        "|",
        "cat",
        "|",
        "wc",
        "-c",
        NULL,
    };

    char output[256] = {0};
    int result = capture_stdout(args, output, sizeof(output));
    assert_int_equal(result, 1);
    trim_newline(output);
    assert_string_equal(output, "6");
}

// test 3: leading pipe should error
private void test_pipeLeadingError(void **state)
{
    (void)state;
    char *args[] = {
        "|",
        "ls",
        NULL,
    };

    char output[256] = {0};
    int result = capture_stdout(args, output, sizeof(output));
    assert_int_not_equal(result, 0);
}

// test 4: trailing pipe should error
private void test_pipeTrailingError(void **state)
{
    (void)state;
    char *args[] = {
        "ls",
        "|",
        NULL,
    };

    char output[256] = {0};
    int result = capture_stdout(args, output, sizeof(output));
    assert_int_not_equal(result, 0);
}

// test 5: echo to wc - w (word count)
private void test_pipeEchoWc(void **state)
{
    (void)state;
    char *args[] = {
        "echo",
        "one two three four five",
        "|",
        "wc",
        "-w",
        NULL,
    };

    char output[256] = {0};
    int result = capture_stdout(args, output, sizeof(output));
    assert_int_equal(result, 1);
    trim_newline(output);
    assert_string_equal(output, "5");
}

// test 6: grep in pipe
private void test_pipeGrep(void **state)
{
    (void)state;
    char fileName[] = "/tmp/testgrepXXXXXX";
    int fd = mkstemp(fileName);
    write(fd, "apple\nbanana\napricot\nblueberry\n", 31);
    close(fd);
    char *args[] = {
        "cat",
        fileName,
        "|",
        "grep",
        "^a",
        NULL,
    };

    char output[256] = {0};
    int result = capture_stdout(args, output, sizeof(output));
    assert_int_equal(result, 1);
    assert_non_null(strstr(output, "apple"));
    assert_non_null(strstr(output, "apricot"));
    unlink(fileName);
}

int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_pipeSimple),
        cmocka_unit_test(test_pipeMultiple),
        cmocka_unit_test(test_pipeLeadingError),
        cmocka_unit_test(test_pipeTrailingError),
        cmocka_unit_test(test_pipeEchoWc),
        cmocka_unit_test(test_pipeGrep),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}