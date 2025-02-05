#include <iostream>
#include <unistd.h>
#include <cstdint>
#include <stdexcept>
#include <filesystem>

uint64_t CalculateFactorial(int32_t factorialNum);

uint64_t CalculateFibonacci(int32_t fibNum);

void ListExecDirectoryContent();

void PrintProcessInfo(pid_t childPid);

int main(int argc, char *argv[]) {
    int32_t baseNum = -1;

    int32_t opt;
    while ((opt = getopt(argc, argv, "n:h")) != -1) {
        switch (opt) {
            case 'n':
                baseNum = std::stoi(optarg);
                break;
            case 'h':
                std::cout << "Use this approach to launch the program :\n"
                          << "./process_fork -n <base_number>\n"
                          << "Arguments description :\n"
                          << "<base_number> - number to calculate its factorial and fibonacci number";
                return 0;
            default:
                std::cout << "Invalid CLI arguments" << std::endl;
                return 1;
        }
    }

    if (baseNum < 0) {
        std::cout << "Invalid <base_number> argument value: " << baseNum << std::endl;
        return 1;
    }

    pid_t chpid = fork();

    if (chpid == -1) {
        std::cout << "Incorrect fork call" << std::endl;
        return 1;
    } else if (chpid == 0) {
        PrintProcessInfo(chpid);
        uint64_t factorialAns = 0;
        try {
            factorialAns = CalculateFactorial(baseNum);
            std::cout << "Factorial of " << baseNum << " : " << factorialAns << std::endl;
        }
        catch (const std::overflow_error &ex) {
            std::cout << "Overflow occurred during factorial calculation." << std::endl;
        }
        exit(0);
    } else {
        PrintProcessInfo(chpid);
        uint64_t fibAns = 0;
        try {
            fibAns = CalculateFibonacci(baseNum);
            std::cout << "Fibonacci " << baseNum << " number : " << fibAns << std::endl;
        }
        catch (const std::overflow_error &ex) {
            std::cout << "Overflow occurred during fibonacci calculation." << std::endl;
        }
    }


    chpid = fork();
    if (chpid == -1) {
        std::cout << "Incorrect fork call" << std::endl;
        return 1;
    } else if (chpid == 0) {
        PrintProcessInfo(chpid);
        ListExecDirectoryContent();
        exit(0);
    }

    return 0;
}

uint64_t CalculateFibonacci(int32_t fibNum) {
    uint64_t prevNum = 0;
    uint64_t curNum = 1;
    uint64_t fib = 0;

    uint64_t maxNum = UINT64_MAX;

    if (fibNum == 0) {
        return prevNum;
    } else if (fibNum == 1) {
        return curNum;
    }

    for (int32_t i = 1; i < fibNum; ++i) {
        if (maxNum - curNum < prevNum) {
            throw std::overflow_error("Overflow occurred.");
        }
        fib = curNum + prevNum;
        prevNum = curNum;
        curNum = fib;
    }

    return fib;
}

uint64_t CalculateFactorial(int32_t factorialNum) {
    uint64_t factorial = 1;
    uint64_t maxNum = UINT64_MAX;

    for (int32_t i = 1; i < factorialNum + 1; ++i) {
        if ((maxNum / factorial) < i) {
            throw std::overflow_error("Overflow occurred.");
        }
        factorial *= i;
    }

    return factorial;
}

void ListExecDirectoryContent() {
    std::filesystem::path execDir = std::filesystem::current_path().parent_path();

    std::cout << "Contents of the executable's directory (" << execDir << "):" << std::endl;
    for (const auto &entry: std::filesystem::directory_iterator(execDir)) {
        std::cout << entry.path().filename() << std::endl;
    }
}

void PrintProcessInfo(pid_t childPid) {
    pid_t pid = getpid();
    pid_t ppid = getppid();
    std::cout << "\nPID: " << pid << " . PPID: " << ppid << " . ChPID: " << childPid << std::endl;
}
