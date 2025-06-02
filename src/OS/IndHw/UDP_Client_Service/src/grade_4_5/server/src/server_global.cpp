#include "server_global.h"

const char *WAITER_SEM_NAME = "/philosopher_waiter_sem_";
const char *FORK_SEM_BASE_NAME = "/philosopher_fork_sem_";

philosopher_state_t philosophers[NUM_PHILOSOPHERS];
sem_t *forks_sem_ptr[NUM_PHILOSOPHERS];
sem_t *waiter_semaphore_ptr;
pthread_mutex_t server_mutex;

int active_philosophers_count = 0;
int server_udp_fd_global = -1;
volatile sig_atomic_t server_shutdown_flag = 0;

void init_server_state() {
    pthread_mutex_init(&server_mutex, nullptr);

    char sem_name_buffer[256];
    print_log("Server", -1, "Unlinking pre-existing semaphores (if any)...");
    sem_unlink(WAITER_SEM_NAME);
    for (int i = 0; i < NUM_PHILOSOPHERS; ++i) {
        snprintf(sem_name_buffer, sizeof(sem_name_buffer), "%s%d", FORK_SEM_BASE_NAME, i);
        sem_unlink(sem_name_buffer);
    }
    print_log("Server", -1, "Pre-unlinking finished.");

    waiter_semaphore_ptr = sem_open(WAITER_SEM_NAME, O_CREAT | O_EXCL, 0666, NUM_PHILOSOPHERS - 1);
    if (waiter_semaphore_ptr == SEM_FAILED) {
        if (errno == EEXIST) {
            print_log("Server", -1, "Waiter semaphore already exists, attempting to open.");
            waiter_semaphore_ptr = sem_open(WAITER_SEM_NAME, 0);
            if (waiter_semaphore_ptr == SEM_FAILED) {
                perror("sem_open (existing) for waiter_semaphore failed");
                exit(EXIT_FAILURE);
            }
        } else {
            perror("sem_open (O_CREAT | O_EXCL) for waiter_semaphore failed");
            exit(EXIT_FAILURE);
        }
    }
    snprintf(sem_name_buffer, sizeof(sem_name_buffer), "Waiter semaphore %s opened/created.", WAITER_SEM_NAME);
    print_log("Server", -1, sem_name_buffer);


    for (int i = 0; i < NUM_PHILOSOPHERS; ++i) {
        snprintf(sem_name_buffer, sizeof(sem_name_buffer), "%s%d", FORK_SEM_BASE_NAME, i);
        forks_sem_ptr[i] = sem_open(sem_name_buffer, O_CREAT | O_EXCL, 0666, 1); // Binary semaphore
        if (forks_sem_ptr[i] == SEM_FAILED) {
            if (errno == EEXIST) {
                char log_buf[300];
                snprintf(log_buf, sizeof(log_buf), "Fork semaphore %s already exists, attempting to open.", sem_name_buffer);
                print_log("Server", -1, log_buf);
                forks_sem_ptr[i] = sem_open(sem_name_buffer, 0);
                if (forks_sem_ptr[i] == SEM_FAILED) {
                    char err_buf[300];
                    snprintf(err_buf, sizeof(err_buf), "sem_open (existing) for fork %s failed", sem_name_buffer);
                    perror(err_buf);
                    // Cleanup already opened semaphores before exiting
                    for(int k=0; k<i; ++k) if(forks_sem_ptr[k] != SEM_FAILED) sem_close(forks_sem_ptr[k]);
                    if(waiter_semaphore_ptr != SEM_FAILED) sem_close(waiter_semaphore_ptr);
                    exit(EXIT_FAILURE);
                }
            } else {
                char err_buf[300];
                snprintf(err_buf, sizeof(err_buf), "sem_open (O_CREAT | O_EXCL) for fork_semaphore %s failed", sem_name_buffer);
                perror(err_buf);
                // Cleanup
                for(int k=0; k<i; ++k) if(forks_sem_ptr[k] != SEM_FAILED) sem_close(forks_sem_ptr[k]);
                if(waiter_semaphore_ptr != SEM_FAILED) sem_close(waiter_semaphore_ptr);
                exit(EXIT_FAILURE);
            }
        }
        char log_msg[300];
        snprintf(log_msg, sizeof(log_msg), "Fork semaphore %s opened/created.", sem_name_buffer);
        print_log("Server", -1, log_msg);
    }

    for (int i = 0; i < NUM_PHILOSOPHERS; ++i) {
        philosophers[i].id = i + 1;
        memset(&philosophers[i].client_addr, 0, sizeof(philosophers[i].client_addr));
        philosophers[i].client_addr_len = sizeof(philosophers[i].client_addr);
        philosophers[i].comm_pipe_fd[0] = -1;
        philosophers[i].comm_pipe_fd[1] = -1;
        philosophers[i].thread_id = 0;
        philosophers[i].active = 0;
        philosophers[i].has_waiter_permission = 0;
        philosophers[i].has_left_fork = 0;
        philosophers[i].has_right_fork = 0;
        philosophers[i].left_fork_idx = -1;
        philosophers[i].right_fork_idx = -1;
    }
    active_philosophers_count = 0;
}

