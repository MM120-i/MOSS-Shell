#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <unistd.h>

int main() {
    int fd = socket(AF_INET, SOCK_STREAM, 0);

    if (fd < 0) { 
        perror("socket"); 
        return 1; 
    }
     
    struct sockaddr_in a = {0};
    a.sin_family = AF_INET;
    a.sin_port = htons(8082);
    a.sin_addr.s_addr = INADDR_ANY;

    if (bind(fd, (void*)&a, sizeof(a)) < 0) { 
        perror("bind"); 
        close(fd); 
        return 1; 
    }

    if (listen(fd, 5) < 0) { 
        perror("listen"); 
        close(fd); 
        return 1; 
    }

    printf("OK fd=%d\n", fd);
    close(fd);

    return 0;
}
