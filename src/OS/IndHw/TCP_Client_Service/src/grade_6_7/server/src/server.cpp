#include "common/common_sockets.h"
#include "server_global.h"
#include <fcntl.h> 

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <bind_ip_address> <port>\n", argv[0]);
        return 1;
    }
    const char *bind_ip_address = argv[1];
    int port = atoi(argv[2]);

    struct sigaction sa_server_shutdown;
    memset(&sa_server_shutdown, 0, sizeof(sa_server_shutdown));
    sa_server_shutdown.sa_handler = server_signal_handler;
    sigaction(SIGINT, &sa_server_shutdown, NULL);
    sigaction(SIGTERM, &sa_server_shutdown, NULL);

    init_server_state();
    print_log("Server", -1, "Server initialized with named semaphores.");
    broadcast_to_loggers("[Server internal] Server initialized.");


    int client_sock_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    server_fd_global = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server_fd_global == -1) {
        perror("socket failed");
        cleanup_all_named_semaphores();
        return 1;
    }
    print_log("Server", -1, "Socket created.");
    

    int opt = 1;
    if (setsockopt(server_fd_global, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt SO_REUSEADDR failed");
        close(server_fd_global);
        cleanup_all_named_semaphores();
        return 1;
    }

    server_addr.sin_family = AF_INET;
    if (inet_pton(AF_INET, bind_ip_address, &server_addr.sin_addr) <= 0) {
        char err_msg[100];
        snprintf(err_msg, sizeof(err_msg), "Invalid bind IP address: %s", bind_ip_address);
        print_log("Server",-1, err_msg); 
        perror("inet_pton for bind IP failed");
        close(server_fd_global);
        cleanup_all_named_semaphores();
        return 1;
    }
    server_addr.sin_port = htons(port);

    if (bind(server_fd_global, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        char err_msg[120];
        snprintf(err_msg, sizeof(err_msg), "bind failed for %s:%d", bind_ip_address, port);
        print_log("Server",-1, err_msg);
        perror("bind failed");
        close(server_fd_global);
        cleanup_all_named_semaphores();
        return 1;
    }
    char log_buf_bind[100];
    snprintf(log_buf_bind, sizeof(log_buf_bind), "Bind successful to %s:%d.", bind_ip_address, port);
    print_log("Server", -1, log_buf_bind);
    

    if (listen(server_fd_global, NUM_PHILOSOPHERS + MAX_LOGGERS + 2) < 0) {
        perror("listen failed");
        close(server_fd_global);
        cleanup_all_named_semaphores();
        return 1;
    }
    char log_buf_listen[50];
    snprintf(log_buf_listen, sizeof(log_buf_listen), "Server listening on port %d", port);
    print_log("Server", -1, log_buf_listen);
    broadcast_to_loggers("[Server internal] Server listening.");


    while (!server_shutdown_flag) {
        client_sock_fd = accept(server_fd_global, (struct sockaddr *) &client_addr, &client_addr_len);

        if (server_shutdown_flag) {
            if (client_sock_fd >= 0) {
                close(client_sock_fd);
            }
            break;
        }

        if (client_sock_fd < 0) {
            if (errno == EINTR && server_shutdown_flag) {
                print_log("Server", -1, "accept() interrupted by signal during shutdown. Exiting loop.");
                break;
            } else if (errno == EINTR) {
                print_log("Server", -1, "accept() interrupted by signal. Continuing...");
                continue;
            } else if (server_fd_global == -1 && server_shutdown_flag) {
                 print_log("Server", -1, "accept() failed as server_fd closed by signal handler. Shutting down...");
                 break;
            }
            perror("accept failed");
            continue;
        }
        

        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
        char conn_log[128];
        snprintf(conn_log, sizeof(conn_log), "Connection accepted from %s:%d (socket %d)", client_ip, ntohs(client_addr.sin_port), client_sock_fd);
        print_log("Server", -1, conn_log);
        char broadcast_conn_log[160];
        snprintf(broadcast_conn_log, sizeof(broadcast_conn_log), "[Server internal] Connection accepted from %s:%d (socket %d)", client_ip, ntohs(client_addr.sin_port), client_sock_fd);
        broadcast_to_loggers(broadcast_conn_log);

        // Определяем тип клиента по первому сообщению
        char initial_buffer[BUFFER_SIZE];
        ssize_t n_read_initial = recv(client_sock_fd, initial_buffer, BUFFER_SIZE - 1, 0);

        if (n_read_initial <= 0) {
            if (n_read_initial == 0) {
                print_log("Server", -1, "Client disconnected before sending initial command.");
                broadcast_to_loggers("[Server internal] Client disconnected before initial command.");
            } else {
                char err_recv_init[128];
                snprintf(err_recv_init, sizeof(err_recv_init), "Initial recv from socket %d failed: %s", client_sock_fd, strerror(errno));
                print_log("Server", -1, err_recv_init);
                broadcast_to_loggers(err_recv_init);
            }
            close(client_sock_fd);
            continue;
        }
        initial_buffer[n_read_initial] = '\0';
        char* newline = strchr(initial_buffer, '\n');
        if (newline) *newline = '\0';


        if (strncmp(initial_buffer, MSG_JOIN_LOGGER, strlen(MSG_JOIN_LOGGER)) == 0) {
            // Клиент-логгер
            print_log("Server", -1, "Logger client trying to connect.");
            
            pthread_mutex_lock(&loggers_mutex);
            if (active_loggers_count >= MAX_LOGGERS) {
                print_log("Server", -1, "Max loggers reached. Rejecting logger.");
                char reject_msg[BUFFER_SIZE];
                snprintf(reject_msg, sizeof(reject_msg), "%s\n", RSP_LOGGER_REJECTED);
                send(client_sock_fd, reject_msg, strlen(reject_msg), MSG_NOSIGNAL);
                close(client_sock_fd);
                pthread_mutex_unlock(&loggers_mutex);
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
                print_log("Server", -1, "ERROR: Inconsistent state: active_loggers_count < MAX but no free slot for logger.");
                 char reject_msg_inconsistent[BUFFER_SIZE];
                snprintf(reject_msg_inconsistent, sizeof(reject_msg_inconsistent), "%s\n", RSP_LOGGER_REJECTED);
                send(client_sock_fd, reject_msg_inconsistent, strlen(reject_msg_inconsistent), MSG_NOSIGNAL);
                close(client_sock_fd);
                pthread_mutex_unlock(&loggers_mutex);
                continue;
            }

            loggers[l_idx].client_socket_fd = client_sock_fd;
            loggers[l_idx].active = 1;
            loggers[l_idx].thread_id = 0;
            active_loggers_count++;

            char logger_conn_s[128];
            snprintf(logger_conn_s, sizeof(logger_conn_s), "Logger connected (socket %d). Active loggers: %d", client_sock_fd, active_loggers_count);
            print_log("Server", -1, logger_conn_s);
            char bc_logger_conn_s[160];
            snprintf(bc_logger_conn_s, sizeof(bc_logger_conn_s), "[Server internal] Logger connected (socket %d). Active loggers: %d", client_sock_fd, active_loggers_count);

            char accept_msg[BUFFER_SIZE];
            snprintf(accept_msg, sizeof(accept_msg), "%s\n", RSP_LOGGER_ACCEPTED);
            
            if(send(client_sock_fd, accept_msg, strlen(accept_msg), MSG_NOSIGNAL) < 0) {
                char err_send_accept[128];
                snprintf(err_send_accept, sizeof(err_send_accept), "Send RSP_LOGGER_ACCEPTED to socket %d failed: %s", client_sock_fd, strerror(errno));
                print_log("Server", -1, err_send_accept);
                
                loggers[l_idx].active = 0;
                loggers[l_idx].client_socket_fd = -1;
                active_loggers_count--;
                close(client_sock_fd);
                pthread_mutex_unlock(&loggers_mutex);
                continue;
            }

            char log_accepted[128];
            snprintf(log_accepted, sizeof(log_accepted), "Logger (socket %d) confirmed. Active loggers: %d.", client_sock_fd, active_loggers_count);
            print_log("Server", -1, log_accepted);
        
            if (pthread_create(&loggers[l_idx].thread_id, NULL, handle_logger_client, &loggers[l_idx]) != 0) {
                char pth_err_msg[200];
                snprintf(pth_err_msg, sizeof(pth_err_msg), "pthread_create for logger (socket %d) failed: %s. Rolling back.", client_sock_fd, strerror(errno));
                perror("pthread_create for logger failed (main server log)");
                print_log("Server", -1, pth_err_msg);
                
                loggers[l_idx].active = 0;
                loggers[l_idx].client_socket_fd = -1;
                active_loggers_count--;
                close(client_sock_fd);
            }
            pthread_mutex_unlock(&loggers_mutex);

        } else if (strncmp(initial_buffer, MSG_JOIN, strlen(MSG_JOIN)) == 0) {
            // Клиент-философ
            pthread_mutex_lock(&server_mutex);
            if (active_philosophers_count >= NUM_PHILOSOPHERS) {
                print_log("Server", -1, "Table is full. Rejecting philosopher.");
                broadcast_to_loggers("[Server internal] Table is full. Rejecting new philosopher.");
                char full_msg[BUFFER_SIZE];
                snprintf(full_msg, sizeof(full_msg), "%s\n", RSP_FULL);
                send(client_sock_fd, full_msg, strlen(full_msg), MSG_NOSIGNAL);
                close(client_sock_fd);
                pthread_mutex_unlock(&server_mutex);
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
                print_log("Server", -1, "ERROR: Inconsistent state: active_count < MAX but no free slot for philosopher.");
                broadcast_to_loggers("[Server internal] ERROR: No free slot for philosopher (inconsistent state).");
                char full_msg_inconsistent[BUFFER_SIZE];
                snprintf(full_msg_inconsistent, sizeof(full_msg_inconsistent), "%s\n", RSP_FULL);
                send(client_sock_fd, full_msg_inconsistent, strlen(full_msg_inconsistent), MSG_NOSIGNAL);
                close(client_sock_fd);
                pthread_mutex_unlock(&server_mutex);
                continue;
            }

            philosophers[p_idx].client_socket_fd = client_sock_fd;
            philosophers[p_idx].active = 1;
            philosophers[p_idx].has_waiter_permission = 0;
            philosophers[p_idx].has_left_fork = 0;
            philosophers[p_idx].has_right_fork = 0;
            philosophers[p_idx].thread_id = 0;
            active_philosophers_count++;

            char id_response[BUFFER_SIZE];
            snprintf(id_response, BUFFER_SIZE, "%s %d\n", RSP_ID, philosophers[p_idx].id);
            if(send(client_sock_fd, id_response, strlen(id_response), MSG_NOSIGNAL) < 0){
                char err_send_id[128];
                snprintf(err_send_id, sizeof(err_send_id), "Send RSP_ID to P%d (socket %d) failed: %s", philosophers[p_idx].id, client_sock_fd, strerror(errno));
                print_log("Server", -1, err_send_id);
                broadcast_to_loggers(err_send_id);
                philosophers[p_idx].active = 0;
                active_philosophers_count--;
                close(client_sock_fd);
                philosophers[p_idx].client_socket_fd = -1;
                pthread_mutex_unlock(&server_mutex);
                continue;
            }
            
            char log_msg_assign[256];
            snprintf(log_msg_assign, sizeof(log_msg_assign), "Assigned philosopher ID P%d (socket %d). Active philosophers: %d",
                     philosophers[p_idx].id, client_sock_fd, active_philosophers_count);
            print_log("Server", -1, log_msg_assign);
            char bc_log_msg_assign[BUFFER_SIZE];
            snprintf(bc_log_msg_assign, sizeof(bc_log_msg_assign), "[Srv -> P%d] Assigned ID %d. Socket %d. Active philosophers: %d",
                     philosophers[p_idx].id, philosophers[p_idx].id, client_sock_fd, active_philosophers_count);
            broadcast_to_loggers(bc_log_msg_assign);

            if (pthread_create(&philosophers[p_idx].thread_id, NULL, handle_client, &philosophers[p_idx]) != 0) {
                perror("pthread_create for philosopher failed");
                broadcast_to_loggers("[Server internal] pthread_create for philosopher failed.");
                philosophers[p_idx].active = 0;
                active_philosophers_count--;
                close(client_sock_fd);
                philosophers[p_idx].client_socket_fd = -1;
            }
            pthread_mutex_unlock(&server_mutex);

        } else {
            // Неизвестная начальная команда
            char unknown_cmd_log[BUFFER_SIZE];
            snprintf(unknown_cmd_log, sizeof(unknown_cmd_log), "Unknown initial command from client (socket %d): '%.50s'. Closing connection.", client_sock_fd, initial_buffer);
            print_log("Server", -1, unknown_cmd_log);
            broadcast_to_loggers(unknown_cmd_log);
            char error_msg_unknown_init[BUFFER_SIZE];
            snprintf(error_msg_unknown_init, sizeof(error_msg_unknown_init), "%s Unknown initial command\n", RSP_ERROR);
            send(client_sock_fd, error_msg_unknown_init, strlen(error_msg_unknown_init), MSG_NOSIGNAL);
            close(client_sock_fd);
        }
    } 

    print_log("Server", -1, "Server loop ended. Notifying clients and loggers, waiting for them to finish...");
    broadcast_to_loggers("[Server internal] Server main loop ended. Initiating shutdown sequence.");

    pthread_mutex_lock(&server_mutex);
    for (int i = 0; i < NUM_PHILOSOPHERS; ++i) {
        if (philosophers[i].active && philosophers[i].client_socket_fd != -1) {
            char notify_log[128];
            snprintf(notify_log, sizeof(notify_log), "Notifying P%d (socket %d) about server shutdown.", philosophers[i].id, philosophers[i].client_socket_fd);
            print_log("Server", -1, notify_log);
            
            
            char server_down_msg_philo[BUFFER_SIZE];
            snprintf(server_down_msg_philo, sizeof(server_down_msg_philo), "%s\n", MSG_SERVER_DOWN);
            if (send(philosophers[i].client_socket_fd, server_down_msg_philo, strlen(server_down_msg_philo), MSG_NOSIGNAL) < 0) {
                if (errno != EPIPE) { 
                    char err_send_log[150];
                    snprintf(err_send_log, sizeof(err_send_log), "Error sending SERVER_DOWN to P%d: %s", philosophers[i].id, strerror(errno));
                    print_log("Server", -1, err_send_log);
                }
            }
        }
    }
    pthread_mutex_unlock(&server_mutex);

    pthread_mutex_lock(&loggers_mutex);
    for (int i = 0; i < MAX_LOGGERS; ++i) {
        if (loggers[i].active && loggers[i].client_socket_fd != -1) {
            char notify_log_l[128];
            snprintf(notify_log_l, sizeof(notify_log_l), "Notifying Logger (socket %d) about server shutdown.", loggers[i].client_socket_fd);
            print_log("Server", -1, notify_log_l);
            
            char server_down_msg_logger[BUFFER_SIZE];
            snprintf(server_down_msg_logger, sizeof(server_down_msg_logger), "%s\n", MSG_SERVER_DOWN);
            if (send(loggers[i].client_socket_fd, server_down_msg_logger, strlen(server_down_msg_logger), MSG_NOSIGNAL) < 0) {
                 if (errno != EPIPE) {
                    char err_send_log_l[150];
                    snprintf(err_send_log_l, sizeof(err_send_log_l), "Error sending SERVER_DOWN to Logger socket %d: %s", loggers[i].client_socket_fd, strerror(errno));
                    print_log("Server", -1, err_send_log_l);
                }
            }
        }
    }
    pthread_mutex_unlock(&loggers_mutex);

    print_log("Server", -1, "Joining philosopher client threads...");
    broadcast_to_loggers("[Server internal] Joining philosopher client threads...");
    for (int i = 0; i < NUM_PHILOSOPHERS; ++i) {
        pthread_t tid_to_join = 0;
        int sock_fd_to_shutdown = -1;
        int philo_id_to_join = -1;

        pthread_mutex_lock(&server_mutex);
        if (philosophers[i].active && philosophers[i].thread_id != 0) {
            tid_to_join = philosophers[i].thread_id;
            sock_fd_to_shutdown = philosophers[i].client_socket_fd;
            philo_id_to_join = philosophers[i].id;
        }
        pthread_mutex_unlock(&server_mutex);

        if (tid_to_join != 0) {
            char join_log[128];
            snprintf(join_log, sizeof(join_log), "Attempting to shutdown and join P%d thread (TID %lu)...", philo_id_to_join, (unsigned long)tid_to_join);
            print_log("Server", -1, join_log);

            if(sock_fd_to_shutdown != -1) {
                shutdown(sock_fd_to_shutdown, SHUT_RDWR); 
            }

            if (pthread_join(tid_to_join, NULL) != 0) {
                char err_log[150];
                snprintf(err_log, sizeof(err_log), "pthread_join for P%d (TID %lu) failed: %s", philo_id_to_join, (unsigned long)tid_to_join, strerror(errno));
                print_log("Server",-1, err_log);
            } else {
                snprintf(join_log, sizeof(join_log), "P%d thread (TID %lu) joined.", philo_id_to_join, (unsigned long)tid_to_join);
                print_log("Server", -1, join_log);
            }

            pthread_mutex_lock(&server_mutex);
            philosophers[i].active = 0; 
            if(philosophers[i].client_socket_fd != -1) {
                philosophers[i].client_socket_fd = -1;
            }
            philosophers[i].thread_id = 0;
            pthread_mutex_unlock(&server_mutex);
        }
    }
    
    print_log("Server", -1, "Joining logger client threads...");

    for (int i = 0; i < MAX_LOGGERS; ++i) {
        pthread_t tid_to_join_l = 0;
        int sock_fd_to_shutdown_l = -1;
        int logger_was_active_and_had_thread = 0;

        pthread_mutex_lock(&loggers_mutex);
        if (loggers[i].active && loggers[i].thread_id != 0) { 
            tid_to_join_l = loggers[i].thread_id;
            sock_fd_to_shutdown_l = loggers[i].client_socket_fd;
            logger_was_active_and_had_thread = 1;
        }
        pthread_mutex_unlock(&loggers_mutex);

        if (logger_was_active_and_had_thread) {
            char join_log_l[128];
            snprintf(join_log_l, sizeof(join_log_l), 
                     "Attempting to shutdown socket %d and join Logger thread (TID %lu)...", 
                     sock_fd_to_shutdown_l, (unsigned long)tid_to_join_l);
            print_log("Server", -1, join_log_l);

            if(sock_fd_to_shutdown_l != -1) {
                if (shutdown(sock_fd_to_shutdown_l, SHUT_RDWR) < 0) {
                    char shut_err_log[100];
                    snprintf(shut_err_log, sizeof(shut_err_log), "shutdown() for logger socket %d failed: %s", sock_fd_to_shutdown_l, strerror(errno));
                    print_log("Server", -1, shut_err_log);
                }
            }

            if (pthread_join(tid_to_join_l, NULL) != 0) {
                char err_log_l[150];
                snprintf(err_log_l, sizeof(err_log_l), 
                         "pthread_join for Logger (socket %d, TID %lu) failed: %s", 
                         sock_fd_to_shutdown_l, (unsigned long)tid_to_join_l, strerror(errno));
                print_log("Server",-1, err_log_l);
            } else {
                snprintf(join_log_l, sizeof(join_log_l), 
                         "Logger thread (TID %lu, associated with former socket %d) joined.", 
                         (unsigned long)tid_to_join_l, sock_fd_to_shutdown_l);
                print_log("Server", -1, join_log_l);
            }

            pthread_mutex_lock(&loggers_mutex);
            loggers[i].active = 0;
            if(loggers[i].client_socket_fd != -1) {
                loggers[i].client_socket_fd = -1;
            }
            loggers[i].thread_id = 0;
            pthread_mutex_unlock(&loggers_mutex);
        }
    }

    pthread_mutex_lock(&server_mutex);
    int final_active_count = 0;
    for(int i=0; i<NUM_PHILOSOPHERS; ++i) { 
        if(philosophers[i].active) {
            final_active_count++; 
        }
    }
    active_philosophers_count = final_active_count;
    char final_count_log[100];
    snprintf(final_count_log, sizeof(final_count_log), "All client threads processed. Final active philosopher count: %d", active_philosophers_count);
    print_log("Server",-1,final_count_log);
    broadcast_to_loggers(final_count_log);
    pthread_mutex_unlock(&server_mutex);

    pthread_mutex_lock(&loggers_mutex);
    int final_active_loggers = 0;
    for(int i=0; i<MAX_LOGGERS; ++i) { 
        if(loggers[i].active) { 
            final_active_loggers++;
        } 
    }
    active_loggers_count = final_active_loggers;
    char final_loggers_count_log[100];
    snprintf(final_loggers_count_log, sizeof(final_loggers_count_log), "Final active logger count: %d", active_loggers_count);
    print_log("Server", -1, final_loggers_count_log);
    pthread_mutex_unlock(&loggers_mutex);

    print_log("Server", -1, "Proceeding with final resource cleanup...");
    if (server_fd_global != -1) {
        server_fd_global = -1;
    }
    cleanup_all_named_semaphores();
    pthread_mutex_destroy(&server_mutex);
    pthread_mutex_destroy(&loggers_mutex);
    print_log("Server", -1, "Server shut down completely.");
    return 0;
}