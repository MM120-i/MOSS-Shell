#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "include/jobs.h"

Job *jobs = NULL;
private int jobCount = 0;
private int nextJobId = 1;

void jobs_init()
{
    if (jobs == NULL)
    {
        jobs = (Job *)calloc(MAX_JOBS, sizeof(Job));
        jobCount = 0;
        nextJobId = 1;
    }
}

int jobs_add(pid_t pid, pid_t pgid, const char *cmd)
{
    if (!jobs || jobCount >= MAX_JOBS)
        return -1;

    Job *job = &jobs[jobCount];
    job->id = nextJobId++;
    job->pgid = pgid;
    job->pid = pid;
    job->state = JOB_RUNNING;
    job->cmd = strdup(cmd);

    jobCount++;

    return job->id;
}

void jobs_remove(pid_t pid)
{
    for (size_t i = 0; i < jobCount; i++)
    {
        if (jobs[i].pgid == pid)
        {
            free(jobs[i].cmd);

            for (size_t j = i; j < jobCount - 1; j++)
                jobs[j] = jobs[j + 1];

            jobCount--;
            return;
        }
    }
}

Job *jobs_get(int id)
{
    for (size_t i = 0; i < jobCount; i++)
        if (jobs[i].id == id)
            return &jobs[i];

    return NULL;
}

void jobs_list()
{
    for (size_t i = 0; i < jobCount; i++)
    {
        const char *stateStr = "Running";

        if (jobs[i].state == JOB_STOPPED)
            stateStr = "Stopped";
        else if (jobs[i].state == JOB_DONE)
            stateStr = "Done";

        printf("[%d] %s\t%s\n", jobs[i].id, stateStr, jobs[i].cmd);
    }
}

int jobs_count()
{
    return jobCount;
}

Job *jobs_get_by_pid(pid_t pid)
{
    for (int i = 0; i < jobCount; i++)
        if (jobs[i].pid == pid)
            return &jobs[i];

    return NULL;
}

void jobs_destroy()
{
    if (jobs)
    {
        for (size_t i = 0; i < jobCount; i++)
            free(jobs[i].cmd);

        free(jobs);
        jobs = NULL;
        jobCount = 0;
    }
}