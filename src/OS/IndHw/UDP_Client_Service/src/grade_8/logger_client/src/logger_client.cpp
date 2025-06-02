#include "common/common_sockets.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <csignal>
#include <cerrno>

volatile sig_atomic_t logger_client_shutdown_flag = 0;
int logger_sock_fd_global = -1;
bool g_logger_was_accepted = false;
bool g_server_reported_down = false;

void logger_client_signal_handler(int sig) {
    if (sig == SIGINT || sig == SIGTERM) {
        if (!logger_client_shutdown_flag) {
            print_log("LoggerClient", -1, "SIGINT/SIGTERM received. Initiating shutdown...");
            logger_client_shutdown_flag = 1;
        }
    }
}

int send_message_to_server_udp(int sock_fd, const char* message) {
    if (sock_fd == -1) {
        print_log("LoggerClient", -1, "Attempted to send on invalid socket.");
        return -1;
    }
    if (message == NULL || message[0] == '\0') {
        print_log("LoggerClient", -1, "Attempted to send NULL or empty message.");
        return -1;
    }
    if (send(sock_fd, message, strlen(message), 0) < 0) {
        char error_log[150];
        snprintf(error_log, sizeof(error_log), "send() failed for message (first 20 chars): '%.20s': %s", message, strerror(errno));
        print_log("LoggerClient", -1, error_log);
        return -1;
    }
    return 0;
}

ssize_t receive_message_from_server_udp(int sock_fd, char* buffer, size_t buffer_len) {
    if (sock_fd == -1 || buffer == NULL || buffer_len == 0) {
        return -1; 
    }
    ssize_t n_read = recv(sock_fd, buffer, buffer_len - 1, 0);
    if (n_read < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return -2; 
        } else if (errno == EINTR) { 
            if (logger_client_shutdown_flag) {
                print_log("LoggerClient", -1, "recv() interrupted by shutdown signal.");
                return -3; 
            } else {
                return -4; 
            }
        }
        char error_log[100];
        snprintf(error_log, sizeof(error_log), "recv() error: %s", strerror(errno));
        print_log("LoggerClient", -1, error_log);
    } else if (n_read > 0) {
        buffer[n_read] = '\0'; 
    }
    return n_read;
}


