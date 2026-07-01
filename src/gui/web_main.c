#include "mongoose.h"
#include "web/index.html.h"
#include "web/styles.css.h"
#include "web/main.js.h"

#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <errno.h>
#include <string.h>

#ifdef __linux__
    #include <pty.h>
#elif defined(__APPLE__)
    #include <util.h>
#else
    #include <pty.h>
#endif

static volatile sig_atomic_t s_running = 1;
static int s_master_fd = -1;
static pid_t s_shell_pid = -1;
static struct mg_connection *s_ws_conn = NULL;
static char s_pty_buf[65536];
static size_t s_pty_buf_len = 0;

enum {
    HTTP_OK = 200,
    HTTP_NOT_FOUND = 404,
    HTTP_SERVER_ERROR = 500,
};

static void signal_handler(int signo) {
    (void)signo;
    s_running = 0;
}

static void pty_poll_cb(void *arg) {
    (void)arg;
    char buf[4096];
    ssize_t n = read(s_master_fd, buf, sizeof(buf));

    if (n > 0) {
        if (s_ws_conn) {
            mg_ws_send(s_ws_conn, buf, (size_t)n, WEBSOCKET_OP_TEXT);
        } 
        else if (s_pty_buf_len + (size_t)n < sizeof(s_pty_buf)) {
            memcpy(s_pty_buf + s_pty_buf_len, buf, (size_t)n);
            s_pty_buf_len += (size_t)n;
        }
    }

    if (n == 0) 
        s_running = 0;
}

static int pty_fork_and_exec(void) {
    int master_fd;
    pid_t pid = forkpty(&master_fd, NULL, NULL, NULL);

    if (pid == -1) {
        perror("forkpty");
        return -1;
    }

    if (pid == 0) {
        setenv("TERM", "xterm-256color", 1);
        char *args[] = {"./shell", NULL};
        execvp("./shell", args);
        perror("execvp");
        _exit(127);
    }

    int flags = fcntl(master_fd, F_GETFL, 0);
    fcntl(master_fd, F_SETFL, flags | O_NONBLOCK);
    struct winsize ws = { 50, 140, 0, 0 };
    ioctl(master_fd, TIOCSWINSZ, &ws);
    s_master_fd = master_fd;
    s_shell_pid = pid;

    return master_fd;
}

static void ev_handler(struct mg_connection *c, int ev, void *ev_data) {
    switch (ev){
        case MG_EV_HTTP_MSG: {
            struct mg_http_message *hm = (struct mg_http_message *) ev_data;

            if (mg_match(hm->uri, mg_str("/ws"), NULL)) 
                mg_ws_upgrade(c, hm, NULL);
            else if (mg_match(hm->uri, mg_str("/styles.css"), NULL))
                mg_http_reply(c, HTTP_OK, "Content-Type: text/css; charset=utf-8\r\n", "%.*s", (int)styles_css_len, styles_css);
            else if (mg_match(hm->uri, mg_str("/main.js"), NULL))
                mg_http_reply(c, HTTP_OK, "Content-Type: application/javascript; charset=utf-8\r\n", "%.*s", (int)main_js_len, main_js);
            else 
                mg_http_reply(c, HTTP_OK, "Content-Type: text/html; charset=utf-8\r\n", "%.*s", (int)index_html_len, index_html);

            break;
        }
            
        case MG_EV_WS_OPEN:
            s_ws_conn = c;

            if (s_pty_buf_len > 0) {
                mg_ws_send(c, s_pty_buf, s_pty_buf_len, WEBSOCKET_OP_BINARY);
                s_pty_buf_len = 0;
            }

            printf("WebSocket connected\n");
            break;

        case MG_EV_WS_MSG: {
            struct mg_ws_message *wm = (struct mg_ws_message *) ev_data;

            if (s_master_fd == -1) 
                return;

            double cols, rows;

            if (wm->data.len > 0 && wm->data.buf[0] == '{' && mg_json_get_num(wm->data, "$.cols", &cols) && mg_json_get_num(wm->data, "$.rows", &rows)) {
                struct winsize ws;
                ws.ws_row = (unsigned short)rows;
                ws.ws_col = (unsigned short)cols;
                ws.ws_xpixel = ws.ws_ypixel = 0;
                ioctl(s_master_fd, TIOCSWINSZ, &ws);
            } 
            else {
                write(s_master_fd, wm->data.buf, wm->data.len);
            }

            break;
        }

        case MG_EV_CLOSE:
            if (c == s_ws_conn) {
                s_ws_conn = NULL;
                printf("WebSocket disconnected\n");
            }

            break;
    }
}

static void cleanup(void) {
    if (s_shell_pid > 0) {
        kill(s_shell_pid, SIGHUP);
        waitpid(s_shell_pid, NULL, 0);
    }

    if (s_master_fd != -1) 
        close(s_master_fd);
}

int main(void) {
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    atexit(cleanup);

    if (pty_fork_and_exec() == -1) {
        fprintf(stderr, "Failed to start shell in PTY\n");
        return EXIT_FAILURE;
    }

    struct mg_mgr mgr;
    mg_mgr_init(&mgr);

    const char *port = getenv("MOSS_PORT");

    if (!port) 
        port = "8080";

    char url[64];

    snprintf(url, sizeof(url), "http://0.0.0.0:%s", port);

    struct mg_connection *c = mg_http_listen(&mgr, url, ev_handler, NULL);

    if (c == NULL) {
        fprintf(stderr, "Failed to listen on %s\n", url);
        mg_mgr_free(&mgr);
        return EXIT_FAILURE;
    }

    struct mg_timer pty_timer;
    mg_timer_init(&mgr.timers, &pty_timer, 50, MG_TIMER_REPEAT, pty_poll_cb, NULL);

    printf("MOSS Web Shell: %s\n", url);
    printf("Shell PID: %d\n", s_shell_pid);

    while (s_running) 
        mg_mgr_poll(&mgr, 50);

    mg_mgr_free(&mgr);
    printf("MOSS Web Shell stopped.\n");

    return EXIT_SUCCESS;
}
