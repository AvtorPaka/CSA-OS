#include <iostream>
#include <unistd.h>
#include <csignal>
#include <random>
#include <sys/mman.h>
#include <fcntl.h>
#include <vector>
#include <cstdlib>
#include <sys/stat.h>

const char* shar_object = "posix-shar-object";

std::vector<pid_t> serverPids;
int* sharedMem;
int shm_fd;

void handleUserSigInt(int nsig) {
    if (munmap(sharedMem, sizeof(int)) == -1) {
        std::cout  << '\n' << "Unable to delete shared memory from local process" << '\n';
    }

    for (int32_t i = 0; i < serverPids.size(); ++i) {
        kill(serverPids[i], SIGUSR1);
    }

    close(shm_fd);
    exit(0);
}

void handleServerSigInt(int nsig) {
    std::cout << "\nClient-process stopped by server" << std::endl;
    if (munmap(sharedMem, sizeof(int)) == -1) {
        std::cout << '\n' << "Unable to delete shared memory from local process" << '\n';
    }

    close(shm_fd);
    exit(0);
}

int main() {
    int32_t randomNumber;

    signal(SIGINT, handleUserSigInt);
    signal(SIGUSR1, handleServerSigInt);

    std::cout << "Client-process PID: " << getpid() << std::endl;

    pid_t serverPid = 0;
    std::cout << "Enter -1 to stop providing PID's" << '\n';
    while (serverPid != -1) {
        std::cout << "Enter Server-process PID: ";
        std::cin >> serverPid;
        if (serverPid != -1) {
            serverPids.push_back(serverPid);
        }
    }

    if ((shm_fd = shm_open(shar_object, O_CREAT | O_RDWR, 0666)) == -1) {
        std::cout << "Unable to get shared memory segment" << '\n';
        exit(1);
    }

    struct stat sb;
    if (fstat(shm_fd, &sb) == -1) {
        std::cout << "Unable to get shared memory status" << '\n';
        close(shm_fd);
        exit(1);
    }

    if (sb.st_size == 0) {
        if (ftruncate(shm_fd, sizeof(int)) == -1) {
            std::cout << "Unable to set shared memory size" << '\n';
            close(shm_fd);
            exit(1);
        }
    }

    sharedMem = static_cast<int*>(mmap(nullptr, sizeof(int),
                                       PROT_READ | PROT_WRITE,
                                       MAP_SHARED,
                                       shm_fd, 0));

    if (sharedMem == MAP_FAILED) {
        std::cout << "Unable to get shared memory segment" << '\n';
        exit(1);
    }

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dist(0, 1000);

    while (true) {
        randomNumber = dist(gen);
        *sharedMem = randomNumber;
        std::cout << '\n' << "Wrote number " << randomNumber << " to shared memory";
        sleep(1);
    }

    return 0;
}