#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#include "include/retry.h"

int calculateDelay(RetryContext *ctx)
{
    int delay = ctx->config.baseDelayms;

    if (ctx->config.useExponentialBackoff)
        delay = ctx->config.baseDelayms * (1 << (ctx->attemptCount - 1));

    if (delay > ctx->config.maxDelayms)
        delay = ctx->config.maxDelayms;

    return delay;
}

const char *mossRetryResult(RetryResult result)
{
    switch (result)
    {
    case RETRY_OK:
        return "Success";
    case RETRY_EXHAUSTED:
        return "Max retries exceeded :(";
    case RETRY_NON_RETRYABLE:
        return "Non-retryable error";
    default:
        return "Unknown error";
    }
}

bool mossRetryShouldRetry(int errnoValue)
{
    switch (errnoValue)
    {
    case EAGAIN:
    case EINTR:
    case ENOMEM:
    case EMFILE:
    case ENFILE:
        return true;
    default:
        return false;
    }
}

void mossRetryInit(RetryContext *ctx, const RetryConfig *config)
{
    ctx->config = *config;
    ctx->attemptCount = 0;
    ctx->lastErrno = 0;
}

RetryResult mossRetryExecute(RetryContext *ctx, RetryOperation op, void *context, RetryPredicate shouldRetry)
{
    ctx->attemptCount = 0;

    while (ctx->attemptCount < ctx->config.maxRetries)
    {
        ctx->attemptCount++;

        const int result = op(context);

        if (result == 0)
            return RETRY_OK;

        ctx->lastErrno = errno;

        if (!shouldRetry(ctx->lastErrno))
            return RETRY_NON_RETRYABLE;

        if (ctx->attemptCount < ctx->config.maxRetries)
            usleep(calculateDelay(ctx) * 1000);

        /**
         * for better visual
         *  ┌─────────────────┐
            │ Run operation   │
            └────────┬────────┘
                    │
                ┌────▼────┐
                │Success? │──Yes──► Return RETRY_OK
                └────┬────┘
                    │No
                ┌────▼─────────┐
                │Worth retry?  │──No──► Return RETRY_NON_RETRYABLE
                └────┬─────────┘
                    │Yes
                ┌────▼─────────┐
                │Retries left? │──No──► Return RETRY_EXHAUSTED
                └────┬─────────┘
                    │Yes
                ┌────▼────┐
                │  Sleep  │
                └────┬────┘
                    │
                    └──────► (loop back to Run operation)
         */
    }

    return RETRY_EXHAUSTED;
}