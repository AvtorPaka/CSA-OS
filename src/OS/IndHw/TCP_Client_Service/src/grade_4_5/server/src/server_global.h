#ifndef CPPTASKS_SERVER_GLOBAL_H
#define CPPTASKS_SERVER_GLOBAL_H

#include "common/common_sockets.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <semaphore.h>
#include <csignal>
#include <ctime>
#include <cerrno>
#include <random>

extern const char *WAITER_SEM_NAME;
extern const char *FORK_SEM_BASE_NAME;

typedef struct {
    int id;
    int client_socket_fd;
    pthread_t thread_id;
    int active;
    int has_waiter_permission;
    int has_left_fork;
    int has_right_fork;
    int left_fork_idx;
    int right_fork_idx;
} philosopher_state_t;

extern philosopher_state_t philosophers[NUM_PHILOSOPHERS];
extern sem_t *forks_sem_ptr[NUM_PHILOSOPHERS];     // Массив указателей на семафоры вилок
extern sem_t *waiter_semaphore_ptr;                // Указатель на семафор-счетчик "официанта"
extern  pthread_mutex_t server_mutex;               // Мьютекс для защиты общих данных сервера

extern int active_philosophers_count;
extern int server_fd_global;
extern volatile sig_atomic_t server_shutdown_flag;

// Функция инициализации семафор и данных сервера (стола)
void init_server_state();

// Функция для очистки (закрытия и удаления) именованных семафор
void cleanup_all_named_semaphores();

// Обработчик сигналов для сервера
void server_signal_handler(int sig);

// Функция очистки ресурсов на стороне сервера, связанных с клиентом-философом
void cleanup_philosopher_resources_locked(philosopher_state_t* p_state);

// Функция-обработчик клиента для каждого принятого tcp-соединения со стороны клиента
void *handle_client(void *arg);

#endif //CPPTASKS_SERVER_GLOBAL_H
