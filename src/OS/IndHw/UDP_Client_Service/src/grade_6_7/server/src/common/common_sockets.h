#ifndef CPPTASKS_COMMON_SOCKETS_H
#define CPPTASKS_COMMON_SOCKETS_H

#include <random>
#include <unistd.h>
#include <cstdio>
#include <cstring>
#include <sys/socket.h>

#define NUM_PHILOSOPHERS 5
#define DEFAULT_PORT 7070
#define BUFFER_SIZE 1024

// Сообщения от клиента к серверу
#define MSG_JOIN "JOIN"
#define MSG_ACQUIRE "ACQUIRE"
#define MSG_RELEASE "RELEASE"
#define MSG_LEAVE "LEAVE"
#define MSG_JOIN_LOGGER "JOIN_LOGGER"
#define MSG_LEAVE_LOGGER "LEAVE_LOGGER"

// Сообщения от сервера к клиенту
#define RSP_ID "ID"
#define RSP_FULL "FULL"
#define RSP_GRANTED "GRANTED"
#define RSP_OK "OK"
#define RSP_BYE "BYE"
#define RSP_ERROR "ERROR"
#define MSG_SERVER_DOWN "SERVER_DOWN"
#define RSP_LOGGER_ACCEPTED "LOGGER_ACCEPTED"
#define RSP_LOGGER_REJECTED "LOGGER_REJECTED"

// Функция для логирования с префиксом
void print_log(const char* Tsource, int id, const char* message);

// Функция для генерация случайного времени для размышлений/еды
void random_sleep();

#endif //CPPTASKS_COMMON_SOCKETS_H