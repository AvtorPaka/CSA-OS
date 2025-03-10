#include <unistd.h>
#include <fcntl.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/stat.h>

#define BUFFER_SIZE 128

int OpenFileWithCheck(char *pathToFile, int flags, int mode = 0);

int OpenFifoWithCheck(char pathToFile[], int flags, int mode = 0);

void PrintHelp();

void ReadCliArgs(int argc, char *argv[], char **loadP, char **uploadP);


int main(int argc, char *argv[]) {
    char *loadP = nullptr;
    char *uploadP = nullptr;
    char fifoRw2PName[] = "../../fifo/rw2p.fifo";
    char fifoP2RwName[] = "../../fifo/p2rw.fifo";

    ReadCliArgs(argc, argv, &loadP, &uploadP);

    if (strcmp(loadP, uploadP) == 0) {
        fprintf(stderr, "Load file path must be different from upload file path\n");
        exit(1);
    }


    //---------------------------------------------------
    // Полученные данных из файла и запись в FIFO
    (void)umask(0);
    mknod(fifoRw2PName, S_IFIFO | 0666, 0);

    char asciiBuffer[BUFFER_SIZE];
    int loadDesc = OpenFileWithCheck(loadP, O_RDONLY);
    int fifoRw2PD = OpenFifoWithCheck(fifoRw2PName, O_WRONLY);

    ssize_t readBytes = -1;
    ssize_t writeChannelBytes;

    while ((readBytes = read(loadDesc, asciiBuffer, BUFFER_SIZE)) > 0) {
        writeChannelBytes = write(fifoRw2PD, asciiBuffer, readBytes);
        if (writeChannelBytes != readBytes) {
            fprintf(stderr, "R-w process: Unable to write all bytes to rw2p FIFO\n");
            exit(1);
        }
    }

    if (readBytes < 0) {
        fprintf(stderr, "R-w process: Unable to read from the file\n");
        close(loadDesc);
        exit(1);
    }

    close(loadDesc);
    
    if (close(fifoRw2PD) < 0) {
        fprintf(stderr, "R-w process: Unable to close rw2p FIFO\n");
        exit(1);
    }

    //---------------------------------------------------
    // Полученные данных через FIFO и запись в файл
    char processedInfoBuffer[BUFFER_SIZE];

    int fifoP2RwD = OpenFifoWithCheck(fifoP2RwName, O_RDONLY);

    ssize_t readChannelBytes;
    readChannelBytes = read(fifoP2RwD, processedInfoBuffer, BUFFER_SIZE);
    if (readChannelBytes < 0) {
        fprintf(stderr, "R-w: Unable to read from p2rw FIFO.");
        exit(1);
    }

    if (close(fifoP2RwD) < 0) {
        fprintf(stderr, "R-w process: Unable to close p2rw FIFO\n");
        exit(1);
    }

    int uploadD = OpenFileWithCheck(uploadP, O_WRONLY | O_CREAT | O_TRUNC,
                                    S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

    ssize_t uploadBytes;
    uploadBytes = write(uploadD, processedInfoBuffer, readChannelBytes);

    if (uploadBytes != readChannelBytes) {
        fprintf(stderr, "R-w process: Unable to write all string to the file.\n");
        close(uploadD);
        exit(1);
    }

    close(uploadD);


    return 0;
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

int OpenFifoWithCheck(char pathToFile[], int flags, int mode) {
    int fileDescriptor = (mode == 0 ? open(pathToFile, flags) : open(pathToFile, flags, mode));
    if (fileDescriptor < 0) {
        fprintf(stderr, "Unable to open fifo '%s': ", pathToFile);
        perror("");
        exit(1);
    }

    return fileDescriptor;
}

void PrintHelp() {
    printf("Use this approach to launch the program :\n"
           "./channel_ipc_{grade} -i <path_to_load> -o <path_to_upload>\n"
           "Arguments description :\n"
           "<path_to_load> - relative or absolute path to the file with ASCII-string.\n"
           "<path_to_upload> - relative or absolute path to the output file. If file doesn't exist it will be created\n");
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

