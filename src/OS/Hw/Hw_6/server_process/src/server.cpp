#include <iostream>
#include <unistd.h>
#include <csignal>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <vector>

#define PERMISSIONS 0666

std::vector<pid_t> clientPids;
int* sharedMem;
int32_t shmId;

void handleUserSigInt(int nsig) {
    if (shmdt(sharedMem) < 0) {
        std::cout << '\n' << "Unable to delete shared memory from local process" << '\n';
    }

    for (int32_t i = 0; i < clientPids.size(); ++i) {
        kill(clientPids[i], SIGUSR1);
    }

    if (shmctl(shmId, IPC_RMID, nullptr) >= 0) {
        std::cout << '\n' << "SystemV ipc shared memory deleted from system." << '\n';
    }

    exit(0);
}

void handleClientSigInt(int nsig) {
    std::cout << '\n' << "Server-process stopped by client" << '\n';
    if (shmdt(sharedMem) < 0) {
        std::cout << '\n' << "Unable to delete shared memory from local process" << '\n';
    }
    if (shmctl(shmId, IPC_RMID, nullptr) >= 0) {
        std::cout << '\n' << "SystemV ipc shared memory deleted from system." << '\n';
    }
    exit(0);
}


int main() {
    key_t shmKey;

    signal(SIGINT, handleUserSigInt);
    signal(SIGUSR1, handleClientSigInt);

    std::cout << "Server-process PID: " << getpid() << '\n';
    pid_t clientPid = 0;

    std::cout << "Enter -1 to stop providing PID's" << '\n';
    while (clientPid != -1) {
        std::cout << "Enter Client-process PID: ";
        std::cin >> clientPid;
        if (clientPid != -1) {
            clientPids.push_back(clientPid);
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

    if ((sharedMem = (int*) shmat(shmId, nullptr, SHM_RDONLY)) == nullptr) {
        std::cout << "Unable to get shared memory segment" << '\n';
        exit(1);
    }

    while (true) {
        std::cout << '\n' << "Read number " << *sharedMem << " from shared memory";
        sleep(1);
    }

    return 0;
}

