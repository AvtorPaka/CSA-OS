#include "server_global.h"

void *handle_client(void *arg) {
    auto *p_state = (philosopher_state_t *)arg;
    int client_sock = p_state->client_socket_fd;
    char buffer[BUFFER_SIZE];
    char response[BUFFER_SIZE];
    int philosopher_id = -1;
    char log_buffer[256];

    ssize_t n_read = recv(client_sock, buffer, BUFFER_SIZE - 1, 0);
    if (server_shutdown_flag) {
        goto cleanup_and_exit_thread;
    }

    if (n_read <= 0) {
        if (n_read == 0) {
            snprintf(log_buffer, sizeof(log_buffer), "P%d: Client disconnected before JOIN.", p_state->id);
        } else {
            snprintf(log_buffer, sizeof(log_buffer), "P%d: recv (join) failed: %s", p_state->id, strerror(errno));
        }
        print_log("Server", -1, log_buffer);
        goto cleanup_and_exit_thread;
    }
    buffer[n_read] = '\0';

    if (strncmp(buffer, MSG_JOIN, strlen(MSG_JOIN)) == 0) {
        philosopher_id = p_state->id;
        snprintf(response, BUFFER_SIZE, "%s %d", RSP_ID, philosopher_id);
        if(send(client_sock, response, strlen(response), 0) < 0){
            snprintf(log_buffer, sizeof(log_buffer), "P%d: send RSP_ID failed: %s", philosopher_id, strerror(errno));
            print_log("Server", -1, log_buffer);
            goto cleanup_and_exit_thread;
        }
        snprintf(log_buffer, sizeof(log_buffer), "P%d joined. Socket fd: %d", philosopher_id, client_sock);
        print_log("Server", -1, log_buffer);
    } else {
        snprintf(log_buffer, sizeof(log_buffer), "P%d: Invalid command. Expected JOIN. Got: %s", p_state->id, buffer);
        print_log("Server", -1, log_buffer);
        send(client_sock, RSP_ERROR " Expected JOIN", strlen(RSP_ERROR " Expected JOIN"), 0);
        goto cleanup_and_exit_thread;
    }

    p_state->left_fork_idx = (philosopher_id - 1);
    p_state->right_fork_idx = (philosopher_id % NUM_PHILOSOPHERS);

    while (!server_shutdown_flag) {
        n_read = recv(client_sock, buffer, BUFFER_SIZE - 1, 0);

        if (server_shutdown_flag) break;

        if (n_read < 0) {
            snprintf(log_buffer, sizeof(log_buffer), "P%d: recv failed in loop: %s. Likely server shutdown.", philosopher_id, strerror(errno));
            print_log("Server", -1, log_buffer);
            break;
        }
        if (n_read == 0) {
            snprintf(log_buffer, sizeof(log_buffer), "P%d disconnected by client.", philosopher_id);
            print_log("Server", -1, log_buffer);
            break;
        }
        buffer[n_read] = '\0';
        char* newline = strchr(buffer, '\n');
        if (newline) *newline = '\0';

        snprintf(log_buffer, sizeof(log_buffer), "P%d sent: %s", philosopher_id, buffer);
        print_log("Server", -1, log_buffer);

        char sem_name_log[64];

        if (strncmp(buffer, MSG_ACQUIRE, strlen(MSG_ACQUIRE)) == 0) {
            if (server_shutdown_flag) break;

            snprintf(log_buffer, sizeof(log_buffer), "P%d Waiting for waiter permission: %s.", philosopher_id, WAITER_SEM_NAME);
            print_log("Server", -1, log_buffer);

            sem_wait(waiter_semaphore_ptr);
            if (server_shutdown_flag) {
                sem_post(waiter_semaphore_ptr);
                break;
            }
            p_state->has_waiter_permission = 1;
            snprintf(log_buffer, sizeof(log_buffer), "P%d Got waiter permission.", philosopher_id);
            print_log("Server", -1, log_buffer);

            if (server_shutdown_flag) {
                sem_post(waiter_semaphore_ptr);
                p_state->has_waiter_permission=0;
                break;
            }

            snprintf(sem_name_log, sizeof(sem_name_log), "%s%d", FORK_SEM_BASE_NAME, p_state->left_fork_idx);
            snprintf(log_buffer, sizeof(log_buffer), "P%d Trying to grab left fork: %s.", philosopher_id, sem_name_log);
            print_log("Server", -1, log_buffer);

            sem_wait(forks_sem_ptr[p_state->left_fork_idx]);
            if (server_shutdown_flag) {
                sem_post(forks_sem_ptr[p_state->left_fork_idx]);
                sem_post(waiter_semaphore_ptr);
                p_state->has_waiter_permission=0;
                break;
            }
            p_state->has_left_fork = 1;

            snprintf(log_buffer, sizeof(log_buffer), "P%d Grabbed left fork.", philosopher_id);
            print_log("Server", -1, log_buffer);

            if (server_shutdown_flag) {
                sem_post(forks_sem_ptr[p_state->left_fork_idx]);
                p_state->has_left_fork=0;
                sem_post(waiter_semaphore_ptr);
                p_state->has_waiter_permission=0;
                break;
            }

            snprintf(sem_name_log, sizeof(sem_name_log), "%s%d", FORK_SEM_BASE_NAME, p_state->right_fork_idx);
            snprintf(log_buffer, sizeof(log_buffer), "P%d Trying to grab right fork: %s.", philosopher_id, sem_name_log);
            print_log("Server", -1, log_buffer);
            sem_wait(forks_sem_ptr[p_state->right_fork_idx]);

            if (server_shutdown_flag) {
                sem_post(forks_sem_ptr[p_state->right_fork_idx]);
                sem_post(forks_sem_ptr[p_state->left_fork_idx]);
                p_state->has_left_fork=0;
                sem_post(waiter_semaphore_ptr);
                p_state->has_waiter_permission=0;
                break;

            }
            p_state->has_right_fork = 1;
            snprintf(log_buffer, sizeof(log_buffer), "P%d Grabbed right fork.", philosopher_id);
            print_log("Server", -1, log_buffer);

            if(send(client_sock, RSP_GRANTED, strlen(RSP_GRANTED), 0) < 0) {
                snprintf(log_buffer, sizeof(log_buffer), "P%d: send GRANTED failed: %s", philosopher_id, strerror(errno));
                print_log("Server", -1, log_buffer);
                if(p_state->has_right_fork) {
                    sem_post(forks_sem_ptr[p_state->right_fork_idx]);
                    p_state->has_right_fork=0;
                }
                if(p_state->has_left_fork) {
                    sem_post(forks_sem_ptr[p_state->left_fork_idx]);
                    p_state->has_left_fork=0;
                }
                if(p_state->has_waiter_permission) {
                    sem_post(waiter_semaphore_ptr);
                    p_state->has_waiter_permission=0;
                }
                break;
            }
            snprintf(log_buffer, sizeof(log_buffer), "Sent GRANTED to P%d.", philosopher_id);
            print_log("Server", -1, log_buffer);

        } else if (strncmp(buffer, MSG_RELEASE, strlen(MSG_RELEASE)) == 0) {
            if (p_state->has_left_fork) {
                sem_post(forks_sem_ptr[p_state->left_fork_idx]); p_state->has_left_fork = 0;
                snprintf(log_buffer, sizeof(log_buffer), "P%d Release left fork.", philosopher_id);
                print_log("Server",-1,log_buffer);
            }
            if (p_state->has_right_fork) {
                sem_post(forks_sem_ptr[p_state->right_fork_idx]); p_state->has_right_fork = 0;
                snprintf(log_buffer, sizeof(log_buffer), "P%d Release right fork.", philosopher_id);
                print_log("Server",-1,log_buffer);
            }
            if (p_state->has_waiter_permission) {
                sem_post(waiter_semaphore_ptr); p_state->has_waiter_permission = 0;
                snprintf(log_buffer, sizeof(log_buffer), "P%d Release waiter permission.", philosopher_id);
                print_log("Server",-1,log_buffer);
            }

            if(send(client_sock, RSP_OK, strlen(RSP_OK), 0) < 0) {
                snprintf(log_buffer, sizeof(log_buffer), "P%d: send OK failed: %s", philosopher_id, strerror(errno));
                print_log("Server", -1, log_buffer);
                break;
            }
            snprintf(log_buffer, sizeof(log_buffer), "Sent OK to P%d.", philosopher_id);
            print_log("Server", -1, log_buffer);

        } else if (strncmp(buffer, MSG_LEAVE, strlen(MSG_LEAVE)) == 0) {
            send(client_sock, RSP_BYE, strlen(RSP_BYE), 0);
            snprintf(log_buffer, sizeof(log_buffer), "P%d sent LEAVE. Sent BYE.", philosopher_id);
            print_log("Server", -1, log_buffer);
            break;
        } else {
            snprintf(log_buffer, sizeof(log_buffer), "P%d sent unknown command: %s", philosopher_id, buffer);
            print_log("Server", -1, log_buffer);
            send(client_sock, RSP_ERROR " Unknown command", strlen(RSP_ERROR " Unknown command"), 0);
        }
    }

    cleanup_and_exit_thread:
    snprintf(log_buffer, sizeof(log_buffer), "P%d thread entering cleanup...", p_state->id);
    print_log("Server", -1, log_buffer);

    pthread_mutex_lock(&server_mutex);
    cleanup_philosopher_resources_locked(p_state);

    if (p_state->active) {
        p_state->active = 0;
        active_philosophers_count--;
        snprintf(log_buffer, sizeof(log_buffer), "P%d deactivated. Active now: %d", p_state->id, active_philosophers_count);
        print_log("Server", -1, log_buffer);
    }
    if (p_state->client_socket_fd != -1) {
        close(p_state->client_socket_fd);
        p_state->client_socket_fd = -1;
    }
    pthread_mutex_unlock(&server_mutex);

    snprintf(log_buffer, sizeof(log_buffer), "P%d thread finished.", p_state->id);
    print_log("Server", -1, log_buffer);
    return nullptr;
}