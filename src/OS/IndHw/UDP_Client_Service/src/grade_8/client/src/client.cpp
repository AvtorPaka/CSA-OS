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

volatile sig_atomic_t client_shutdown_flag = 0;
int philosopher_id_global = -1;
int sock_fd_global = -1;
long eat_times_global = 0;
long think_times_global = 0;
int has_forks_currently = 0; 


void client_signal_handler(int sig) {
    if (sig == SIGINT || sig == SIGTERM) {
        if (!client_shutdown_flag) {
            const char* Tsource = "Client";
            if (philosopher_id_global != -1) {
                 print_log(Tsource, philosopher_id_global, "SIGINT/SIGTERM received. Initiating shutdown...");
            } else {
                 print_log(Tsource, -1, "SIGINT/SIGTERM received. Initiating shutdown...");
            }
            client_shutdown_flag = 1;
        }
    }
}

int send_message_to_server(int sock_fd, const char* message) {
    if (send(sock_fd, message, strlen(message), 0) < 0) {
        char error_log[150];
        snprintf(error_log, sizeof(error_log), "send() failed for message '%s': %s", message, strerror(errno));
        print_log("Client", philosopher_id_global, error_log);
        return -1;
    }
    return 0;
}

ssize_t receive_message_from_server(int sock_fd, char* buffer, size_t buffer_len) {
    ssize_t n_read = recv(sock_fd, buffer, buffer_len - 1, 0);
    if (n_read < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            print_log("Client", philosopher_id_global, "recv() timed out waiting for server response.");
            return -2; 
        } else if (errno == EINTR && client_shutdown_flag) {
            print_log("Client", philosopher_id_global, "recv() interrupted by shutdown signal.");
            return -3; 
        }
        char error_log[100];
        snprintf(error_log, sizeof(error_log), "recv() error: %s", strerror(errno));
        print_log("Client", philosopher_id_global, error_log);
    } else if (n_read > 0) {
        buffer[n_read] = '\0';
    }
    return n_read;
}


