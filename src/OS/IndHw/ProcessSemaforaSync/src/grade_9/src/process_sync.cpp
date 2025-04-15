#include "common/common.h"
#include <random>
#include <csignal>

int main() {
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
        P(forkWaiterSemId);
        has_waiter_permission = true;
        std::cout << '[' << processPid << ']' << " | " << "Got permission from the waiter to grab forks." << '\n';

        // Получение левой вилки
        std::cout << '[' << processPid << ']' << " | " << "Trying to grab left fork: " << leftForkSemId <<  '\n';
        P(leftForkSemId);
        has_left_fork = true;
        std::cout << '[' << processPid << ']' << " | " << "Grab left fork " << leftForkSemId << '\n';

        // Получение правой вилки
        std::cout << '[' << processPid << ']' << " | " << "Trying to grab right fork: " << rightForkSemId << '\n';
        P(rightForkSemId);
        has_right_fork = true;
        std::cout << '[' << processPid << ']' << " | " << "Grab right fork: " << rightForkSemId << '\n';

        std::cout << '[' << processPid << ']' << " | " << "Start eating." << '\n';
        usleep(dist_time(gen));
        eatTimes++;
        std::cout << '[' << processPid << ']' << " | " << "Done eating." << '\n';

        // Освобождение семафор
        V(leftForkSemId);
        has_left_fork = false;
        std::cout << '[' << processPid << ']' << " | " << "Release left fork: " << leftForkSemId <<  '\n';
        V(rightForkSemId);
        has_right_fork = false;
        std::cout << '[' << processPid << ']' << " | " << "Release right fork: " << rightForkSemId <<  '\n';
        V(forkWaiterSemId);
        has_waiter_permission = false;

        // Размышления
        std::cout << '[' << processPid << ']' << " | " << "Start thinking." << '\n';
        usleep(dist_time(gen));
        thinkTimes++;
        std::cout << '[' << processPid << ']' << " | " << "Done thinking." << '\n';
    }

    return 0;
}

