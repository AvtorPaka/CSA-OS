#include <iostream>
#include <unistd.h>
#include <csignal>
#include <random>
#include <sys/ipc.h>
#include <sys/shm.h>

#define PERMISSIONS 0666

std::vector<pid_t> serverPids;
int* sharedMem;

void handleUserSigInt(int nsig) {
    if (shmdt(sharedMem) < 0) {
        std::cout  << '\n' << "Unable to delete shared memory from local process" << '\n';
    }

    for (int32_t i = 0; i < serverPids.size(); ++i) {
        kill(serverPids[i], SIGUSR1);
    }
    
    exit(0);
}

void handleServerSigInt(int nsig) {
    std::cout << '\n' << "Client-process stopped by server" << '\n';
    if (shmdt(sharedMem) < 0) {
        std::cout << '\n' << "Unable to delete shared memory from local process" << '\n';
    }
    exit(0);
}


int main() {
    key_t shmKey;
    int32_t shmId;
    int32_t randomNumber;

    signal(SIGINT, handleUserSigInt);
    signal(SIGUSR1, handleServerSigInt);
    std::cout << "Client-process PID: " << getpid() << '\n';
    
    pid_t serverPid = 0;
    std::cout << "Enter -1 to stop providing PID's" << '\n';
    while (serverPid != -1) {
        std::cout << "Enter Server-process PID: ";
        std::cin >> serverPid;
        if (serverPid != -1) {
            serverPids.push_back(serverPid);
        }
    }

    if ((shmKey = ftok("../../.", 0)) < 0) {
        std::cout << "Unable to create SystemV ipc key" << '\n';
        exit(1);
    }

    if ((shmId = shmget(shmKey, getpagesize(), PERMISSIONS | IPC_CREAT)) < 0) {
        std::cout << "Unable to get shared memory segment" << '\n';
        exit(1);
    }

    if ((sharedMem = (int*) shmat(shmId, nullptr, 0)) == nullptr) {
        std::cout << "Unable to get shared memory segment" << '\n';
        exit(1);
    }

    while (true) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, 1000);

        randomNumber = dis(gen);
        *sharedMem = randomNumber;
        std::cout << '\n' << "Wrote number " << randomNumber << " to shared memory";
        sleep(1);
    }

    return 0;
}

