#include "server_global.h"

const char *WAITER_SEM_NAME = "/philosopher_waiter_sem";
const char *FORK_SEM_BASE_NAME = "/philosopher_fork_sem_";

philosopher_state_t philosophers[NUM_PHILOSOPHERS];
sem_t *forks_sem_ptr[NUM_PHILOSOPHERS];
sem_t *waiter_semaphore_ptr;
pthread_mutex_t server_mutex;

logger_client_state_t loggers[MAX_LOGGERS];
int active_loggers_count = 0;
pthread_mutex_t loggers_mutex;

int active_philosophers_count = 0;
int server_fd_global = -1;
volatile sig_atomic_t server_shutdown_flag = 0;

void init_server_state() {
    pthread_mutex_init(&server_mutex, nullptr);
    pthread_mutex_init(&loggers_mutex, nullptr);

    char sem_name_buffer[256];
    print_log("Server", -1, "Unlinking pre-existing semaphores (if any)...");
    sem_unlink(WAITER_SEM_NAME);


    for (int i = 0; i < NUM_PHILOSOPHERS; ++i) {
        snprintf(sem_name_buffer, sizeof(sem_name_buffer), "%s%d", FORK_SEM_BASE_NAME, i);
        sem_unlink(sem_name_buffer);
    }
    print_log("Server", -1, "Pre-unlinking finished.");

    waiter_semaphore_ptr = sem_open(WAITER_SEM_NAME, O_CREAT | O_EXCL, 0666, NUM_PHILOSOPHERS - 1);
    if (waiter_semaphore_ptr == SEM_FAILED) {
        if (errno == EEXIST) {
            waiter_semaphore_ptr = sem_open(WAITER_SEM_NAME, 0);
            if (waiter_semaphore_ptr == SEM_FAILED) {
                perror("sem_open (O_EXCL failed, then open existing) for waiter_semaphore failed");
                exit(EXIT_FAILURE);
            }
        } else {
            perror("sem_open (O_CREAT | O_EXCL) for waiter_semaphore failed");
            exit(EXIT_FAILURE);
        }
    }
    snprintf(sem_name_buffer, sizeof(sem_name_buffer), "Waiter semaphore %s opened/created.", WAITER_SEM_NAME);
    print_log("Server", -1, sem_name_buffer);

    // Инициализация семафоров вилок
    for (int i = 0; i < NUM_PHILOSOPHERS; ++i) {
        snprintf(sem_name_buffer, sizeof(sem_name_buffer), "%s%d", FORK_SEM_BASE_NAME, i);
        forks_sem_ptr[i] = sem_open(sem_name_buffer, O_CREAT | O_EXCL, 0666, 1);
        if (forks_sem_ptr[i] == SEM_FAILED) {
            if (errno == EEXIST) {
                forks_sem_ptr[i] = sem_open(sem_name_buffer, 0);
                if (forks_sem_ptr[i] == SEM_FAILED) {
                    char err_buf[300];
                    snprintf(err_buf, sizeof(err_buf), "sem_open (O_EXCL failed, then open existing) for fork %s failed", sem_name_buffer);
                    perror(err_buf);
                    exit(EXIT_FAILURE);
                }
            } else {
                char err_buf[300];
                snprintf(err_buf, sizeof(err_buf), "sem_open (O_CREAT | O_EXCL) for fork_semaphore %s failed", sem_name_buffer);
                perror(err_buf);
                exit(EXIT_FAILURE);
            }
        }
        char log_msg[300];
        snprintf(log_msg, sizeof(log_msg), "Fork semaphore %s opened/created.", sem_name_buffer);
        print_log("Server", -1, log_msg);
    }

    for (int i = 0; i < NUM_PHILOSOPHERS; ++i) {
        philosophers[i].id = i + 1;
        philosophers[i].client_socket_fd = -1;
        philosophers[i].thread_id = 0;
        philosophers[i].active = 0;
        philosophers[i].has_waiter_permission = 0;
        philosophers[i].has_left_fork = 0;
        philosophers[i].has_right_fork = 0;
    }
    active_philosophers_count = 0;

    for (int i = 0; i < MAX_LOGGERS; ++i) {
        loggers[i].client_socket_fd = -1;
        loggers[i].thread_id = 0;
        loggers[i].active = 0;
    }
    active_loggers_count = 0;
}

void broadcast_to_loggers(const char* message) {
    pthread_mutex_lock(&loggers_mutex);
    char message_with_newline[BUFFER_SIZE + 2];
    strncpy(message_with_newline, message, BUFFER_SIZE);
    message_with_newline[BUFFER_SIZE] = '\0';


    size_t len = strlen(message_with_newline);
    if (len > 0 && message_with_newline[len - 1] != '\n') {
        if (len < BUFFER_SIZE) {
            message_with_newline[len] = '\n';
            message_with_newline[len + 1] = '\0';
        } else {
            message_with_newline[BUFFER_SIZE -1] = '\n';
        }
    }


    for (int i = 0; i < MAX_LOGGERS; ++i) {
        if (loggers[i].active && loggers[i].client_socket_fd != -1) {
            if (send(loggers[i].client_socket_fd, message_with_newline, strlen(message_with_newline), MSG_NOSIGNAL) < 0) {
                if (errno == EPIPE || errno == ECONNRESET) {
                    char log_buf[128];
                    snprintf(log_buf, sizeof(log_buf), "Failed to send to logger socket %d (disconnected?). Will be cleaned up by its thread.", loggers[i].client_socket_fd);
                    print_log("Server", -1, log_buf);
                } else {
                    
                }
            }
        }
    }
    pthread_mutex_unlock(&loggers_mutex);
}


