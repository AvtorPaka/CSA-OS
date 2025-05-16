#include "server_global.h"

void *handle_client(void *arg) {
    philosopher_state_t *p_state = (philosopher_state_t *)arg;
    int client_sock = p_state->client_socket_fd;
    char buffer[BUFFER_SIZE];
    int philosopher_id = p_state->id;
    char log_buffer[BUFFER_SIZE];
    char broadcast_buffer[BUFFER_SIZE + 64];
    ssize_t n_read;

    p_state->left_fork_idx = (philosopher_id - 1);
    p_state->right_fork_idx = (philosopher_id % NUM_PHILOSOPHERS);

    char temp_log[256];

    while (!server_shutdown_flag) {
        n_read = recv(client_sock, buffer, BUFFER_SIZE - 1, 0);

        if (server_shutdown_flag) {
            
            snprintf(temp_log, sizeof(temp_log), "P%d: Server shutting down, thread loop exiting.", philosopher_id);
            print_log("Server", -1, temp_log);
            snprintf(broadcast_buffer, sizeof(broadcast_buffer), "[Server for P%d] Server shutting down, client handler loop exiting.", philosopher_id);
            broadcast_to_loggers(broadcast_buffer);
            break;
        }

        if (n_read < 0) {
            if (errno == EINTR && server_shutdown_flag) { 
                 break;
            }
            snprintf(log_buffer, sizeof(log_buffer), "P%d: recv failed in loop: %s.", philosopher_id, strerror(errno));
            print_log("Server", -1, log_buffer);
            snprintf(broadcast_buffer, sizeof(broadcast_buffer), "[Server for P%d] recv failed in loop: %s.", philosopher_id, strerror(errno));
            broadcast_to_loggers(broadcast_buffer);
            break; 
        }
        if (n_read == 0) {
            snprintf(log_buffer, sizeof(log_buffer), "P%d disconnected by client.", philosopher_id);
            print_log("Server", -1, log_buffer);
            snprintf(broadcast_buffer, sizeof(broadcast_buffer), "[Server for P%d] Client disconnected.", philosopher_id);
            broadcast_to_loggers(broadcast_buffer);
            break;
        }
        buffer[n_read] = '\0';
        char* newline = strchr(buffer, '\n');
        if (newline) *newline = '\0';

        snprintf(log_buffer, sizeof(log_buffer), "P%d sent: %s", philosopher_id, buffer);
        print_log("Server", -1, log_buffer);
        snprintf(broadcast_buffer, sizeof(broadcast_buffer), "[P%d -> Server] %s", philosopher_id, buffer);
        broadcast_to_loggers(broadcast_buffer);

        char sem_name_log[64];

        if (strncmp(buffer, MSG_ACQUIRE, strlen(MSG_ACQUIRE)) == 0) {
            if (server_shutdown_flag) break;

            snprintf(log_buffer, sizeof(log_buffer), "P%d Waiting for waiter permission: %s.", philosopher_id, WAITER_SEM_NAME);
            print_log("Server", -1, log_buffer);
            snprintf(broadcast_buffer, sizeof(broadcast_buffer), "[Server for P%d] Waiting for waiter permission: %s.", philosopher_id, WAITER_SEM_NAME);
            broadcast_to_loggers(broadcast_buffer);
            
            sem_wait(waiter_semaphore_ptr); 
            if (server_shutdown_flag) { 
                sem_post(waiter_semaphore_ptr); 
                break;
            }
            p_state->has_waiter_permission = 1;
            snprintf(log_buffer, sizeof(log_buffer), "P%d Got waiter permission.", philosopher_id);
            print_log("Server", -1, log_buffer);
            snprintf(broadcast_buffer, sizeof(broadcast_buffer), "[Server for P%d] Got waiter permission.", philosopher_id);
            broadcast_to_loggers(broadcast_buffer);

            // Захват левой вилки
            snprintf(sem_name_log, sizeof(sem_name_log), "%s%d", FORK_SEM_BASE_NAME, p_state->left_fork_idx);
            snprintf(log_buffer, sizeof(log_buffer), "P%d Trying to grab left fork: %s.", philosopher_id, sem_name_log);
            print_log("Server", -1, log_buffer);
            snprintf(broadcast_buffer, sizeof(broadcast_buffer), "[Server for P%d] Trying to grab left fork: %s.", philosopher_id, sem_name_log);
            broadcast_to_loggers(broadcast_buffer);

            sem_wait(forks_sem_ptr[p_state->left_fork_idx]);
            if (server_shutdown_flag) {
                sem_post(forks_sem_ptr[p_state->left_fork_idx]);
                sem_post(waiter_semaphore_ptr); p_state->has_waiter_permission=0;
                break;
            }
            p_state->has_left_fork = 1;
            snprintf(log_buffer, sizeof(log_buffer), "P%d Grabbed left fork %s.", philosopher_id, sem_name_log);
            print_log("Server", -1, log_buffer);
            snprintf(broadcast_buffer, sizeof(broadcast_buffer), "[Server for P%d] Grabbed left fork %s.", philosopher_id, sem_name_log);
            broadcast_to_loggers(broadcast_buffer);

            // Захват правой вилки
            snprintf(sem_name_log, sizeof(sem_name_log), "%s%d", FORK_SEM_BASE_NAME, p_state->right_fork_idx);
            snprintf(log_buffer, sizeof(log_buffer), "P%d Trying to grab right fork: %s.", philosopher_id, sem_name_log);
            print_log("Server", -1, log_buffer);
            snprintf(broadcast_buffer, sizeof(broadcast_buffer), "[Server for P%d] Trying to grab right fork: %s.", philosopher_id, sem_name_log);
            broadcast_to_loggers(broadcast_buffer);

            sem_wait(forks_sem_ptr[p_state->right_fork_idx]);
            if (server_shutdown_flag) {
                sem_post(forks_sem_ptr[p_state->right_fork_idx]);
                sem_post(forks_sem_ptr[p_state->left_fork_idx]); p_state->has_left_fork=0;
                sem_post(waiter_semaphore_ptr); p_state->has_waiter_permission=0;
                break;
            }
            p_state->has_right_fork = 1;
            snprintf(log_buffer, sizeof(log_buffer), "P%d Grabbed right fork %s.", philosopher_id, sem_name_log);
            print_log("Server", -1, log_buffer);
            snprintf(broadcast_buffer, sizeof(broadcast_buffer), "[Server for P%d] Grabbed right fork %s.", philosopher_id, sem_name_log);
            broadcast_to_loggers(broadcast_buffer);

            // Отправка GRANTED клиенту
            char granted_msg[BUFFER_SIZE];
            snprintf(granted_msg, sizeof(granted_msg), "%s\n", RSP_GRANTED);
            if(send(client_sock, granted_msg, strlen(granted_msg), MSG_NOSIGNAL) < 0) {
                snprintf(log_buffer, sizeof(log_buffer), "P%d: send GRANTED failed: %s", philosopher_id, strerror(errno));
                print_log("Server", -1, log_buffer);
                snprintf(broadcast_buffer, sizeof(broadcast_buffer), "[Server -> P%d] Send GRANTED failed: %s", philosopher_id, strerror(errno));
                broadcast_to_loggers(broadcast_buffer);

                
                if(p_state->has_right_fork) { sem_post(forks_sem_ptr[p_state->right_fork_idx]); p_state->has_right_fork=0; }
                if(p_state->has_left_fork) { sem_post(forks_sem_ptr[p_state->left_fork_idx]); p_state->has_left_fork=0; }
                if(p_state->has_waiter_permission) { sem_post(waiter_semaphore_ptr); p_state->has_waiter_permission=0; }
                break;
            }
            snprintf(log_buffer, sizeof(log_buffer), "Sent GRANTED to P%d.", philosopher_id);
            print_log("Server", -1, log_buffer);
            snprintf(broadcast_buffer, sizeof(broadcast_buffer), "[Server -> P%d] GRANTED", philosopher_id);
            broadcast_to_loggers(broadcast_buffer);

        } else if (strncmp(buffer, MSG_RELEASE, strlen(MSG_RELEASE)) == 0) {
            if (p_state->has_left_fork) {
                sem_post(forks_sem_ptr[p_state->left_fork_idx]); p_state->has_left_fork = 0;
                snprintf(log_buffer, sizeof(log_buffer), "P%d Released left fork.", philosopher_id);
                print_log("Server",-1,log_buffer);
                snprintf(broadcast_buffer, sizeof(broadcast_buffer), "[Server for P%d] Released left fork %d.", philosopher_id, p_state->left_fork_idx);
                broadcast_to_loggers(broadcast_buffer);
            }
            if (p_state->has_right_fork) {
                sem_post(forks_sem_ptr[p_state->right_fork_idx]); p_state->has_right_fork = 0;
                snprintf(log_buffer, sizeof(log_buffer), "P%d Released right fork.", philosopher_id);
                print_log("Server",-1,log_buffer);
                snprintf(broadcast_buffer, sizeof(broadcast_buffer), "[Server for P%d] Released right fork %d.", philosopher_id, p_state->right_fork_idx);
                broadcast_to_loggers(broadcast_buffer);
            }
            if (p_state->has_waiter_permission) {
                sem_post(waiter_semaphore_ptr); p_state->has_waiter_permission = 0;
                snprintf(log_buffer, sizeof(log_buffer), "P%d Released waiter permission.", philosopher_id);
                print_log("Server",-1,log_buffer);
                snprintf(broadcast_buffer, sizeof(broadcast_buffer), "[Server for P%d] Released waiter permission.", philosopher_id);
                broadcast_to_loggers(broadcast_buffer);
            }

            char ok_msg[BUFFER_SIZE];
            snprintf(ok_msg, sizeof(ok_msg), "%s\n", RSP_OK);
            if(send(client_sock, ok_msg, strlen(ok_msg), MSG_NOSIGNAL) < 0) {
                snprintf(log_buffer, sizeof(log_buffer), "P%d: send OK failed: %s", philosopher_id, strerror(errno));
                print_log("Server", -1, log_buffer);
                snprintf(broadcast_buffer, sizeof(broadcast_buffer), "[Server -> P%d] Send OK failed: %s", philosopher_id, strerror(errno));
                broadcast_to_loggers(broadcast_buffer);
                break;
            }
            snprintf(log_buffer, sizeof(log_buffer), "Sent OK to P%d.", philosopher_id);
            print_log("Server", -1, log_buffer);
            snprintf(broadcast_buffer, sizeof(broadcast_buffer), "[Server -> P%d] OK", philosopher_id);
            broadcast_to_loggers(broadcast_buffer);

        } else if (strncmp(buffer, MSG_LEAVE, strlen(MSG_LEAVE)) == 0) {
            char bye_msg[BUFFER_SIZE];
            snprintf(bye_msg, sizeof(bye_msg), "%s\n", RSP_BYE);
            send(client_sock, bye_msg, strlen(bye_msg), MSG_NOSIGNAL); 
            snprintf(log_buffer, sizeof(log_buffer), "P%d sent LEAVE. Sent BYE. Client handler loop finishing.", philosopher_id);
            print_log("Server", -1, log_buffer);
            snprintf(broadcast_buffer, sizeof(broadcast_buffer), "[P%d -> Server] LEAVE, [Server -> P%d] BYE. Client handler loop finishing.", philosopher_id, philosopher_id);
            broadcast_to_loggers(broadcast_buffer);
            break;
        } else {
            snprintf(log_buffer, sizeof(log_buffer), "P%d sent unknown command: %s", philosopher_id, buffer);
            print_log("Server", -1, log_buffer);
            snprintf(broadcast_buffer, sizeof(broadcast_buffer), "[P%d -> Server] Unknown command: %s", philosopher_id, buffer);
            broadcast_to_loggers(broadcast_buffer);
            
            char error_msg_unknown[BUFFER_SIZE];
            snprintf(error_msg_unknown, sizeof(error_msg_unknown), "%s Unknown command\n", RSP_ERROR);
            send(client_sock, error_msg_unknown, strlen(error_msg_unknown), MSG_NOSIGNAL);
        }
    }

    snprintf(log_buffer, sizeof(log_buffer), "P%d thread entering cleanup...", p_state->id);
    print_log("Server", -1, log_buffer);
    snprintf(broadcast_buffer, sizeof(broadcast_buffer), "[Server for P%d] Thread entering cleanup...", p_state->id);
    broadcast_to_loggers(broadcast_buffer);

    pthread_mutex_lock(&server_mutex);
    cleanup_philosopher_resources_locked(p_state);

    if (p_state->active) {
        p_state->active = 0;
        active_philosophers_count--;
        snprintf(log_buffer, sizeof(log_buffer), "P%d deactivated. Active philosophers now: %d", p_state->id, active_philosophers_count);
        print_log("Server", -1, log_buffer);
        snprintf(broadcast_buffer, sizeof(broadcast_buffer), "[Server] P%d deactivated. Active philosophers now: %d", p_state->id, active_philosophers_count);
        broadcast_to_loggers(broadcast_buffer);
    }
    if (p_state->client_socket_fd != -1) {
        close(p_state->client_socket_fd);
        p_state->client_socket_fd = -1;
    }
    pthread_mutex_unlock(&server_mutex);

    snprintf(log_buffer, sizeof(log_buffer), "P%d thread finished.", p_state->id);
    print_log("Server", -1, log_buffer);
    snprintf(broadcast_buffer, sizeof(broadcast_buffer), "[Server for P%d] Thread finished.", p_state->id);
    broadcast_to_loggers(broadcast_buffer);
    return NULL;
}