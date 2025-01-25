#include "ConfigParser.h"

#include <fstream>

std::unordered_map<std::string, std::string> ConfigParser::ParseConfigFile(const std::string &configFilePath) {
    std::unordered_map<std::string, std::string> configMap;
    std::ifstream cfgFile;
    cfgFile.open(configFilePath);

    if (!cfgFile.is_open()) {
        return configMap;
    }

    std::string delimiter = "=";
    std::string argLine;
    size_t delimiterPos = 0;
    while (std::getline(cfgFile, argLine)) {
        delimiterPos = argLine.find(delimiter);
        if (delimiterPos != std::string::npos) {
            std::string arg = argLine.substr(0, delimiterPos);
            std::string argValue = argLine.substr(delimiterPos + 1, std::string::npos);
            configMap[arg] = argValue;
        }
    }

    return configMap;
}
