#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "include/jobs.h"

private int setup(void **state)
{
    (void)state;
    jobs_init();
    return 0;
}

private int teardown(void **state)
{
    (void)state;
    jobs_destroy();
    return 0;
}

private void test_jobs_init(void **state)
{
    (void)state;
    assert_non_null(jobs);
    assert_int_equal(jobs_count(), 0);
}

private void test_jobs_add_single(void **state)
{
    (void)state;
    int id = jobs_add(1234, 1234, "sleep 100");
    assert_int_equal(id, 1);
    assert_int_equal(jobs_count(), 1);
}

private void test_jobs_add_multiple(void **state)
{
    (void)state;
    int id1 = jobs_add(1001, 1001, "sleep 100");
    int id2 = jobs_add(1002, 1002, "vim");
    int id3 = jobs_add(1003, 1003, "make");

    assert_int_equal(id1, 1);
    assert_int_equal(id2, 2);
    assert_int_equal(id3, 3);
    assert_int_equal(jobs_count(), 3);
}

private void test_jobs_get_by_id(void **state)
{
    (void)state;
    jobs_add(1001, 1001, "sleep 100");
    jobs_add(1002, 1002, "vim");

    Job *job = jobs_get(1);
    assert_non_null(job);
    assert_int_equal(job->pid, 1001);
    assert_string_equal(job->cmd, "sleep 100");
    assert_int_equal(job->state, JOB_RUNNING);

    job = jobs_get(2);
    assert_non_null(job);
    assert_int_equal(job->pid, 1002);
    assert_string_equal(job->cmd, "vim");
}

private void test_jobs_get_invalid_id(void **state)
{
    (void)state;
    jobs_add(1001, 1001, "sleep 100");

    Job *job = jobs_get(999);
    assert_null(job);
}

private void test_jobs_remove(void **state)
{
    (void)state;
    jobs_add(1001, 1001, "sleep 100");
    jobs_add(1002, 1002, "vim");

    assert_int_equal(jobs_count(), 2);

    jobs_remove(1001);
    assert_int_equal(jobs_count(), 1);

    Job *job = jobs_get(1);
    assert_null(job);

    job = jobs_get(2);
    assert_non_null(job);
    assert_string_equal(job->cmd, "vim");
}

private void test_jobs_remove_by_pid(void **state)
{
    (void)state;
    jobs_add(1001, 1001, "sleep 100");
    jobs_add(1002, 1002, "vim");

    jobs_remove(1002);

    assert_int_equal(jobs_count(), 1);
    assert_non_null(jobs_get(1));
}

private void test_jobs_update_state(void **state)
{
    (void)state;
    jobs_add(1001, 1001, "sleep 100");

    Job *job = jobs_get(1);
    assert_non_null(job);
    assert_int_equal(job->state, JOB_RUNNING);

    job->state = JOB_STOPPED;
    assert_int_equal(jobs_get(1)->state, JOB_STOPPED);

    job->state = JOB_DONE;
    assert_int_equal(jobs_get(1)->state, JOB_DONE);
}

private void test_jobs_max_capacity(void **state)
{
    (void)state;
    int added = 0;

    for (size_t i = 0; i < 70; i++)
    {
        int id = jobs_add(1000 + i, 1000 + i, "test");
        if (id > 0)
            added++;
    }

    assert_int_equal(added, 64);
    assert_int_equal(jobs_count(), 64);
}

private void test_job_id_sequence(void **state)
{
    (void)state;
    jobs_add(1001, 1001, "cmd1");
    jobs_add(1002, 1002, "cmd2");
    jobs_remove(1001);
    jobs_add(1003, 1003, "cmd3");

    assert_int_equal(jobs_get(3)->pid, 1003);
}

int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_jobs_init, setup, teardown),
        cmocka_unit_test_setup_teardown(test_jobs_add_single, setup, teardown),
        cmocka_unit_test_setup_teardown(test_jobs_add_multiple, setup, teardown),
        cmocka_unit_test_setup_teardown(test_jobs_get_by_id, setup, teardown),
        cmocka_unit_test_setup_teardown(test_jobs_get_invalid_id, setup, teardown),
        cmocka_unit_test_setup_teardown(test_jobs_remove, setup, teardown),
        cmocka_unit_test_setup_teardown(test_jobs_remove_by_pid, setup, teardown),
        cmocka_unit_test_setup_teardown(test_jobs_update_state, setup, teardown),
        cmocka_unit_test_setup_teardown(test_jobs_max_capacity, setup, teardown),
        cmocka_unit_test_setup_teardown(test_job_id_sequence, setup, teardown),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
