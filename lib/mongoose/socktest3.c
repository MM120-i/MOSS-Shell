#include "mongoose.h"
#include <sys/socket.h>
#include <stdio.h>
#include <unistd.h>

int main() {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    printf("socket() returned: %d, errno: %d (%s)\n", fd, errno, strerror(errno));
    return 0;
}
