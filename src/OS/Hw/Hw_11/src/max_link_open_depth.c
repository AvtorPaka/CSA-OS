#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <limits.h>

#define BASE_FILENAME "initial"
#define LINK_PREFIX "link_"
#define TEMP_DIR_TEMPLATE "/tmp/symlink_depth_test"

char temp_dir_path[PATH_MAX];
int cleanup_registered = 0;

void cleanup_temp_dir(void) {
    if (!cleanup_registered || temp_dir_path[0] == '\0') {
        return;
    }

    char current_wd_before_cleanup[PATH_MAX];
    if (getcwd(current_wd_before_cleanup, sizeof(current_wd_before_cleanup)) == NULL) {
        perror("getcwd before cleanup");
    } else {
        if (strncmp(current_wd_before_cleanup, temp_dir_path, strlen(temp_dir_path)) == 0) {
            if (chdir("/") == -1) {
                perror("chdir to / for cleanup");
            }
        }
    }

    char command[PATH_MAX + 100];
    snprintf(command, sizeof(command), "rm -rf %s", temp_dir_path);

    if (system(command) == -1) {
        fprintf(stderr, "Error: Failed to remove temporary directory %s with system().\n", temp_dir_path);
        fprintf(stderr, "Please remove it manually.\n");
    } else {
        printf("Temporary directory %s removed.\n", temp_dir_path);
    }
    temp_dir_path[0] = '\0';
}


int main(void) {
    char original_cwd[PATH_MAX];
    if (getcwd(original_cwd, sizeof(original_cwd)) == NULL) {
        perror("Failed to get current working directory");
        return EXIT_FAILURE;
    }

    strncpy(temp_dir_path, TEMP_DIR_TEMPLATE, sizeof(temp_dir_path) -1);
    temp_dir_path[sizeof(temp_dir_path)-1] = '\0';

    if (mkdtemp(temp_dir_path) == NULL) {
        perror("Failed to create temporary directory with mkdtemp");
        return EXIT_FAILURE;
    }
    printf("Created temporary directory: %s\n", temp_dir_path);
    cleanup_registered = 1;
    atexit(cleanup_temp_dir);

    if (chdir(temp_dir_path) == -1) {
        perror("Failed to change to temporary directory");
        return EXIT_FAILURE;
    }
    printf("Changed working directory to: %s\n", temp_dir_path);

    int base_fd = open(BASE_FILENAME, O_CREAT | O_RDWR | O_TRUNC, 0644);
    if (base_fd == -1) {
        perror("Failed to create base file 'a'");
        if (chdir(original_cwd) == -1) {
            perror("Failed to chdir back to original_cwd during error handling");
        }
        return EXIT_FAILURE;
    }
    if (close(base_fd) == -1) {
        perror("Failed to close base file 'a'");
        if (chdir(original_cwd) == -1) {
            perror("Failed to chdir back to original_cwd during error handling");
        }
        return EXIT_FAILURE;
    }

    int depth = 0;
    char current_target_name[FILENAME_MAX];
    char new_link_name[FILENAME_MAX];

    strncpy(current_target_name, BASE_FILENAME, FILENAME_MAX -1);
    current_target_name[FILENAME_MAX-1] = '\0';

    while (1) {
        snprintf(new_link_name, sizeof(new_link_name), "%s%d", LINK_PREFIX, depth);

        if (symlink(current_target_name, new_link_name) == -1) {
            perror("symlink failed");
            fprintf(stderr, "Failed to create symbolic link '%s' -> '%s' at depth %d.\n",
                    new_link_name, current_target_name, depth);
            printf("Maximum symbolic link depth (symlink creation failure): %d\n", depth > 0 ? depth -1 : 0);
            break;
        }
         printf("Created link: %s -> %s\n", new_link_name, current_target_name);

        int link_fd = open(new_link_name, O_RDONLY);
        if (link_fd == -1) {
            if (errno == ELOOP) {
                printf("Reached maximum symbolic link depth.\n");
                printf("Failed to open '%s' \n", new_link_name);
                printf("Maximum recursion depth for symbolic links: %d\n", depth);
            } else {
                perror("open failed for a reason other than ELOOP");
                fprintf(stderr, "Failed to open link '%s' at depth %d.\n", new_link_name, depth);
                printf("Effective depth reached before non-ELOOP error: %d\n", depth);

            }

            unlink(new_link_name);
            break;
        }

        if (close(link_fd) == -1) {
            perror("Failed to close opened link");
            printf("Error closing link '%s' at depth %d. Previous successful depth: %d\n", new_link_name, depth, depth);
            unlink(new_link_name);
            break;
        }

        strncpy(current_target_name, new_link_name, FILENAME_MAX -1);
        current_target_name[FILENAME_MAX-1] = '\0';
        depth++;

        if (depth > 200) {
            printf("Safety break: Reached depth %d without ELOOP. Stopping.\n", depth);
            break;
        }
    }

    if (chdir(original_cwd) == -1) {
        perror("Failed to change back to original directory");
        return 1;
    }

    return 0;
}
