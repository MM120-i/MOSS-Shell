#pragma once

#include <stdbool.h>

#define private static

typedef enum
{
    RETRY_OK,
    RETRY_EXHAUSTED,
    RETRY_NON_RETRYABLE,
} RetryResult;

typedef bool (*RetryPredicate)(int errnoValue);
typedef int (*RetryOperation)(void *context);

typedef struct
{
    int maxRetries;
    int baseDelayms;
    int maxDelayms;
    bool useExponentialBackoff;
} RetryConfig;

typedef struct
{
    RetryConfig config;
    int attemptCount;
    int lastErrno;
} RetryContext;

int calculateDelay(RetryContext *);

const char *mossRetryResult(RetryResult);
void mossRetryInit(RetryContext *, const RetryConfig *);
RetryResult mossRetryExecute(RetryContext *, RetryOperation, void *, RetryPredicate);
bool mossRetryShouldRetry(int errnoValue);