void cleanup_all_named_semaphores() {
    char sem_name_buffer[256];
    print_log("Server", -1, "Attempting to clean up named semaphores...");

    if (waiter_semaphore_ptr != SEM_FAILED && waiter_semaphore_ptr != nullptr) {
        sem_close(waiter_semaphore_ptr);
        waiter_semaphore_ptr = nullptr;
    }
    
    if (sem_unlink(WAITER_SEM_NAME) == 0) {
        snprintf(sem_name_buffer, sizeof(sem_name_buffer), "Semaphore %s unlinked.", WAITER_SEM_NAME);
        print_log("Server", -1, sem_name_buffer);
    } else {
        if (errno != ENOENT) {
            snprintf(sem_name_buffer, sizeof(sem_name_buffer), "Warning: Failed to unlink semaphore %s: %s", WAITER_SEM_NAME, strerror(errno));
            print_log("Server", -1, sem_name_buffer);
        }
    }

    for (int i = 0; i < NUM_PHILOSOPHERS; ++i) {
        if (forks_sem_ptr[i] != SEM_FAILED && forks_sem_ptr[i] != nullptr) {
            sem_close(forks_sem_ptr[i]);
            forks_sem_ptr[i] = nullptr;
        }
        snprintf(sem_name_buffer, sizeof(sem_name_buffer), "%s%d", FORK_SEM_BASE_NAME, i);
        if(sem_unlink(sem_name_buffer) == 0) {
            char log_buf_temp[300];
            snprintf(log_buf_temp, sizeof(log_buf_temp), "Semaphore %s unlinked.", sem_name_buffer);
            print_log("Server", -1, log_buf_temp);
        } else {
            if (errno != ENOENT) {
                char log_buf_temp[300];
                snprintf(log_buf_temp, sizeof(log_buf_temp), "Warning: Failed to unlink semaphore %s: %s", sem_name_buffer, strerror(errno));
                print_log("Server", -1, log_buf_temp);
            }
        }
    }
    print_log("Server", -1, "Named semaphores cleanup finished.");
}


void cleanup_philosopher_semaphores_if_held(philosopher_state_t* p_state) {
    char log_buffer[120];
    int p_id = p_state->id;

    if (p_state->has_right_fork) {
        sem_post(forks_sem_ptr[p_state->right_fork_idx]);
        p_state->has_right_fork = 0;
        char sem_name[64];
        snprintf(sem_name, sizeof(sem_name), "%s%d", FORK_SEM_BASE_NAME, p_state->right_fork_idx);
        snprintf(log_buffer, sizeof(log_buffer), "P%d (thread) released right fork %s.", p_id, sem_name);
        print_log("Server", -1, log_buffer);
    }
    if (p_state->has_left_fork) {
        sem_post(forks_sem_ptr[p_state->left_fork_idx]);
        p_state->has_left_fork = 0;
        char sem_name[64];
        snprintf(sem_name, sizeof(sem_name), "%s%d", FORK_SEM_BASE_NAME, p_state->left_fork_idx);
        snprintf(log_buffer, sizeof(log_buffer), "P%d (thread) released left fork %s.", p_id, sem_name);
        print_log("Server", -1, log_buffer);
    }
    if (p_state->has_waiter_permission) {
        sem_post(waiter_semaphore_ptr);
        p_state->has_waiter_permission = 0;
        snprintf(log_buffer, sizeof(log_buffer), "P%d (thread) released waiter permission (%s).", p_id, WAITER_SEM_NAME);
        print_log("Server", -1, log_buffer);
    }

    snprintf(log_buffer, sizeof(log_buffer), "P%d (thread) ensured its semaphores are released.", p_id);
    print_log("Server", -1, log_buffer);
}


void server_signal_handler(int sig) {
    if (sig == SIGINT || sig == SIGTERM) {
        print_log("Server", -1, "Shutdown signal received. Setting flag...");
        server_shutdown_flag = 1;
        
        if (server_udp_fd_global != -1) {
            int fd_to_close = server_udp_fd_global;
            server_udp_fd_global = -1;
            close(fd_to_close); 
            print_log("Server", -1, "Main UDP socket closed by signal handler.");
        }
    }
}