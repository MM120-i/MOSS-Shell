#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "include/retry.h"

/* ==================== mossRetryShouldRetry Tests ==================== */

// test 1: EAGAIN should retry
private void test_shouldRetry_eagain(void **state)
{
    (void)state;
    bool result = mossRetryShouldRetry(EAGAIN);
    assert_true(result);
}

// test 2: EINTR should retry
private void test_shouldRetry_eintr(void **state)
{
    (void)state;
    bool result = mossRetryShouldRetry(EINTR);
    assert_true(result);
}

// test 3: ENOMEM should retry
private void test_shouldRetry_enomem(void **state)
{
    (void)state;
    bool result = mossRetryShouldRetry(ENOMEM);
    assert_true(result);
}

// test 4: EMFILE should retry
private void test_shouldRetry_emfile(void **state)
{
    (void)state;
    bool result = mossRetryShouldRetry(EMFILE);
    assert_true(result);
}

// test 5: ENFILE should retry
private void test_shouldRetry_enfile(void **state)
{
    (void)state;
    bool result = mossRetryShouldRetry(ENFILE);
    assert_true(result);
}

// test 6: Other errno should NOT retry
private void test_shouldRetry_other(void **state)
{
    (void)state;
    bool result = mossRetryShouldRetry(ENOENT);
    assert_false(result);
}

/* ==================== mossRetryInit Tests ==================== */

// test 1: Initialize with config values
private void test_retryInit_config(void **state)
{
    (void)state;
    RetryConfig config = {
        .maxRetries = 5,
        .baseDelayms = 100,
        .maxDelayms = 5000,
        .useExponentialBackoff = true,
    };

    RetryContext ctx;
    mossRetryInit(&ctx, &config);

    assert_int_equal(ctx.config.maxRetries, 5);
    assert_int_equal(ctx.config.baseDelayms, 100);
    assert_int_equal(ctx.config.maxDelayms, 5000);
    assert_true(ctx.config.useExponentialBackoff);
}

// test 2: attemptCount starts at 0
private void test_retryInit_attemptCount(void **state)
{
    (void)state;
    RetryConfig config = {
        .maxRetries = 3,
        .baseDelayms = 10,
        .maxDelayms = 100,
        .useExponentialBackoff = false,
    };

    RetryContext ctx;
    mossRetryInit(&ctx, &config);

    assert_int_equal(ctx.attemptCount, 0);
    assert_int_equal(ctx.lastErrno, 0);
}

/* ==================== calculateDelay Tests ==================== */

// test 1: Linear backoff - returns baseDelay
private void test_calculateDelay_linear(void **state)
{
    (void)state;
    RetryConfig config = {
        .maxRetries = 3,
        .baseDelayms = 100,
        .maxDelayms = 5000,
        .useExponentialBackoff = false,
    };

    RetryContext ctx;
    mossRetryInit(&ctx, &config);
    ctx.attemptCount = 1;

    int result = calculateDelay(&ctx);
    assert_int_equal(result, 100);
}

// test 2: Exponential backoff - first attempt
private void test_calculateDelay_exponential_first(void **state)
{
    (void)state;
    RetryConfig config = {
        .maxRetries = 3,
        .baseDelayms = 100,
        .maxDelayms = 5000,
        .useExponentialBackoff = true};

    RetryContext ctx;
    mossRetryInit(&ctx, &config);
    ctx.attemptCount = 1;

    int result = calculateDelay(&ctx);
    assert_int_equal(result, 100); /* 100 * 2^(1-1) = 100 */
}

// test 3: Exponential backoff - increases each attempt
private void test_calculateDelay_exponential_third(void **state)
{
    (void)state;
    RetryConfig config = {
        .maxRetries = 10,
        .baseDelayms = 100,
        .maxDelayms = 5000,
        .useExponentialBackoff = true,
    };

    RetryContext ctx;
    mossRetryInit(&ctx, &config);
    ctx.attemptCount = 3;

    int result = calculateDelay(&ctx);
    assert_int_equal(result, 400); /* 100 * 2^(3-1) = 400 */
}

// test 4: Exponential backoff - capped at maxDelay
private void test_calculateDelay_capped(void **state)
{
    (void)state;
    RetryConfig config = {
        .maxRetries = 10,
        .baseDelayms = 100,
        .maxDelayms = 5000,
        .useExponentialBackoff = true,
    };

    RetryContext ctx;
    mossRetryInit(&ctx, &config);
    ctx.attemptCount = 10;

    int result = calculateDelay(&ctx);
    assert_int_equal(result, 5000);
}

