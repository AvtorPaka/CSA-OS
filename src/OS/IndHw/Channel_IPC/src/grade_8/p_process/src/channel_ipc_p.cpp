#include <unistd.h>
#include <fcntl.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/stat.h>

#define PROCESSING_BUFFER_SIZE 128

int OpenFifoWithCheck(char pathToFile[], int flags, int mode = 0);

char *ConvertCountsToString(int vCount, int cCount, size_t *size);

int main(int argc, char *argv[]) {
    char processingSlidingBuffer[PROCESSING_BUFFER_SIZE];
    char fifoRw2PName[] = "../../fifo/rw2p.fifo";
    char fifoP2RwName[] = "../../fifo/p2rw.fifo";


    //---------------------------------------------------
    // Полученные данных из FIFO и обработка
    int fifoRw2PD = OpenFifoWithCheck(fifoRw2PName, O_RDONLY);

    char vowelChars[] = {'A', 'a', 'E', 'e', 'I', 'i', 'O', 'o', 'U', 'u'};
    char consonantChars[] = {'B', 'b', 'C', 'c', 'D', 'd', 'F', 'f', 'G', 'g', 'H', 'h', 'J', 'j', 'K', 'k', 'L',
                             'l', 'M', 'm', 'N', 'n', 'P', 'p', 'Q', 'q', 'R', 'r', 'S', 's', 'T', 't', 'V', 'v',
                             'W', 'w', 'X', 'x', 'Y', 'y', 'Z', 'z'};

    int consonantCount = 0;
    int vowelCount = 0;
    ssize_t readChannelBytes;
    while ((readChannelBytes = read(fifoRw2PD, processingSlidingBuffer, PROCESSING_BUFFER_SIZE)) > 0) {
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
        fprintf(stderr, "Processing process: Unable to read from rw2p FIFO.\n");
        exit(1);
    }

    if (close(fifoRw2PD) < 0) {
        fprintf(stderr, "Processing process: Unable to close rw2p FIFO\n");
        exit(1);
    }

    size_t writeSize;
    char *resultString = ConvertCountsToString(vowelCount, consonantCount, &writeSize);
    if (resultString == nullptr) {
        fprintf(stderr, "Processing process: Unable to make result string\n");
        exit(1);
    }

    //---------------------------------------------------
    // Запись полученных данных в FIFO
    (void)umask(0);
    mknod(fifoP2RwName, S_IFIFO | 0666, 0);

    int fifoP2RwD = OpenFifoWithCheck(fifoP2RwName, O_WRONLY);
    ssize_t actualWriteSize;
    actualWriteSize = write(fifoP2RwD, resultString, writeSize);

    if (actualWriteSize != writeSize) {
        fprintf(stderr, "Processing process: Unable to write all bytes to p2rw FIFO\n");
        exit(1);
    }

    if (close(fifoP2RwD) < 0) {
        fprintf(stderr, "Processing process: Unable to close p2rw FIFO\n");
        exit(1);
    }

    free(resultString);

    return 0;
}

char *ConvertCountsToString(int vCount, int cCount, size_t *size) {
    int needed = snprintf(nullptr, 0, "Vowel count: %d\nConsonant count: %d", vCount, cCount);
    if (needed < 0) {
        *size = 0;
        return nullptr;
    }

    *size = (size_t) needed;
    char *result = (char *) malloc(*size + 1);
    if (!result) {
        *size = 0;
        return nullptr;
    }

    snprintf(result, *size + 1, "Vowel count: %d\nConsonant count: %d", vCount, cCount);
    return result;
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