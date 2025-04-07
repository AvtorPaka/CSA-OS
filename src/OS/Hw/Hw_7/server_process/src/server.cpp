#include <iostream>
#include <unistd.h>
#include <csignal>
#include <sys/mman.h>
#include <fcntl.h>
#include <vector>
#include <sys/stat.h>

const char* shar_object = "posix-shar-object";

std::vector<pid_t> clientPids;
int* sharedMem;
int shm_fd;

void handleUserSigInt(int nsig) {
    if (munmap(sharedMem, sizeof(int)) == -1) {
        std::cout << '\n' << "Unable to unmap shared memory from local process" << '\n';
    }

    for (size_t i = 0; i < clientPids.size(); ++i) {
        kill(clientPids[i], SIGUSR1);
    }

    if (shm_unlink(shar_object) == 0) {
        std::cout << '\n' << "POSIX shared memory deleted from system." << '\n';
    }

    exit(0);
}

void handleClientSigInt(int nsig) {
    std::cout << '\n' << "Server-process stopped by client" << '\n';
    if (munmap(sharedMem, sizeof(int)) == -1) {
        std::cout << '\n' << "Unable to unmap shared memory from local process" << '\n';
    }
    if (shm_unlink(shar_object) == 0) {
        std::cout << '\n' << "POSIX shared memory deleted from system." << '\n';
    }
    exit(0);
}

int main() {
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

    shm_fd = shm_open(shar_object, O_CREAT | O_RDONLY, 0666);
    if (shm_fd == -1) {
        std::cout << "Unable to create/open shared memory object" << '\n';
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

    sharedMem = (int*)mmap(nullptr, sizeof(int), PROT_READ, MAP_SHARED, shm_fd, 0);
    close(shm_fd);

    if (sharedMem == MAP_FAILED) {
        std::cout << "Unable to map shared memory segment" << '\n';
        exit(1);
    }

    while (true) {
        std::cout << '\n' << "Read number " << *sharedMem << " from shared memory";
        sleep(1);
    }

    return 0;
}