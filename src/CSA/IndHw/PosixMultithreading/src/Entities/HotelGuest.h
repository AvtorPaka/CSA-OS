#ifndef POSIXMULTITHREADING_HOTELGUEST_H
#define POSIXMULTITHREADING_HOTELGUEST_H

#include <cstdint>

// Enum для определения пола посетителя отеля
enum GuestGender {
    Any, // Служит для определения того, что в номере для 2х гостей никого нет
    Male,
    Female
};

// Enum для опредения типа комнаты, занятой посетителем
enum RoomType {
    None, // Служит началным значением при инициализации нового посетителя
    Single,
    Double
};

struct HotelGuest {
    int32_t guestId;
    GuestGender gender;
    RoomType roomType = None;
    int32_t roomId = -1;

    HotelGuest(int32_t guestId);
};


#endif //POSIXMULTITHREADING_HOTELGUEST_H
