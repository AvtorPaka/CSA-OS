#include <unistd.h>
#include <fcntl.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#define BUFFER_SIZE 5000
#define PROCESSING_BUFFER_SIZE 128

int OpenFileWithCheck(char *pathToFile, int flags, int mode = 0);


char* ConvertCountsToString(int vCount, int cCount, size_t* size) ;

void PrintHelp() {
    printf("Use this approach to launch the program :\n"
           "./channel_ipc_{grade} -i <path_to_load> -o <path_to_upload>\n"
           "Arguments description :\n"
           "<path_to_load> - relative or absolute path to the file with ASCII-string.\n"
           "<path_to_upload> - relative or absolute path to the output file. If file doesn't exist it will be created\n");
}

void ReadCliArgs(int argc, char *argv[], char **loadP, char **uploadP);


int main(int argc, char* argv[]) {
    char *loadP= nullptr;
    char *uploadP= nullptr;

    ReadCliArgs(argc, argv, &loadP, &uploadP);

    if (strcmp(loadP, uploadP) == 0) {
        fprintf(stderr, "Load file path must be different from upload file path\n");
        exit(1);
    }

    int fd_read_process[2];
    if (pipe(fd_read_process) < 0) {
        fprintf(stderr, "Unable to open read-process pipe\n");
        exit(1);
    }

    pid_t ch_count_id = fork();
    if (ch_count_id < 0) {
        fprintf(stderr, "Reading process: Unable to fork processing child\n");
        exit(1);
    } else if (ch_count_id > 0) { // Процесс для чтения строки с файла
        char asciiBuffer[BUFFER_SIZE];
        int loadDesc = OpenFileWithCheck(loadP, O_RDONLY);

        if (close(fd_read_process[0]) < 0) {
            fprintf(stderr, "Reading process: Unable to close reading side of read-process pipe\n");
            exit(1);
        }

        ssize_t readBytes = -1;
        if ((readBytes = read(loadDesc, asciiBuffer, BUFFER_SIZE)) < 0) {
            fprintf(stderr, "Reading process: Unable to read from the file\n");
            close(loadDesc);
            exit(1);
        }
        close(loadDesc);

        ssize_t writeChannelBytes;
        writeChannelBytes = write(fd_read_process[1], asciiBuffer, readBytes);
        if (writeChannelBytes != readBytes) {
            fprintf(stderr, "Reading process: Unable to write all bytes to read-process pipe\n");
            exit(1);
        }

        if (close(fd_read_process[1]) < 0) {
            fprintf(stderr, "Reading process: Unable to close writing side of read-process pipe\n");
            exit(1);
        }

        waitpid(ch_count_id, nullptr, 0);
    } else {

        int fd_process_write[2];
        if (pipe(fd_process_write) < 0) {
            fprintf(stderr, "Processing process: Unable to open process-write pipe\n");
            exit(1);
        }

        pid_t ch_write_id = fork();

        if (ch_write_id < 0) {
            fprintf(stderr, "Processing process: Unable to fork writing child\n");
            exit(1);
        } else if (ch_write_id > 0) {   // Процесс для обработки ASCII строки
            char processingSlidingBuffer[PROCESSING_BUFFER_SIZE];

            if (close(fd_read_process[1]) < 0) {
                fprintf(stderr, "Processing process: Unable to close writing side of read-process pipe\n");
                exit(1);
            }
            if (close(fd_process_write[0]) < 0) {
                fprintf(stderr, "Processing process: Unable to close reading side of process-write pipe\n");
                exit(1);
            }

            char vowelChars[] = {'A', 'a', 'E', 'e', 'I', 'i', 'O', 'o', 'U', 'u'};
            char consonantChars[] = {'B', 'b', 'C', 'c', 'D', 'd', 'F', 'f', 'G', 'g', 'H', 'h', 'J', 'j', 'K', 'k', 'L', 'l', 'M', 'm', 'N', 'n', 'P', 'p', 'Q', 'q', 'R', 'r', 'S', 's', 'T', 't', 'V', 'v', 'W', 'w', 'X', 'x', 'Y', 'y', 'Z', 'z'};

            int consonantCount = 0;
            int vowelCount = 0;
            ssize_t readChannelBytes;
            while ((readChannelBytes = read(fd_read_process[0], processingSlidingBuffer, PROCESSING_BUFFER_SIZE)) > 0) {
                for (int i = 0; i < readChannelBytes; ++i) {
                    char curChar = processingSlidingBuffer[i];

                    for (int j = 0; j < 10; ++j) {
                        if (curChar == vowelChars[j]) {
                            vowelCount++;
                            continue;
                        }
                    }

                    for (int j = 0; j < 42; ++j) {
                        if (curChar == consonantChars[j]) {
                            consonantCount++;
                            continue;
                        }
                    }
                }
            }


            if (readChannelBytes < 0) {
                fprintf(stderr, "Processing process: Unable to read from read-process pipe.\n");
                exit(1);
            }

            if (close(fd_read_process[0]) < 0) {
                fprintf(stderr, "Processing process: Unable to close reading side of read-process pipe\n");
                exit(1);
            }

            size_t writeSize;
            char* resultString = ConvertCountsToString(vowelCount, consonantCount, &writeSize);
            if (resultString == nullptr) {
                fprintf(stderr, "Processing process: Unable to make result string\n");
                exit(1);
            }

            ssize_t actualWriteSize;
            actualWriteSize = write(fd_process_write[1], resultString, writeSize);

            if (actualWriteSize != writeSize) {
                fprintf(stderr, "Processing process: Unable to write all bytes to process-write pipe\n");
                exit(1);
            }

            if (close(fd_process_write[1]) < 0) {
                fprintf(stderr, "Processing process: Unable to close writing side of process-write pipe\n");
                exit(1);
            }

            free(resultString);
            waitpid(ch_write_id, nullptr, 0);

        } else  { // Процесс для записи полученных значений в файл
            char processedInfoBuffer[BUFFER_SIZE];

            if (close(fd_read_process[1]) < 0) {
                fprintf(stderr, "Writing process:: Unable to close writing side of read-process pipe\n");
                exit(1);
            }
            if (close(fd_read_process[0]) < 0) {
                fprintf(stderr, "Writing process:: Unable to close reading side of read-process pipe\n");
                exit(1);
            }

            if (close(fd_process_write[1]) < 0) {
                fprintf(stderr, "Writing process: Unable to close writing side of process-write pipe\n");
                exit(1);
            }

            ssize_t readChannelBytes;
            readChannelBytes = read(fd_process_write[0], processedInfoBuffer, BUFFER_SIZE);
            if (readChannelBytes < 0) {
                fprintf(stderr, "Reading: Unable to read from process-write pipe.\n");
                exit(1);
            }

            if (close(fd_process_write[0]) < 0) {
                fprintf(stderr, "Writing process: Unable to close reading side of process-write pipe\n");
                exit(1);
            }

            int uploadD = OpenFileWithCheck(uploadP, O_WRONLY | O_CREAT | O_TRUNC,
                                            S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

            ssize_t uploadBytes;
            uploadBytes = write(uploadD, processedInfoBuffer, readChannelBytes);

            if (uploadBytes != readChannelBytes) {
                fprintf(stderr, "Writing process: Unable to write all string to the file.\n");
                close(uploadD);
                exit(1);
            }

            close(uploadD);
        }

    }



    return 0;
}

char* ConvertCountsToString(int vCount, int cCount, size_t* size) {
    int needed = snprintf(nullptr, 0, "Vowel count: %d\nConsonant count: %d", vCount, cCount);
    if (needed < 0) {
        *size = 0;
        return nullptr;
    }

    *size = (size_t)needed;
    char* result = (char*)malloc(*size + 1);
    if (!result) {
        *size = 0;
        return nullptr;
    }

    snprintf(result, *size + 1, "Vowel count: %d\nConsonant count: %d", vCount, cCount);
    return result;
}



int OpenFileWithCheck(char *pathToFile, int flags, int mode) {
    int fileDescriptor = (mode == 0 ? open(pathToFile, flags) : open(pathToFile, flags, mode));
    if (fileDescriptor < 0) {
        fprintf(stderr, "Unable to open file '%s': ", pathToFile);
        perror("");
        exit(1);
    }

    return fileDescriptor;
}

void ReadCliArgs(int argc, char *argv[], char **loadP, char **uploadP) {
    int opt;
    while ((opt = getopt(argc, argv, "i:o:h")) != -1) {
        switch (opt) {
            case 'i':
                *loadP = optarg;
                break;
            case 'o':
                *uploadP = optarg;
                break;
            case 'h':
                PrintHelp();
                exit(0);
            default:
                fprintf(stderr, "Invalid CLI arguments\n");
                PrintHelp();
                exit(1);
        }
    }

    if (*loadP == nullptr || *uploadP == nullptr) {
        fprintf(stderr, "Both -i and -o options are required\n");
        PrintHelp();
        exit(1);
    }
}

