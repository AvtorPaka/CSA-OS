#include <pthread.h>
#include <cstdint>
#include <unistd.h>
#include <iostream>
#include "Entities/HotelGuest.h"
#include "Entities/GenderHotel.h"
#include "ArgsParser/ConfigParser.h"

// Общий разделяемый потоками-посетителями инстанс отеля
GenderHotel hotel;

// Потоковая функция посетителей
void *ComeToTheHotel(void *arg);

int main(int argc, char *argv[]) {
    // Ввод параметров запуска программы через аргументы командой строки или конфигурационного файла
    uint32_t guestNumber;
    useconds_t singleRoomStayTime;
    useconds_t doubleRoomStayTime;
    useconds_t newGuestArrivalTime;
    std::string logFilePath;
    std::string configFilePath;
    bool configFileUsed = false;

    // Проверка введенных аргументов командой строки на соотвествующие ключи
    int opt;
    std::string helpDelimiter = "\n" + std::string(55, '-') + "\n";
    while ((opt = getopt(argc, argv, "f:h")) != -1) {
        switch (opt) {
            // Ключ для ввода пути к конфигурационному файлу
            case 'f':
                configFileUsed = true;
                configFilePath = optarg;
                break;
            // Ключ для вывода информационной помощи в запуске программы
            case 'h':
                std::cout << helpDelimiter
                          << "Use one of this two approaches to launch the program"
                          << helpDelimiter
                          << "./gender_hotel <guests_number> <single_room_stay_time> <double_room_stay_time> <new_guest_arrival_time> <log_file_path>"
                          << "\n./gender_hotel -c <config_file_path>"
                          << helpDelimiter
                          << "Arguments description :"
                          << helpDelimiter
                          << "<guests_number> - number of guests (threads) who will try to get room in a hotel\n"
                          << "<single_room_stay_time> - time of stay of a guest in a single hotel room in microseconds (>= 300000 or 300000 elsewhere) \n"
                          << "<double_room_stay_time> - time of stay of a guest in a double hotel room in microseconds (>= 300000 or 300000 elsewhere)\n"
                          << "<new_guest_arrival_time> - delay between each new guest (thread) attempt to get a hotel room in microseconds (>= 20000 or 20000 elsewhere)\n"
                          << "<log_file_path> - path to the file to write logs to."
                          << helpDelimiter
                          << "Config file structure :"
                          << helpDelimiter
                          << "guestNumber=<guests_number>\n"
                          << "singleRoomStayTime=<single_room_stay_time>\n"
                          << "doubleRoomStayTime=<double_room_stay_time>\n"
                          << "newGuestArrivalTime=<new_guest_arrival_time>\n"
                          << "logFilePath=<log_file_path>"
                          << std::endl;
                return 0;
            default:
                std::cout << "Invalid CLI argument." << std::endl;
                return 1;
        }
    }


    // Чтение конфигурационного файла в случае ввода аргументов через него
    // или чтение аргументов напрямую
    if (configFileUsed) {
        std::unordered_map<std::string, std::string> argsMap = ConfigParser::ParseConfigFile(configFilePath);
        if (argsMap.empty()) {
            std::cout << "Unable to resolve config file, ensure -f argument has a correct value." << std::endl;
            return 1;
        }

        guestNumber = std::stoi(argsMap["guestNumber"]);
        int32_t tmp = std::stoi(argsMap["singleRoomStayTime"]);
        singleRoomStayTime = tmp >= 300000 ? tmp : 300000;
        tmp = std::stoi(argsMap["doubleRoomStayTime"]);
        doubleRoomStayTime = tmp >= 300000 ? tmp : 300000;
        tmp = std::stoi(argsMap["newGuestArrivalTime"]);
        newGuestArrivalTime = tmp >= 20000 ? tmp : 20000;
        logFilePath = argsMap["logFilePath"];

    } else {
        guestNumber = std::stoi(argv[optind]);
        int32_t tmp = std::stoi(argv[optind + 1]);
        singleRoomStayTime = tmp >= 300000 ? tmp : 300000;
        tmp = std::stoi(argv[optind + 2]);
        doubleRoomStayTime = tmp >= 300000 ? tmp : 300000;
        tmp = std::stoi(argv[optind + 3]);
        newGuestArrivalTime = tmp >= 20000 ? tmp : 20000;
        logFilePath = argv[optind + 4];
    }


    pthread_t guestThreads[guestNumber];

    hotel.logInfoFile.open(logFilePath, std::ios::out);
    if (!hotel.logInfoFile.is_open()) {
        std::cout << "Unable to resolve file to write to, ensure logFilePath argument has a correct value."
                  << std::endl;
        return 1;
    }
    hotel.singleRoomStayTimeMicro = singleRoomStayTime;
    hotel.doubleRoomStayTimeMicro = doubleRoomStayTime;

    // Инициализация потоков, создание инстанца посетятеля для каждого потока и передача потоковой функции
    // с задержкой в <new_guest_arrival_time> макросекунд
    for (int i = 0; i < guestNumber; ++i) {
        pthread_create(&guestThreads[i], nullptr, ComeToTheHotel, new HotelGuest(i + 1));
        usleep(newGuestArrivalTime);
    }

    for (int i = 0; i < guestNumber; ++i) {
        pthread_join(guestThreads[i], nullptr);
    }
    return 0;
}

void *ComeToTheHotel(void *arg) {
    auto *guest = static_cast<HotelGuest *>(arg);

    // Генерация случайных данных: 0/1 - для определения пола посетителя
    //  0 - Male , 1 - Female
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 1);

    guest->gender = (dis(gen) == 0 ? GuestGender::Male : GuestGender::Female);

    hotel.LogInfo("New guest with: <id> = " + std::to_string(guest->guestId) + " <gender> = " +
                  (guest->gender == GuestGender::Male ? "Male" : "Female") + " trying to stay in the hotel");
    // Попытка посетителя-потока получить номер в отеле
    if (hotel.TryGiveGuestNewRoom(*guest)) {
        hotel.LogInfo("Guest with: <id> = " + std::to_string(guest->guestId) + " <gender> = " +
                      (guest->gender == GuestGender::Male ? "Male" : "Female") + " got a room with\nType : " +
                      (guest->roomType == RoomType::Single ? "Single" : "Double") + "\nID : " +
                      std::to_string(guest->roomId));

        // Гость-поток занимает (ождиает) номер в отеле в случае успешного получения
        hotel.StayInRoom(*guest);
        // Гость-поток уходит из отеля и освобождает номер
        hotel.FreeRoom(*guest);

        hotel.LogInfo("Guest with: <id> = " + std::to_string(guest->guestId) + " <gender> = " +
                      (guest->gender == GuestGender::Male ? "Male" : "Female") + " left a room with\nType : " +
                      (guest->roomType == RoomType::Single ? "Single" : "Double") + "\nID : " +
                      std::to_string(guest->roomId));
    } else { // Гость-поток сразу уходит из отеля при отсутствии свободных комнат
        hotel.LogInfo("Guest with: <id> = " + std::to_string(guest->guestId) + " <gender> = " +
                      (guest->gender == GuestGender::Male ? "Male" : "Female") + " didn't get a room.");
    }

    delete guest;   // Уничтожение посетителя по завершению потоковой функции
    return nullptr;
}