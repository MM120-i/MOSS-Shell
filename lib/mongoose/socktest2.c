#include "mongoose.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <unistd.h>

int main() {
    struct mg_mgr mgr;
    mg_mgr_init(&mgr);
    printf("mgr init done\n");
    struct mg_connection *c = mg_http_listen(&mgr, "http://0.0.0.0:8083", NULL, NULL);

    if(c) 
        printf("HTTP listen OK\n");
    else 
        printf("HTTP listen FAILED\n");
    
    mg_mgr_free(&mgr);

    return 0;
}
