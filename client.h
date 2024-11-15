#include "color.h"
#include <stdint.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define CMD_SIZE 32
#define ACCOUNT_SIZE 32
#define PASSWORD_SIZE 32

#define RCV_FLAGS MSG_WAITALL

struct Request {
    char command[CMD_SIZE];
    char account[ACCOUNT_SIZE];
    char password[PASSWORD_SIZE];
};

struct Response {
    char status[CMD_SIZE];
    uint32_t subsequent_data_size;
};

struct Thread_Argument {
    int fd;
    pthread_t th;
    sockaddr_in addr;
    socklen_t addr_len;
};