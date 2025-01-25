#include <iostream>
#include <vector>
#include <cstdint>
#include <iomanip>

void CheckAssemblyAutoTestResults() {
    std::vector<double> xValueArray{1.0, -2.0, 54.124, 11.78, 2.5, 1786.444, -57.22};
    std::vector<uint32_t> mValueArray{14, 19, 3, 5, 9, 2, 3};

    std::cout << std::fixed << std::setprecision(10);
    for (int32_t i = 0; i < xValueArray.size(); ++i) {
        std::cout << "Current x value: " << xValueArray[i] << '\n' << "Current m value: " << mValueArray[i] << "\n";
        std::cout << pow((1.0 + xValueArray[i]), mValueArray[i]) << '\n' << std::string(30, '-') << "\n";
    }
}