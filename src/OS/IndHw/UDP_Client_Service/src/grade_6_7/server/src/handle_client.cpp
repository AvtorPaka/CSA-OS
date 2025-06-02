#include "server_global.h"
#include <fcntl.h>

void *handle_philosopher_requests(void *arg) {
    philosopher_state_t *p_state = (philosopher_state_t *)arg;

    char response[BUFFER_SIZE];
    int philosopher_id = p_state->id;
    char log_buffer[BUFFER_SIZE];
    char broadcast_buffer[BUFFER_SIZE + 64];
    ssize_t n_read_pipe;
    bool continue_processing_client = true;

    snprintf(response, BUFFER_SIZE, "%s %d", RSP_ID, philosopher_id);
    if (sendto(server_udp_fd_global, response, strlen(response), 0,
               (struct sockaddr *)&p_state->client_addr, p_state->client_addr_len) < 0) {
        snprintf(log_buffer, sizeof(log_buffer), "P%d: sendto RSP_ID failed: %s. Thread exiting.", philosopher_id, strerror(errno));
        print_log("Server", -1, log_buffer);
        snprintf(broadcast_buffer, sizeof(broadcast_buffer), "[Server -> P%d] Send RSP_ID FAILED: %s. Thread exiting.", philosopher_id, strerror(errno));
        broadcast_to_loggers(broadcast_buffer);
        goto cleanup_and_exit_philosopher_thread;
    }
    snprintf(log_buffer, sizeof(log_buffer), "P%d joined. Sent ID. Listening on pipe read fd %d", philosopher_id, p_state->comm_pipe_fd[0]);
    print_log("Server", -1, log_buffer);
    snprintf(broadcast_buffer, sizeof(broadcast_buffer), "[Server -> P%d] Sent ID %d. Pipe R=%d. Thread active.", philosopher_id, philosopher_id, p_state->comm_pipe_fd[0]);
    broadcast_to_loggers(broadcast_buffer);


    p_state->left_fork_idx = (philosopher_id - 1);
    p_state->right_fork_idx = (philosopher_id % NUM_PHILOSOPHERS);

    while (continue_processing_client && !server_shutdown_flag) {
        n_read_pipe = read(p_state->comm_pipe_fd[0], p_state->last_received_message, BUFFER_SIZE - 1);

        if (server_shutdown_flag) {
            print_log("Server", philosopher_id, "Shutdown flag detected in P thread after pipe read check.");
            continue_processing_client = false; 
            break; 
        }

        if (n_read_pipe < 0) {
            if (errno == EINTR && !server_shutdown_flag) {
                print_log("Server", philosopher_id, "P thread: pipe read interrupted, retrying.");
                continue; 
            }
            snprintf(log_buffer, sizeof(log_buffer), "P%d: read from pipe failed: %s. Thread exiting.", philosopher_id, strerror(errno));
            print_log("Server", -1, log_buffer);
            snprintf(broadcast_buffer, sizeof(broadcast_buffer), "[Server for P%d] Pipe read failed: %s. Thread exiting.", philosopher_id, strerror(errno));
            broadcast_to_loggers(broadcast_buffer);
            continue_processing_client = false; 
            break; 
        }
        if (n_read_pipe == 0) {
            snprintf(log_buffer, sizeof(log_buffer), "P%d: Pipe read 0 bytes (closed by writer - main thread). Thread exiting.", philosopher_id);
            print_log("Server", -1, log_buffer);
            snprintf(broadcast_buffer, sizeof(broadcast_buffer), "[Server for P%d] Pipe closed by main thread. Thread exiting.", philosopher_id);
            broadcast_to_loggers(broadcast_buffer); 
            continue_processing_client = false; 
            break; 
        }
        p_state->last_received_message[n_read_pipe] = '\0';
        
        char temp_raw_log[BUFFER_SIZE + 50];
        snprintf(temp_raw_log, sizeof(temp_raw_log), "P%d (thread) raw data from pipe: '%.*s'", philosopher_id, (int)n_read_pipe, p_state->last_received_message);
        print_log("Server", philosopher_id, temp_raw_log);

        char *current_cmd_ptr = p_state->last_received_message;
        ssize_t bytes_remaining_in_buffer = n_read_pipe;

        while (bytes_remaining_in_buffer > 0 && continue_processing_client && !server_shutdown_flag) {
            size_t cmd_len_processed = 0;
            bool known_command_found = false;

            char sem_name_log[64];

            if (bytes_remaining_in_buffer >= strlen(MSG_ACQUIRE) &&
                strncmp(current_cmd_ptr, MSG_ACQUIRE, strlen(MSG_ACQUIRE)) == 0) {
                known_command_found = true;
                cmd_len_processed = strlen(MSG_ACQUIRE);
                snprintf(log_buffer, sizeof(log_buffer), "P%d (thread) processing ACQUIRE from pipe buffer", philosopher_id);
                print_log("Server", philosopher_id, log_buffer);

                if (server_shutdown_flag) {
                    print_log("Server", philosopher_id, "ACQUIRE: shutdown detected before sem ops.");
                    continue_processing_client = false; break; 
                }

                snprintf(log_buffer, sizeof(log_buffer), "P%d Waiting for waiter: %s.", philosopher_id, WAITER_SEM_NAME);
                print_log("Server", philosopher_id, log_buffer);
                snprintf(broadcast_buffer, sizeof(broadcast_buffer), "[Server P%d] Waits waiter: %s", philosopher_id, WAITER_SEM_NAME);
                broadcast_to_loggers(broadcast_buffer);

                sem_wait(waiter_semaphore_ptr);

                if (server_shutdown_flag) {
                    sem_post(waiter_semaphore_ptr);
                    print_log("Server", philosopher_id, "ACQUIRE: shutdown after waiter sem_wait. Released waiter.");
                    continue_processing_client = false; break;
                }

                p_state->has_waiter_permission = 1;
                snprintf(log_buffer, sizeof(log_buffer), "P%d Got waiter.", philosopher_id);
                print_log("Server", philosopher_id, log_buffer);
                snprintf(broadcast_buffer, sizeof(broadcast_buffer), "[Server P%d] Got waiter", philosopher_id);
                broadcast_to_loggers(broadcast_buffer);

                if (server_shutdown_flag) {
                    sem_post(waiter_semaphore_ptr);
                    p_state->has_waiter_permission = 0;
                    print_log("Server", philosopher_id, "ACQUIRE: shutdown before fork acquisition. Released waiter.");
                    continue_processing_client = false; break;
                }

                snprintf(sem_name_log, sizeof(sem_name_log), "F%d", p_state->left_fork_idx);
                snprintf(log_buffer, sizeof(log_buffer), "P%d Trying left fork: %s.", philosopher_id, sem_name_log);
                print_log("Server", philosopher_id, log_buffer);
                snprintf(broadcast_buffer, sizeof(broadcast_buffer), "[Server P%d] Tries L-fork: %s", philosopher_id, sem_name_log);
                broadcast_to_loggers(broadcast_buffer);

                sem_wait(forks_sem_ptr[p_state->left_fork_idx]);
                if (server_shutdown_flag) {
                    sem_post(forks_sem_ptr[p_state->left_fork_idx]); 
                    if(p_state->has_waiter_permission) { sem_post(waiter_semaphore_ptr); p_state->has_waiter_permission=0; }
                    print_log("Server", philosopher_id, "ACQUIRE: shutdown after L-fork sem_wait. Released L-fork and waiter.");
                    continue_processing_client = false; break;
                }
                p_state->has_left_fork = 1;
                snprintf(log_buffer, sizeof(log_buffer), "P%d Grabbed left fork %s.", philosopher_id, sem_name_log);
                print_log("Server", philosopher_id, log_buffer);
                snprintf(broadcast_buffer, sizeof(broadcast_buffer), "[Server P%d] Grabbed L-fork %s", philosopher_id, sem_name_log);
                broadcast_to_loggers(broadcast_buffer);

                if (server_shutdown_flag) { 
                    if(p_state->has_left_fork) { sem_post(forks_sem_ptr[p_state->left_fork_idx]); p_state->has_left_fork=0; }
                    if(p_state->has_waiter_permission) { sem_post(waiter_semaphore_ptr); p_state->has_waiter_permission=0; }
                    print_log("Server", philosopher_id, "ACQUIRE: shutdown before R-fork. Released L-fork and waiter.");
                    continue_processing_client = false; break;
                }

                snprintf(sem_name_log, sizeof(sem_name_log), "F%d", p_state->right_fork_idx);
                snprintf(log_buffer, sizeof(log_buffer), "P%d Trying right fork: %s.", philosopher_id, sem_name_log);
                print_log("Server", philosopher_id, log_buffer);
                snprintf(broadcast_buffer, sizeof(broadcast_buffer), "[Server P%d] Tries R-fork: %s", philosopher_id, sem_name_log);
                broadcast_to_loggers(broadcast_buffer);

                sem_wait(forks_sem_ptr[p_state->right_fork_idx]);
                if (server_shutdown_flag) {
                    sem_post(forks_sem_ptr[p_state->right_fork_idx]);
                    if(p_state->has_left_fork)  { sem_post(forks_sem_ptr[p_state->left_fork_idx]); p_state->has_left_fork=0; }
                    if(p_state->has_waiter_permission) { sem_post(waiter_semaphore_ptr); p_state->has_waiter_permission=0; }
                    print_log("Server", philosopher_id, "ACQUIRE: shutdown after R-fork sem_wait. Released all.");
                    continue_processing_client = false; break;
                }
                p_state->has_right_fork = 1;
                snprintf(log_buffer, sizeof(log_buffer), "P%d Grabbed right fork %s.", philosopher_id, sem_name_log);
                print_log("Server", philosopher_id, log_buffer);
                snprintf(broadcast_buffer, sizeof(broadcast_buffer), "[Server P%d] Grabbed R-fork %s", philosopher_id, sem_name_log);
                broadcast_to_loggers(broadcast_buffer);

                if (server_shutdown_flag) {
                    if(p_state->has_right_fork) { sem_post(forks_sem_ptr[p_state->right_fork_idx]); p_state->has_right_fork=0; }
                    if(p_state->has_left_fork)  { sem_post(forks_sem_ptr[p_state->left_fork_idx]); p_state->has_left_fork=0; }
                    if(p_state->has_waiter_permission) { sem_post(waiter_semaphore_ptr); p_state->has_waiter_permission=0; }
                    print_log("Server", philosopher_id, "ACQUIRE: shutdown before sending GRANTED. Released all.");
                    continue_processing_client = false; break;
                }

                snprintf(response, BUFFER_SIZE, "%s", RSP_GRANTED);
                if(sendto(server_udp_fd_global, response, strlen(response), 0, (struct sockaddr *)&p_state->client_addr, p_state->client_addr_len) < 0) {
                    snprintf(log_buffer, sizeof(log_buffer), "P%d: sendto GRANTED failed: %s. Releasing semaphores and exiting thread.", philosopher_id, strerror(errno));
                    print_log("Server", -1, log_buffer);
                    snprintf(broadcast_buffer, sizeof(broadcast_buffer), "[Server -> P%d] Send GRANTED FAILED: %s. Releasing semaphores.", philosopher_id, strerror(errno));
                    broadcast_to_loggers(broadcast_buffer);
                    continue_processing_client = false; break;
                }
                snprintf(log_buffer, sizeof(log_buffer), "Sent GRANTED to P%d.", philosopher_id);
                print_log("Server", philosopher_id, log_buffer);
                snprintf(broadcast_buffer, sizeof(broadcast_buffer), "[Server -> P%d] GRANTED", philosopher_id);
                broadcast_to_loggers(broadcast_buffer);

            } else if (bytes_remaining_in_buffer >= strlen(MSG_RELEASE) &&
                       strncmp(current_cmd_ptr, MSG_RELEASE, strlen(MSG_RELEASE)) == 0) {
                known_command_found = true;
                cmd_len_processed = strlen(MSG_RELEASE);
                snprintf(log_buffer, sizeof(log_buffer), "P%d (thread) processing RELEASE from pipe buffer", philosopher_id);
                print_log("Server", philosopher_id, log_buffer);

                if (p_state->has_left_fork) {
                    sem_post(forks_sem_ptr[p_state->left_fork_idx]); p_state->has_left_fork = 0;
                    snprintf(log_buffer, sizeof(log_buffer), "P%d Released left fork F%d.", philosopher_id, p_state->left_fork_idx);
                    print_log("Server",philosopher_id,log_buffer);
                    snprintf(broadcast_buffer, sizeof(broadcast_buffer), "[Server P%d] Released L-fork F%d", philosopher_id, p_state->left_fork_idx);
                    broadcast_to_loggers(broadcast_buffer);
                } else {
                    snprintf(log_buffer, sizeof(log_buffer), "P%d (thread) received RELEASE but didn't hold L-fork.", philosopher_id);
                    print_log("Server",philosopher_id,log_buffer);
                    broadcast_to_loggers(log_buffer);
                }

                if (p_state->has_right_fork) {
                    sem_post(forks_sem_ptr[p_state->right_fork_idx]); p_state->has_right_fork = 0;
                    snprintf(log_buffer, sizeof(log_buffer), "P%d Released right fork F%d.", philosopher_id, p_state->right_fork_idx);
                    print_log("Server",philosopher_id,log_buffer);
                    snprintf(broadcast_buffer, sizeof(broadcast_buffer), "[Server P%d] Released R-fork F%d", philosopher_id, p_state->right_fork_idx);
                    broadcast_to_loggers(broadcast_buffer);
                } else {
                    snprintf(log_buffer, sizeof(log_buffer), "P%d (thread) received RELEASE but didn't hold R-fork.", philosopher_id);
                    print_log("Server",philosopher_id,log_buffer);
                    broadcast_to_loggers(log_buffer);
                }

                if (p_state->has_waiter_permission) {
                    sem_post(waiter_semaphore_ptr); p_state->has_waiter_permission = 0;
                    snprintf(log_buffer, sizeof(log_buffer), "P%d Released waiter permission.", philosopher_id);
                    print_log("Server",philosopher_id,log_buffer);
                    snprintf(broadcast_buffer, sizeof(broadcast_buffer), "[Server P%d] Released waiter", philosopher_id);
                    broadcast_to_loggers(broadcast_buffer);
                } else {
                    snprintf(log_buffer, sizeof(log_buffer), "P%d (thread) received RELEASE but didn't hold waiter permission.", philosopher_id);
                    print_log("Server",philosopher_id,log_buffer);
                    broadcast_to_loggers(log_buffer);
                }

                snprintf(response, BUFFER_SIZE, "%s", RSP_OK);
                if(sendto(server_udp_fd_global, response, strlen(response), 0, (struct sockaddr *)&p_state->client_addr, p_state->client_addr_len) < 0) {
                    snprintf(log_buffer, sizeof(log_buffer), "P%d: sendto OK failed: %s", philosopher_id, strerror(errno));
                    print_log("Server", -1, log_buffer);
                    snprintf(broadcast_buffer, sizeof(broadcast_buffer), "[Server -> P%d] Send OK FAILED: %s", philosopher_id, strerror(errno));
                    broadcast_to_loggers(broadcast_buffer);
                    continue_processing_client = false; break; 
                }
                snprintf(log_buffer, sizeof(log_buffer), "Sent OK to P%d.", philosopher_id);
                print_log("Server", philosopher_id, log_buffer);
                snprintf(broadcast_buffer, sizeof(broadcast_buffer), "[Server -> P%d] OK", philosopher_id);
                broadcast_to_loggers(broadcast_buffer);

            } else if (bytes_remaining_in_buffer >= strlen(MSG_LEAVE) &&
                       strncmp(current_cmd_ptr, MSG_LEAVE, strlen(MSG_LEAVE)) == 0) {
                known_command_found = true;
                cmd_len_processed = strlen(MSG_LEAVE);
                snprintf(log_buffer, sizeof(log_buffer), "P%d (thread) processing LEAVE from pipe buffer", philosopher_id);
                print_log("Server", philosopher_id, log_buffer);
                
                snprintf(response, BUFFER_SIZE, "%s", RSP_BYE);
                sendto(server_udp_fd_global, response, strlen(response), 0, (struct sockaddr *)&p_state->client_addr, p_state->client_addr_len);

                snprintf(log_buffer, sizeof(log_buffer), "P%d processed LEAVE from pipe. Sent BYE. Thread loop finishing.", philosopher_id);
                print_log("Server", philosopher_id, log_buffer);
                snprintf(broadcast_buffer, sizeof(broadcast_buffer), "[Server for P%d] Processed LEAVE. Sent BYE. Thread loop finishing.", philosopher_id);
                broadcast_to_loggers(broadcast_buffer);
                
                continue_processing_client = false; 
            } else {
                if (bytes_remaining_in_buffer > 0) { 
                    snprintf(log_buffer, sizeof(log_buffer), "P%d (thread) unknown/incomplete data in pipe buffer: '%.*s'", philosopher_id, (int)bytes_remaining_in_buffer, current_cmd_ptr);
                    print_log("Server", philosopher_id, log_buffer);
                    snprintf(broadcast_buffer, sizeof(broadcast_buffer), "[Server for P%d] Unknown command from pipe: '%.*s'", philosopher_id, (int)bytes_remaining_in_buffer, current_cmd_ptr);
                    broadcast_to_loggers(broadcast_buffer);
                    snprintf(response, BUFFER_SIZE, "%s Unknown command from pipe", RSP_ERROR);
                    sendto(server_udp_fd_global, response, strlen(response), 0, (struct sockaddr *)&p_state->client_addr, p_state->client_addr_len);
                }
                break; 
            }

            if (known_command_found) {
                current_cmd_ptr += cmd_len_processed;
                bytes_remaining_in_buffer -= cmd_len_processed;
            } else {
                if (bytes_remaining_in_buffer > 0) { 
                    snprintf(log_buffer, sizeof(log_buffer), "P%d (thread) failed to parse remaining data in pipe: '%.*s'. Breaking parse.", philosopher_id, (int)bytes_remaining_in_buffer, current_cmd_ptr);
                    print_log("Server", philosopher_id, log_buffer);
                }
                break; 
            }
        } 
    } 

    cleanup_and_exit_philosopher_thread:
    snprintf(log_buffer, sizeof(log_buffer), "P%d thread entering cleanup phase...", p_state->id);
    print_log("Server", p_state->id, log_buffer);
    snprintf(broadcast_buffer, sizeof(broadcast_buffer), "[Server for P%d] Thread entering cleanup phase...", p_state->id);
    broadcast_to_loggers(broadcast_buffer);

    cleanup_philosopher_semaphores_if_held(p_state);

    if (p_state->comm_pipe_fd[0] != -1) {
        close(p_state->comm_pipe_fd[0]);
        p_state->comm_pipe_fd[0] = -1;
    }

    pthread_mutex_lock(&server_mutex);
    if (p_state->active) {
        p_state->active = 0;
        active_philosophers_count = active_philosophers_count - 1;
        if (active_philosophers_count < 0) active_philosophers_count = 0;
        snprintf(log_buffer, sizeof(log_buffer), "P%d deactivated by its thread. Active philosophers now: %d", p_state->id, active_philosophers_count);
        print_log("Server", -1, log_buffer);
        snprintf(broadcast_buffer, sizeof(broadcast_buffer), "[Server] P%d deactivated by thread. Active philosophers: %d", p_state->id, active_philosophers_count);
        broadcast_to_loggers(broadcast_buffer);
    }
    pthread_mutex_unlock(&server_mutex);

    snprintf(log_buffer, sizeof(log_buffer), "P%d thread finished.", p_state->id);
    print_log("Server", p_state->id, log_buffer);
    snprintf(broadcast_buffer, sizeof(broadcast_buffer), "[Server for P%d] Thread finished.", p_state->id);
    broadcast_to_loggers(broadcast_buffer);
    return nullptr;
}