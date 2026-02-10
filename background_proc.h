//***************************************************************************
//* Name: background_proc.h                                                 *
//* Description:   Part 8 - Background Processing & Job Bookkeeping header  *
//* Author: Katelyna Pastrana                                               *
//* Date:        2026-02-06                                                 *
//* References:                                                             *
//*    - COP4610 Project 1 specification (Part 8)                           *
//*    - POSIX: fork(2), waitpid(2)                                         *
//*    - Design inspired by Stephen Brennan's "Write a Shell in C"          *
//***************************************************************************
#ifndef PART_EIGHT_H
#define PART_EIGHT_H

#include <sys/types.h>

/* Initialize job system (call once at shell startup) */
void part_eight_init(void);

/*
 * Add a background job.
 * - cmdline: human-readable command line (function will strdup it internally).
 * - pids: array of child PIDs for this job (e.g., pipeline processes), length nprocs.
 * - nprocs: number of PIDs in pids (1..3).
 * - leader_pid: PID to print when job starts (for pipelines, typically the last PID).
 *
 * Returns job number (>0) on success, -1 if cannot add (e.g., too many active jobs).
 * On success, a start message is printed: "[jobno] leader_pid\n"
 */
int part_eight_add_job(const char *cmdline, pid_t *pids, int nprocs, pid_t leader_pid);

/* Should be called periodically in the main loop to reap finished background processes.
 * It will print completion messages for jobs that finished.
 */
void part_eight_check_jobs(void);

/* Built-in 'jobs' command: prints active background jobs */
void part_eight_jobs_builtin(void);

/* Shutdown job system (free resources) */
void part_eight_shutdown(void);

#endif
