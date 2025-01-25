#ifndef POSIXMULTITHREADING_GENDERHOTEL_H
#define POSIXMULTITHREADING_GENDERHOTEL_H

#include "HotelGuest.h"
#include <cstdint>
#include <pthread.h>
#include <unistd.h>
#include <random>
#include <fstream>
#include <sstream>

// Общий разделяемый потоками-посетителями отель
struct GenderHotel {
    static constexpr int32_t singleRoomsAmount = 10;    // Общее количество одноместных номеров
    static constexpr int32_t doubleRoomsAmount = 15;    // Общее двуместных одноместных номеров
    useconds_t singleRoomStayTimeMicro = 6000000;       // Время пребывания посетителя в одноместном номере
    useconds_t doubleRoomStayTimeMicro = 3000000;       // Время пребывания посетителя в двуместном номере
    std::string logSeparator;

    int32_t freeSingleRooms;    // Количество свободных одноместных номеров
    int32_t freeDoubleRooms;    // Количество свободных двуместных номеров

    // Вектор для определение того, свободен ли одиночный номер и выдачи ID номера посетителю
    std::vector<bool> singleRoomsAvailability;

    // Вектор для определение того, каким полом сейчас занят номер для двух посетителей
    std::vector<GuestGender> doubleRoomsCurrentGender;
    // Вектор мьютексов для синхронизации работы с данными для двухместных номерв
    std::vector<pthread_mutex_t> doubleRoomsMutexes;
    // Вектор для определени того, сколько человек сейчас занимает двухместный номер
    std::vector<int32_t> doubleRoomsNumberOfGuests;
    // Вектор для определения того, сколько человек занимало двухместный номер до его полного освобождения
    std::vector<uint32_t> doubleRoomTakenAmount;

    // Мьютекс для синхронизации доступа к данным отеля
    pthread_mutex_t hotelMutex;
    // Мьютекс для синхронизации доступа записи в файл логирования
    pthread_mutex_t logHotelInfoMutex;
    // Файловый поток для записи в файл логирования
    std::ofstream logInfoFile;

    GenderHotel();

    // Функция для выдачи посетителю номера в отеле, если это возможно
    bool TryGiveGuestNewRoom(HotelGuest &guest);

    // Функция , симулирующая пребывание потока-посетителя в номере отеля
    void StayInRoom(const HotelGuest &guest);

    // Функция для освобождение потоком-посетителем номера в отеле
    void FreeRoom(HotelGuest &guest);

    // Функция для логирования информации в файл и консоль
    void LogInfo(const std::string &info);

    ~GenderHotel() {
        for (int32_t i = 0; i < doubleRoomsAmount; ++i) {
            pthread_mutex_destroy(&doubleRoomsMutexes[i]);
        }

        pthread_mutex_destroy(&hotelMutex);
        pthread_mutex_destroy(&logHotelInfoMutex);
        logInfoFile.close();
    }
};


#endif //POSIXMULTITHREADING_GENDERHOTEL_H
