.include "../macrolib.s"

.global load_from_file

# Подпрограмма для чтения данных из файла в динамическую память с использованием буффера данных размером 512 байт
# методом скользящего окна
# Входные параметры:
# a0 - указатель на область памяти со строкой - путем к файлу
# a1 - максимальный размер буффера пути к файлу (256)
# a2 - буффер для хранения части текста при считывании
# a3 - максимальный размер буффера данных (512)
# Возвращаемое значение
# a0 - адрес в динамической памяти (heap), где размещены данные
.text
load_from_file:
	stack_push_w(ra)
	stack_push_w(a2)
	stack_push_w(s1)
	stack_push_w(s2)
	stack_push_w(s3)
	stack_push_w(s4)
	stack_push_w(s5)
	stack_push_w(s6)
	mv s4 a3		# Сохранение размера буффера
	mv s3 a2		# Сохранение начального адреса буфферного окна для данных
	
	# Вызов макроса открытия файла и валидации корркетного открытия
	# a0 - буффер (область памяти), где хранится путь к файлу
	open_and_validate(a0, READ_ONLY)
	# Возвращаемое значение:
	# a0 - дескриптор файла
	mv s1 a0

    	mv s6 zero	# Адрес изначально выделенной памяти на хипе (указатель на начало текста)
    	mv s5 zero	# Динимический указатель на текущий адрес в хипе
    	li t1 -1	# Регистр проверки на bad read
	for_read_loop:
		
		#Вызов макроса чтения данных из файла в память (в буффер)
		# s1 - дексприптор файла
		# s3 - область памяти, куда будут писаться данные (в данном случае буффер для данных)
    		read_addr_reg(s1, s3, s4)
		
		# Проверка на чтение и выборс исключения при невозможности чтения
    		bne    a0 t1 end_if_error_read
    		if_error_read:
    			close(s1)
    			throw_read_error()
    		end_if_error_read:
    		
    		
    		mv     s2 a0		# Сохраняем кол-во записанных байтов
    		
    		allocate(s2)		# Выделяем память в куче
    		
    		# Если аллокация впервые, запоминаем изначальный адрес алоцированной памяти
    		bnez s5 end_if_hf
    		if_heap_first:
    		mv s6 a0
    		mv s5 a0
    		end_if_hf:
    		
    		# Вызов макроса копирования текста из памяти в память
    		# s5 - адресс выделенной памяти в куче
    		# s3 - адрес начала буффера, откуда будут скопированы данные
    		# s2 - размер только что записанных данных в байтах (количество байтов для копирования)
    		strncpy(s5, s3, s2)
    		add s5 s5 s2	# Увеличиваем актуальный указатель на кучу с данными
    
    		bne    s2 s4 end_read_loop 	# Выход из цикла при текущей записи меньше чем размер буфера

    		b for_read_loop
	end_read_loop:
	
   	close(s1)	# Закрытие файла
	
	# Добавление null-char к концу строки в куче
    	mv  t0 s5
    	addi t0 t0 1
    	sb  zero (t0)
	
	# Возврат адресса записанных данных на хипе и восстановление регистров
	mv a0 s6
	stack_pop_w(s6)
	stack_pop_w(s5)
	stack_pop_w(s4)
	stack_pop_w(s3)
	stack_pop_w(s2)
	stack_pop_w(s1)
	stack_pop_w(a2)
	stack_pop_w(ra)
	jalr zero 0(ra)
