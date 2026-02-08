#pragma once

#include <signal.h>
#include <sys/types.h>

#define private static

extern volatile sig_atomic_t moss_running;
extern volatile sig_atomic_t moss_got_sigint;
extern pid_t moss_foreground_pgid; // gets group id of foreground processes

void moss_init_signals();