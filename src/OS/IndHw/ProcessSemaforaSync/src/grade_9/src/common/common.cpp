#include "common.h"

namespace {
    std::string build_ipc_path(const char *relative_path) {
        const char *base_dir = std::getenv("PATH_TO_IPC_DIR");
        if (!base_dir) {
            base_dir = "../ipc";
        }
        return std::string(base_dir) + "/" + relative_path;
    }

    const std::string MESSAGE_QUERY_PATH = build_ipc_path("mq/philosopher_mq");
    const std::string MUTEX_SEM_PATH = build_ipc_path("sem/philosopher_mutex_sem");
    const std::string WAITER_SEM_PATH = build_ipc_path("sem/philosopher_waiter_sem");
    const std::string FORK_BASE_PATH = build_ipc_path("sem/philosopher_fork_");
}

const char *message_query_path = MESSAGE_QUERY_PATH.c_str();
int mqId;

// Семафора-мьютекс для синхронизации доступа к разделяемой памяти
const char *mutex_sem_path = MUTEX_SEM_PATH.c_str();
int mutexSemId;

// Семафора-счетчик количество свободных вилок - 1, для предотвращения ситуации дедлока
const char *waiter_sem_path = WAITER_SEM_PATH.c_str();
int forkWaiterSemId;

// Префикс имен семафор-вилок
const char *sem_fork_base_path = FORK_BASE_PATH.c_str();

// Левый и правый семафор-вилка для процесса-философа
int leftForkSemId;
int rightForkSemId;

int philosopherNumber;
pid_t processPid;

std::atomic<bool> has_waiter_permission{false};
std::atomic<bool> has_left_fork{false};
std::atomic<bool> has_right_fork{false};

int eatTimes;
int thinkTimes;