/* ==================== mossRetryExecute Tests ==================== */

private int mockOp_alwaysSucceed(void *context)
{
    int *counter = (int *)context;
    (*counter)++;
    return 0;
}

private int mockOp_failNTimes(void *context)
{
    int *counter = (int *)context;
    (*counter)++;

    if (*counter <= 2)
    {
        errno = EAGAIN;
        return -1;
    }

    return 0;
}

private int mockOp_alwaysFail(void *context)
{
    int *counter = (int *)context;
    (*counter)++;
    errno = EAGAIN;
    return -1;
}

private int mockOp_nonRetryable(void *context)
{
    int *counter = (int *)context;
    (*counter)++;
    errno = ENOENT;
    return -1;
}

private bool mockPredicate_retryEagain(int err)
{
    return (err == EAGAIN);
}

/* Test 1: Success on first try */
private void test_execute_successFirst(void **state)
{
    (void)state;
    int counter = 0;

    RetryConfig config = {.maxRetries = 3, .baseDelayms = 10, .maxDelayms = 100, .useExponentialBackoff = false};
    RetryContext ctx;
    mossRetryInit(&ctx, &config);

    RetryResult result = mossRetryExecute(&ctx, mockOp_alwaysSucceed, &counter, mockPredicate_retryEagain);

    assert_int_equal(result, RETRY_OK);
    assert_int_equal(counter, 1);
}

/* Test 2: Fails twice, succeeds on third attempt */
private void test_execute_succeedAfterRetries(void **state)
{
    (void)state;
    int counter = 0;

    RetryConfig config = {.maxRetries = 3, .baseDelayms = 10, .maxDelayms = 100, .useExponentialBackoff = false};
    RetryContext ctx;
    mossRetryInit(&ctx, &config);

    RetryResult result = mossRetryExecute(&ctx, mockOp_failNTimes, &counter, mockPredicate_retryEagain);

    assert_int_equal(result, RETRY_OK);
    assert_int_equal(counter, 3);
}

/* Test 3: Non-retryable error */
private void test_execute_nonRetryable(void **state)
{
    (void)state;
    int counter = 0;

    RetryConfig config = {.maxRetries = 3, .baseDelayms = 10, .maxDelayms = 100, .useExponentialBackoff = false};
    RetryContext ctx;
    mossRetryInit(&ctx, &config);

    RetryResult result = mossRetryExecute(&ctx, mockOp_nonRetryable, &counter, mockPredicate_retryEagain);

    assert_int_equal(result, RETRY_NON_RETRYABLE);
    assert_int_equal(counter, 1);
}

/* Test 4: Retries exhausted */
private void test_execute_exhausted(void **state)
{
    (void)state;
    int counter = 0;

    RetryConfig config = {.maxRetries = 3, .baseDelayms = 10, .maxDelayms = 100, .useExponentialBackoff = false};
    RetryContext ctx;
    mossRetryInit(&ctx, &config);

    RetryResult result = mossRetryExecute(&ctx, mockOp_alwaysFail, &counter, mockPredicate_retryEagain);

    assert_int_equal(result, RETRY_EXHAUSTED);
    assert_int_equal(counter, 3);
}

/* ==================== Main ==================== */

int main(void)
{
    const struct CMUnitTest tests[] = {
        /* mossRetryShouldRetry tests */
        cmocka_unit_test(test_shouldRetry_eagain),
        cmocka_unit_test(test_shouldRetry_eintr),
        cmocka_unit_test(test_shouldRetry_enomem),
        cmocka_unit_test(test_shouldRetry_emfile),
        cmocka_unit_test(test_shouldRetry_enfile),
        cmocka_unit_test(test_shouldRetry_other),

        /* mossRetryInit tests */
        cmocka_unit_test(test_retryInit_config),
        cmocka_unit_test(test_retryInit_attemptCount),

        /* calculateDelay tests */
        cmocka_unit_test(test_calculateDelay_linear),
        cmocka_unit_test(test_calculateDelay_exponential_first),
        cmocka_unit_test(test_calculateDelay_exponential_third),
        cmocka_unit_test(test_calculateDelay_capped),

        /* mossRetryExecute tests */
        cmocka_unit_test(test_execute_successFirst),
        cmocka_unit_test(test_execute_succeedAfterRetries),
        cmocka_unit_test(test_execute_nonRetryable),
        cmocka_unit_test(test_execute_exhausted),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
