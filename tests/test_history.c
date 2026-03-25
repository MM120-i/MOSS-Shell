#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

#include "../include/history.h"

private char *create_temp_file()
{
    char filePath[] = "/tmp/moss_test_XXXXXX";
    int fd = mkstemp(filePath);

    if (fd >= 0)
    {
        close(fd);
        unlink(filePath);
    }

    return strdup(filePath);
}

// example: add 'ls' -> add 'ls' again -> expect false
private void test_duplicate_immediate(void **state)
{
    (void)state;

    history_init();
    history_add("ls");

    bool result = history_should_add("ls");
    assert_false(result);
    history_destroy();
}

// example: add 'ls', 'cd' -> add 'ls' -> expect true (different from last)
private void test_duplicate_after_other(void **state)
{
    (void)state;

    history_init();
    history_add("ls");
    history_add("cd");

    bool result = history_should_add("ls");
    assert_true(result);
    history_destroy();
}

// example: add 'ls' -> add 'cd' -> expect true
private void test_not_duplicate(void **state)
{
    (void)state;

    history_init();
    history_add("ls");

    bool result = history_should_add("cd");
    assert_true(result);
    history_destroy();
}

// example: add 'ls', 'ls' '' -> first duplicate returns false, empty also returns false
private void test_empty_after_duplicate(void **state)
{
    (void)state;

    history_init();
    history_add("ls");
    history_add("ls");

    bool dupResult = history_should_add("");
    assert_false(dupResult);
    history_add("");
    bool emptyResult = history_should_add("");
    assert_false(emptyResult);
    history_destroy();
}

// example: fill to 1000 -> add duplicate -> still works correctly
private void test_duplicate_at_capacity(void **state)
{
    (void)state;

    history_init();

    for (size_t i = 0; i < DEFAULT_HISTORY_SIZE; i++)
    {
        char cmd[20];
        snprintf(cmd, sizeof(cmd), "cmd_%ld", i);
        history_add(cmd);
    }

    bool result = history_should_add("cmd_999");
    assert_false(result);
    history_destroy();
}

// empty history -> file with only header
private void test_save_empty(void **state)
{
    (void)state;

    char *filePath = create_temp_file();
    history_init();
    int result = history_save(filePath);
    assert_int_equal(result, 0);

    FILE *file = fopen(filePath, "r");
    assert_non_null(file);

    char line[256];
    assert_string_equal(fgets(line, sizeof(line), file), "# moss shell history\n");
    assert_false(fgets(line, sizeof(line), file));

    fclose(file);
    unlink(filePath);
    free(filePath);
    history_destroy();
}

// save a few commands -> verify exact file contents
private void test_save_with_entries(void **state)
{
    (void)state;

    char *filePath = create_temp_file();

    history_init();
    history_add("ls -la");
    history_add("cd /home");
    history_add("pwd");

    int result = history_save(filePath);
    assert_int_equal(result, 0);

    FILE *file = fopen(filePath, "r");
    assert_non_null(file);

    char line[256];
    assert_string_equal(fgets(line, sizeof(line), file), "# moss shell history\n");
    assert_string_equal(fgets(line, sizeof(line), file), "ls -la\n");
    assert_string_equal(fgets(line, sizeof(line), file), "cd /home\n");
    assert_string_equal(fgets(line, sizeof(line), file), "pwd\n");
    assert_false(fgets(line, sizeof(line), file));

    fclose(file);
    unlink(filePath);
    free(filePath);
    history_destroy();
}

// load a valid file -> correct count + content
private void test_load_valid(void **state)
{
    (void)state;
    char *filepath = create_temp_file();

    FILE *f = fopen(filepath, "w");
    fprintf(f, "# moss shell history\n");
    fprintf(f, "ls -la\n");
    fprintf(f, "cd /home\n");
    fprintf(f, "pwd\n");
    fclose(f);

    history_init();
    int result = history_load(filepath);

    assert_int_equal(result, 0);
    assert_int_equal(history_count(), 3);
    assert_string_equal(history_get(0), "ls -la");
    assert_string_equal(history_get(1), "cd /home");
    assert_string_equal(history_get(2), "pwd");

    unlink(filepath);
    free(filepath);
    history_destroy();
}

// load a missing file -> return -1
private void test_load_nonexistent(void **state)
{
    (void)state;
    history_init();

    int result = history_load("/tmp/nonexistent_moss_history_12345.txt");

    assert_int_equal(result, -1);
    history_destroy();
}

// loads a corrupted file -> return -1
private void test_load_corrupted(void **state)
{
    (void)state;
    char *filepath = create_temp_file();

    FILE *f = fopen(filepath, "w");
    fprintf(f, "ls -la\n");
    fprintf(f, "pwd\n");
    fclose(f);

    history_init();
    int result = history_load(filepath);

    assert_int_equal(result, -1);

    unlink(filepath);
    free(filepath);
    history_destroy();
}

// do a 'roundtrip' so like: Save -> load -> identical history
private void test_save_load_roundtrip(void **state)
{
    (void)state;
    char *filepath = create_temp_file();

    history_init();
    history_add("cmd1");
    history_add("cmd2");
    history_add("cmd3");
    history_save(filepath);
    history_destroy();

    history_init();
    int result = history_load(filepath);

    assert_int_equal(result, 0);
    assert_int_equal(history_count(), 3);
    assert_string_equal(history_get(0), "cmd1");
    assert_string_equal(history_get(1), "cmd2");
    assert_string_equal(history_get(2), "cmd3");

    unlink(filepath);
    free(filepath);
    history_destroy();
}

int main()
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_duplicate_immediate),
        cmocka_unit_test(test_duplicate_after_other),
        cmocka_unit_test(test_not_duplicate),
        cmocka_unit_test(test_empty_after_duplicate),
        cmocka_unit_test(test_duplicate_at_capacity),

        cmocka_unit_test(test_save_empty),
        cmocka_unit_test(test_save_with_entries),
        cmocka_unit_test(test_load_valid),
        cmocka_unit_test(test_load_nonexistent),
        cmocka_unit_test(test_load_corrupted),
        cmocka_unit_test(test_save_load_roundtrip),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}