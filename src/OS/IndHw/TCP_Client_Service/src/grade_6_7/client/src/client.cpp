#include "common/common_sockets.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <csignal>

volatile sig_atomic_t client_shutdown_flag = 0;
int philosopher_id_global = -1;
int sock_fd_global = -1;
long eat_times_global = 0;
long think_times_global = 0;

void client_signal_handler(int sig) {
    if (sig == SIGINT || sig == SIGTERM) {
        if (!client_shutdown_flag) {
            print_log("Client", philosopher_id_global, "SIGINT/SIGTERM received. Initiating shutdown...");
            client_shutdown_flag = 1;
        }
    }
}

void send_message_to_client_socket(int sock_fd, const char* message) {
    char msg_buffer_send[BUFFER_SIZE];
    snprintf(msg_buffer_send, BUFFER_SIZE, "%s\n", message);
    if (send(sock_fd, msg_buffer_send, strlen(msg_buffer_send), MSG_NOSIGNAL) < 0) {
        if (errno != EPIPE) {
            perror("send_message_to_client_socket failed");
        }
    }
}

int main(int argc, char *argv[]) {
    struct sockaddr_in serv_addr{};
    char buffer[BUFFER_SIZE];
    char log_buffer[100];
    char acquire_msg[50];
    char release_msg[50];
    char leave_msg[50];
    ssize_t n_read;
    char* newline;

    if (argc != 3) {
        fprintf(stderr, "Usage: %s <server_ip> <server_port>\n", argv[0]);
        return 1;
    }
    const char *server_ip = argv[1];
    int server_port = atoi(argv[2]);

    struct sigaction sa{};
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = client_signal_handler;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    if ((sock_fd_global = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation error");
        return 1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(server_port);

    if (inet_pton(AF_INET, server_ip, &serv_addr.sin_addr) <= 0) {
        perror("Invalid address/ Address not supported");
        close(sock_fd_global);
        return 1;
    }

    if (connect(sock_fd_global, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        snprintf(log_buffer, sizeof(log_buffer), "Connection Failed to %s:%d", server_ip, server_port);
        perror(log_buffer);
        close(sock_fd_global);
        return 1;
    }

    print_log("Client", -1, "Connected to server.");

    send_message_to_client_socket(sock_fd_global, MSG_JOIN);
    n_read = recv(sock_fd_global, buffer, BUFFER_SIZE - 1, 0);

    if (client_shutdown_flag) { 
        goto client_cleanup_exit;
    }

    if (n_read <= 0) {
        if (client_shutdown_flag && n_read < 0 && (errno == EINTR || errno == ECONNABORTED)) {
            print_log("Client", -1, "JOIN recv interrupted by shutdown signal or server closed connection.");
        } else if (n_read == 0) {
            print_log("Client", -1, "Server closed connection during JOIN.");
        } else {
            snprintf(log_buffer, sizeof(log_buffer), "Error receiving JOIN response: %s", strerror(errno));
            print_log("Client", -1, log_buffer);
        }
        close(sock_fd_global);
        sock_fd_global = -1;
        return 1;
    }
    buffer[n_read] = '\0';
    newline = strchr(buffer, '\n');
    if (newline) *newline = '\0';


    if (strncmp(buffer, RSP_ID, strlen(RSP_ID)) == 0) {
        sscanf(buffer, RSP_ID " %d", &philosopher_id_global);
        snprintf(log_buffer, sizeof(log_buffer), "Joined table as Philosopher %d", philosopher_id_global);
        print_log("Client", philosopher_id_global, log_buffer);
    } else if (strncmp(buffer, RSP_FULL, strlen(RSP_FULL)) == 0) {
        print_log("Client", -1, "Table is full. Exiting.");
        goto client_cleanup_exit;
    } else if (strncmp(buffer, MSG_SERVER_DOWN, strlen(MSG_SERVER_DOWN)) == 0) {
        print_log("Client", philosopher_id_global, "Received SERVER_DOWN from server during JOIN. Exiting.");
        client_shutdown_flag = 1;
        goto client_cleanup_exit;
    }
    else {
        snprintf(log_buffer, sizeof(log_buffer), "Unexpected response after JOIN: %s", buffer);
        print_log("Client", -1, log_buffer);
        goto client_cleanup_exit;
    }


    while (!client_shutdown_flag) {
        // Размышления
        snprintf(log_buffer, sizeof(log_buffer), "Thinking...");
        print_log("Client", philosopher_id_global, log_buffer);
        random_sleep();
        think_times_global++;
        if (client_shutdown_flag) break;

        // Попытка поесть
        snprintf(log_buffer, sizeof(log_buffer), "Hungry, trying to acquire forks.");
        print_log("Client", philosopher_id_global, log_buffer);

        snprintf(acquire_msg, sizeof(acquire_msg), "%s %d", MSG_ACQUIRE, philosopher_id_global);
        send_message_to_client_socket(sock_fd_global, acquire_msg);

        n_read = recv(sock_fd_global, buffer, BUFFER_SIZE - 1, 0);
        if (client_shutdown_flag && n_read < 0 && errno == EINTR) { 
            break;
         }

        if (n_read <= 0) {
            if (client_shutdown_flag && n_read < 0 && (errno == ECONNABORTED || errno == EBADF)) {
                print_log("Client", philosopher_id_global, "Socket closed by server (likely shutdown) while waiting for GRANTED.");
            } else if (n_read == 0) {
                print_log("Client", philosopher_id_global, "Server disconnected while waiting for GRANTED.");
            } else {
                snprintf(log_buffer, sizeof(log_buffer), "Error waiting for GRANTED: %s", strerror(errno));
                print_log("Client", philosopher_id_global, log_buffer);
            }
            client_shutdown_flag = 1;
            break;
        }
        buffer[n_read] = '\0';
        newline = strchr(buffer, '\n'); if (newline) *newline = '\0';

        int got_forks_before_shutdown_check = 0;
        if (strncmp(buffer, RSP_GRANTED, strlen(RSP_GRANTED)) == 0) {
            snprintf(log_buffer, sizeof(log_buffer), "Got forks. Eating...");
            print_log("Client", philosopher_id_global, log_buffer);
            got_forks_before_shutdown_check = 1;
        } else if (strncmp(buffer, MSG_SERVER_DOWN, strlen(MSG_SERVER_DOWN)) == 0) {
            print_log("Client", philosopher_id_global, "Received SERVER_DOWN while waiting for GRANTED. Exiting.");
            client_shutdown_flag = 1;
            break;
        } else {
            snprintf(log_buffer, sizeof(log_buffer), "Error or unexpected response for GRANTED: %s", buffer);
            print_log("Client", philosopher_id_global, log_buffer);
            client_shutdown_flag = 1;
            break;
        }

        if (client_shutdown_flag) {
            if (got_forks_before_shutdown_check) {
                snprintf(log_buffer, sizeof(log_buffer), "Shutdown signal after getting forks, releasing them.");
                print_log("Client", philosopher_id_global, log_buffer);
                snprintf(release_msg, sizeof(release_msg), "%s %d", MSG_RELEASE, philosopher_id_global);
                send_message_to_client_socket(sock_fd_global, release_msg);
            }
            break;
        }

        // Процесс принятия пищи
        random_sleep();
        eat_times_global++;
        snprintf(log_buffer, sizeof(log_buffer), "Finished eating. Ate %ld times.", eat_times_global);
        print_log("Client", philosopher_id_global, log_buffer);
        if (client_shutdown_flag) {
            snprintf(log_buffer, sizeof(log_buffer), "Shutdown signal during/after eating, releasing forks.");
            print_log("Client", philosopher_id_global, log_buffer);
            snprintf(release_msg, sizeof(release_msg), "%s %d", MSG_RELEASE, philosopher_id_global);
            send_message_to_client_socket(sock_fd_global, release_msg);
            break;
        }

        // Освобождение вилок
        snprintf(log_buffer, sizeof(log_buffer), "Releasing forks.");
        print_log("Client", philosopher_id_global, log_buffer);

        snprintf(release_msg, sizeof(release_msg), "%s %d", MSG_RELEASE, philosopher_id_global);
        send_message_to_client_socket(sock_fd_global, release_msg);

        n_read = recv(sock_fd_global, buffer, BUFFER_SIZE - 1, 0);
        if (client_shutdown_flag && n_read < 0 && errno == EINTR) { break; }

        if (n_read <= 0) {
            if (client_shutdown_flag && n_read < 0 && (errno == ECONNABORTED || errno == EBADF)) {
                print_log("Client", philosopher_id_global, "Socket closed by server (likely shutdown) while waiting for OK.");
            } else if (n_read == 0) {
                print_log("Client", philosopher_id_global, "Server disconnected while waiting for OK.");
            } else {
                snprintf(log_buffer, sizeof(log_buffer), "Error waiting for OK: %s", strerror(errno));
                print_log("Client", philosopher_id_global, log_buffer);
            }
            client_shutdown_flag = 1;
            break;
        }
        buffer[n_read] = '\0';
        newline = strchr(buffer, '\n'); if (newline) *newline = '\0';

        if (strncmp(buffer, RSP_OK, strlen(RSP_OK)) == 0) {
        } else if (strncmp(buffer, MSG_SERVER_DOWN, strlen(MSG_SERVER_DOWN)) == 0) {
            print_log("Client", philosopher_id_global, "Received SERVER_DOWN while waiting for OK. Exiting.");
            client_shutdown_flag = 1;
            break;
        } else {
            snprintf(log_buffer, sizeof(log_buffer), "Error or unexpected response for OK: %s", buffer);
            print_log("Client", philosopher_id_global, log_buffer);
            client_shutdown_flag = 1;
            break;
        }
    }

    client_cleanup_exit:
    if (philosopher_id_global != -1) {
        snprintf(log_buffer, sizeof(log_buffer), "Leaving table. Ate %ld times. Thought %ld times.", eat_times_global, think_times_global);
        print_log("Client", philosopher_id_global, log_buffer);

        if (sock_fd_global != -1 && ! (client_shutdown_flag && strncmp(buffer, MSG_SERVER_DOWN, strlen(MSG_SERVER_DOWN))==0 && n_read > 0 ) ) {
            snprintf(leave_msg, sizeof(leave_msg), "%s %d", MSG_LEAVE, philosopher_id_global);
            send_message_to_client_socket(sock_fd_global, leave_msg);
        }
    } else {
        print_log("Client", -1, "Exiting before joining table fully or got SERVER_DOWN early.");
    }

    if (sock_fd_global != -1) {
        close(sock_fd_global);
        sock_fd_global = -1;
    }
    print_log("Client", philosopher_id_global == -1 ? -1 : philosopher_id_global, "Connection closed. Exiting.");
    return 0;
}