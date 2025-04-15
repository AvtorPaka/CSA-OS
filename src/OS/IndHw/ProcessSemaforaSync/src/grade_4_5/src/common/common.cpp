#include "common.h"

const char *shared_memory_name = "/philosopher-shared-mem";
int shmId;
shared_memory_struct *sharedMem;

const char *mutex_sem_shm_name = "/philosopher-mutex-semaphore";
sem_t *mutexShm;

const char *sem_fork_waiter_name = "/philosopher-waiter-semaphore";
sem_t *forkWaiter;

const char *sem_fork_base_name = "/philosopher-fork-semaphore-";

std::string left_fork_name;
std::string right_fork_name;
sem_t *left_fork;
sem_t *right_fork;

int philosopherNumber;
pid_t processPid;

std::atomic<bool> has_waiter_permission{false};
std::atomic<bool> has_left_fork{false};
std::atomic<bool> has_right_fork{false};

int eatTimes;
int thinkTimes;

void sigFunc(int sig) {
    if (sig != SIGTERM) {
        return;
    }

    std::cout << '[' << processPid << ']' << " | " << "Signal received. Leaving table. Ate " << eatTimes << " times. Thought " << thinkTimes << " times." << '\n';

    // Освобождения ресурсов в случае их занятости в момент выхода
    if (has_right_fork.load()) {
        std::cout << '[' << processPid << ']' << " | " << "Releasing awaited right fork semaphore: " << right_fork_name
                  << '\n';
        sem_post(right_fork);
    }
    if (has_left_fork.load()) {
        std::cout << '[' << processPid << ']' << " | " << "Releasing awaited left fork semaphore: " << left_fork_name
                  << '\n';
        sem_post(left_fork);
    }
    if (has_waiter_permission.load()) {
        std::cout << '[' << processPid << ']' << " | " << "Releasing awaited waiter semaphore: " << sem_fork_waiter_name
                  << '\n';
        sem_post(forkWaiter);
    }

    // Уменьшение числа занятых мест в разделяемой памяти
    int placesLeft;
    sem_wait(mutexShm);

    placesLeft = sharedMem->philosopher_count - 1;
    if (placesLeft >= 0) {
        sharedMem->philosopher_count = placesLeft;
    }
    sem_post(mutexShm);

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

    // Инициализация общих семафор
    if ((mutexShm = sem_open(mutex_sem_shm_name, O_CREAT, 0666, 1)) == nullptr) {
        std::cout << '[' << processPid << ']' << " | " << "Unable to open semaphore with name: " << mutex_sem_shm_name
                  << '\n';
        exit(-1);
    }

    if ((forkWaiter = sem_open(sem_fork_waiter_name, O_CREAT, 0666, 4)) == nullptr) {
        std::cout << '[' << processPid << ']' << " | " << "Unable to open semaphore with name: " << sem_fork_waiter_name
                  << '\n';
        exit(-1);
    }

    // Определение номера философа через разделяемую память
    sem_wait(mutexShm);

    if ((shmId = shm_open(shared_memory_name, O_CREAT | O_RDWR | O_EXCL, 0666)) == -1) {
        if (errno == EEXIST) {
            //Случай если память уже существует
            if ((shmId = shm_open(shared_memory_name, O_RDWR, 0666)) == -1) {
                std::cout << '[' << processPid << ']' << " | " << "Unable to open shared memory with name: "
                          << shared_memory_name
                          << '\n';
                sem_post(mutexShm);
                exit(1);
            } else {
                sharedMem = static_cast<shared_memory_struct *>(mmap(nullptr, sizeof(shared_memory_struct),
                                                                     PROT_WRITE | PROT_READ, MAP_SHARED,
                                                                     shmId, 0));
            }
        } else {
            std::cout << '[' << processPid << ']' << " | " << "Unable to open shared memory with name: "
                      << shared_memory_name
                      << '\n';
            sem_post(mutexShm);
            exit(1);
        }
    } else {
        // Случай первичной инициализации памяти

        if (ftruncate(shmId, sizeof(shared_memory_struct)) == -1) {
            std::cout << '[' << processPid << ']' << " | " << "Unable to truncate shared memory with name: "
                      << shared_memory_name
                      << ", id: " << shmId << '\n';
            sem_post(mutexShm);
            exit(1);
        } else {
            std::cout << '[' << processPid << ']' << " | " << "Shared memory with name: " << shared_memory_name
                      << ", id: " << shmId << " size set to " << sizeof(shared_memory_struct) << '\n';
        }

        sharedMem = static_cast<shared_memory_struct *>(mmap(nullptr, sizeof(shared_memory_struct),
                                                             PROT_WRITE | PROT_READ, MAP_SHARED,
                                                             shmId, 0));
        sharedMem->philosopher_count = 0;
    }

    philosopherNumber = sharedMem->philosopher_count + 1;

    if (philosopherNumber <= 5) {
        sharedMem->philosopher_count = philosopherNumber;
    }


    std::cout << '[' << processPid << ']' << " | " << "Process-philosopher got number: " << philosopherNumber << '\n';

    if (philosopherNumber > 5) {
        std::cout << '[' << processPid << ']' << " | " << "No place left at the table (>_<). Process leaving." << '\n';
        sem_post(mutexShm);
        exit(0);
    } else {
        std::cout << '[' << processPid << ']' << " | " << "There are " << 5 - philosopherNumber
                  << " more free sits at the table." << "Process joining the table." << '\n';
    }

    sem_post(mutexShm);

    // Инициализация семафор-вилок процесса
    char forkName[40];

    // Левая вилка-семафор
    snprintf(forkName, sizeof(forkName), "%s%d", sem_fork_base_name, (philosopherNumber - 1));
    left_fork_name = forkName;
    std::cout << '[' << processPid << ']' << " | " << "Left fork-semaphore has name: " << forkName << '\n';

    if ((left_fork = sem_open(forkName, O_CREAT, 0666, 1)) == nullptr) {
        std::cout << '[' << processPid << ']' << " | " << "Unable to open semaphore with name: " << forkName
                  << '\n';
        exit(-1);
    }

    // Правая вилка-семафор
    snprintf(forkName, sizeof(forkName), "%s%d", sem_fork_base_name, (philosopherNumber % 5));
    right_fork_name = forkName;
    std::cout << '[' << processPid << ']' << " | " << "Right fork-semaphore has name: " << forkName << '\n';
    if ((right_fork = sem_open(forkName, O_CREAT, 0666, 1)) == nullptr) {
        std::cout << '[' << processPid << ']' << " | " << "Unable to open semaphore with name: " << forkName
                  << '\n';
        exit(-1);
    }
}

