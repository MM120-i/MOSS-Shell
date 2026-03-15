#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

static void test_addition(void **state)
{
    assert_int_equal(2 + 2, 4);
}

int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_addition),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}