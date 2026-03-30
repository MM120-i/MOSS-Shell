#pragma once

#include <sys/types.h>

#define private static
#define MAX_JOBS 64

typedef enum
{
    JOB_RUNNING,
    JOB_STOPPED,
    JOB_DONE
} JobState;

typedef struct
{
    int id;
    pid_t pid;
    pid_t pgid;
    JobState state;
    char *cmd;
} Job;

extern Job *jobs;

void jobs_init();
int jobs_add(pid_t, pid_t, const char *);
void jobs_remove(pid_t);
Job *jobs_get(int);
Job *jobs_get_by_pid(pid_t);
void jobs_list();
size_t jobs_count();
void jobs_destroy();
