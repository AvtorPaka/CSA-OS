#include "common_sockets.h"


void random_sleep() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dist(500000, 1500000); // 0.5 - 1.5 seconds in microseconds
    usleep(dist(gen));
}

void print_log(const char* Tsource, int id, const char* message) {
    if (id != -1) {
        printf("[%s %d] %s\n", Tsource, id, message);
    } else {
        printf("[%s] %s\n", Tsource, message);
    }
    fflush(stdout);
}