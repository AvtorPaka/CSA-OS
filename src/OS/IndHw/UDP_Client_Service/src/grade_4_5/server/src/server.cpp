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
    sigaction(SIGINT, &sa_server_shutdown, nullptr);
    sigaction(SIGTERM, &sa_server_shutdown, nullptr);

    init_server_state();
    print_log("Server", -1, "Server initialized with named semaphores.");

    struct sockaddr_in server_addr{}, client_addr_from_recv{};
    socklen_t client_addr_len = sizeof(client_addr_from_recv);
    char recv_buffer[BUFFER_SIZE];

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
        cleanup_all_named_semaphores();
        return 1;
    }

    server_addr.sin_family = AF_INET;
    if (inet_pton(AF_INET, bind_ip_address, &server_addr.sin_addr) <= 0) {
        char err_msg[100];
        snprintf(err_msg, sizeof(err_msg), "Invalid bind IP address: %s", bind_ip_address);
        print_log("Server", -1, err_msg); 
        perror("inet_pton");
        close(server_udp_fd_global);
        cleanup_all_named_semaphores();
        return 1;
    }
    server_addr.sin_port = htons(port);

    if (bind(server_udp_fd_global, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        char err_msg[120];
        snprintf(err_msg, sizeof(err_msg), "bind failed for %s:%d", bind_ip_address, port);
        print_log("Server", -1, err_msg);
        perror("bind");
        close(server_udp_fd_global);
        cleanup_all_named_semaphores();
        return 1;
    }
    char log_buf_bind[100];
    snprintf(log_buf_bind, sizeof(log_buf_bind), "Bind successful to UDP %s:%d.", bind_ip_address, port);
    print_log("Server", -1, log_buf_bind);

    char log_buf[100];
    snprintf(log_buf, sizeof(log_buf), "Server listening for UDP packets on port %d", port);
    print_log("Server", -1, log_buf);

    while (!server_shutdown_flag) {
        client_addr_len = sizeof(client_addr_from_recv); 
        ssize_t n_read = recvfrom(server_udp_fd_global, recv_buffer, BUFFER_SIZE - 1, 0,
                                  (struct sockaddr *)&client_addr_from_recv, &client_addr_len);

        if (server_shutdown_flag) {
            print_log("Server", -1, "Shutdown flag detected in main loop, exiting recvfrom loop.");
            break;
        }

        if (n_read < 0) {
            if (errno == EINTR && !server_shutdown_flag) {
                print_log("Server", -1, "recvfrom interrupted by signal, continuing...");
                continue;
            }
            if (server_udp_fd_global == -1 || errno == EBADF) {
                 print_log("Server", -1, "recvfrom failed because main UDP socket closed. Shutting down...");
            } else {
                char err_msg[120];
                snprintf(err_msg, sizeof(err_msg), "recvfrom failed: %s", strerror(errno));
                print_log("Server", -1, err_msg);
            }
            break; 
        }
        recv_buffer[n_read] = '\0'; 

        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr_from_recv.sin_addr, client_ip, INET_ADDRSTRLEN);
        snprintf(log_buf, sizeof(log_buf), "Received packet from %s:%d : '%s'", client_ip, ntohs(client_addr_from_recv.sin_port), recv_buffer);
        print_log("Server", -1, log_buf);

        char command[BUFFER_SIZE];
        int msg_philosopher_id = -1;
        if (sscanf(recv_buffer, "%s %d", command, &msg_philosopher_id) < 1) {
            snprintf(log_buf, sizeof(log_buf), "Malformed packet from %s:%d: '%s'. Ignoring.", client_ip, ntohs(client_addr_from_recv.sin_port), recv_buffer);
            print_log("Server",-1,log_buf);
            continue;
        }
        if (strncmp(recv_buffer, MSG_JOIN, strlen(MSG_JOIN)) == 0) { 
            pthread_mutex_lock(&server_mutex);
            if (active_philosophers_count >= NUM_PHILOSOPHERS) {
                pthread_mutex_unlock(&server_mutex);
                print_log("Server", -1, "Table is full. Rejecting JOIN request.");
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
                print_log("Server", -1, "ERROR: Inconsistent state: active_count < MAX but no free slot. Rejecting.");
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
                philosophers[p_idx].active = 0;
                pthread_mutex_unlock(&server_mutex);
                continue;
            }
            active_philosophers_count++;
            snprintf(log_buf, sizeof(log_buf), "Assigning P%d to %s:%d. Pipe fds: R=%d, W=%d. Active philosophers: %d",
                     philosophers[p_idx].id, client_ip, ntohs(client_addr_from_recv.sin_port),
                     philosophers[p_idx].comm_pipe_fd[0], philosophers[p_idx].comm_pipe_fd[1],
                     active_philosophers_count);
            print_log("Server", -1, log_buf);

            if (pthread_create(&philosophers[p_idx].thread_id, nullptr, handle_philosopher_requests, &philosophers[p_idx]) != 0) {
                perror("pthread_create for philosopher failed");
                philosophers[p_idx].active = 0; 
                active_philosophers_count--;
                close(philosophers[p_idx].comm_pipe_fd[0]); philosophers[p_idx].comm_pipe_fd[0] = -1;
                close(philosophers[p_idx].comm_pipe_fd[1]); philosophers[p_idx].comm_pipe_fd[1] = -1;
            }
            pthread_mutex_unlock(&server_mutex);

        } else { 
            char actual_command[32]; 
            int philo_id_from_msg;
            if (sscanf(recv_buffer, "%31s %d", actual_command, &philo_id_from_msg) != 2) {
                 snprintf(log_buf, sizeof(log_buf), "Malformed non-JOIN command from %s:%d: '%s'. Expecting 'COMMAND <id>'. Ignoring.", client_ip, ntohs(client_addr_from_recv.sin_port), recv_buffer);
                 print_log("Server",-1,log_buf);
                 sendto(server_udp_fd_global, RSP_ERROR " Malformed command", strlen(RSP_ERROR " Malformed command"), 0, (struct sockaddr *)&client_addr_from_recv, client_addr_len);
                 continue;
            }

            pthread_mutex_lock(&server_mutex);
            philosopher_state_t* p_state = find_philosopher_by_id_locked(philo_id_from_msg);

            if (!p_state || !p_state->active) {
                pthread_mutex_unlock(&server_mutex);
                snprintf(log_buf, sizeof(log_buf), "Message from %s:%d for inactive or unknown P%d ('%s'). Ignoring.", client_ip, ntohs(client_addr_from_recv.sin_port), philo_id_from_msg, recv_buffer);
                print_log("Server", -1, log_buf);
                continue;
            }

            if (p_state->client_addr.sin_addr.s_addr != client_addr_from_recv.sin_addr.s_addr ||
                p_state->client_addr.sin_port != client_addr_from_recv.sin_port) {
                pthread_mutex_unlock(&server_mutex);
                char stored_ip[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &p_state->client_addr.sin_addr, stored_ip, INET_ADDRSTRLEN);
                snprintf(log_buf, sizeof(log_buf),
                         "P%d: Address mismatch for message '%s'. Expected %s:%d, got %s:%d. Ignoring.",
                         philo_id_from_msg, recv_buffer, stored_ip, ntohs(p_state->client_addr.sin_port),
                         client_ip, ntohs(client_addr_from_recv.sin_port));
                print_log("Server", -1, log_buf);
                continue;
            }

            ssize_t written = write(p_state->comm_pipe_fd[1], actual_command, strlen(actual_command));
            if (written < 0) {
                char err_pipe[100];
                snprintf(err_pipe, sizeof(err_pipe), "P%d: Error writing '%s' to pipe fd %d: %s",
                         p_state->id, actual_command, p_state->comm_pipe_fd[1], strerror(errno));
                print_log("Server", -1, err_pipe);
            } else {
                 snprintf(log_buf, sizeof(log_buf), "Forwarded '%s' to P%d via pipe fd %d.",
                         actual_command, p_state->id, p_state->comm_pipe_fd[1]);
                 print_log("Server", -1, log_buf);
            }
            pthread_mutex_unlock(&server_mutex);
        }
    }

    print_log("Server", -1, "Server loop ended. Notifying clients and preparing for shutdown...");

    pthread_mutex_lock(&server_mutex);
    for (int i = 0; i < NUM_PHILOSOPHERS; ++i) {
        if (philosophers[i].active && philosophers[i].thread_id != 0) { 
            char notify_log[120];
            snprintf(notify_log, sizeof(notify_log), "Notifying P%d about server shutdown.", philosophers[i].id);
            print_log("Server", -1, notify_log);

            sendto(server_udp_fd_global, MSG_SERVER_DOWN, strlen(MSG_SERVER_DOWN), 0,
                   (struct sockaddr *)&philosophers[i].client_addr, philosophers[i].client_addr_len);

            if (philosophers[i].comm_pipe_fd[1] != -1) {
                snprintf(notify_log, sizeof(notify_log), "Closing pipe write_fd %d for P%d.", philosophers[i].comm_pipe_fd[1], philosophers[i].id);
                print_log("Server", -1, notify_log);
                close(philosophers[i].comm_pipe_fd[1]);
                philosophers[i].comm_pipe_fd[1] = -1;
            }
        }
    }
    pthread_mutex_unlock(&server_mutex);

    print_log("Server", -1, "Joining client threads...");
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
            char join_log[100];
            snprintf(join_log, sizeof(join_log), "Attempting to join P%d thread (TID %lu)...", philo_id_to_join, (unsigned long)tid_to_join);
            print_log("Server", -1, join_log);


            if (pthread_join(tid_to_join, nullptr) != 0) {
                char err_log[120];
                snprintf(err_log, sizeof(err_log), "pthread_join for P%d (TID %lu) failed: %s", philo_id_to_join, (unsigned long)tid_to_join, strerror(errno));
                print_log("Server",-1, err_log);
            } else {
                snprintf(join_log, sizeof(join_log), "P%d thread (TID %lu) joined.", philo_id_to_join, (unsigned long)tid_to_join);
                print_log("Server", -1, join_log);
            }

            pthread_mutex_lock(&server_mutex);
            if (philosophers[i].comm_pipe_fd[0] != -1) {
                close(philosophers[i].comm_pipe_fd[0]);
                philosophers[i].comm_pipe_fd[0] = -1;
            }
            if (philosophers[i].comm_pipe_fd[1] != -1) {
                close(philosophers[i].comm_pipe_fd[1]);
                philosophers[i].comm_pipe_fd[1] = -1;
            }
            philosophers[i].thread_id = 0; 
            if (philosophers[i].active) { 
                print_log("Server", philosophers[i].id, "Warning: Thread joined but philosopher still marked active. Deactivating.");
                philosophers[i].active = 0;
                active_philosophers_count--; 
            }
            pthread_mutex_unlock(&server_mutex);
        }
    }

    pthread_mutex_lock(&server_mutex);
    int final_active_count = 0;
    for(int i=0; i<NUM_PHILOSOPHERS; ++i) {
        if(philosophers[i].active) final_active_count++;
    }
    snprintf(log_buf, sizeof(log_buf), "All client threads processed. Current active_philosophers_count from global: %d. Recalculated active: %d", active_philosophers_count, final_active_count);
    print_log("Server",-1,log_buf);
    active_philosophers_count = final_active_count; 
    pthread_mutex_unlock(&server_mutex);

    print_log("Server", -1, "Proceeding with final resource cleanup...");
    if (server_udp_fd_global != -1) {
        int fd = server_udp_fd_global;
        server_udp_fd_global = -1; 
        close(fd);
        print_log("Server", -1, "Main UDP socket closed in final cleanup.");

    }
    cleanup_all_named_semaphores();
    pthread_mutex_destroy(&server_mutex);
    print_log("Server", -1, "Server shut down completely.");
    return 0;
}