#include <sys/socket.h>

int socket(int domain, int type, int protocol) {
    return syscall( domain, type, protocol);
}