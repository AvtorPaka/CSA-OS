#ifndef CPPTASKS_COMMON_H
#define CPPTASKS_COMMON_H

#include <cstdlib>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <sys/mman.h>
#include <csignal>
#include <iostream>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/msg.h>

#define NUM_OF_SITS 5

// Структура информации в очереди сообщений
struct shared_memory_struct{
    int philosopher_count;  // Количество философов за столом
    bool hasSit[NUM_OF_SITS]; // Массив булевых значений для определения места на которое сядет философ, в случае их ухода не по порядку
};

struct Message {
    long mtype;
    shared_memory_struct data;
};

// Данные очереди сообщений
extern const char* message_query_path;
extern int mqId;

// Семафора-мьютекс для синхронизации доступа к очереди сообщений
extern const char* mutex_sem_path;
extern int mutexSemId;

// Семафора-счетчик количество свободных вилок - 1, для предотвращения ситуации дедлока
extern const char* waiter_sem_path;
extern int forkWaiterSemId;

// Префикс имен семафор-вилок
extern const char* sem_fork_base_path;

// Левый и правый семафор-вилка для процесса-философа
extern int leftForkSemId;
extern int rightForkSemId;

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
void unlink_sem_one(int semId);

// Функция, осуществляющая обработку сигнала прерывания работы.
// Очищает ресурсы семафоров и разделяемой памяти из локального процесса.
// Уменьшает количество процессов-философов в разделяемой памяти.
// Если данный процесс последний, удаляет ресурсы семафоров и разделяемой памяти из системы.
void sigFunc(int sig);

// Вспомогательные функции для работы с семафорами System V
void P(int semId);  // wait 1
void V(int semId);  // post 1

// Функция инициализации семафоры
int initSystemVSemaphore(const char* path, int initValue);


#endif //CPPTASKS_COMMON_H
