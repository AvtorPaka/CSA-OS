#include "common/common.h"
#include <random>
#include <csignal>
#include <sys/wait.h>

std::vector<pid_t> child_pids;
std::atomic<bool> running{true};

void parent_sig_handler(int sig) {
    if (sig != SIGINT && sig != SIGTERM) {
        return;
    }

    // Ждем завершение всех дочерних процессов
    running = false;
    for (pid_t pid : child_pids) {
        kill(pid, SIGTERM);
    }
}

// Идентичный процесс для каждого философа
void philosopherProcess() {
    struct sigaction sa;
    sa.sa_handler = sigFunc;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, nullptr);
    sigaction(SIGTERM, &sa, nullptr);

    // распределение времени засыпания от 0,5 до 1,5 секунды
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution dist_time(500000, 1500000);

    init_philosopher(); // Инициализация

    while (true) {

        // Попытка поесть
        // Получение разрешения на взятие вилок
        std::cout << '[' << processPid << ']' << " | " << "Trying to get permission from the waiter to grab forks." << '\n';
        sem_wait(&sharedMem->forkWaiter);
        has_waiter_permission = true;
        std::cout << '[' << processPid << ']' << " | " << "Got permission from the waiter to grab forks." << '\n';

        // Получение левой вилки
        std::cout << '[' << processPid << ']' << " | " << "Trying to grab left fork with idx: " << leftForkSemIdx<<  '\n';
        sem_wait(&sharedMem->forks[leftForkSemIdx]);
        has_left_fork = true;
        std::cout << '[' << processPid << ']' << " | " << "Grab left fork with idx: " << leftForkSemIdx << '\n';

        // Получение правой вилки
        std::cout << '[' << processPid << ']' << " | " << "Trying to grab right fork with idx: " << rightForkSemIdx << '\n';
        sem_wait(&sharedMem->forks[rightForkSemIdx]);
        has_right_fork = true;
        std::cout << '[' << processPid << ']' << " | " << "Grab right fork with idx: " << rightForkSemIdx << '\n';

        std::cout << '[' << processPid << ']' << " | " << "Start eating." << '\n';
        usleep(dist_time(gen));
        eatTimes++;
        std::cout << '[' << processPid << ']' << " | " << "Done eating." << '\n';

        // Освобождение семафор
        sem_post(&sharedMem->forks[leftForkSemIdx]);
        has_left_fork = false;
        std::cout << '[' << processPid << ']' << " | " << "Release left fork with idx: " << leftForkSemIdx <<  '\n';
        sem_post(&sharedMem->forks[rightForkSemIdx]);
        has_right_fork = false;
        std::cout << '[' << processPid << ']' << " | " << "Release right fork with idx: " << rightForkSemIdx <<  '\n';
        sem_post(&sharedMem->forkWaiter);
        has_waiter_permission = false;

        // Размышления
        std::cout << '[' << processPid << ']' << " | " << "Start thinking." << '\n';
        usleep(dist_time(gen));
        thinkTimes++;
        std::cout << '[' << processPid << ']' << " | " << "Done thinking." << '\n';
    }
}

int main() {
    struct sigaction sa;
    sa.sa_handler = parent_sig_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, nullptr);
    sigaction(SIGTERM, &sa, nullptr);

    for(int i = 0; i < 5; ++i) {
        pid_t pid = fork();

        if(pid == 0) {
            philosopherProcess();
            exit(1);
        }
        else if(pid > 0) {
            child_pids.push_back(pid);
        }
        else {
            perror("fork");
            exit(1);
        }
    }

    while (running) {
        pid_t pid = waitpid(-1, nullptr, WNOHANG);
        if (pid > 0) {
            child_pids.erase(std::remove(child_pids.begin(), child_pids.end(), pid), child_pids.end());
        }
        usleep(100000);
    }

    for (pid_t pid : child_pids) {
        waitpid(pid, nullptr, 0);
    }


    return 0;
}

