#ifndef CPPTASKS_COMMON_H
#define CPPTASKS_COMMON_H

#include <cstdlib>
#include <fcntl.h>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <csignal>
#include <iostream>
#include <sys/stat.h>

// Структура в разделяемой процессами памяти
struct shared_memory_struct {
    int philosopher_count;          // Количество философов за столом
    // Неименованные семафоры в разделяемой памяти
    sem_t mutexShm;                 // Мьютекс для доступа к памяти
    sem_t forkWaiter;               // Семафор-счетчик "официант"
    sem_t forks[5];                 // Семафоры-вилки
};


extern const char* shared_memory_name;
extern int shmId;
extern shared_memory_struct* sharedMem;

// Номер процесса-философа за столом
extern int philosopherNumber;
// PID процесса-философа
extern pid_t processPid;

// Булевые флаги для освобождения ресурсов в обработчике сигналов
// в случае завершения работы процесса в момент захвата
extern std::atomic<bool> has_waiter_permission;
extern std::atomic<bool> has_left_fork;
extern std::atomic<bool> has_right_fork;

// Счетчик количества приемов пищи и размышлений для каждого процесса-философа
extern int eatTimes;
extern int thinkTimes;

// Индексы левого и правого семафора-вилки в массиве семафоров-вилок в разделяемой памяти
extern int leftForkSemIdx;
extern int rightForkSemIdx;

// Функция инициализации процесса-философа.
// Узнает номер процесса-философа, в случае если мест за столом нет, завершает процесс.
// Инициализирует неименованные семафоры в разделяемой памяти
// Иначе инициализирует соответствующие номеру философа семафоры-вилки
void init_philosopher();

// Функция для очистки занимаемой разделяемой памятью ресурсов процесса-философа
void close_philosopher();

// Функция удаления разделяемой памяти вместе с семафорами из системы
void unlink_all();

// Функция, осуществляющая обработку сигнала прерывания работы.
// Очищает ресурсы разделяемой памяти из локального процесса.
// Уменьшает количество процессов-философов в разделяемой памяти.
// Если данный процесс последний, удаляет ресурсы  разделяемой памяти вместе с неименованными семафорами из системы.
void sigFunc(int sig);


#endif //CPPTASKS_COMMON_H
