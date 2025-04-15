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
struct shared_memory_struct{
    int philosopher_count;  // Количество философов за столом
};

extern const char* shared_memory_name;
extern int shmId;
extern shared_memory_struct* sharedMem;

// Семафора-мьютекс для синхронизации доступа к разделяемой памяти
extern const char* mutex_sem_shm_name;
extern sem_t* mutexShm;

// Семафора-счетчик количество свободных вилок - 1, для предотвращения ситуации дедлока
extern const char* sem_fork_waiter_name;
extern sem_t* forkWaiter;

// Префикс имен семафор-вилок
extern const char* sem_fork_base_name;

// Левый и правый семафор-вилка для процесса-философа
extern std::string left_fork_name;
extern std::string right_fork_name;
extern sem_t* left_fork;
extern sem_t* right_fork;

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


// Функция инициализации процесса-философа.
// Инициализирует семафоры, кроме семафор-вилок, и разделяемую память.
// Узнает номер процесса-философа, в случае если мест за столом нет, завершает процесс.
// Иначе инициализирует соответствующие номеру философа семафоры-вилки
void init_philosopher();

// Функция для очистки занимаемых семафорами и разделяемой памятью ресурсов процесса-философа
void close_philosopher();

// Функция удаления семафоров и разделяемой памяти из системы
void unlink_all();

// Функция удаления одного семафора из системы
void unlink_sem_one(const char* posixName);

// Функция, осуществляющая обработку сигнала прерывания работы.
// Очищает ресурсы семафоров и разделяемой памяти из локального процесса.
// Уменьшает количество процессов-философов в разделяемой памяти.
// Если данный процесс последний, удаляет ресурсы семафоров и разделяемой памяти из системы.
void sigFunc(int sig);


#endif //CPPTASKS_COMMON_H