void close_philosopher() {
    if (sem_close(mutexShm) == -1) {
        std::cout << '[' << processPid << ']' << " | " << "Unable to close mutex semaphore." << '\n';
    }
    if (sem_close(forkWaiter) == -1) {
        std::cout << '[' << processPid << ']' << " | " << "Unable to close waiter-counter semaphore." << '\n';
    }
    if (sem_close(left_fork) == -1) {
        std::cout << '[' << processPid << ']' << " | " << "Unable to close left fork semaphore." << '\n';
    }
    if (sem_close(right_fork) == -1) {
        std::cout << '[' << processPid << ']' << " | " << "Unable to close right fork semaphore." << '\n';
    }

    if (munmap(sharedMem, sizeof(shared_memory_struct)) == -1) {
        std::cout << '[' << processPid << ']' << " | " << "Unable to unmap shared memory from local process." << '\n';
    }
    close(shmId);
}

void unlink_all() {
    unlink_sem_one(mutex_sem_shm_name);
    unlink_sem_one(sem_fork_waiter_name);

    char forkName[40];
    for (int i = 0; i < 5; ++i) {
        snprintf(forkName, sizeof(forkName), "%s%d", sem_fork_base_name, i);
        unlink_sem_one(forkName);
    }

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

void unlink_sem_one(const char *posixName) {
    if (sem_unlink(posixName) != -1) {
        std::cout << '[' << processPid << ']' << " | " << "Semaphore with name: "
                  << posixName << " deleted from system."
                  << '\n';
    }
}
