#include "GenderHotel.h"

#include <random>
#include <iostream>

GenderHotel::GenderHotel()  {
    freeDoubleRooms = doubleRoomsAmount;
    freeSingleRooms = singleRoomsAmount;

    this->singleRoomsAvailability = std::vector<bool>(singleRoomsAmount, true);
    this->doubleRoomsCurrentGender = std::vector<GuestGender>(doubleRoomsAmount, GuestGender::Any);
    this->doubleRoomsNumberOfGuests = std::vector<int32_t>(doubleRoomsAmount, 0);
    this->doubleRoomsMutexes = std::vector<pthread_mutex_t>(doubleRoomsAmount);
    this->doubleRoomTakenAmount = std::vector<uint32_t>(doubleRoomsAmount, 0);

    for (int32_t i = 0; i < doubleRoomsAmount; ++i) {
        pthread_mutex_init(&doubleRoomsMutexes[i], nullptr);
    }

    pthread_mutex_init(&this->logHotelInfoMutex, nullptr);
    pthread_mutex_init(&this->hotelMutex, nullptr);

    logSeparator = std::string(20, '-');
}

// Функция для выдачи посетителю номера в отеле, если это возможно
bool GenderHotel::TryGiveGuestNewRoom(HotelGuest &guest)  {
    pthread_mutex_lock(&this->hotelMutex);

    // Сначала пытаемся выдать посетителю одноместный номер, если это возможно
    if (this->freeSingleRooms > 0) {
        for (int32_t i = 0; i < singleRoomsAvailability.size(); ++i) {
            if (singleRoomsAvailability[i]) {
                singleRoomsAvailability[i] = false;
                guest.roomId = i + 1;
                break;
            }
        }
        guest.roomType = RoomType::Single;

        this->freeSingleRooms--;
        pthread_mutex_unlock(&this->hotelMutex);
        return true;
    }

    // Если одноместных номеров нет, пытаемся выдать посетителю двуместный номер
    if (this->freeDoubleRooms > 0) {
        
        // Сначала пытаемся подселить посетителя в двуместный номер, где уже расположен посетитель с таким же полом
        for (int32_t i = 0; i < doubleRoomsCurrentGender.size(); ++i) {
            if (doubleRoomsCurrentGender[i] != GuestGender::Any) {
                if (doubleRoomsCurrentGender[i] == guest.gender && doubleRoomsNumberOfGuests[i] == 1) {

                    pthread_mutex_lock(&this->doubleRoomsMutexes[i]);
                    doubleRoomsNumberOfGuests[i]++;
                    doubleRoomTakenAmount[i]++;
                    if (doubleRoomTakenAmount[i] <= 2) {
                        freeDoubleRooms--;
                    }
                    pthread_mutex_unlock(&this->doubleRoomsMutexes[i]);

                    guest.roomId = i + 1;
                    guest.roomType = RoomType::Double;

                    pthread_mutex_unlock(&this->hotelMutex);
                    return true;
                }
            }
        }

        // Если таких номеров нет, то ищем первый пустой двойной номер для заселения
        for (int32_t i = 0; i < doubleRoomsCurrentGender.size(); ++i) {
            if (doubleRoomsCurrentGender[i] == GuestGender::Any && doubleRoomsNumberOfGuests[i] == 0) {

                pthread_mutex_lock(&this->doubleRoomsMutexes[i]);
                doubleRoomsCurrentGender[i] = guest.gender;
                doubleRoomsNumberOfGuests[i]++;
                doubleRoomTakenAmount[i]++;
                pthread_mutex_unlock(&this->doubleRoomsMutexes[i]);

                guest.roomId = i + 1;
                guest.roomType = RoomType::Double;

                pthread_mutex_unlock(&this->hotelMutex);
                return true;
            }
        }

    }

    pthread_mutex_unlock(&this->hotelMutex);
    return false;
}

// Функция , симулирующая пребывание потока-посетителя в номере отеля
void GenderHotel::StayInRoom(const HotelGuest &guest)  {

    if (guest.roomType == RoomType::Single) {
        usleep(singleRoomStayTimeMicro);
    } else if (guest.roomType == RoomType::Double) {
        usleep(doubleRoomStayTimeMicro);
    }
}
// Функция для освобождение потоком-посетителем номера в отеле
void GenderHotel::FreeRoom(HotelGuest &guest) {
    pthread_mutex_lock(&this->hotelMutex);

    if (guest.roomType == RoomType::Single) {
        this->freeSingleRooms++;
        this->singleRoomsAvailability[guest.roomId - 1] = true;
    } else if (guest.roomType == RoomType::Double) {
        int32_t roomIdx = guest.roomId - 1;
        pthread_mutex_lock(&this->doubleRoomsMutexes[roomIdx]);
        // Если ешё один посетитель есть в двуместном номере, уменьшаем количество посетителей в номере
        if (this->doubleRoomsNumberOfGuests[roomIdx] == 2) {
            doubleRoomsNumberOfGuests[roomIdx]--;
        } else if (this->doubleRoomsNumberOfGuests[roomIdx] == 1) { // Если посетитель является послдним в двуместном номере, освобождаем номер
            doubleRoomsNumberOfGuests[roomIdx]--;
            doubleRoomsCurrentGender[roomIdx] = GuestGender::Any;
            doubleRoomTakenAmount[roomIdx] = 0;
            if (freeDoubleRooms < 15) {
                freeDoubleRooms++;
            }
        }
        pthread_mutex_unlock(&this->doubleRoomsMutexes[roomIdx]);
    }

    pthread_mutex_unlock(&this->hotelMutex);
}

// Функция для логирования информации в файл и консоль
void GenderHotel::LogInfo(const std::string &info)  {
    pthread_mutex_lock(&this->logHotelInfoMutex);

    std::cout << logSeparator << '\n'
              << info << '\n'
              << "Number of free single rooms: " << freeSingleRooms << " | "
              << "Number of free double rooms: " << freeDoubleRooms << '\n';

    if (this->logInfoFile.is_open()) {
        logInfoFile << logSeparator << '\n'
                    << info << '\n'
                    << "Number of free single rooms: " << freeSingleRooms << " | "
                    << "Number of free double rooms: " << freeDoubleRooms << '\n';
    }

    pthread_mutex_unlock(&this->logHotelInfoMutex);
}