void sigFunc(int sig) {
    if (sig != SIGINT && sig != SIGTERM) {
        return;
    }

    std::cout << '[' << processPid << ']' << " | " << "Signal received. Leaving table. Ate " << eatTimes
              << " times. Thought " << thinkTimes << " times." << '\n';

    // Освобождения ресурсов в случае их занятости в момент выхода
    if (has_right_fork.load()) {
        std::cout << '[' << processPid << ']' << " | " << "Releasing awaited right fork semaphore: " << rightForkSemId
                  << '\n';
        V(rightForkSemId);
    }
    if (has_left_fork.load()) {
        std::cout << '[' << processPid << ']' << " | " << "Releasing awaited left fork semaphore: " << leftForkSemId
                  << '\n';
        V(leftForkSemId);
    }
    if (has_waiter_permission.load()) {
        std::cout << '[' << processPid << ']' << " | " << "Releasing awaited waiter semaphore: " << forkWaiterSemId
                  << '\n';
        V(forkWaiterSemId);
    }

    // Уменьшение числа занятых мест в разделяемой памяти
    int placesLeft;
    P(mutexSemId);

    struct Message msg;
    shared_memory_struct current_state{};

    if (msgrcv(mqId, &msg, sizeof(msg.data), 1, IPC_NOWAIT) > 0) {
        current_state = msg.data;
    }

    if (philosopherNumber <= 5) {
        current_state.hasSit[philosopherNumber - 1] = false;
    }

    placesLeft = current_state.philosopher_count - 1;
    if (placesLeft >= 0) {
        current_state.philosopher_count = placesLeft;
    }

    msg.mtype = 1; // Указываем тип сообщения
    msg.data = current_state;
    msgsnd(mqId, &msg, sizeof(msg.data), IPC_NOWAIT);

    V(mutexSemId);

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

int initSystemVSemaphore(const char *path, int initValue) {
    int semId;
    key_t semKey;
    if ((semKey = ftok(path, 0)) < 0) {
        std::cout << '[' << processPid << ']' << " | " << "Unable to create SystemV ipc key for path: "
                  << path << '\n';
        exit(1);
    }

    if ((semId = semget(semKey, 1, 0666 | IPC_CREAT | IPC_EXCL)) < 0) {
        if ((semId = semget(semKey, 1, 0)) < 0) {
            std::cout << '[' << processPid << ']' << " | " << "Unable to connect to semaphore with path: "
                      << path << '\n';
            exit(1);
        }
    } else {
        semctl(semId, 0, SETVAL, initValue);
        std::cout << '[' << processPid << ']' << " | " << "Semaphore for path: " << path << " created and have id: "
                  << semId << '\n';
    }


    return semId;
}

void init_philosopher() {
    processPid = getpid();

    // Инициализация мьютекс-семафора доступа к разделяемой памяти
    mutexSemId = initSystemVSemaphore(mutex_sem_path, 1);

    // Инициализация семафора официанта-счетчика
    forkWaiterSemId = initSystemVSemaphore(waiter_sem_path, 4);


    // Определение номера философа через очередь сообщений
    P(mutexSemId);

    key_t mqKey;
    if ((mqKey = ftok(message_query_path, 0)) < 0) {
        std::cout << '[' << processPid << ']' << " | " << "Unable to create SystemV ipc key for path: "
                  << message_query_path << '\n';
        V(mutexSemId);
        exit(1);
    }

    if ((mqId = msgget(mqKey, 0666 | IPC_CREAT | IPC_EXCL)) < 0) {
        if (errno == EEXIST) {
            //Случай если уже существует
            if ((mqId = msgget(mqKey, 0)) < 0) {
                std::cout << '[' << processPid << ']' << " | " << "Unable to open shared memory with path: "
                          << message_query_path
                          << '\n';
                V(mutexSemId);
                exit(1);
            }

        } else {
            std::cout << '[' << processPid << ']' << " | " << "Unable to open shared memory with path: "
                      << message_query_path
                      << '\n';
            V(mutexSemId);
            exit(1);
        }
    } else {
        // Случай первичной инициализации
        Message init_msg{};
        init_msg.mtype = 1;
        init_msg.data.philosopher_count = 0;

        for (int32_t i = 0; i < NUM_OF_SITS; ++i) {
            init_msg.data.hasSit[i] = false;
        }

        if (msgsnd(mqId, &init_msg, sizeof(init_msg.data), 0) == -1) {
            std::cout << "[" << processPid << "] | Initialization failed: " << strerror(errno) << '\n';
            V(mutexSemId);
            exit(1);
        }
    }

    // Читаем последнее актуальное сообщение
    Message msg;
    shared_memory_struct current_state{};
    while (msgrcv(mqId, &msg, sizeof(msg.data), 1, IPC_NOWAIT) > 0) {
        current_state = msg.data;
    }

    philosopherNumber = 6;
    for (int32_t i = 0; i < NUM_OF_SITS; ++i) {
        if (!current_state.hasSit[i]) {
            current_state.hasSit[i] = true;
            current_state.philosopher_count++;
            philosopherNumber = i + 1;
            break;
        }
    }


    Message update_msg{};
    update_msg.mtype = 1;
    update_msg.data = current_state;

    if (msgsnd(mqId, &update_msg, sizeof(update_msg.data), 0) == -1) {
        std::cout << "[" << processPid << "] | Failed to : " << strerror(errno) << '\n';
        V(mutexSemId);
        exit(1);
    }


    std::cout << '[' << processPid << ']' << " | " << "Process-philosopher got number: " << philosopherNumber << '\n';

    if (philosopherNumber > 5) {
        std::cout << '[' << processPid << ']' << " | " << "No place left at the table (>_<). Process leaving." << '\n';
        V(mutexSemId);
        exit(0);
    } else {
        std::cout << '[' << processPid << ']' << " | " << "There are " << 5 - philosopherNumber
                  << " more free sits at the table." << "Process joining the table." << '\n';
    }

    V(mutexSemId);

    // Инициализация семафор-вилок процесса
    char forkPath[64];

    // Левая вилка-семафор
    snprintf(forkPath, sizeof(forkPath), "%s%d", sem_fork_base_path, (philosopherNumber - 1));
    leftForkSemId = initSystemVSemaphore(forkPath, 1);
    std::cout << '[' << processPid << ']' << " | " << "Left fork-semaphore has path: " << forkPath << " ,id : "
              << leftForkSemId << '\n';


    // Правая вилка-семафор
    snprintf(forkPath, sizeof(forkPath), "%s%d", sem_fork_base_path, (philosopherNumber % 5));
    rightForkSemId = initSystemVSemaphore(forkPath, 1);
    std::cout << '[' << processPid << ']' << " | " << "Right fork-semaphore has name: " << forkPath << " ,id : "
              << rightForkSemId << '\n';

}

void close_philosopher() {
    // Ничего не требуется
}

void unlink_all() {
    unlink_sem_one(mutexSemId);
    unlink_sem_one(forkWaiterSemId);

    char forkName[40];
    key_t semForkKey;
    int semForkId;
    for (int i = 0; i < 5; ++i) {
        snprintf(forkName, sizeof(forkName), "%s%d", sem_fork_base_path, i);

        semForkKey = ftok(forkName, 0);

        if ((semForkId = semget(semForkKey, 1, 0)) < 0) {
            continue;
        }

        unlink_sem_one(semForkId);
    }

    if (msgctl(mqId, IPC_RMID, nullptr) >= 0) {
        std::cout << '[' << processPid << ']' << " | " << "SystemV ipc message query with id: " << mqId
                  << " deleted from system." << '\n';
    }
}

void unlink_sem_one(int semId) {
    if (semctl(semId, 0, IPC_RMID, 0) >= 0) {
        std::cout << '[' << processPid << ']' << " | " << "Semaphore with id: "
                  << semId << " deleted from system."
                  << '\n';
    }
}

void P(int semId) {
    sembuf op = {0, -1, 0};
    semop(semId, &op, 1);
}

void V(int semId) {
    sembuf op = {0, 1, 0};
    semop(semId, &op, 1);
}
