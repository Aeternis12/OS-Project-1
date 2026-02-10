//***************************************************************************************************************
//* Name:        background_proc.c                                                                              *
//* Description: Part 8 - Background Processing & Job Bookkeeping implementation                                *
//*              Maintains job table and provides APIs:                                                         *
//*                - part_eight_init()            : initialize job system                                       *
//*                - part_eight_add_job(...)      : register a new background job and print start msg           *
//*                - part_eight_check_jobs()      : poll/reap finished background children and print            *
//*                                              completion messages                                            *
//*                - part_eight_jobs_builtin()    : builtin to list active background jobs                      *
//*                - part_eight_shutdown()        : cleanup resources                                           *
//*              Behavior:                                                                                      *
//*                - Tracks up to MAX_ACTIVE_JOBS concurrently (default 10).                                    *
//*                - Job numbers are monotonic and never reused.                                                *
//*                - Supports jobs consisting of up to 3 processes (2 pipes => 3 procs).                        *
//*                - Prints start message: [jobno] leader_pid                                                   *
//*                - Prints completion message: [jobno]  + done <cmdline>                                       *
//* Author: Katelyna Pastrana                                                                                   *
//* Date:        2026-02-07                                                                                     *
//* References:                                                                                                 *
//*    - COP4610 Project 1 specification (Part 8)                                                               *
//*    - POSIX: fork(2), waitpid(2)                                                                             *
//*    - Stephen Brennan, "Write a Shell in C" (design notes)                                                   *
//*    - CodeVault YouTube videos                                                                               *
//* Compile:     gcc -std=c11 -Wall -Wextra -O2 -D_POSIX_C_SOURCE=200809L -o background_proc background_proc.c  *
//***************************************************************************************************************

#define _POSIX_C_SOURCE 200809L

#include "background_proc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <errno.h>

#define MAX_ACTIVE_JOBS 10
#define MAX_JOB_HISTORY 1024 /* capacity for job records (job numbers monotonic) */
#define MAX_PROCS_PER_JOB 3  /* supports up to 3 commands per job (2 pipes => 3 procs) */

typedef struct job {
    int active;                 /* 1 if active, 0 if finished */
    int jobno;                  /* monotonic job number */
    pid_t pids[MAX_PROCS_PER_JOB];
    int nprocs;                 /* number of pids stored */
    pid_t leader_pid;           /* pid printed at start (last pid in pipeline) */
    int remaining;              /* how many procs still running */
    char *cmdline;              /* strdup'd command line for messages */
} job_t;

/* Job table / bookkeeping state */
static job_t job_table[MAX_JOB_HISTORY];
static int next_job_index = 0;   /* next slot to use (keeps history) */
static int next_job_number = 1;  /* monotonic job number */
static int active_job_count = 0;

void part_eight_init(void) {
    memset(job_table, 0, sizeof(job_table));
    next_job_index = 0;
    next_job_number = 1;
    active_job_count = 0;
}

/* Helper: find job index containing pid, or -1 */
static int find_job_by_pid(pid_t pid) {
    for (int i = 0; i < next_job_index; ++i) {
        if (!job_table[i].active) continue;
        for (int j = 0; j < job_table[i].nprocs; ++j) {
            if (job_table[i].pids[j] == pid) return i;
        }
    }
    return -1;
}

/* Helper: find index of most recent active job (highest jobno) or -1 */
static int find_most_recent_active_job_index(void) {
    int best = -1;
    int best_jobno = -1;
    for (int i = 0; i < next_job_index; ++i) {
        if (!job_table[i].active) continue;
        if (job_table[i].jobno > best_jobno) {
            best_jobno = job_table[i].jobno;
            best = i;
        }
    }
    return best;
}

