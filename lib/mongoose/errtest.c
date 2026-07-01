#include <stdio.h>
#include <string.h>
#include <errno.h>
int main() {
    printf("errno 88: %s\n", strerror(88));
    return 0;
}
