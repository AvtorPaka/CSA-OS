#include <semaphore.h>
#include <random>
#include "common/common.h"
#include <csignal>
#include <sys/stat.h>
#include <iostream>

// Семафор для отсекания лишних писаталей
// То есть, для формирования только одного процесса-писателя
// Закрывает критическую секцию для анализа наличия писателей
const char *writer_sem_name = "/writer-semaphore";
sem_t *writer;   // указатель на семафор пропуска писателей
// Семафор, играющий роль флага для допуска писателей
const char *first_writer_sem_name = "/first_writer-semaphore";
sem_t *first_writer;   // указатель на семафор допуска

// Функция, осуществляющая обработку сигнала прерывания работы
// Осществляет удаление всех семафоров и памяти. Заодно "убивает" читателя
// независимо от его текущего состояния
void sigfunc(int sig) {
    if (sig != SIGINT && sig != SIGTERM) {
        return;
    }
    if (sig == SIGINT) {
        kill(buffer->reader_pid, SIGTERM);
        printf("Writer(SIGINT) ---> Reader(SIGTERM)\n");
    } else if (sig == SIGTERM) {
        printf("Writer(SIGTERM) <--- Reader(SIGINT)\n");
    }
    // Закрывает видимые семафоры и файлы, доступные процессу
    if (sem_close(writer) == -1) {
        perror("sem_close: Incorrect close of writer semaphore");
        exit(-1);
    }
    if (sem_close(first_writer) == -1) {
        perror("sem_close: Incorrect close of first_writer semaphore");
        exit(-1);
    }
    close_common_semaphores();

    // Удаляет все свои семафоры и разделяемую память
    if (sem_unlink(writer_sem_name) == -1) {
        perror("sem_unlink: Incorrect unlink of writer semaphore");
    }
    if (sem_unlink(first_writer_sem_name) == -1) {
        perror("sem_unlink: Incorrect unlink of first_writer semaphore");
    }
    unlink_all();
    printf("Writer: bye!!!\n");
    exit(10);
}

