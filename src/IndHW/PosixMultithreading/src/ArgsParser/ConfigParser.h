#ifndef POSIXMULTITHREADING_CONFIGPARSER_H
#define POSIXMULTITHREADING_CONFIGPARSER_H

#include <unordered_map>
#include <string>

// Класс для парсинга аргументов из конфигурационного файла
class ConfigParser {
public:
    // Функция парсинга данных из конфигурационного файла
    static std::unordered_map<std::string, std::string> ParseConfigFile(const std::string& configFilePath);
};


#endif //POSIXMULTITHREADING_CONFIGPARSER_H
