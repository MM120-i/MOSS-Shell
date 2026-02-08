#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>
#include <string.h>

#include "include/signals.h"

volatile sig_atomic_t moss_running = 1;
volatile sig_atomic_t moss_got_sigint = 0;
pid_t moss_foreground_pgid; // gets group id of foreground processes

// when ctrl + C (^C) is pressed
private void sigint_handler(int signo)
{
    (void)signo;
    moss_got_sigint = 1;

    if (moss_foreground_pgid > 0)
        kill(-moss_foreground_pgid, SIGINT);
}

// request clean shutdown
private void sigterm_handler(int signo)
{
    (void)signo;
    moss_running = 0;
}

// reap children and avoid zombies 💀💀
private void sigchld_handler(int signo)
{
    (void)signo;
    int savedErrno = errno;

    while (1)
    {
        int status;
        pid_t pid = waitpid(-1, &status, WNOHANG);

        if (pid <= 0)
            break;
    }

    errno = savedErrno;
}

private int install_handler(int signum, void (*handler)(int), const int flags)
{
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handler;
    sa.sa_flags = flags;
    sigemptyset(&sa.sa_mask);

    if (sigaction(signum, &sa, NULL) == -1)
    {
        perror("MOSS: Sigaction");
        return -1;
    }

    return 0;
}

// public initializer. This is called only once at startup.
void moss_init_signals()
{
    signal(SIGTTOU, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);

    /**
     * install any handlers necessary
     * SIGINT: Interrupt foreground jobs but not kill the shell
     * SIGTERM: Requests shutdown
     * SIGCHLD: Reap child
     */
    if (install_handler(SIGINT, sigint_handler, SA_RESTART) == -1)
        perror("MOSS: SIGINT Handler"); // FATAL. But we continue to run

    if (install_handler(SIGTERM, sigterm_handler, 0) == -1)
        perror("MOSS: SIGTERN Handler");

    if (install_handler(SIGCHLD, sigchld_handler, SA_RESTART | SA_NOCLDSTOP) == -1)
        perror("MOSS: SIGCHLD Handler");
}