int main(int argc, char *argv[]) {
    struct sockaddr_in serv_addr{};
    char buffer[BUFFER_SIZE];
    char log_buffer[150];
    char command_msg[60];
    ssize_t n_read;

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

    if ((sock_fd_global = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
        perror("UDP Socket creation error");
        return 1;
    }

    struct timeval tv;
    tv.tv_sec = 5;
    tv.tv_usec = 0;
    if (setsockopt(sock_fd_global, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv) < 0) {
        perror("setsockopt SO_RCVTIMEO failed");
        close(sock_fd_global);
        return 1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(server_port);

    if (inet_pton(AF_INET, server_ip, &serv_addr.sin_addr) <= 0) {
        perror("Invalid server address/ Address not supported");
        close(sock_fd_global);
        return 1;
    }

    if (connect(sock_fd_global, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        snprintf(log_buffer, sizeof(log_buffer), "UDP connect() (setting default peer) Failed to %s:%d: %s", server_ip, server_port, strerror(errno));
        print_log("Client", -1, log_buffer);
        close(sock_fd_global);
        return 1;
    }
    print_log("Client", -1, "UDP socket configured to send to server.");

    if (send_message_to_server(sock_fd_global, MSG_JOIN) != 0) {
        print_log("Client", -1, "Failed to send JOIN. Exiting.");
        goto client_cleanup_exit;
    }
    print_log("Client", -1, "Sent JOIN, waiting for ID or FULL...");

    n_read = receive_message_from_server(sock_fd_global, buffer, BUFFER_SIZE);

    if (client_shutdown_flag) {
        goto client_cleanup_exit;
    }

    if (n_read <= 0) { 
        if (n_read == -2) {
             print_log("Client", -1, "Timeout waiting for JOIN response. Server might be down or not responding.");
        } else if (n_read == -3) {
             print_log("Client", -1, "JOIN recv interrupted by shutdown signal.");
        } else if (n_read == 0) {
            print_log("Client", -1, "Received empty datagram after JOIN (unexpected).");
        } else {
            snprintf(log_buffer, sizeof(log_buffer), "Error receiving JOIN response (n_read=%zd, errno=%d).", (long)n_read, errno);
            print_log("Client", -1, log_buffer);
        }
        goto client_cleanup_exit;
    }

    if (strncmp(buffer, RSP_ID, strlen(RSP_ID)) == 0) {
        if (sscanf(buffer, RSP_ID " %d", &philosopher_id_global) == 1) {
            snprintf(log_buffer, sizeof(log_buffer), "Joined table as Philosopher %d", philosopher_id_global);
            print_log("Client", philosopher_id_global, log_buffer);
        } else {
            snprintf(log_buffer, sizeof(log_buffer), "Malformed RSP_ID response: '%s'", buffer);
            print_log("Client", -1, log_buffer);
            goto client_cleanup_exit;
        }
    } else if (strncmp(buffer, RSP_FULL, strlen(RSP_FULL)) == 0) {
        print_log("Client", -1, "Table is full. Exiting.");
        goto client_cleanup_exit;
    } else if (strncmp(buffer, MSG_SERVER_DOWN, strlen(MSG_SERVER_DOWN)) == 0) {
        print_log("Client", philosopher_id_global, "Received SERVER_DOWN from server during JOIN. Exiting.");
        client_shutdown_flag = 1;
        goto client_cleanup_exit;
    } else {
        snprintf(log_buffer, sizeof(log_buffer), "Unexpected response after JOIN: '%s'", buffer);
        print_log("Client", -1, log_buffer);
        goto client_cleanup_exit;
    }


    while (!client_shutdown_flag) {
        snprintf(log_buffer, sizeof(log_buffer), "Thinking...");
        print_log("Client", philosopher_id_global, log_buffer);
        random_sleep();
        think_times_global++;
        if (client_shutdown_flag) break;

        snprintf(log_buffer, sizeof(log_buffer), "Hungry, trying to acquire forks.");
        print_log("Client", philosopher_id_global, log_buffer);

        snprintf(command_msg, sizeof(command_msg), "%s %d", MSG_ACQUIRE, philosopher_id_global);
        if (send_message_to_server(sock_fd_global, command_msg) != 0) {
            client_shutdown_flag = 1;
            break;
        }

        n_read = receive_message_from_server(sock_fd_global, buffer, BUFFER_SIZE);

        if (client_shutdown_flag && (n_read == -3 || (n_read < 0 && errno == EINTR))) {
             print_log("Client", philosopher_id_global, "ACQUIRE recv interrupted by shutdown.");
             break;
        }

        if (n_read <= 0) {
            if (n_read == -2) {
                print_log("Client", philosopher_id_global, "Timeout waiting for GRANTED. Server might be down or busy.");
            } else {
                 snprintf(log_buffer, sizeof(log_buffer), "Error or no response for GRANTED (n_read=%zd, errno=%d).", (long)n_read, errno);
                 print_log("Client", philosopher_id_global, log_buffer);
            }
            client_shutdown_flag = 1;
            break;
        }

        if (strncmp(buffer, RSP_GRANTED, strlen(RSP_GRANTED)) == 0) {
            snprintf(log_buffer, sizeof(log_buffer), "Got forks. Eating...");
            print_log("Client", philosopher_id_global, log_buffer);
            has_forks_currently = 1;
        } else if (strncmp(buffer, MSG_SERVER_DOWN, strlen(MSG_SERVER_DOWN)) == 0) {
            print_log("Client", philosopher_id_global, "Received SERVER_DOWN while waiting for GRANTED. Exiting.");
            client_shutdown_flag = 1;
            break;
        } else {
            snprintf(log_buffer, sizeof(log_buffer), "Error or unexpected response for GRANTED: '%s'", buffer);
            print_log("Client", philosopher_id_global, log_buffer);
            client_shutdown_flag = 1;
            break;
        }

        if (client_shutdown_flag) {
            if (has_forks_currently) {
                snprintf(log_buffer, sizeof(log_buffer), "Shutdown signal after getting forks, releasing them before eating.");
                print_log("Client", philosopher_id_global, log_buffer);
                snprintf(command_msg, sizeof(command_msg), "%s %d", MSG_RELEASE, philosopher_id_global);
                send_message_to_server(sock_fd_global, command_msg);
                has_forks_currently = 0;
            }
            break;
        }

        random_sleep();
        eat_times_global++;
        snprintf(log_buffer, sizeof(log_buffer), "Finished eating. Ate %ld times.", eat_times_global);
        print_log("Client", philosopher_id_global, log_buffer);

        if (client_shutdown_flag) {
            if (has_forks_currently) {
                snprintf(log_buffer, sizeof(log_buffer), "Shutdown signal during/after eating, releasing forks.");
                print_log("Client", philosopher_id_global, log_buffer);
                snprintf(command_msg, sizeof(command_msg), "%s %d", MSG_RELEASE, philosopher_id_global);
                send_message_to_server(sock_fd_global, command_msg);
                has_forks_currently = 0;
            }
            break;
        }

        snprintf(log_buffer, sizeof(log_buffer), "Releasing forks.");
        print_log("Client", philosopher_id_global, log_buffer);

        snprintf(command_msg, sizeof(command_msg), "%s %d", MSG_RELEASE, philosopher_id_global);
        if (send_message_to_server(sock_fd_global, command_msg) != 0) {
            client_shutdown_flag = 1;
            has_forks_currently = 0;
            break;
        }
        has_forks_currently = 0;

        n_read = receive_message_from_server(sock_fd_global, buffer, BUFFER_SIZE);
        if (client_shutdown_flag && (n_read == -3 || (n_read < 0 && errno == EINTR))) { break; }

        if (n_read <= 0) {
            if (n_read == -2) {
                print_log("Client", philosopher_id_global, "Timeout waiting for OK after RELEASE. Assuming forks released by server.");
            } else {
                snprintf(log_buffer, sizeof(log_buffer), "Error or no response for OK (n_read=%zd, errno=%d).", (long)n_read, errno);
                print_log("Client", philosopher_id_global, log_buffer);
            }
            client_shutdown_flag = 1;
            break;
        }

        if (strncmp(buffer, RSP_OK, strlen(RSP_OK)) == 0) {
            print_log("Client", philosopher_id_global, "Forks released, server confirmed with OK.");
        } else if (strncmp(buffer, MSG_SERVER_DOWN, strlen(MSG_SERVER_DOWN)) == 0) {
            print_log("Client", philosopher_id_global, "Received SERVER_DOWN while waiting for OK. Exiting.");
            client_shutdown_flag = 1;
            break;
        } else {
            snprintf(log_buffer, sizeof(log_buffer), "Error or unexpected response for OK: '%s'", buffer);
            print_log("Client", philosopher_id_global, log_buffer);
            client_shutdown_flag = 1;
            break;
        }
    }

client_cleanup_exit:
    if (philosopher_id_global != -1 && client_shutdown_flag) {
        if (has_forks_currently) {
            snprintf(log_buffer, sizeof(log_buffer), "Shutdown initiated while holding forks. Sending final RELEASE.");
            print_log("Client", philosopher_id_global, log_buffer);
            snprintf(command_msg, sizeof(command_msg), "%s %d", MSG_RELEASE, philosopher_id_global);
            send_message_to_server(sock_fd_global, command_msg);
            has_forks_currently = 0;
        }
    }

    int should_send_leave = 0;
    if (philosopher_id_global != -1) {
        if (client_shutdown_flag) {
            if (!(n_read > 0 && strncmp(buffer, MSG_SERVER_DOWN, strlen(MSG_SERVER_DOWN)) == 0)) {
                should_send_leave = 1;
                print_log("Client", philosopher_id_global, "Shutdown initiated. Sending LEAVE to server.");
            } else {
                print_log("Client", philosopher_id_global, "Not sending LEAVE as SERVER_DOWN was received.");
            }
        } else { 
            should_send_leave = 1; 
            print_log("Client", philosopher_id_global, "Loop finished. Sending LEAVE to server.");
        }

        if (should_send_leave) {
            snprintf(command_msg, sizeof(command_msg), "%s %d", MSG_LEAVE, philosopher_id_global);
            send_message_to_server(sock_fd_global, command_msg); 
        }
    }


    snprintf(log_buffer, sizeof(log_buffer), "Exiting. Ate %ld times. Thought %ld times.", eat_times_global, think_times_global);
    print_log("Client", philosopher_id_global == -1 ? -1 : philosopher_id_global, log_buffer);

    if (sock_fd_global != -1) {
        close(sock_fd_global);
        sock_fd_global = -1;
    }
    print_log("Client", philosopher_id_global == -1 ? -1 : philosopher_id_global, "Socket closed. Shutdown complete.");
    return 0;
}