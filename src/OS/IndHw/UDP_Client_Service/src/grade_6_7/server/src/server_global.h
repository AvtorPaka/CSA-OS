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
#include <vector>

#define MAX_LOGGERS 10

extern const char *WAITER_SEM_NAME;
extern const char *FORK_SEM_BASE_NAME;

typedef struct {
    int id;
    struct sockaddr_in client_addr;
    socklen_t client_addr_len;
    int comm_pipe_fd[2];
    pthread_t thread_id;
    int active;
    int has_waiter_permission;
    int has_left_fork;
    int has_right_fork;
    int left_fork_idx;
    int right_fork_idx;
    char last_received_message[BUFFER_SIZE];
} philosopher_state_t;

typedef struct {
    struct sockaddr_in client_addr;
    socklen_t client_addr_len;
    int active;
} logger_client_state_t;

extern philosopher_state_t philosophers[NUM_PHILOSOPHERS];
extern sem_t *forks_sem_ptr[NUM_PHILOSOPHERS];
extern sem_t *waiter_semaphore_ptr;
extern pthread_mutex_t server_mutex;

extern logger_client_state_t loggers[MAX_LOGGERS];
extern int active_loggers_count;
extern pthread_mutex_t loggers_mutex;

extern int active_philosophers_count;
extern int server_udp_fd_global;
extern sig_atomic_t server_shutdown_flag;

void init_server_state();
void cleanup_all_named_semaphores();
void server_signal_handler(int sig);
void cleanup_philosopher_semaphores_if_held(philosopher_state_t* p_state);
void *handle_philosopher_requests(void *arg);
void broadcast_to_loggers(const char* message);

#endif