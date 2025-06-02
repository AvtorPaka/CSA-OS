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
int server_udp_fd_global = -1;
sig_atomic_t server_shutdown_flag = 0;

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
        memset(&philosophers[i].client_addr, 0, sizeof(philosophers[i].client_addr));
        philosophers[i].client_addr_len = sizeof(philosophers[i].client_addr);
        philosophers[i].comm_pipe_fd[0] = -1;
        philosophers[i].comm_pipe_fd[1] = -1;
        philosophers[i].thread_id = 0;
        philosophers[i].active = 0;
        philosophers[i].has_waiter_permission = 0;
        philosophers[i].has_left_fork = 0;
        philosophers[i].has_right_fork = 0;
        philosophers[i].left_fork_idx = -1;
        philosophers[i].right_fork_idx = -1;
    }
    active_philosophers_count = 0;

    for (int i = 0; i < MAX_LOGGERS; ++i) {
        memset(&loggers[i].client_addr, 0, sizeof(loggers[i].client_addr));
        loggers[i].client_addr_len = sizeof(loggers[i].client_addr);
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
    } else if (len == 0 && BUFFER_SIZE > 0) {
        message_with_newline[0] = '\n';
        message_with_newline[1] = '\0';
    }


    for (int i = 0; i < MAX_LOGGERS; ++i) {
        if (loggers[i].active && server_udp_fd_global != -1) {
            if (sendto(server_udp_fd_global, message_with_newline, strlen(message_with_newline), 0,
                       (struct sockaddr *)&loggers[i].client_addr, loggers[i].client_addr_len) < 0) {
                char log_buf[128];
                snprintf(log_buf, sizeof(log_buf), "sendto to logger (idx %d) failed: %s. Logger might be unresponsive.", i, strerror(errno));
                print_log("Server", -1, log_buf);
            }
        }
    }
    pthread_mutex_unlock(&loggers_mutex);
}


void cleanup_all_named_semaphores() {
    char sem_name_buffer[256];
    print_log("Server", -1, "Attempting to clean up named semaphores...");

    if (waiter_semaphore_ptr != SEM_FAILED && waiter_semaphore_ptr != nullptr) {
        if (sem_close(waiter_semaphore_ptr) == -1) {
             perror("sem_close waiter failed");
        }
        waiter_semaphore_ptr = SEM_FAILED;
    }
    if (sem_unlink(WAITER_SEM_NAME) == 0) {
        snprintf(sem_name_buffer, sizeof(sem_name_buffer), "Semaphore %s unlinked.", WAITER_SEM_NAME);
        print_log("Server", -1, sem_name_buffer);
    } else if (errno != ENOENT) {
        snprintf(sem_name_buffer, sizeof(sem_name_buffer), "Warning: Failed to unlink semaphore %s: %s", WAITER_SEM_NAME, strerror(errno));
        print_log("Server", -1, sem_name_buffer);
    }

    for (int i = 0; i < NUM_PHILOSOPHERS; ++i) {
        if (forks_sem_ptr[i] != SEM_FAILED && forks_sem_ptr[i] != nullptr) {
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
            snprintf(log_buf_temp, sizeof(log_buf_temp), "Warning: Failed to unlink semaphore %s: %s", sem_name_buffer, strerror(errno));
            print_log("Server", -1, log_buf_temp);
        }
    }
    print_log("Server", -1, "Named semaphores cleanup finished.");
}

void cleanup_philosopher_semaphores_if_held(philosopher_state_t* p_state) {
    char log_buffer[128];
    char broadcast_msg[192];
    int p_id = p_state->id;

    if (p_state->has_right_fork) {
        sem_post(forks_sem_ptr[p_state->right_fork_idx]);
        p_state->has_right_fork = 0;
        char sem_name[64];
        snprintf(sem_name, sizeof(sem_name), "%s%d", FORK_SEM_BASE_NAME, p_state->right_fork_idx);
        snprintf(log_buffer, sizeof(log_buffer), "P%d (thread) released right fork %s.", p_id, sem_name);
        print_log("Server", -1, log_buffer);
        snprintf(broadcast_msg, sizeof(broadcast_msg), "[Server for P%d] (Thread) Released right fork %s.", p_id, sem_name);
        broadcast_to_loggers(broadcast_msg);
    }
    if (p_state->has_left_fork) {
        sem_post(forks_sem_ptr[p_state->left_fork_idx]);
        p_state->has_left_fork = 0;
        char sem_name[64];
        snprintf(sem_name, sizeof(sem_name), "%s%d", FORK_SEM_BASE_NAME, p_state->left_fork_idx);
        snprintf(log_buffer, sizeof(log_buffer), "P%d (thread) released left fork %s.", p_id, sem_name);
        print_log("Server", -1, log_buffer);
        snprintf(broadcast_msg, sizeof(broadcast_msg), "[Server for P%d] (Thread) Released left fork %s.", p_id, sem_name);
        broadcast_to_loggers(broadcast_msg);
    }
    if (p_state->has_waiter_permission) {
        sem_post(waiter_semaphore_ptr);
        p_state->has_waiter_permission = 0;
        snprintf(log_buffer, sizeof(log_buffer), "P%d (thread) released waiter permission (%s).", p_id, WAITER_SEM_NAME);
        print_log("Server", -1, log_buffer);
        snprintf(broadcast_msg, sizeof(broadcast_msg), "[Server for P%d] (Thread) Released waiter permission (%s).", p_id, WAITER_SEM_NAME);
        broadcast_to_loggers(broadcast_msg);
    }

    snprintf(log_buffer, sizeof(log_buffer), "P%d (thread) resources (semaphores) ensured released.", p_id);
    print_log("Server", -1, log_buffer);
}


void server_signal_handler(int sig) {
    if (sig == SIGINT || sig == SIGTERM) {
        if (!server_shutdown_flag) {
            print_log("Server", -1, "Shutdown signal received. Setting flag...");
            server_shutdown_flag = 1;
        }
    }
}