int main() {
    // Задание обработчика, завершающего работу писателя и обеспечивающего
    // уничтожение контекста и процессов
    signal(SIGINT, sigfunc);
    signal(SIGTERM, sigfunc);

    init();         // Начальная инициализация общих семафоров

    // Формирование объекта разделяемой памяти для общего доступа к кольцевому буферу
    if ((buf_id = shm_open(shar_object, O_CREAT | O_RDWR, 0666)) == -1) {
        perror("shm_open");
        exit(-1);
    } else {
        printf("Object is open: name = %s, id = 0x%x\n", shar_object, buf_id);
    }

    struct stat sb;
    if (fstat(buf_id, &sb) == -1) {
        std::cout << "Unable to get shared memory status" << '\n';
        close(buf_id);
        exit(1);
    }

    // Задание размера объекта памяти
    if (sb.st_size == 0) {
        if (ftruncate(buf_id, sizeof(shared_memory)) == -1) {
            perror("ftruncate");
            exit(-1);
        } else {
            printf("Memory size set and = %lu\n", sizeof(shared_memory));
        }
    }

    //получить доступ к памяти
    buffer = static_cast<shared_memory *>(mmap(nullptr, sizeof(shared_memory), PROT_WRITE | PROT_READ, MAP_SHARED,
                                               buf_id, 0));
    
    buffer->admin_initialized = 0;

    if (buffer == (shared_memory *) -1) {
        perror("writer: mmap");
        exit(-1);
    }
    printf("mmap checkout\n");

    // Разборки писателей. Семафор для конкуренции за работу
    if ((writer = sem_open(writer_sem_name, O_CREAT, 0666, 1)) == nullptr) {
        perror("sem_open: Can not create writer semaphore");
        exit(-1);
    }

    // Дополнительный семафор, играющий роль счетчика-ограничителя
    if ((first_writer = sem_open(first_writer_sem_name, O_CREAT, 0666, 1)) == nullptr) {
        perror("sem_open: Can not create first_writer semaphore");
        exit(-1);
    }

    // Первый просочившийся запрещает доступ остальным за счет установки флага
    if (sem_wait(writer) == -1) {
        perror("sem_wait: Incorrect wait of writer semaphore");
        exit(-1);
    }
    // Он проверяет значение семафора, к которому другие не имеют доступ
    // и устанавливает его в 0, если является первым писателем.
    // Остальные писатели при проверке завершают работу

    // sem_getvalue не реализовано на OS X, поэтому её использование для контролирования количества писателей невозможно
    // вместо неё используем sem_trywait
    if (sem_trywait(first_writer) == -1) {
        if (errno == EAGAIN) {
            // Семафор уже заблокирован (значение 0) - другой писатель активен
            printf("Writer %d: I have lost this work :(\n", getpid());

            // Освобождаем основной семафор writer для других процессов
            if (sem_post(writer) == -1) {
                perror("sem_post: Consumer can not increment writer semaphore");
                exit(-1);
            }
            exit(13);
        } else {
            // Другая ошибка
            perror("sem_trywait: Incorrect trywait on first_writer semaphore");
            exit(-1);
        }
    }

    // Пропуск других писателей для определения возможности поработать
    if (sem_post(writer) == -1) {
        perror("sem_post: Consumer can not increment writer semaphore");
        sem_unlink(first_writer_sem_name);
        exit(-1);
    }
    // сохранение pid для корректного взаимодействия с писателем
    buffer->writer_pid = getpid();
    printf("Writer %d: I am first for this work! ^-^\n", getpid());

    buffer->have_reader = 0;
    buffer->writer_index = 0;
    // buffer->have_writer = 0;

    // Инициализация генератора случайных чисел
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dist(0, 10);
    std::uniform_int_distribution<> dist_time(0, 2);

    // Подъем администратора, разрешающего продолжение процессов читателя
    // После того, как сформировался объект в памяти и прошла его инициализация.
    // В начале идет проверка на запуск хотя бы одного писателя, то есть,
    // на то, что семафор уже поднят, и писатели поступают.
    // Это позволяет новым писателям не увеличивать значение семафора.

    // sem_getvalue не реализовано на OS X, поэтому её использование для контролирования количества писателей невозможно
    // вместо неё используем флаг в разделяемой памяти
    if (sem_wait(mutex) == -1) {
        perror("sem_wait: Failed to lock mutex");
        sem_unlink(first_writer_sem_name);
        exit(-1);
    }

    // Если флаг не установлен, это первый писатель
    if (buffer->admin_initialized == 0) {
        buffer->admin_initialized = 1; // Помечаем как инициализированное
        if (sem_post(admin) == -1) {   // Разрешаем читателям работать
            perror("sem_post: Failed to increment admin");
            sem_post(mutex);
            sem_unlink(first_writer_sem_name);
            exit(-1);
        }
    }

    if (sem_post(mutex) == -1) {
        perror("sem_post: Failed to unlock mutex");
        sem_unlink(first_writer_sem_name);
        exit(-1);
    }


    // Алгоритм писателя
    while (true) {
        // Проверка заполнения буфера (ждать если полон)
        if (sem_wait(empty) == -1) { //защита операции записи
            perror("sem_wait: Incorrect wait of empty semaphore");
            sem_unlink(first_writer_sem_name);
            exit(-1);
        }
        //критическая секция, конкуренция с читателем
        if (sem_wait(mutex) == -1) {
            perror("sem_wait: Incorrect wait of mutex");
            sem_unlink(first_writer_sem_name);
            exit(-1);
        }

        int i = buffer->writer_index;
        buffer->store[i] = dist(gen);
        buffer->writer_index = (i + 1) % BUF_SIZE;

        //количество занятых ячеек увеличилось на единицу
        if (sem_post(full) == -1) {
            perror("sem_post: Incorrect post of full semaphore");
            sem_unlink(first_writer_sem_name);
            exit(-1);
        }
        // Вывод информации об операции записи
        pid_t pid = getpid();
        printf("Producer %d writes value = %d to cell [%d]\n",
               pid, buffer->store[i], i);
        // pid, data, buffer->tail - 1 < 0 ? BUF_SIZE - 1: buffer->tail - 1 ;
        // Выход из критической секции
        if (sem_post(mutex) == -1) {
            perror("sem_post: Incorrect post of mutex semaphore");
            sem_unlink(first_writer_sem_name);
            exit(-1);
        }
        sleep(dist_time(gen) + 1);
    }

    return 0;
}

