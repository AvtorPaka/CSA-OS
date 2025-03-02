#include <iostream>
#include <cstdint>
#include <unistd.h>
#include <csignal>

volatile sig_atomic_t confirmServer = 0;

void handleSendBit(int32_t bit, pid_t& serverPid) {
    int32_t signal = bit == 0 ? SIGUSR2 : SIGUSR1;
    kill(serverPid, signal);
}

void handleServerConfirm(int32_t nsig) {
    confirmServer = 1;
}

int main() {
    int32_t requestNumber;
    pid_t serverPid;
    signal(SIGUSR1, handleServerConfirm);

    std::cout << "Client-process PID: " << getpid() << '\n'
            << "Enter Server-process PID: ";
    std::cin >> serverPid;


    std::cout << "Enter number to send to the Server-process: ";
    std::cin >> requestNumber;
    uint32_t bits = static_cast<uint32_t>(requestNumber);


    sigset_t mask, oldMask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGUSR1);
    sigprocmask(SIG_BLOCK, &mask, &oldMask);

    for (int32_t i = 0; i < 32; ++i) {
        int32_t curBit = (bits >> i) & 1;

        handleSendBit(curBit, serverPid);
        confirmServer = 0;

        while (!confirmServer) {
            sigsuspend(&oldMask);
        }
    }

    sigprocmask(SIG_UNBLOCK, &mask, NULL);
    kill(serverPid, SIGINT);
    std::cout << "Client-process sucessfully send number" << std::endl;

    return 0;
}

