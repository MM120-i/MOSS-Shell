#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#include "include/signals.h"

static int setup(void **state)
{
    (void)state;
    moss_running = 1;
    moss_got_sigint = 0;
    moss_foreground_pgid = 0;

    return 0;
}

// test 1: check gloabal variables start with correct values
static void testGlobalsInit(void **state)
{
    (void)state;
    assert_int_equal(moss_running, 1);
    assert_int_equal(moss_got_sigint, 0);
    assert_int_equal(moss_foreground_pgid, 0);
}

// test 2: setting moss_foreground_pgid presists
static void testForegroundPgid(void **state)
{
    (void)state;
    moss_foreground_pgid = 12345;
    assert_int_equal(moss_foreground_pgid, 12345);
}

static void myHandler(int sig)
{
    (void)sig;
}

// test 3: install_handler returns 0 on success
static void testInstallHandler(void **state)
{
    (void)state;
    const int result = install_handler(SIGTERM, myHandler, 0);
    assert_int_equal(result, 0);
}

// test 4: moss_init_signals runs without crashing
static void testInitSignals(void **state)
{
    (void)state;
    moss_init_signals();
    assert_true(1);
}

int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(testGlobalsInit),
        cmocka_unit_test(testForegroundPgid),
        cmocka_unit_test(testInstallHandler),
        cmocka_unit_test(testInitSignals),
    };

    return cmocka_run_group_tests(tests, setup, NULL);
}