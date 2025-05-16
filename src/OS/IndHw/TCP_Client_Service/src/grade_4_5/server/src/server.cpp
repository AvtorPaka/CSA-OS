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

    struct sigaction sa_server_shutdown{};
    memset(&sa_server_shutdown, 0, sizeof(sa_server_shutdown));
    sa_server_shutdown.sa_handler = server_signal_handler;
    sigaction(SIGINT, &sa_server_shutdown, nullptr);
    sigaction(SIGTERM, &sa_server_shutdown, nullptr);

    init_server_state();
    print_log("Server", -1, "Server initialized with named semaphores.");

    int client_sock;
    struct sockaddr_in server_addr{}, client_addr{};
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
        perror(err_msg);
        close(server_fd_global);
        cleanup_all_named_semaphores();
        return 1;
    }
    server_addr.sin_port = htons(port);

    if (bind(server_fd_global, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        char err_msg[120];
        snprintf(err_msg, sizeof(err_msg), "bind failed for %s:%d", bind_ip_address, port);
        perror(err_msg);
        close(server_fd_global);
        cleanup_all_named_semaphores();
        return 1;
    }
    char log_buf_bind[100];
    snprintf(log_buf_bind, sizeof(log_buf_bind), "Bind successful to %s:%d.", bind_ip_address, port);
    print_log("Server", -1, log_buf_bind);

    if (listen(server_fd_global, NUM_PHILOSOPHERS + 2) < 0) {
        perror("listen failed");
        close(server_fd_global);
        cleanup_all_named_semaphores();
        return 1;
    }
    char log_buf[50];
    snprintf(log_buf, sizeof(log_buf), "Server listening on port %d", port);
    print_log("Server", -1, log_buf);

    while (!server_shutdown_flag) {
        client_sock = accept(server_fd_global, (struct sockaddr *) &client_addr, &client_addr_len);

        if (server_shutdown_flag) {
            if (client_sock >=0) {
                close(client_sock);
            }
            break;
        }

        if (client_sock < 0) {
            if (errno == EINTR && server_fd_global == -1) {
                print_log("Server", -1, "accept() interrupted, server_fd closed by signal handler. Shutting down...");
                break;
            } else if (errno == EINTR) {
                print_log("Server", -1, "accept() interrupted by signal. Continuing...");
                continue;
            }
            else if (server_fd_global == -1) {
                break;
            }
            perror("accept failed");
            continue;
        }

        if (server_fd_global == -1) {
            print_log("Server", -1, "Server socket closed during accept. Shutting down.");
            if (client_sock >= 0) {
                close(client_sock);
            }
            break;
        }

        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
        snprintf(log_buf, sizeof(log_buf), "Connection accepted from %s:%d", client_ip, ntohs(client_addr.sin_port));
        print_log("Server", -1, log_buf);

        pthread_mutex_lock(&server_mutex);
        if (active_philosophers_count >= NUM_PHILOSOPHERS) {
            print_log("Server", -1, "Table is full. Rejecting client.");
            send(client_sock, RSP_FULL, strlen(RSP_FULL), 0);
            close(client_sock);
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
            print_log("Server", -1, "ERROR: Inconsistent state: active_count < MAX but no free slot.");
            send(client_sock, RSP_FULL, strlen(RSP_FULL), 0);
            close(client_sock);
            pthread_mutex_unlock(&server_mutex);
            continue;
        }

        philosophers[p_idx].client_socket_fd = client_sock;
        philosophers[p_idx].active = 1;
        philosophers[p_idx].has_waiter_permission = 0;
        philosophers[p_idx].has_left_fork = 0;
        philosophers[p_idx].has_right_fork = 0;
        philosophers[p_idx].thread_id = nullptr;

        active_philosophers_count++;
        snprintf(log_buf, sizeof(log_buf), "Assigning philosopher ID %d.Active philosophers: %d",
                 philosophers[p_idx].id, active_philosophers_count);
        print_log("Server", -1, log_buf);

        if (pthread_create(&philosophers[p_idx].thread_id, nullptr, handle_client, &philosophers[p_idx]) != 0) {
            perror("pthread_create failed");
            philosophers[p_idx].active = 0;
            active_philosophers_count--;
            close(client_sock);
            philosophers[p_idx].client_socket_fd = -1;
        }

        pthread_mutex_unlock(&server_mutex);
    }

    print_log("Server", -1, "Server loop ended. Notifying clients and waiting for them to finish...");


    pthread_mutex_lock(&server_mutex);
    for (int i = 0; i < NUM_PHILOSOPHERS; ++i) {
        if (philosophers[i].active && philosophers[i].client_socket_fd != -1) {
            char notify_log[100];
            snprintf(notify_log, sizeof(notify_log), "Notifying P%d (socket %d) about server shutdown.", philosophers[i].id, philosophers[i].client_socket_fd);
            print_log("Server", -1, notify_log);
            if (send(philosophers[i].client_socket_fd, MSG_SERVER_DOWN "\n", strlen(MSG_SERVER_DOWN) + 1, MSG_NOSIGNAL) < 0) {
                if (errno != EPIPE) {
                    char err_send_log[120];
                    snprintf(err_send_log, sizeof(err_send_log), "Error sending SERVER_DOWN to P%d: %s", philosophers[i].id, strerror(errno));
                    print_log("Server", -1, err_send_log);
                }
            }
        }
    }
    pthread_mutex_unlock(&server_mutex);

    print_log("Server", -1, "Joining client threads...");
    for (int i = 0; i < NUM_PHILOSOPHERS; ++i) {
        pthread_t tid_to_join = nullptr;
        int sock_fd_to_shutdown = -1;
        int philo_id_to_join = -1;

        pthread_mutex_lock(&server_mutex);
        if (philosophers[i].active && philosophers[i].thread_id != nullptr) {
            tid_to_join = philosophers[i].thread_id;
            sock_fd_to_shutdown = philosophers[i].client_socket_fd;
            philo_id_to_join = philosophers[i].id;
        }
        pthread_mutex_unlock(&server_mutex);

        if (tid_to_join != nullptr) {
            char join_log[100];
            snprintf(join_log, sizeof(join_log), "Attempting to join P%d thread (TID %lu)...", philo_id_to_join, (unsigned long)tid_to_join);
            print_log("Server", -1, join_log);

            if(sock_fd_to_shutdown != -1) {
                shutdown(sock_fd_to_shutdown, SHUT_RDWR);
            }

            if (pthread_join(tid_to_join, nullptr) != 0) {
                char err_log[120];
                snprintf(err_log, sizeof(err_log), "pthread_join for P%d (TID %lu) failed: %s", philo_id_to_join, (unsigned long)tid_to_join, strerror(errno));
                print_log("Server",-1, err_log);
            } else {
                snprintf(join_log, sizeof(join_log), "P%d thread (TID %lu) joined.", philo_id_to_join, (unsigned long)tid_to_join);
                print_log("Server", -1, join_log);
            }

            pthread_mutex_lock(&server_mutex);
            philosophers[i].active = 0;
            if(philosophers[i].client_socket_fd != -1) {
                close(philosophers[i].client_socket_fd);
                philosophers[i].client_socket_fd = -1;
            }
            philosophers[i].thread_id = nullptr;
            pthread_mutex_unlock(&server_mutex);
        }
    }

    pthread_mutex_lock(&server_mutex);
    int final_active_count = 0;
    for(int i=0; i<NUM_PHILOSOPHERS; ++i) {
        if(philosophers[i].active) final_active_count++;
    }
    active_philosophers_count = final_active_count;
    char final_count_log[100];
    snprintf(final_count_log, sizeof(final_count_log), "All client threads processed. Final active count: %d", active_philosophers_count);
    print_log("Server",-1,final_count_log);
    pthread_mutex_unlock(&server_mutex);

    print_log("Server", -1, "Proceeding with final resource cleanup...");
    if (server_fd_global != -1) {
        close(server_fd_global);
        server_fd_global = -1;
    }
    cleanup_all_named_semaphores();
    pthread_mutex_destroy(&server_mutex);
    print_log("Server", -1, "Server shut down completely.");
    return 0;
}