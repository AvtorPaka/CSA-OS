#ifndef CPPTASKS_COMMON_SOCKETS_H
#define CPPTASKS_COMMON_SOCKETS_H

#include <random>
#include <unistd.h>

#define NUM_PHILOSOPHERS 5
#define DEFAULT_PORT 7070
#define BUFFER_SIZE 1024

// Сообщения от клиента к серверу
#define MSG_JOIN "JOIN"
#define MSG_ACQUIRE "ACQUIRE"
#define MSG_RELEASE "RELEASE"
#define MSG_LEAVE "LEAVE"

// Сообщения от сервера к клиенту
#define RSP_ID "ID"
#define RSP_FULL "FULL"
#define RSP_GRANTED "GRANTED"
#define RSP_OK "OK"
#define RSP_BYE "BYE"
#define RSP_ERROR "ERROR"
#define MSG_SERVER_DOWN "SERVER_DOWN"

// Функция для логирования с префиксом
void print_log(const char* Tsource, int id, const char* message);

// Генерация случайного времени для размышлений/еды
void random_sleep();

#endif //CPPTASKS_COMMON_SOCKETS_H
