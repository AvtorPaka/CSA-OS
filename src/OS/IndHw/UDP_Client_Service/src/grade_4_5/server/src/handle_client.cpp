#include "server_global.h"
#include <fcntl.h> 

void *handle_philosopher_requests(void *arg) {
    auto *p_state = (philosopher_state_t *)arg;

    char response[BUFFER_SIZE];
    int philosopher_id = p_state->id;
    char log_buffer[256];

    snprintf(response, BUFFER_SIZE, "%s %d", RSP_ID, philosopher_id);
    if (sendto(server_udp_fd_global, response, strlen(response), 0,
               (struct sockaddr *)&p_state->client_addr, p_state->client_addr_len) < 0) {
        snprintf(log_buffer, sizeof(log_buffer), "P%d: sendto RSP_ID failed: %s. Thread exiting.", philosopher_id, strerror(errno));
        print_log("Server", -1, log_buffer);
        goto cleanup_and_exit_thread;
    }
    snprintf(log_buffer, sizeof(log_buffer), "P%d joined. Sent ID. Listening on pipe read fd %d", philosopher_id, p_state->comm_pipe_fd[0]);
    print_log("Server", -1, log_buffer);

    p_state->left_fork_idx = (philosopher_id - 1);
    p_state->right_fork_idx = (philosopher_id % NUM_PHILOSOPHERS);

    while (!server_shutdown_flag) {
        ssize_t n_read = read(p_state->comm_pipe_fd[0], p_state->last_received_message, BUFFER_SIZE - 1);

        if (server_shutdown_flag) {
            print_log("Server", philosopher_id, "Shutdown flag detected after read attempt.");
            break;
        }

        if (n_read < 0) {
            if (errno == EINTR && !server_shutdown_flag) {
                print_log("Server", philosopher_id, "read from pipe interrupted, retrying.");
                continue;
            }
            snprintf(log_buffer, sizeof(log_buffer), "P%d: read from pipe failed: %s. Thread exiting.", philosopher_id, strerror(errno));
            print_log("Server", -1, log_buffer);
            break; 
        }
        if (n_read == 0) {
            snprintf(log_buffer, sizeof(log_buffer), "P%d: Pipe read 0 bytes (closed by writer). Thread exiting.", philosopher_id);
            print_log("Server", -1, log_buffer);
            break;
        }
        p_state->last_received_message[n_read] = '\0';

        char* current_message = p_state->last_received_message;

        snprintf(log_buffer, sizeof(log_buffer), "P%d received from main via pipe: '%s'", philosopher_id, current_message);
        print_log("Server", -1, log_buffer);

        char sem_name_log[64];

        if (strncmp(current_message, MSG_ACQUIRE, strlen(MSG_ACQUIRE)) == 0) {
            if (server_shutdown_flag) { print_log("Server", -1, "ACQUIRE: shutdown detected."); break; }

            snprintf(log_buffer, sizeof(log_buffer), "P%d Waiting for waiter permission: %s.", philosopher_id, WAITER_SEM_NAME);
            print_log("Server",-1, log_buffer);
            sem_wait(waiter_semaphore_ptr);
            if (server_shutdown_flag) {
                sem_post(waiter_semaphore_ptr);
                print_log("Server", -1, "ACQUIRE: shutdown after waiter sem_wait.");
                break;
            }
            p_state->has_waiter_permission = 1;
            snprintf(log_buffer, sizeof(log_buffer), "P%d Got waiter permission.", philosopher_id);
            print_log("Server", -1, log_buffer);

            if (server_shutdown_flag) { sem_post(waiter_semaphore_ptr); p_state->has_waiter_permission=0; break; }

            snprintf(sem_name_log, sizeof(sem_name_log), "%s%d", FORK_SEM_BASE_NAME, p_state->left_fork_idx);
            snprintf(log_buffer, sizeof(log_buffer), "P%d Trying to grab left fork: %s.", philosopher_id, sem_name_log);
            print_log("Server", -1, log_buffer);
            sem_wait(forks_sem_ptr[p_state->left_fork_idx]);
            if (server_shutdown_flag) {
                sem_post(forks_sem_ptr[p_state->left_fork_idx]);
                if(p_state->has_waiter_permission) sem_post(waiter_semaphore_ptr); p_state->has_waiter_permission=0;
                print_log("Server", -1, "ACQUIRE: shutdown after left fork sem_wait.");
                break;
            }
            p_state->has_left_fork = 1;
            snprintf(log_buffer, sizeof(log_buffer), "P%d Grabbed left fork.", philosopher_id);
            print_log("Server", -1, log_buffer);

            if (server_shutdown_flag) {
                if(p_state->has_left_fork) sem_post(forks_sem_ptr[p_state->left_fork_idx]); p_state->has_left_fork=0;
                if(p_state->has_waiter_permission) sem_post(waiter_semaphore_ptr); p_state->has_waiter_permission=0;
                break;
            }

            snprintf(sem_name_log, sizeof(sem_name_log), "%s%d", FORK_SEM_BASE_NAME, p_state->right_fork_idx);
            snprintf(log_buffer, sizeof(log_buffer), "P%d Trying to grab right fork: %s.", philosopher_id, sem_name_log);
            print_log("Server", -1, log_buffer);
            sem_wait(forks_sem_ptr[p_state->right_fork_idx]);
            if (server_shutdown_flag) {
                sem_post(forks_sem_ptr[p_state->right_fork_idx]);
                if(p_state->has_left_fork) sem_post(forks_sem_ptr[p_state->left_fork_idx]); p_state->has_left_fork=0;
                if(p_state->has_waiter_permission) sem_post(waiter_semaphore_ptr); p_state->has_waiter_permission=0;
                print_log("Server", -1, "ACQUIRE: shutdown after right fork sem_wait.");
                break;
            }
            p_state->has_right_fork = 1;
            snprintf(log_buffer, sizeof(log_buffer), "P%d Grabbed right fork.", philosopher_id);
            print_log("Server", -1, log_buffer);

            
            if(sendto(server_udp_fd_global, RSP_GRANTED, strlen(RSP_GRANTED), 0, (struct sockaddr *)&p_state->client_addr, p_state->client_addr_len) < 0) {
                snprintf(log_buffer, sizeof(log_buffer), "P%d: sendto GRANTED failed: %s", philosopher_id, strerror(errno));
                print_log("Server", -1, log_buffer);
                if(p_state->has_right_fork) { sem_post(forks_sem_ptr[p_state->right_fork_idx]); p_state->has_right_fork=0; }
                if(p_state->has_left_fork) { sem_post(forks_sem_ptr[p_state->left_fork_idx]); p_state->has_left_fork=0; }
                if(p_state->has_waiter_permission) { sem_post(waiter_semaphore_ptr); p_state->has_waiter_permission=0; }
                break;
            }
            snprintf(log_buffer, sizeof(log_buffer), "Sent GRANTED to P%d.", philosopher_id);
            print_log("Server", -1, log_buffer);

        } else if (strncmp(current_message, MSG_RELEASE, strlen(MSG_RELEASE)) == 0) {
            if (p_state->has_left_fork) {
                sem_post(forks_sem_ptr[p_state->left_fork_idx]); p_state->has_left_fork = 0;
                snprintf(log_buffer, sizeof(log_buffer), "P%d Released left fork.", philosopher_id);
                print_log("Server", -1 ,log_buffer);
            }
            if (p_state->has_right_fork) {
                sem_post(forks_sem_ptr[p_state->right_fork_idx]); p_state->has_right_fork = 0;
                snprintf(log_buffer, sizeof(log_buffer), "P%d Released right fork.", philosopher_id);
                print_log("Server", -1 ,log_buffer);
            }
            if (p_state->has_waiter_permission) {
                sem_post(waiter_semaphore_ptr); p_state->has_waiter_permission = 0;
                snprintf(log_buffer, sizeof(log_buffer), "P%d Released waiter permission.", philosopher_id);
                print_log("Server", -1 ,log_buffer);
            }

            if(sendto(server_udp_fd_global, RSP_OK, strlen(RSP_OK), 0, (struct sockaddr *)&p_state->client_addr, p_state->client_addr_len) < 0) {
                snprintf(log_buffer, sizeof(log_buffer), "P%d: sendto OK failed: %s", philosopher_id, strerror(errno));
                print_log("Server", -1, log_buffer);
                break;
            }
            snprintf(log_buffer, sizeof(log_buffer), "Sent OK to P%d.", philosopher_id);
            print_log("Server", -1 , log_buffer);

        } else if (strncmp(current_message, MSG_LEAVE, strlen(MSG_LEAVE)) == 0) {
            snprintf(log_buffer, sizeof(log_buffer), "P%d sent LEAVE. Sending BYE.", philosopher_id);
            print_log("Server", -1 , log_buffer);
            sendto(server_udp_fd_global, RSP_BYE, strlen(RSP_BYE), 0, (struct sockaddr *)&p_state->client_addr, p_state->client_addr_len);
            break;
        } else {
            snprintf(log_buffer, sizeof(log_buffer), "P%d sent unknown command via pipe: '%s'", philosopher_id, current_message);
            print_log("Server", -1, log_buffer);
            char error_msg[BUFFER_SIZE];
            snprintf(error_msg, BUFFER_SIZE, "%s Unknown command", RSP_ERROR);
            sendto(server_udp_fd_global, error_msg, strlen(error_msg), 0, (struct sockaddr *)&p_state->client_addr, p_state->client_addr_len);
        }
    }

    cleanup_and_exit_thread:
    snprintf(log_buffer, sizeof(log_buffer), "P%d thread entering cleanup phase...", p_state->id);
    print_log("Server", p_state->id, log_buffer);

    cleanup_philosopher_semaphores_if_held(p_state);

    if (p_state->comm_pipe_fd[0] != -1) {
        close(p_state->comm_pipe_fd[0]);
        p_state->comm_pipe_fd[0] = -1;
    }

    pthread_mutex_lock(&server_mutex);
    if (p_state->active) {
        p_state->active = 0;
        active_philosophers_count--;
        snprintf(log_buffer, sizeof(log_buffer), "P%d deactivated by its thread. Active now: %d", p_state->id, active_philosophers_count);
        print_log("Server", -1, log_buffer);
    }
    pthread_mutex_unlock(&server_mutex);

    snprintf(log_buffer, sizeof(log_buffer), "P%d thread finished.", p_state->id);
    print_log("Server", p_state->id, log_buffer);
    return nullptr;
}