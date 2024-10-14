#include <iostream>
#include <fstream>
#include <random>

class WrongTestResultException : public std::exception {
public:
    [[nodiscard]] const char *what() const noexcept override {
        return "Wrong test result";
    }
};

class IntegerDivisionAssemblyTester {
private:
    const char *_pathToBoostrap;
    std::string _pathToBuffer;

    void RunTest(int32_t dividend, int32_t divisor) const {
        //Arrange
        UploadDataToBuffer(dividend, divisor);
        std::cout << std::string(30, '-') << '\n' << "Dividend: " << dividend << "\tDivisor: " << divisor << std::endl;

        //Act
        //!!!Be sure to "chmod +x boostrap_test.sh"
        std::system(_pathToBoostrap);

        //Assert
        int32_t assembly_quotient = 0;
        int32_t assembly_remainder = 0;
        LoadDataFromBuffer(assembly_quotient, assembly_remainder);

        int32_t expected_quotient = dividend / divisor;
        int32_t expected_remainder = dividend % divisor;

        std::cout << "С++ quotient: " << expected_quotient << "\tAssembly quotient: " << assembly_quotient
                  << "\nС++ remainder: " << expected_remainder << "\tAssembly remainder: " << assembly_remainder <<
                  '\n' << std::endl;

        if (assembly_quotient != expected_quotient || assembly_remainder != expected_remainder) {
            throw WrongTestResultException();
        } else {
            std::cout << "Ok" << std::endl;
        }
    }

    void UploadDataToBuffer(int32_t random_dividend, int32_t random_divisor) const {
        std::ofstream input_buffer_stream(_pathToBuffer, std::ios::out | std::ios::binary | std::ios::trunc);

        if (!input_buffer_stream.is_open()) {
            throw std::invalid_argument("Couldn't open file.");
        }

        input_buffer_stream.write(reinterpret_cast<const char *>(&random_dividend), sizeof(int32_t));
        input_buffer_stream.write(reinterpret_cast<const char *>(&random_divisor), sizeof(int32_t));

        input_buffer_stream.close();
    };

    void LoadDataFromBuffer(int32_t &assembly_quotient, int32_t &assembly_remainder) const {
        std::ifstream output_buffer_stream(_pathToBuffer, std::ios::out | std::ios::binary);

        if (!output_buffer_stream.is_open()) {
            throw std::invalid_argument("Couldn't open file.");
        }

        output_buffer_stream.read(reinterpret_cast<char *>(&assembly_quotient), sizeof(int32_t));
        output_buffer_stream.read(reinterpret_cast<char *>(&assembly_remainder), sizeof(int32_t));

        output_buffer_stream.close();
    }

public:

    IntegerDivisionAssemblyTester(const char *boostrap_path, std::string &buffer_path) {
        _pathToBoostrap = boostrap_path;
        _pathToBuffer = buffer_path;
    }

    void Run(int32_t numberOfTests) const {
        std::random_device rand;
        std::mt19937 gen(rand());
        for (uint32_t i = 0; i < numberOfTests; ++i) {
            std::uniform_int_distribution<int32_t> dist_dividend(std::numeric_limits<int32_t>::min(),
                                                                 std::numeric_limits<int32_t>::max());
            int32_t random_dividend = dist_dividend(gen);

            //Generating random int32_t values the way that divisor <= |dividend|
            std::uniform_int_distribution<int32_t> dist_divisor(-std::abs(random_dividend), std::abs(random_dividend));
            int32_t random_divisor = dist_divisor(gen);

            RunTest(random_dividend, random_divisor);
        }
        std::cout << std::string(30, '-') << std::endl;
    }

};


int main() {
    const char *boostrap_path = "{YOUR_PATH}/boostrap_test.sh";
    std::string buffer_path = "{YOUR_PATH}/buffer.bin";
    auto tester = IntegerDivisionAssemblyTester(boostrap_path, buffer_path);
    tester.Run(20);
    return 0;
}