int part_eight_add_job(const char *cmdline, pid_t *pids, int nprocs, pid_t leader_pid) {
    if (!cmdline || !pids || nprocs <= 0 || nprocs > MAX_PROCS_PER_JOB) {
        errno = EINVAL;
        return -1;
    }
    if (active_job_count >= MAX_ACTIVE_JOBS) {
        /* too many concurrent background jobs */
        errno = EBUSY;
        return -1;
    }
    if (next_job_index >= MAX_JOB_HISTORY) {
        /* shouldn't happen for student shell, but guard */
        errno = ENOMEM;
        return -1;
    }

    job_t *job = &job_table[next_job_index];
    job->active = 1;
    job->jobno = next_job_number++;
    job->nprocs = nprocs;
    job->remaining = nprocs;
    job->leader_pid = leader_pid;
    job->cmdline = strdup(cmdline ? cmdline : "");
    if (!job->cmdline) {
        errno = ENOMEM;
        return -1;
    }
    for (int i = 0; i < nprocs; ++i) job->pids[i] = pids[i];

    /* Print job start message: [jobno] leader_pid */
    /* Use %ld and (long) cast for portability of pid_t */
    printf("[%d] %ld\n", job->jobno, (long)job->leader_pid);
    fflush(stdout);

    ++next_job_index;
    ++active_job_count;
    return job->jobno;
}

/* Called periodically from main loop. Reaps any finished children (non-blocking)
 * and prints completion messages. Uses waitpid(-1, WNOHANG).
 */
void part_eight_check_jobs(void) {
    int status;
    pid_t pid;

    /* Loop until no more reaped children */
    while (1) {
        pid = waitpid(-1, &status, WNOHANG);
        if (pid > 0) {
            int idx = find_job_by_pid(pid);
            if (idx == -1) {
                /* Not one of our tracked background jobs — could be a foreground child
                 * or something else already reaped. Ignore. */
                continue;
            }
            job_t *job = &job_table[idx];
            job->remaining -= 1;
            if (job->remaining <= 0) {
                /* Job fully finished */
                /* Print completion message: [jobno]  + done [cmdline] */
                printf("[%d]  + done %s\n", job->jobno, job->cmdline ? job->cmdline : "");
                fflush(stdout);

                /* Free resources and mark inactive */
                free(job->cmdline);
                job->cmdline = NULL;
                job->active = 0;
                job->nprocs = 0;
                job->remaining = 0;
                --active_job_count;
            }
            /* continue loop to reap any additional exited children */
            continue;
        } else if (pid == 0) {
            /* No (more) children ready */
            break;
        } else {
            /* pid == -1 */
            if (errno == ECHILD) {
                /* No child processes */
                break;
            } else if (errno == EINTR) {
                /* Interrupted, retry */
                continue;
            } else {
                perror("waitpid (part_eight_check_jobs)");
                break;
            }
        }
    }
}

/* Built-in 'jobs' command: prints active background jobs.
 * Format per spec: [Job number]+ [CMD's PID] [CMD's command line]
 * We append '+' to the most-recent active job (if any) as a marker.
 */
void part_eight_jobs_builtin(void) {
    int most_recent_idx = find_most_recent_active_job_index();

    for (int i = 0; i < next_job_index; ++i) {
        job_t *job = &job_table[i];
        if (!job->active) continue;
        /* leader pid printed in job listing; add '+' after job number for most recent */
        if (i == most_recent_idx) {
            printf("[%d]+ %ld %s\n", job->jobno, (long)job->leader_pid, job->cmdline ? job->cmdline : "");
        } else {
            printf("[%d]  %ld %s\n", job->jobno, (long)job->leader_pid, job->cmdline ? job->cmdline : "");
        }
    }
    fflush(stdout);
}

void part_eight_shutdown(void) {
    /* Free any remaining resources */
    for (int i = 0; i < next_job_index; ++i) {
        if (job_table[i].cmdline) {
            free(job_table[i].cmdline);
            job_table[i].cmdline = NULL;
        }
    }
    /* reset counts (optional) */
    next_job_index = 0;
    active_job_count = 0;
    next_job_number = 1;
}
