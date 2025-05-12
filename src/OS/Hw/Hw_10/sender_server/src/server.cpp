#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define MAXSENDSTRING 256

void DieWithError(const char *errorMessage) {
    perror(errorMessage);
    exit(1);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <Broadcast IP> <Port>\n", argv[0]);
        exit(1);
    }

    const char *broadcastIP = argv[1];
    unsigned short broadcastPort = atoi(argv[2]);

    int sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock < 0) {
        DieWithError("socket() failed");
    }

    int broadcastPermission = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &broadcastPermission, sizeof(broadcastPermission)) < 0) {
        DieWithError("setsockopt() failed");
    }

    struct sockaddr_in broadcastAddr{};
    memset(&broadcastAddr, 0, sizeof(broadcastAddr));
    broadcastAddr.sin_family = AF_INET;
    broadcastAddr.sin_addr.s_addr = inet_addr(broadcastIP);
    broadcastAddr.sin_port = htons(broadcastPort);

    printf("Enter messages to broadcast (Ctrl+C to exit):\n");

    char sendString[MAXSENDSTRING];
    while (1) {
        if (fgets(sendString, sizeof(sendString), stdin) == nullptr) {
            break;
        }

        sendString[strcspn(sendString, "\n")] = '\0';

        ssize_t sendLen = strlen(sendString);
        if (sendto(sock, sendString, sendLen, 0, (struct sockaddr *) &broadcastAddr, sizeof(broadcastAddr)) != sendLen) {
            DieWithError("sendto() failed");
        }
    }

    close(sock);
    return 0;
}