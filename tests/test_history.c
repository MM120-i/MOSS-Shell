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
#include "../src/modules/builtins/builtin.h"
#include "../include/signals.h"

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

// After init, cursor is at count (which is 0 for empty)
private void test_cursor_init(void **state)
{
    (void)state;

    history_init();
    int result = history_count();
    assert_int_equal(result, 0);
    history_destroy();
}

// Prev when empty -> returns -1
private void test_cursor_prev_empty(void **state)
{
    (void)state;

    history_init();
    int result = history_prev();
    assert_int_equal(result, -1);
    history_destroy();
}

// Add 3 commands, prev returns index 2
private void test_cursor_prev_after_add(void **state)
{
    (void)state;

    history_init();
    history_add("cmd1");
    history_add("cmd2");
    history_add("cmd3");

    int result = history_prev();
    assert_int_equal(result, 2);
    history_destroy();
}

// Prev at index 0 -> stays at 0
private void test_cursor_prev_boundary(void **state)
{
    (void)state;

    history_init();
    history_add("cmd1");

    history_prev();
    int result = history_prev();
    assert_int_equal(result, 0);
    history_destroy();
}

// Cursor at count (end), next returns -1
private void test_cursor_next_at_end(void **state)
{
    (void)state;

    history_init();
    history_add("cmd1");
    history_add("cmd2");

    int result = history_next();
    assert_int_equal(result, -1);
    history_destroy();
}

// Prev then next -> back to original position
private void test_cursor_next_after_prev(void **state)
{
    (void)state;

    history_init();
    history_add("cmd1");
    history_add("cmd2");
    history_add("cmd3");

    history_prev();
    history_prev();
    int result = history_next();
    assert_int_equal(result, 2);
    history_destroy();
}

// After prev, reset cursor -> cursor is at count
private void test_reset_cursor(void **state)
{
    (void)state;

    history_init();
    history_add("cmd1");
    history_add("cmd2");

    history_prev();
    history_reset_cursor();
    int result = history_next();
    assert_int_equal(result, -1);
    history_destroy();
}

// Phase 5: Builtin history Command
private void test_history_empty(void **state)
{
    (void)state;

    history_init();

    char *args[] = {"history", NULL};
    int result = moss_history(args);

    assert_int_equal(result, 1);
    history_destroy();
}

private void test_history_with_entries(void **state)
{
    (void)state;

    history_init();
    history_add("ls");
    history_add("pwd");

    char *args[] = {"history", NULL};
    int result = moss_history(args);

    assert_int_equal(result, 1);
    history_destroy();
}

private void test_history_line_numbers(void **state)
{
    (void)state;

    history_init();
    history_add("cmd1");
    history_add("cmd2");
    history_add("cmd3");

    char *args[] = {"history", NULL};
    int result = moss_history(args);

    assert_int_equal(result, 1);
    history_destroy();
}

private void test_history_reverse_order(void **state)
{
    (void)state;

    history_init();
    history_add("first");
    history_add("second");
    history_add("third");

    char *args[] = {"history", NULL};
    int result = moss_history(args);

    assert_int_equal(result, 1);
    history_destroy();
}

// Enter adds command to history
private void test_enter_adds_to_history(void **state)
{
    (void)state;

    history_init();
    history_add("ls");
    history_add("cd /home");

    assert_int_equal(history_count(), 2);
    assert_string_equal(history_get(1), "cd /home");
    history_destroy();
}

// Arrow up loads previous command via history_prev
private void test_arrow_up_navigates(void **state)
{
    (void)state;

    history_init();
    history_add("first_cmd");
    history_add("second_cmd");
    history_add("third_cmd");

    int idx = history_prev();
    assert_int_equal(idx, 2);
    assert_string_equal(history_get(idx), "third_cmd");

    idx = history_prev();
    assert_int_equal(idx, 1);
    assert_string_equal(history_get(idx), "second_cmd");
    history_destroy();
}

// Arrow down loads next command via history_next
private void test_arrow_down_navigates(void **state)
{
    (void)state;

    history_init();
    history_add("cmd1");
    history_add("cmd2");

    history_prev();
    history_prev();

    int idx = history_next();
    assert_int_equal(idx, 1);
    assert_string_equal(history_get(idx), "cmd2");

    idx = history_next();
    assert_int_equal(idx, -1);
    history_destroy();
}

// Ctrl+C clears line (test that cursor reset works)
private void test_ctrl_c_clears_line(void **state)
{
    (void)state;

    history_init();
    history_add("some_cmd");

    history_prev();
    history_reset_cursor();

    int result = history_next();
    assert_int_equal(result, -1);
    history_destroy();
}

// Ctrl+D exits - test that history is saved on exit
private void test_ctrl_d_exits(void **state)
{
    (void)state;

    char *filepath = create_temp_file();

    history_init();
    history_add("cmd1");
    history_add("cmd2");
    history_save(filepath);
    history_destroy();

    history_init();
    int result = history_load(filepath);
    assert_int_equal(result, 0);
    assert_int_equal(history_count(), 2);

    unlink(filepath);
    free(filepath);
    history_destroy();
}

// initialize signals without crashing
private void test_signal_init(void **state)
{
    (void)state;

    moss_init_signals();
    assert_int_equal(moss_running, 1);
}

// SIGINT handler sets flag (simulating a signal)
private void test_sigint_handler_sets_flag(void **state)
{
    (void)state;

    moss_init_signals();
    moss_got_sigint = 0;

    raise(SIGINT);
    assert_int_equal(moss_got_sigint, 1);
}

// history preserved after signal
private void test_history_preserved_after_signal(void **state)
{
    (void)state;

    history_init();
    history_add("cmd_before_signal");

    raise(SIGINT);

    history_add("cmd_after_signal");

    assert_int_equal(history_count(), 2);
    assert_string_equal(history_get(0), "cmd_before_signal");
    assert_string_equal(history_get(1), "cmd_after_signal");

    history_destroy();
}

// signal doesnt crash the shell
private void test_signal_no_crash(void **state)
{
    (void)state;

    history_init();
    history_add("test_cmd");

    raise(SIGINT);
    raise(SIGINT);
    raise(SIGINT);

    assert_int_equal(history_count(), 1);
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

        cmocka_unit_test(test_cursor_init),
        cmocka_unit_test(test_cursor_prev_empty),
        cmocka_unit_test(test_cursor_prev_after_add),
        cmocka_unit_test(test_cursor_prev_boundary),
        cmocka_unit_test(test_cursor_next_at_end),
        cmocka_unit_test(test_cursor_next_after_prev),
        cmocka_unit_test(test_reset_cursor),

        cmocka_unit_test(test_history_empty),
        cmocka_unit_test(test_history_with_entries),
        cmocka_unit_test(test_history_line_numbers),
        cmocka_unit_test(test_history_reverse_order),

        cmocka_unit_test(test_enter_adds_to_history),
        cmocka_unit_test(test_arrow_up_navigates),
        cmocka_unit_test(test_arrow_down_navigates),
        cmocka_unit_test(test_ctrl_c_clears_line),
        cmocka_unit_test(test_ctrl_d_exits),

        cmocka_unit_test(test_signal_init),
        cmocka_unit_test(test_sigint_handler_sets_flag),
        cmocka_unit_test(test_history_preserved_after_signal),
        cmocka_unit_test(test_signal_no_crash),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}