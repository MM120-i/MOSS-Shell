#include "../lib/mongoose/mongoose.h"

#include <signal.h>
#include <stdlib.h>
#include <stdio.h>

static volatile sig_atomic_t s_running = 1;

static void signal_handler(int signo) {
    (void)signo;
    s_running = 0;
}

static const char *s_http_port = "http://localhost:8080";

static void ev_handler(struct mg_connection *c, int ev, void *ev_data) {
    if (ev == MG_EV_HTTP_MSG) {
        (void)ev_data;
        mg_http_reply(c, 200,
                      "Content-Type: text/html; charset=utf-8\r\n",
                      "<!DOCTYPE html><html><head><title>MOSS</title>"
                      "<style>body{font-family:monospace;padding:2em;"
                      "background:#1e1e2e;color:#cdd6f4}"
                      "h1{color:#cba6f7}a{color:#89b4fa}</style>"
                      "</head><body>"
                      "<h1>MOSS Shell</h1>"
                      "<p>Web interface running on <code>%s</code></p>"
                      "<p>Terminal coming in next phase.</p>"
                      "</body></html>",
                      s_http_port);
    }
}

int main(void) {
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    struct mg_mgr mgr;
    mg_mgr_init(&mgr);

    struct mg_connection *c = mg_http_listen(&mgr, s_http_port, ev_handler, NULL);

    if (c == NULL) {
        fprintf(stderr, "Failed to listen on %s\n", s_http_port);
        mg_mgr_free(&mgr);
        return EXIT_FAILURE;
    }

    printf("MOSS Web Shell listening on %s\n", s_http_port);

    while (s_running) 
        mg_mgr_poll(&mgr, 1000);

    mg_mgr_free(&mgr);
    printf("\nMOSS Web Shell stopped.\n");
    
    return EXIT_SUCCESS;
}
