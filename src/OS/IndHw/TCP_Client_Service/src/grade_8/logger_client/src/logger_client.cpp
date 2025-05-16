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

void logger_client_signal_handler(int sig) {
    if (sig == SIGINT || sig == SIGTERM) {
        if (!logger_client_shutdown_flag) {
            const char msg[] = "[LoggerClient Signal] SIGINT/SIGTERM received. Initiating shutdown...\n";
            ssize_t unused __attribute__((unused)) = write(STDOUT_FILENO, msg, strlen(msg));
            logger_client_shutdown_flag = 1;
        }
    }
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
    
    struct sigaction sa_pipe_logger;
    memset(&sa_pipe_logger, 0, sizeof(sa_pipe_logger));
    sa_pipe_logger.sa_handler = SIG_IGN;
    sigaction(SIGPIPE, &sa_pipe_logger, NULL);


    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE];
    char log_buffer_cli[128]; 
    char* newline;

    if ((logger_sock_fd_global = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("[LoggerClient] Socket creation error");
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
        snprintf(log_buffer_cli, sizeof(log_buffer_cli), "[LoggerClient] Connection Failed to %s:%d", server_ip, server_port);
        perror(log_buffer_cli);
        close(logger_sock_fd_global);
        return 1;
    }

    printf("[LoggerClient] Connected to server %s:%d.\n", server_ip, server_port);
    fflush(stdout);

    char join_logger_msg[BUFFER_SIZE];
    snprintf(join_logger_msg, BUFFER_SIZE -1, "%s\n", MSG_JOIN_LOGGER);
    join_logger_msg[BUFFER_SIZE-1] = '\0';
    if (send(logger_sock_fd_global, join_logger_msg, strlen(join_logger_msg), 0) < 0) {
        perror("[LoggerClient] Send JOIN_LOGGER failed");
        close(logger_sock_fd_global);
        return 1;
    }
    printf("[LoggerClient] Sent JOIN_LOGGER. Waiting for acceptance...\n");
    fflush(stdout);

    ssize_t n_read = recv(logger_sock_fd_global, buffer, BUFFER_SIZE - 1, 0);

    if (logger_client_shutdown_flag) {
        printf("[LoggerClient] Shutdown during JOIN_LOGGER response.\n");
        fflush(stdout);
        goto logger_cleanup_exit;
    }

    if (n_read <= 0) {
        if (n_read == 0) {
            printf("[LoggerClient] Server closed connection during JOIN_LOGGER response.\n");
        } else { 
            perror("[LoggerClient] Error receiving JOIN_LOGGER response");
        }
        fflush(stdout);
        close(logger_sock_fd_global);
        logger_sock_fd_global = -1;
        return 1;
    }
    buffer[n_read] = '\0';
    newline = strchr(buffer, '\n');
    if (newline) *newline = '\0';

    if (strncmp(buffer, RSP_LOGGER_ACCEPTED, strlen(RSP_LOGGER_ACCEPTED)) == 0) {
        printf("[LoggerClient] Successfully joined as logger.\n");
    } else if (strncmp(buffer, RSP_LOGGER_REJECTED, strlen(RSP_LOGGER_REJECTED)) == 0) {
        printf("[LoggerClient] Server rejected logger connection (e.g., max loggers reached or other issue).\n");
        fflush(stdout);
        goto logger_cleanup_exit;
    } else if (strncmp(buffer, MSG_SERVER_DOWN, strlen(MSG_SERVER_DOWN)) == 0) { 
        printf("[LoggerClient] Received SERVER_DOWN from server during JOIN. Exiting.\n");
        logger_client_shutdown_flag = 1; 
        fflush(stdout);
        goto logger_cleanup_exit;
    }
    else {
        printf("[LoggerClient] Unexpected response after JOIN_LOGGER: %s\n", buffer);
        fflush(stdout);
        goto logger_cleanup_exit;
    }
    fflush(stdout);


    printf("[LoggerClient] Listening for logs from server...\n");
    fflush(stdout);
    while (!logger_client_shutdown_flag) {
        n_read = recv(logger_sock_fd_global, buffer, BUFFER_SIZE - 1, 0);

        if (logger_client_shutdown_flag) {
            if (n_read < 0 && (errno == EINTR || errno == EBADF || errno == ECONNABORTED) ) {
                 printf("[LoggerClient] recv interrupted by shutdown or socket closed by server/self.\n");
            } else if (n_read > 0) {
                buffer[n_read] = '\0';
                printf("%s", buffer); 
                fflush(stdout);
            }
            break;
        }

        if (n_read <= 0) {
            if (n_read == 0) {
                printf("[LoggerClient] Server closed connection.\n");
            } else { 
                if (errno != EINTR) { 
                    perror("[LoggerClient] recv error");
                } else if (!logger_client_shutdown_flag) { 
                    perror("[LoggerClient] recv interrupted by non-shutdown signal");
                }
            }
            fflush(stdout);
            logger_client_shutdown_flag = 1; 
            break;
        }

        buffer[n_read] = '\0';
        printf("%s", buffer); 
        fflush(stdout); 

        if (strstr(buffer, MSG_SERVER_DOWN) != NULL) {
             char *line_start = buffer;
             char *next_line_ptr;
             do {
                 next_line_ptr = strchr(line_start, '\n');
                 if (next_line_ptr != NULL) {
                     *next_line_ptr = '\0'; 
                 }
                 if (strncmp(line_start, MSG_SERVER_DOWN, strlen(MSG_SERVER_DOWN)) == 0) {
                     if (strlen(line_start) == strlen(MSG_SERVER_DOWN)) {
                        printf("[LoggerClient] Detected SERVER_DOWN message in stream. Exiting.\n");
                        fflush(stdout);
                        logger_client_shutdown_flag = 1;
                        break; 
                     }
                 }
                 if (next_line_ptr != NULL) {
                     *next_line_ptr = '\n'; 
                     line_start = next_line_ptr + 1;
                 }
             } while (next_line_ptr != NULL && !logger_client_shutdown_flag && *line_start != '\0');
             
             if(logger_client_shutdown_flag) break; 
        }
    }

logger_cleanup_exit:
    printf("[LoggerClient] Shutting down.\n");
    fflush(stdout);
    if (logger_sock_fd_global != -1) {
        close(logger_sock_fd_global);
        logger_sock_fd_global = -1;
    }
    printf("[LoggerClient] Connection closed. Exiting.\n");
    fflush(stdout);
    return 0;
}