int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <server_ip> <server_port>\n", argv[0]);
        return 1;
    }
    const char *server_ip = argv[1];
    int server_port = atoi(argv[2]);

    struct sigaction sa_logger_shutdown;
    memset(&sa_logger_shutdown, 0, sizeof(sa_logger_shutdown));
    sa_logger_shutdown.sa_handler = logger_client_signal_handler;
    sigaction(SIGINT, &sa_logger_shutdown, NULL);
    sigaction(SIGTERM, &sa_logger_shutdown, NULL);

    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE];
    char log_buffer_cli[128]; 
    ssize_t n_read_current_op = -1;

    if ((logger_sock_fd_global = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
        perror("[LoggerClient] UDP Socket creation error");
        return 1;
    }

    struct timeval tv_join, tv_logs;
    tv_join.tv_sec = 5; 
    tv_join.tv_usec = 0;

    tv_logs.tv_sec = 1; 
    tv_logs.tv_usec = 0; 

    if (setsockopt(logger_sock_fd_global, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv_join, sizeof tv_join) < 0) {
        perror("[LoggerClient] setsockopt SO_RCVTIMEO for JOIN failed");
        close(logger_sock_fd_global);
        return 1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(server_port);

    if (inet_pton(AF_INET, server_ip, &serv_addr.sin_addr) <= 0) {
        perror("[LoggerClient] Invalid address/ Address not supported");
        close(logger_sock_fd_global);
        return 1;
    }

    if (connect(logger_sock_fd_global, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        snprintf(log_buffer_cli, sizeof(log_buffer_cli), "UDP connect() failed to %s:%d: %s", server_ip, server_port, strerror(errno));
        print_log("LoggerClient", -1, log_buffer_cli);
        close(logger_sock_fd_global);
        return 1;
    }
    print_log("LoggerClient", -1, "UDP socket configured to send to server.");

    if (send_message_to_server_udp(logger_sock_fd_global, MSG_JOIN_LOGGER) != 0) {
        goto logger_cleanup_exit;
    }
    print_log("LoggerClient", -1, "Sent JOIN_LOGGER. Waiting for acceptance...");

    n_read_current_op = receive_message_from_server_udp(logger_sock_fd_global, buffer, BUFFER_SIZE);

    if (logger_client_shutdown_flag && n_read_current_op != -3) {
        print_log("LoggerClient",-1, "Shutdown signal received during JOIN_LOGGER response wait (flag was already set).");
        goto logger_cleanup_exit;
    }
    if (n_read_current_op == -3) {
        goto logger_cleanup_exit;
    }


    if (n_read_current_op <= 0) {
        if (n_read_current_op == -2) { 
            print_log("LoggerClient", -1, "Timeout waiting for JOIN_LOGGER response. Server might be down or not responding.");
        } else if (n_read_current_op == 0) {
            print_log("LoggerClient", -1, "Received empty datagram after JOIN_LOGGER (unexpected).");
        } else if (n_read_current_op == -1){
        }
        goto logger_cleanup_exit;
    }
    
    if (strncmp(buffer, RSP_LOGGER_ACCEPTED, strlen(RSP_LOGGER_ACCEPTED)) == 0) {
        print_log("LoggerClient", -1, "Successfully joined as logger.");
        g_logger_was_accepted = true;
    } else if (strncmp(buffer, RSP_LOGGER_REJECTED, strlen(RSP_LOGGER_REJECTED)) == 0) {
        print_log("LoggerClient", -1, "Server rejected logger connection (e.g., max loggers reached or other issue).");
        goto logger_cleanup_exit;
    } else if (strncmp(buffer, MSG_SERVER_DOWN, strlen(MSG_SERVER_DOWN)) == 0) {
        print_log("LoggerClient", -1, "Received SERVER_DOWN from server during JOIN. Exiting.");
        logger_client_shutdown_flag = 1;
        g_server_reported_down = true;  
        goto logger_cleanup_exit;       
    } else {
        snprintf(log_buffer_cli, sizeof(log_buffer_cli), "Unexpected response after JOIN_LOGGER: '%.*s'", (int)n_read_current_op, buffer);
        print_log("LoggerClient", -1, log_buffer_cli);
        goto logger_cleanup_exit;
    }

    if (setsockopt(logger_sock_fd_global, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv_logs, sizeof tv_logs) < 0) {
        perror("[LoggerClient] setsockopt SO_RCVTIMEO for logs failed");
    }

    print_log("LoggerClient", -1, "Listening for logs from server...");
    while (!logger_client_shutdown_flag) {
        n_read_current_op = receive_message_from_server_udp(logger_sock_fd_global, buffer, BUFFER_SIZE);

        if (logger_client_shutdown_flag && n_read_current_op != -3) {
            if (n_read_current_op > 0) {
                printf("%s", buffer); 
                fflush(stdout);
                char* end_marker_sd = buffer + strlen(MSG_SERVER_DOWN);
                if (strncmp(buffer, MSG_SERVER_DOWN, strlen(MSG_SERVER_DOWN)) == 0 && (*end_marker_sd == '\0' || *end_marker_sd == '\n')) {
                    g_server_reported_down = true;
                }
            }
            break;
        }
        if (n_read_current_op == -3) {
            break;
        }


        if (n_read_current_op == -2) {
            continue;
        } else if (n_read_current_op < 0 || n_read_current_op == -4) { 
            logger_client_shutdown_flag = 1;
            break;
        } else if (n_read_current_op == 0) { 
            continue;
        }

        printf("%s", buffer);
        fflush(stdout);

        char* end_of_server_down_marker = buffer + strlen(MSG_SERVER_DOWN);
        if (strncmp(buffer, MSG_SERVER_DOWN, strlen(MSG_SERVER_DOWN)) == 0 && 
            (*end_of_server_down_marker == '\0' || *end_of_server_down_marker == '\n' || *end_of_server_down_marker == '\r')) {
             print_log("LoggerClient", -1, "Detected SERVER_DOWN message. Exiting.");
             logger_client_shutdown_flag = 1; 
             g_server_reported_down = true;   
        }
    }

logger_cleanup_exit:
    print_log("LoggerClient", -1, "Shutting down.");

    if (g_logger_was_accepted && !g_server_reported_down) {
        print_log("LoggerClient", -1, "Sending MSG_LEAVE_LOGGER to server (best-effort).");
        send_message_to_server_udp(logger_sock_fd_global, MSG_LEAVE_LOGGER); 
    } else if (g_server_reported_down) {
        print_log("LoggerClient", -1, "Not sending MSG_LEAVE_LOGGER because server reported it is down.");
    } else if (!g_logger_was_accepted) {
        print_log("LoggerClient", -1, "Not sending MSG_LEAVE_LOGGER because client was not accepted by server.");
    }


    if (logger_sock_fd_global != -1) {
        close(logger_sock_fd_global);
        logger_sock_fd_global = -1;
    }
    print_log("LoggerClient", -1, "Connection closed. Exiting.");
    return 0;
}