void cleanup_all_named_semaphores() {

    char sem_name_buffer[256];
    print_log("Server", -1, "Attempting to clean up named semaphores...");

    if (waiter_semaphore_ptr != SEM_FAILED && waiter_semaphore_ptr != NULL) {
        if (sem_close(waiter_semaphore_ptr) == -1) {
             perror("sem_close waiter failed");
        }
        waiter_semaphore_ptr = SEM_FAILED;
    }
    if (sem_unlink(WAITER_SEM_NAME) == 0) {
        snprintf(sem_name_buffer, sizeof(sem_name_buffer), "Semaphore %s unlinked.", WAITER_SEM_NAME);
        print_log("Server", -1, sem_name_buffer);
    } else if (errno != ENOENT) {
        snprintf(sem_name_buffer, sizeof(sem_name_buffer), "Failed to unlink semaphore %s: %s", WAITER_SEM_NAME, strerror(errno));
        print_log("Server", -1, sem_name_buffer);
    }

    for (int i = 0; i < NUM_PHILOSOPHERS; ++i) {
        if (forks_sem_ptr[i] != SEM_FAILED && forks_sem_ptr[i] != NULL) {
            if (sem_close(forks_sem_ptr[i]) == -1) {
                char err_buf[64];
                snprintf(err_buf, sizeof(err_buf), "sem_close fork %d failed", i);
                perror(err_buf);
            }
            forks_sem_ptr[i] = SEM_FAILED;
        }
        snprintf(sem_name_buffer, sizeof(sem_name_buffer), "%s%d", FORK_SEM_BASE_NAME, i);
        if(sem_unlink(sem_name_buffer) == 0) {
            char log_buf_temp[300];
            snprintf(log_buf_temp, sizeof(log_buf_temp), "Semaphore %s unlinked.", sem_name_buffer);
            print_log("Server", -1, log_buf_temp);
        } else if (errno != ENOENT) {
            char log_buf_temp[300];
            snprintf(log_buf_temp, sizeof(log_buf_temp), "Failed to unlink semaphore %s: %s", sem_name_buffer, strerror(errno));
            print_log("Server", -1, log_buf_temp);
        }
    }
    print_log("Server", -1, "Named semaphores cleanup finished.");
}

void cleanup_philosopher_resources_locked(philosopher_state_t* p_state) {
    char log_buffer[128];
    char broadcast_msg[192];
    int p_id = p_state->id;

    if (p_state->has_right_fork) {
        sem_post(forks_sem_ptr[p_state->right_fork_idx]);
        p_state->has_right_fork = 0;
        char sem_name[64];
        snprintf(sem_name, sizeof(sem_name), "%s%d", FORK_SEM_BASE_NAME, p_state->right_fork_idx);
        snprintf(log_buffer, sizeof(log_buffer), "P%d forcefully released right fork %s.", p_id, sem_name);
        print_log("Server", -1, log_buffer);
        snprintf(broadcast_msg, sizeof(broadcast_msg), "[Srv for P%d] Forcefully released right fork %s.", p_id, sem_name);
        broadcast_to_loggers(broadcast_msg);
    }
    if (p_state->has_left_fork) {
        sem_post(forks_sem_ptr[p_state->left_fork_idx]);
        p_state->has_left_fork = 0;
        char sem_name[64];
        snprintf(sem_name, sizeof(sem_name), "%s%d", FORK_SEM_BASE_NAME, p_state->left_fork_idx);
        snprintf(log_buffer, sizeof(log_buffer), "P%d forcefully released left fork %s.", p_id, sem_name);
        print_log("Server", -1, log_buffer);
        snprintf(broadcast_msg, sizeof(broadcast_msg), "[Srv for P%d] Forcefully released left fork %s.", p_id, sem_name);
        broadcast_to_loggers(broadcast_msg);
    }
    if (p_state->has_waiter_permission) {
        sem_post(waiter_semaphore_ptr);
        p_state->has_waiter_permission = 0;
        snprintf(log_buffer, sizeof(log_buffer), "P%d forcefully released waiter permission (%s).", p_id, WAITER_SEM_NAME);
        print_log("Server", -1, log_buffer);
        snprintf(broadcast_msg, sizeof(broadcast_msg), "[Srv for P%d] Forcefully released waiter permission (%s).", p_id, WAITER_SEM_NAME);
        broadcast_to_loggers(broadcast_msg);
    }

    snprintf(log_buffer, sizeof(log_buffer), "P%d resources (semaphores) ensured released.", p_id);
    print_log("Server", -1, log_buffer);
    snprintf(broadcast_msg, sizeof(broadcast_msg), "[Srv for P%d] Resources (semaphores) ensured released.", p_id);
    broadcast_to_loggers(broadcast_msg);
}


void server_signal_handler(int sig) {
    if (sig == SIGINT || sig == SIGTERM) {
        print_log("Server", -1, "Shutdown signal received. Cleaning up...");
        broadcast_to_loggers("[Server internal] Shutdown signal received. Server is terminating.");
        server_shutdown_flag = 1; 
        
        if (server_fd_global != -1) {
            shutdown(server_fd_global, SHUT_RDWR);
            close(server_fd_global);
            server_fd_global = -1;
        }
    }
}