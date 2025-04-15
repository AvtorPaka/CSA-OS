#include "common.h"

const char *shared_memory_name = "/philosopher-shared-mem";
int shmId;
shared_memory_struct *sharedMem;

int philosopherNumber;
pid_t processPid;

std::atomic<bool> has_waiter_permission{false};
std::atomic<bool> has_left_fork{false};
std::atomic<bool> has_right_fork{false};

int leftForkSemIdx;
int rightForkSemIdx;

int eatTimes;
int thinkTimes;

void sigFunc(int sig) {
    if (sig != SIGTERM) {
        return;
    }

    std::cout << '[' << processPid << ']' << " | " << "Signal received. Leaving table. Ate " << eatTimes
              << " times. Thought " << thinkTimes << " times." << '\n';

    // Освобождения ресурсов в случае их занятости в момент выхода
    if (has_right_fork.load()) {
        std::cout << '[' << processPid << ']' << " | " << "Releasing awaited right fork semaphore: " << rightForkSemIdx
                  << '\n';
        sem_post(&sharedMem->forks[rightForkSemIdx]);
    }
    if (has_left_fork.load()) {
        std::cout << '[' << processPid << ']' << " | " << "Releasing awaited left fork semaphore: " << leftForkSemIdx
                  << '\n';
        sem_post(&sharedMem->forks[leftForkSemIdx]);
    }
    if (has_waiter_permission.load()) {
        std::cout << '[' << processPid << ']' << " | " << "Releasing awaited waiter semaphore"
                  << '\n';
        sem_post(&sharedMem->forkWaiter);
    }

    // Уменьшение числа занятых мест в разделяемой памяти
    int placesLeft;
    sem_wait(&sharedMem->mutexShm);

    placesLeft = sharedMem->philosopher_count - 1;
    if (placesLeft >= 0) {
        sharedMem->philosopher_count = placesLeft;
    }
    sem_post(&sharedMem->mutexShm);

    // Удаление ресурсов из локального процесса
    close_philosopher();

    // Удаление ресурсов из системы в случае если процесс последний
    if (placesLeft == 0) {
        std::cout << '[' << processPid << ']' << " | " << "I'm the last process on the table, start cleaning system."
                  << '\n';
        unlink_all();
    }

    exit(0);
}

void init_philosopher() {
    processPid = getpid();
    bool is_first = false;

    // Инициализация разделяемой памяти
    if ((shmId = shm_open(shared_memory_name, O_CREAT | O_RDWR | O_EXCL, 0666)) == -1) {
        if (errno == EEXIST) {
            //Случай если память уже существует
            if ((shmId = shm_open(shared_memory_name, O_RDWR, 0666)) == -1) {
                std::cout << '[' << processPid << ']' << " | " << "Unable to open shared memory with name: "
                          << shared_memory_name
                          << '\n';
                exit(1);
            }
        } else {
            std::cout << '[' << processPid << ']' << " | " << "Unable to open shared memory with name: "
                      << shared_memory_name
                      << '\n';
            exit(1);
        }
    } else {
        // Случай первичной инициализации памяти
        if (ftruncate(shmId, sizeof(shared_memory_struct)) == -1) {
            std::cout << '[' << processPid << ']' << " | " << "Unable to truncate shared memory with name: "
                      << shared_memory_name
                      << ", id: " << shmId << '\n';
            exit(1);
        } else {
            std::cout << '[' << processPid << ']' << " | " << "Shared memory with name: " << shared_memory_name
                      << ", id: " << shmId << " size set to " << sizeof(shared_memory_struct) << '\n';
        }

        is_first = true;
    }

    sharedMem = static_cast<shared_memory_struct *>(mmap(nullptr, sizeof(shared_memory_struct),
                                                         PROT_WRITE | PROT_READ, MAP_SHARED,
                                                         shmId, 0));

    // Инициализация неименованных семафоров в разделяемой памяти
    // и определение номера процесса-философа
    sem_wait(&sharedMem->mutexShm);

    if (is_first) {
        sem_init(&sharedMem->mutexShm, 1, 1);
        sem_init(&sharedMem->forkWaiter, 1, 4);
        for (int i = 0; i < 5; ++i)
            sem_init(&sharedMem->forks[i], 1, 1);
        sharedMem->philosopher_count = 0;
    }

    philosopherNumber = sharedMem->philosopher_count + 1;

    if (philosopherNumber <= 5) {
        sharedMem->philosopher_count = philosopherNumber;
    }

    std::cout << '[' << processPid << ']' << " | " << "Process-philosopher got number: " << philosopherNumber << '\n';

    if (philosopherNumber > 5) {
        std::cout << '[' << processPid << ']' << " | " << "No place left at the table (>_<). Process leaving." << '\n';
        sem_post(&sharedMem->mutexShm);
        exit(0);
    } else {
        std::cout << '[' << processPid << ']' << " | " << "There are " << 5 - philosopherNumber
                  << " more free sits at the table." << "Process joining the table." << '\n';
    }

    sem_post(&sharedMem->mutexShm);

    // Определение индексов семафоров-вилок процесса в массиве неименованных семафоров-вилок в разделяемой памяти
    leftForkSemIdx = (philosopherNumber - 1);
    std::cout << '[' << processPid << ']' << " | " << "Left fork-semaphore has index: " << leftForkSemIdx<< '\n';
    rightForkSemIdx = (philosopherNumber % 5);
    std::cout << '[' << processPid << ']' << " | " << "Right fork-semaphore has index: " << rightForkSemIdx << '\n';
}

void close_philosopher() {
    if (munmap(sharedMem, sizeof(shared_memory_struct)) == -1) {
        std::cout << '[' << processPid << ']' << " | " << "Unable to unmap shared memory from local process." << '\n';
    }
    close(shmId);
}

void unlink_all() {
    if (shm_unlink(shared_memory_name) == -1) {
        std::cout << '[' << processPid << ']' << " | " << "Unable to delete shared memory with name: "
                  << shared_memory_name << " from system."
                  << '\n';
    } else {
        std::cout << '[' << processPid << ']' << " | " << "Shared memory with name: "
                  << shared_memory_name << " deleted from system."
                  << '\n';
    }
}

