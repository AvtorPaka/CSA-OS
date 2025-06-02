#include "common/common_sockets.h"
#include "server_global.h"
#include <fcntl.h>

philosopher_state_t* find_philosopher_by_id_locked(int id) {
    for (int i = 0; i < NUM_PHILOSOPHERS; ++i) {
        if (philosophers[i].active && philosophers[i].id == id) {
            return &philosophers[i];
        }
    }
    return nullptr;
}

int find_logger_idx_by_addr_locked(const struct sockaddr_in* addr) {
    for (int i = 0; i < MAX_LOGGERS; i++) {
        if (loggers[i].active &&
            loggers[i].client_addr.sin_addr.s_addr == addr->sin_addr.s_addr &&
            loggers[i].client_addr.sin_port == addr->sin_port) {
            return i;
        }
    }
    return -1;
}


int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <bind_ip_address> <port>\n", argv[0]);
        return 1;
    }
    const char *bind_ip_address = argv[1];
    int port = atoi(argv[2]);

    struct sigaction sa_server_shutdown{};
    memset(&sa_server_shutdown, 0, sizeof(sa_server_shutdown));
    sa_server_shutdown.sa_handler = server_signal_handler;
    sigaction(SIGINT, &sa_server_shutdown, NULL);
    sigaction(SIGTERM, &sa_server_shutdown, NULL);

    init_server_state();
    print_log("Server", -1, "Server initialized with named semaphores.");

    struct sockaddr_in server_addr{}, client_addr_from_recv{};
    socklen_t client_addr_len = sizeof(client_addr_from_recv);
    char recv_buffer[BUFFER_SIZE];
    char log_buf[BUFFER_SIZE];
    char broadcast_buf[BUFFER_SIZE + 64];

    server_udp_fd_global = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (server_udp_fd_global == -1) {
        perror("socket(UDP) failed");
        cleanup_all_named_semaphores();
        return 1;
    }
    print_log("Server", -1, "UDP Socket created.");

    int opt = 1;
    if (setsockopt(server_udp_fd_global, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt SO_REUSEADDR failed");
        close(server_udp_fd_global);
        server_udp_fd_global = -1;
        cleanup_all_named_semaphores();
        return 1;
    }

    server_addr.sin_family = AF_INET;
    if (inet_pton(AF_INET, bind_ip_address, &server_addr.sin_addr) <= 0) {
        snprintf(log_buf, sizeof(log_buf), "Invalid bind IP address: %s", bind_ip_address);
        print_log("Server",-1, log_buf);
        perror("inet_pton for bind IP failed");
        close(server_udp_fd_global);
        server_udp_fd_global = -1;
        cleanup_all_named_semaphores();
        return 1;
    }
    server_addr.sin_port = htons(port);

    if (bind(server_udp_fd_global, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        snprintf(log_buf, sizeof(log_buf), "bind failed for UDP %s:%d", bind_ip_address, port);
        print_log("Server",-1, log_buf);
        perror("bind failed");
        close(server_udp_fd_global);
        server_udp_fd_global = -1;
        cleanup_all_named_semaphores();
        return 1;
    }
    snprintf(log_buf, sizeof(log_buf), "Bind successful to UDP %s:%d.", bind_ip_address, port);
    print_log("Server", -1, log_buf);


    snprintf(log_buf, sizeof(log_buf), "Server listening for UDP packets on port %d", port);
    print_log("Server", -1, log_buf);

    while (!server_shutdown_flag) {
        client_addr_len = sizeof(client_addr_from_recv);
        ssize_t n_read = recvfrom(server_udp_fd_global, recv_buffer, BUFFER_SIZE - 1, 0,
                                  (struct sockaddr *)&client_addr_from_recv, &client_addr_len);

        if (server_shutdown_flag) {
            print_log("Server", -1, "Shutdown flag detected in main loop after recvfrom, exiting loop.");
            break;
        }

        if (n_read < 0) {
            if (errno == EINTR && !server_shutdown_flag) {
                print_log("Server", -1, "recvfrom interrupted by signal, continuing...");
                continue;
            }
            if (errno == EBADF) {
                 print_log("Server", -1, "recvfrom failed: Bad file descriptor (socket likely closed). Shutting down...");
            } else {
                snprintf(log_buf, sizeof(log_buf), "recvfrom failed: %s. Shutting down...", strerror(errno));
                print_log("Server", -1, log_buf);
                broadcast_to_loggers(log_buf);
            }
            server_shutdown_flag = 1; 
            break;
        }
        recv_buffer[n_read] = '\0';

        char client_ip_str[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr_from_recv.sin_addr, client_ip_str, INET_ADDRSTRLEN);


        char original_recv_for_log[BUFFER_SIZE];
        strncpy(original_recv_for_log, recv_buffer, BUFFER_SIZE -1);
        original_recv_for_log[BUFFER_SIZE-1] = '\0';

        snprintf(broadcast_buf, sizeof(broadcast_buf), "[%s:%d -> Srv] %s", client_ip_str, ntohs(client_addr_from_recv.sin_port), original_recv_for_log);
        broadcast_to_loggers(broadcast_buf);
        print_log("Server", -1, broadcast_buf); 

        char command_str[BUFFER_SIZE];
        int msg_philosopher_id = -1;
        strncpy(command_str, recv_buffer, BUFFER_SIZE-1);
        command_str[BUFFER_SIZE-1] = '\0';

        char* first_space = strchr(command_str, ' ');
        if (first_space) {
            *first_space = '\0'; 
            char* id_part = first_space + 1;
            char* endptr;
            long val = strtol(id_part, &endptr, 10);
            if (id_part != endptr && (*endptr == '\0' || isspace((unsigned char)*endptr))) { 
                 if (val >= 1 && val <= NUM_PHILOSOPHERS) { 
                    msg_philosopher_id = (int)val;
                 } else {
                 }
            } else {
            }
        }


        if (strcmp(command_str, MSG_JOIN_LOGGER) == 0) {
            pthread_mutex_lock(&loggers_mutex);
            if (active_loggers_count >= MAX_LOGGERS) {
                pthread_mutex_unlock(&loggers_mutex);
                print_log("Server", -1, "Max loggers reached. Rejecting new logger.");
                broadcast_to_loggers("[Server] Max loggers reached. Rejecting logger.");
                sendto(server_udp_fd_global, RSP_LOGGER_REJECTED, strlen(RSP_LOGGER_REJECTED), 0, (struct sockaddr *)&client_addr_from_recv, client_addr_len);
                continue;
            }

            int l_idx = -1;
            for (int i = 0; i < MAX_LOGGERS; ++i) {
                if (!loggers[i].active) {
                    l_idx = i;
                    break;
                }
            }

            if (l_idx == -1) { 
                pthread_mutex_unlock(&loggers_mutex);
                print_log("Server", -1, "ERROR: Inconsistent state: active_loggers_count < MAX but no free slot for logger. Rejecting.");
                broadcast_to_loggers("[Server ERROR] Inconsistent logger count. Rejecting logger.");
                sendto(server_udp_fd_global, RSP_LOGGER_REJECTED, strlen(RSP_LOGGER_REJECTED), 0, (struct sockaddr *)&client_addr_from_recv, client_addr_len);
                continue;
            }

            loggers[l_idx].client_addr = client_addr_from_recv;
            loggers[l_idx].client_addr_len = client_addr_len;
            loggers[l_idx].active = 1;
            active_loggers_count = active_loggers_count + 1;

            char local_log_msg[256];
            snprintf(local_log_msg, sizeof(local_log_msg), "Logger from %s:%d accepted (slot %d). Active loggers: %d. Sending RSP_LOGGER_ACCEPTED.",
                     client_ip_str, ntohs(client_addr_from_recv.sin_port), l_idx, active_loggers_count);
            print_log("Server", -1, local_log_msg);

            bool accepted_sent_ok = false;
            if (sendto(server_udp_fd_global, RSP_LOGGER_ACCEPTED, strlen(RSP_LOGGER_ACCEPTED), 0,
                       (struct sockaddr *)&client_addr_from_recv, client_addr_len) >= 0) {
                accepted_sent_ok = true;
            } else {
                char err_send_log[256];
                snprintf(err_send_log, sizeof(err_send_log), "sendto RSP_LOGGER_ACCEPTED to %s:%d failed: %s. Rolling back logger.",
                         client_ip_str, ntohs(client_addr_from_recv.sin_port), strerror(errno));
                print_log("Server", -1, err_send_log);

                loggers[l_idx].active = 0;
                active_loggers_count = active_loggers_count - 1;
            }

            char broadcast_new_logger_msg[256] = {0};
            if(accepted_sent_ok){
                 snprintf(broadcast_new_logger_msg, sizeof(broadcast_new_logger_msg),
                         "[Server] Logger from %s:%d accepted (slot %d). Active loggers: %d",
                         client_ip_str, ntohs(client_addr_from_recv.sin_port), l_idx, active_loggers_count);
            }
            pthread_mutex_unlock(&loggers_mutex);

            if (accepted_sent_ok) { 
                broadcast_to_loggers(broadcast_new_logger_msg);
            }
            continue;
        } else if (strcmp(command_str, MSG_LEAVE_LOGGER) == 0) {
            pthread_mutex_lock(&loggers_mutex);
            int logger_idx_to_remove = find_logger_idx_by_addr_locked(&client_addr_from_recv);
            char broadcast_leave_logger_msg[256] = {0}; 

            if (logger_idx_to_remove != -1) {
                loggers[logger_idx_to_remove].active = 0;
                active_loggers_count = active_loggers_count - 1;
                if(active_loggers_count < 0) active_loggers_count = 0; 

                snprintf(log_buf, sizeof(log_buf), "Logger from %s:%d (slot %d) sent LEAVE. Active loggers: %d", client_ip_str, ntohs(client_addr_from_recv.sin_port), logger_idx_to_remove, active_loggers_count);
                print_log("Server", -1, log_buf);
                snprintf(broadcast_leave_logger_msg, sizeof(broadcast_leave_logger_msg),"[Server] Logger from %s:%d (slot %d) left. Active: %d", client_ip_str, ntohs(client_addr_from_recv.sin_port), logger_idx_to_remove, active_loggers_count);
            } else {
                snprintf(log_buf, sizeof(log_buf), "Received LEAVE_LOGGER from unknown/inactive logger %s:%d. Ignored.", client_ip_str, ntohs(client_addr_from_recv.sin_port));
                print_log("Server", -1, log_buf);
                snprintf(broadcast_leave_logger_msg, sizeof(broadcast_leave_logger_msg), "[Server] Received LEAVE_LOGGER from unknown logger %s:%d. Ignored.", client_ip_str, ntohs(client_addr_from_recv.sin_port));
            }
            pthread_mutex_unlock(&loggers_mutex);

            if (strlen(broadcast_leave_logger_msg) > 0) {
                 broadcast_to_loggers(broadcast_leave_logger_msg);
            }
            continue; 

        } else if (strcmp(command_str, MSG_JOIN) == 0) {
            pthread_mutex_lock(&server_mutex);
            if (active_philosophers_count >= NUM_PHILOSOPHERS) {
                pthread_mutex_unlock(&server_mutex);
                print_log("Server", -1, "Table is full. Rejecting philosopher JOIN.");
                broadcast_to_loggers("[Server] Table full. Rejecting philosopher JOIN.");
                sendto(server_udp_fd_global, RSP_FULL, strlen(RSP_FULL), 0, (struct sockaddr *)&client_addr_from_recv, client_addr_len);
                continue;
            }

            int p_idx = -1; 
            for (int i = 0; i < NUM_PHILOSOPHERS; ++i) {
                if (!philosophers[i].active) { 
                    p_idx = i;
                    break;
                }
            }

            if (p_idx == -1) {
                pthread_mutex_unlock(&server_mutex);
                print_log("Server", -1, "ERROR: Inconsistent state: active_p_count < MAX but no free P slot. Rejecting.");
                broadcast_to_loggers("[Server ERROR] Inconsistent P count. Rejecting P JOIN.");
                sendto(server_udp_fd_global, RSP_FULL, strlen(RSP_FULL), 0, (struct sockaddr *)&client_addr_from_recv, client_addr_len);
                continue;
            }

            philosophers[p_idx].client_addr = client_addr_from_recv;
            philosophers[p_idx].client_addr_len = client_addr_len;
            philosophers[p_idx].active = 1;
            philosophers[p_idx].has_waiter_permission = 0;
            philosophers[p_idx].has_left_fork = 0;
            philosophers[p_idx].has_right_fork = 0;


            if (pipe(philosophers[p_idx].comm_pipe_fd) == -1) {
                perror("pipe failed for philosopher thread comm");
                broadcast_to_loggers("[Server ERROR] Pipe creation failed for P. Rejecting.");
                philosophers[p_idx].active = 0; 
                pthread_mutex_unlock(&server_mutex);
                sendto(server_udp_fd_global, RSP_ERROR " Server internal error (pipe)", strlen(RSP_ERROR " Server internal error (pipe)"), 0, (struct sockaddr *)&client_addr_from_recv, client_addr_len);
                continue;
            }

            active_philosophers_count++;
            snprintf(log_buf, sizeof(log_buf), "Assigning P%d (slot %d) to %s:%d. Pipe R=%d,W=%d. Active Ps: %d",
                     philosophers[p_idx].id, p_idx, client_ip_str, ntohs(client_addr_from_recv.sin_port),
                     philosophers[p_idx].comm_pipe_fd[0], philosophers[p_idx].comm_pipe_fd[1],
                     active_philosophers_count);
            print_log("Server", -1, log_buf);

            char temp_broadcast_buf[256]; 
            snprintf(temp_broadcast_buf, sizeof(temp_broadcast_buf),"[Server] Assigned P%d to %s:%d. Active Ps: %d", philosophers[p_idx].id, client_ip_str, ntohs(client_addr_from_recv.sin_port), active_philosophers_count);

            if (pthread_create(&philosophers[p_idx].thread_id, nullptr, handle_philosopher_requests, &philosophers[p_idx]) != 0) {
                perror("pthread_create for philosopher failed");
                snprintf(temp_broadcast_buf, sizeof(temp_broadcast_buf),"[Server ERROR] pthread_create for P%d failed. Rolling back.", philosophers[p_idx].id);
                philosophers[p_idx].active = 0;
                active_philosophers_count--;
                if(active_philosophers_count < 0)  {
                    active_philosophers_count = 0;
                }
                close(philosophers[p_idx].comm_pipe_fd[0]); philosophers[p_idx].comm_pipe_fd[0] = -1;
                close(philosophers[p_idx].comm_pipe_fd[1]); philosophers[p_idx].comm_pipe_fd[1] = -1;
                sendto(server_udp_fd_global, RSP_ERROR " Server thread creation failed", strlen(RSP_ERROR " Server thread creation failed"), 0, (struct sockaddr *)&client_addr_from_recv, client_addr_len);

            }
            pthread_mutex_unlock(&server_mutex);
            broadcast_to_loggers(temp_broadcast_buf); 

        } else { 
            if (msg_philosopher_id == -1) { 
                snprintf(log_buf, sizeof(log_buf), "Command '%s' from %s:%d requires a valid Philosopher ID. Message: '%s'. Ignoring.", command_str, client_ip_str, ntohs(client_addr_from_recv.sin_port), original_recv_for_log);
                print_log("Server",-1,log_buf);
                broadcast_to_loggers(log_buf);
                sendto(server_udp_fd_global, RSP_ERROR " Missing or invalid ID for command", strlen(RSP_ERROR " Missing or invalid ID for command"), 0, (struct sockaddr *)&client_addr_from_recv, client_addr_len);
                continue;
            }

            pthread_mutex_lock(&server_mutex);
            philosopher_state_t* p_state = find_philosopher_by_id_locked(msg_philosopher_id);

            if (!p_state) {
                pthread_mutex_unlock(&server_mutex);
                snprintf(log_buf, sizeof(log_buf), "Msg for inactive/unknown P%d from %s:%d ('%s'). Ignoring.", msg_philosopher_id, client_ip_str, ntohs(client_addr_from_recv.sin_port), original_recv_for_log);
                print_log("Server", -1, log_buf);
                broadcast_to_loggers(log_buf);
                continue;
            }

            if (p_state->client_addr.sin_addr.s_addr != client_addr_from_recv.sin_addr.s_addr ||
                p_state->client_addr.sin_port != client_addr_from_recv.sin_port) {
                pthread_mutex_unlock(&server_mutex);
                char stored_ip[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &p_state->client_addr.sin_addr, stored_ip, INET_ADDRSTRLEN);
                snprintf(log_buf, sizeof(log_buf),
                         "P%d: Address mismatch for message '%s'. Expected %s:%d, got %s:%d. Ignoring.",
                         msg_philosopher_id, original_recv_for_log, stored_ip, ntohs(p_state->client_addr.sin_port),
                         client_ip_str, ntohs(client_addr_from_recv.sin_port));
                print_log("Server", -1, log_buf);
                broadcast_to_loggers(log_buf);
                continue;
            }

            ssize_t written_to_pipe = write(p_state->comm_pipe_fd[1], command_str, strlen(command_str));
            if (written_to_pipe < 0) {
                snprintf(log_buf, sizeof(log_buf), "P%d: Error writing '%s' to pipe fd %d: %s. Client P%d may hang or misbehave.",
                         p_state->id, command_str, p_state->comm_pipe_fd[1], strerror(errno), p_state->id);
                print_log("Server", -1, log_buf);
                broadcast_to_loggers(log_buf);
            } else {
                 snprintf(log_buf, sizeof(log_buf), "P%d: Relayed '%s' to its thread via pipe fd %d.",
                         p_state->id, command_str, p_state->comm_pipe_fd[1]);
                print_log("Server", -1, log_buf); 
            }
            pthread_mutex_unlock(&server_mutex);
        }
    } 

    print_log("Server", -1, "Server main loop ended. Initiating shutdown sequence...");
    broadcast_to_loggers("[Server internal] Server main loop ended. Initiating shutdown sequence.");

    pthread_mutex_lock(&server_mutex);
    for (int i = 0; i < NUM_PHILOSOPHERS; ++i) {
        if (philosophers[i].active && philosophers[i].thread_id != 0) {
            snprintf(log_buf, sizeof(log_buf), "Notifying P%d about server shutdown (client UDP and pipe %d).", philosophers[i].id, philosophers[i].comm_pipe_fd[1]);
            print_log("Server", -1, log_buf);

            if (server_udp_fd_global != -1) {
                sendto(server_udp_fd_global, MSG_SERVER_DOWN, strlen(MSG_SERVER_DOWN), 0,
                       (struct sockaddr *)&philosophers[i].client_addr, philosophers[i].client_addr_len);
            }

            if (philosophers[i].comm_pipe_fd[1] != -1) {
                close(philosophers[i].comm_pipe_fd[1]);
                philosophers[i].comm_pipe_fd[1] = -1; 
            }
        }
    }
    pthread_mutex_unlock(&server_mutex);

    if (server_udp_fd_global != -1) {
        broadcast_to_loggers(MSG_SERVER_DOWN); 
    }


    if (server_udp_fd_global != -1) {
        print_log("Server", -1, "Closing main UDP listening socket in shutdown sequence.");
        close(server_udp_fd_global);
        server_udp_fd_global = -1;
    }


    print_log("Server", -1, "Joining philosopher client threads...");
    broadcast_to_loggers("[Server internal] Joining philosopher client threads...");
    for (int i = 0; i < NUM_PHILOSOPHERS; ++i) {
        pthread_t tid_to_join = 0; 
        int philo_id_to_join = -1; 

        pthread_mutex_lock(&server_mutex);
        if (philosophers[i].thread_id != 0) {
            tid_to_join = philosophers[i].thread_id;
            philo_id_to_join = philosophers[i].id;
        }
        pthread_mutex_unlock(&server_mutex);


        if (tid_to_join != 0) {
            snprintf(log_buf, sizeof(log_buf), "Attempting to join P%d thread (TID %lu)...", philo_id_to_join, (unsigned long)tid_to_join);
            print_log("Server", -1, log_buf);

            if (pthread_join(tid_to_join, nullptr) != 0) {
                snprintf(log_buf, sizeof(log_buf), "pthread_join for P%d (TID %lu) failed: %s. Resources might not be fully clean.", philo_id_to_join, (unsigned long)tid_to_join, strerror(errno));
                print_log("Server",-1, log_buf);
                broadcast_to_loggers(log_buf);
            } else {
                snprintf(log_buf, sizeof(log_buf), "P%d thread (TID %lu) joined successfully.", philo_id_to_join, (unsigned long)tid_to_join);
                print_log("Server", -1, log_buf);
            }

            pthread_mutex_lock(&server_mutex);
            philosophers[i].thread_id = 0; 
            if (philosophers[i].comm_pipe_fd[0] != -1) { 
                close(philosophers[i].comm_pipe_fd[0]); philosophers[i].comm_pipe_fd[0] = -1;
            }
            if (philosophers[i].active) { 
                print_log("Server", philosophers[i].id, "Warning: P thread joined but philosopher still marked active in main. Deactivating.");
                philosophers[i].active = 0;
                active_philosophers_count = (active_philosophers_count > 0) ? active_philosophers_count -1 : 0;
            }
            pthread_mutex_unlock(&server_mutex);
        }
    }

    pthread_mutex_lock(&server_mutex);
    int final_p_count_check = 0;
    for(int k=0; k<NUM_PHILOSOPHERS; ++k) if(philosophers[k].active) final_p_count_check++;
    if (active_philosophers_count != final_p_count_check) {
         snprintf(log_buf, sizeof(log_buf), "Warning: Mismatch in final active P count. Stored: %d, Checked: %d. Correcting.", active_philosophers_count, final_p_count_check);
         print_log("Server", -1, log_buf);
         active_philosophers_count = final_p_count_check;
    }
    snprintf(log_buf, sizeof(log_buf), "All P threads processed. Final active P count: %d", active_philosophers_count);
    print_log("Server",-1,log_buf);
    broadcast_to_loggers(log_buf);
    pthread_mutex_unlock(&server_mutex);

    pthread_mutex_lock(&loggers_mutex);
    int final_l_count_check = 0;
    for(int k=0; k<MAX_LOGGERS; ++k) if(loggers[k].active) final_l_count_check++;
     if (active_loggers_count != final_l_count_check) {
         snprintf(log_buf, sizeof(log_buf), "Warning: Mismatch in final active L count. Stored: %d, Checked: %d. Correcting.", active_loggers_count, final_l_count_check);
         print_log("Server", -1, log_buf);
         active_loggers_count = final_l_count_check;
    }
    snprintf(log_buf, sizeof(log_buf), "Final active logger count: %d", active_loggers_count);
    print_log("Server", -1, log_buf);
    pthread_mutex_unlock(&loggers_mutex);

    print_log("Server", -1, "Proceeding with final resource cleanup (semaphores, mutexes)...");
    cleanup_all_named_semaphores();
    pthread_mutex_destroy(&server_mutex);
    pthread_mutex_destroy(&loggers_mutex);
    print_log("Server", -1, "Server shut down completely.");
    return 0;
}