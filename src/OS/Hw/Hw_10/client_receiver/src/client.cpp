#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define MAXRECVSTRING 255

void DieWithError(const char *errorMessage) {
    perror(errorMessage);
    exit(1);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <Port>\n", argv[0]);
        exit(1);
    }

    unsigned short broadcastPort = atoi(argv[1]);

    int sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock < 0) {
        DieWithError("socket() failed");
    }

    int reuse = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse)) < 0) {
        DieWithError("setsockopt(SO_REUSEPORT) failed");
    }

    struct sockaddr_in broadcastAddr{};
    memset(&broadcastAddr, 0, sizeof(broadcastAddr));
    broadcastAddr.sin_family = AF_INET;
    broadcastAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    broadcastAddr.sin_port = htons(broadcastPort);

    if (bind(sock, (struct sockaddr*) &broadcastAddr, sizeof(broadcastAddr)) < 0) {
        DieWithError("bind() failed");
    }

    char recvString[MAXRECVSTRING + 1];
    while (1) {
        struct sockaddr_in fromAddr;
        socklen_t fromAddrLen = sizeof(fromAddr);

        ssize_t recvLen = recvfrom(sock, recvString, MAXRECVSTRING, 0, (struct sockaddr*) &fromAddr, &fromAddrLen);
        if (recvLen < 0) {
            DieWithError("recvfrom() failed");
        }

        recvString[recvLen] = '\0';
        printf("Received: %s\n", recvString);
    }

    close(sock);
    return 0;
}