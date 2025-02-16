#include <iostream>
#include <cstdint>
#include <unistd.h>
#include <tuple>
#include <fcntl.h>

#define BUFFER_SIZE 128

int32_t OpenFileWithCheck(char *pathToFile, int flags, int mode = 0);

std::tuple<char *, char *> ReadCliArgs(int argc, char *argv[]);

void CopyFilesWithSlidingWindow(int32_t loadFileD, int32_t uploadFileD);

int main(int argc, char *argv[]) {
    std::tuple<char *, char *> pathsToFiles = ReadCliArgs(argc, argv);

    char* loadD = std::get<0>(pathsToFiles);
    char* uploadD = std::get<1>(pathsToFiles);

    if (strcmp(loadD, uploadD) == 0) {
        std::cout << "Destination file path must be different from copied file path\n";
        return 1;
    }

    int32_t loadFileDescriptor = OpenFileWithCheck(loadD, O_RDONLY);
    int32_t uploadFileDescriptor = OpenFileWithCheck(uploadD, O_WRONLY | O_CREAT | O_TRUNC,
                                                     S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

    CopyFilesWithSlidingWindow(loadFileDescriptor, uploadFileDescriptor);

    close(loadFileDescriptor);
    close(uploadFileDescriptor);
    return 0;
}

void CopyFilesWithSlidingWindow(int32_t loadFileD, int32_t uploadFileD) {
    ssize_t readBytes = -1;
    ssize_t writenBytes = -1;
    char buffer[BUFFER_SIZE];

    while ((readBytes = read(loadFileD, buffer, BUFFER_SIZE)) > 0) {
        writenBytes = write(uploadFileD, buffer, readBytes);
        if (writenBytes != readBytes) {
            std::cout << "Cant write to the file.\n";
            close(loadFileD);
            close(uploadFileD);
            exit(-1);
        }
    }

    if (readBytes < 0) {
        std::cout << "Cant read from the file.\n";
        close(loadFileD);
        close(uploadFileD);
        exit(-1);
    }
}

int32_t OpenFileWithCheck(char *pathToFile, int flags, int mode) {
    int32_t fileDescriptor = (mode == 0 ? open(pathToFile, flags) : open(pathToFile, flags, mode));
    if (fileDescriptor < 0) {
        std::cout << "Unable to open file for path: " << pathToFile << '\n';
        exit(-1);
    }

    return fileDescriptor;
}

std::tuple<char *, char *> ReadCliArgs(int argc, char *argv[]) {
    char *pathToLoadFile;
    char *pathToUploadFile;

    int32_t opt;
    while ((opt = getopt(argc, argv, "i:o:h")) != -1) {
        switch (opt) {
            case 'i':
                pathToLoadFile = optarg;
                break;
            case 'o':
                pathToUploadFile = optarg;
                break;
            case 'h':
                std::cout << "Use this approach to launch the program :\n"
                          << "./file_copy -i <path_to_load> -o <path_to_upload>\n"
                          << "Arguments description :\n"
                          << "<path_to_load> - relative or absolute path to the copied file.\n"
                          << "<path_to_upload> - relative or absolute path to the created copy of the file. If file doesnt exists it will be created\n";
                exit(0);
            default:
                std::cout << "Invalid CLI arguments\n" << std::endl;
                exit(1);
        }
    }

    return std::make_tuple(pathToLoadFile, pathToUploadFile);
}