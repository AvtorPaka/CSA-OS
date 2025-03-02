#include <iostream>
#include <cstdint>
#include <unistd.h>
#include <csignal>

volatile sig_atomic_t responseNumberBits = 0;
volatile sig_atomic_t bitCounter = 0;
pid_t clientPid;

void handleReceivedBit(int32_t nsig) {
    int32_t bit = nsig == SIGUSR2 ? 0 : 1;
    if (bitCounter <= 31) {
        responseNumberBits |= (bit << bitCounter);
        bitCounter = bitCounter + 1;
    }
    kill(clientPid, SIGUSR1);
}

void handleSigInt(int nsig) {
    int32_t responseNumber = static_cast<int32_t>(responseNumberBits);
    std::cout << "Number received from Client-process: " << responseNumber << std::endl;
    exit(0);
}


int main() {
    std::cout << "Server-process PID: " << getpid() << '\n'
            << "Enter Client-process PID: ";
    std::cin >> clientPid;

    signal(SIGUSR1, handleReceivedBit);
    signal(SIGUSR2, handleReceivedBit);
    signal(SIGINT, handleSigInt);

    while (true) {
        pause();
    }

    return 0;
}

