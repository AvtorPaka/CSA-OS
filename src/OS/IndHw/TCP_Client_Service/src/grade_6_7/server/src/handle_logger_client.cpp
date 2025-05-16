#include "server_global.h"
#include <sys/select.h> 
#include <cerrno>

void *handle_logger_client(void *arg) {
    logger_client_state_t *logger_state = (logger_client_state_t *)arg;
    int client_sock = logger_state->client_socket_fd;
    char buffer[BUFFER_SIZE];
    char log_msg[256];
    char broadcast_buf[BUFFER_SIZE];


    snprintf(log_msg, sizeof(log_msg), "Logger thread started for socket %d.", client_sock);
    print_log("Server", -1, log_msg);


    fd_set read_fds;
    struct timeval tv;

    while (!server_shutdown_flag) {
        FD_ZERO(&read_fds);
        FD_SET(client_sock, &read_fds);
        
        tv.tv_sec = 1; 
        tv.tv_usec = 0;
        int activity = select(client_sock + 1, &read_fds, NULL, NULL, &tv);

        if (server_shutdown_flag) {
            break;
        }

        if (activity < 0) {
            if (errno == EINTR) { 
                continue; 
            }
            snprintf(log_msg, sizeof(log_msg), "select() error in logger thread for socket %d: %s. Disconnecting logger.", client_sock, strerror(errno));
            print_log("Server", -1, log_msg);
            snprintf(broadcast_buf, sizeof(broadcast_buf), "[Server internal] select() error for logger socket %d: %s. Disconnecting.", client_sock, strerror(errno));
            broadcast_to_loggers(broadcast_buf);
            break;
        }
        
        if (FD_ISSET(client_sock, &read_fds)) {
            ssize_t n_read = recv(client_sock, buffer, BUFFER_SIZE - 1, 0);
            if (n_read <= 0) {
                if (n_read == 0) {
                    snprintf(log_msg, sizeof(log_msg), "Logger (socket %d) disconnected gracefully.", client_sock);
                } else { 
                    snprintf(log_msg, sizeof(log_msg), "Logger (socket %d) recv error: %s. Assuming disconnection.", client_sock, strerror(errno));
                }
                print_log("Server", -1, log_msg);
                snprintf(broadcast_buf, sizeof(broadcast_buf), "[Server internal] Logger (socket %d) disconnected.", client_sock);
                broadcast_to_loggers(broadcast_buf);
                break; 
            }
            buffer[n_read] = '\0';
        }
    }

    snprintf(log_msg, sizeof(log_msg), "Logger thread for socket %d stopping.", client_sock);
    print_log("Server", -1, log_msg);

    pthread_mutex_lock(&loggers_mutex);
    if (logger_state->active) {
        logger_state->active = 0;
        active_loggers_count--;
        snprintf(log_msg, sizeof(log_msg), "Logger (socket %d) deactivated. Active loggers now: %d", client_sock, active_loggers_count);
        print_log("Server", -1, log_msg);
    }
    if (logger_state->client_socket_fd != -1) {
        close(logger_state->client_socket_fd);
        logger_state->client_socket_fd = -1;
    }
    pthread_mutex_unlock(&loggers_mutex);

    snprintf(log_msg, sizeof(log_msg), "Logger thread for socket %d finished cleanup and exiting.", client_sock);
    print_log("Server", -1, log_msg);

    return NULL;
}