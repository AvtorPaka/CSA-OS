.data
input_size: .asciz "Input array size (1-10): "
input_array_elements: .asciz "Input array data:\n"
ArraySizeOutOfRangeException: .asciz "ArraySizeOutOfRangeException -> array size must be 1<= _size <= 10"
empty_array_case: .asciz "Array is empty."
nxt: .asciz " "


# Макрос для ввода размера массива и его наполнения
# Параметры: 
# %x - адресс массива в памяти, куда пользователь будет записывать данные
# Возвращаемое значение: 
# a0 -  размер заполненого массива (в случае допустимого значение)
.macro input_array_data(%x)
	# Инициализация переменных на стэке
	addi sp sp -12
	sw a1 8(sp)	# Сохранение предыдущего значения a1
	mv a1 %x
	sw a1 4(sp)		# Сохранение адресса массива для ввода
	sw zero (sp)	# Место для введенного размера массива
	
	# Ввод пользователем размера массива
	la a0 input_size
	li a7 4
	ecall 
	
	li a7 5
	ecall
	
	sw a0 (sp)	# Сохранение размера массива
	
	# Вызов макроса проверки размера массива
				# Передаваемые параметры:
				# (%x) a0 - Размер массива, введенный пользователем (уже в a0 после ecall 5)
	li a1 1			# (%y) a1 - нижняя граница размера массива (1)
	li a2 10		# (%z) a2 - верхняя граница размера массива (10)
	check_array_size(a0 a1 a2)
	# Возвращаемое значение:
	# a0 - 0 если размер недопустим, 1 в другом случае
	
	# Выброс исключения и завершение работы в случае недопустмого размера (a0 = 0)
	bnez a0 end_b_size
	if_b_size:
		lw a1 8(sp)
		addi sp sp 12
		throw_ArraySizeOutOfRangeException
	end_b_size:
	
	la a0 input_array_elements
	li a7 4
	ecall
	
	lw t2 (sp)	# Размер массива, введенный ранее пользователем
	lw t0 4(sp)	# Адресс памяти (массива), куда будет происходить запись
	li t1 0		# Счетчик для цикла ввода массива
	
	#for (t1 = 0; t1 < t2; t1++) {...}
	for_fill_loop:
	#Ввод числа пользотвалем
	li a7 5
	ecall
	
	#Запись числа в память(массив)
	sw a0, 0(t0)
	addi t0 t0 4
	addi t1 t1 1
	blt t1 t2 for_fill_loop
	end_fill_loop:

	# Сохранение возвращаемых значений, воссатновление сохраненых на стеке значений
	lw a1 8(sp)
	lw a0 (sp)
	addi sp sp 12
.end_macro



# Макрос для проверки введенного размера массива на корркетность
# Параметры:
# %x - Размер массива, введенный пользователем
# %y - нижняя граница размера массива (1)
# %z - верхняя граница размера массива (10)
# Возвращаемое значение:
# a0 - 0 если размер недопустим, 1 в другом случае
.macro check_array_size(%x %y %z)
	addi sp sp -12
	sw a0 8(sp)
	sw a1 4(sp)
	sw a2 (sp)
	
	mv a0 %x
	mv a1 %y
	mv a2 %z
	
	blt a0 a1 if_size_error
	ble a0 a2 if_size_ok
	if_size_error:
		li a0 0
		b end_if_size_check
	if_size_ok:
		li a0 1
	end_if_size_check:
	
	
	lw a1 4(sp)
	lw a2 (sp)
	addi sp sp 12
.end_macro

# Макрос для вывода значение из массива в консоль
# Параметры:  
# %x - адрес массива, данные которого надо вывести
# %y - размер массива
# %z - указатель на строку с сообщением вывода
# Возвращаемое значение: 
# a0 - адресс переданного  массива
.macro func_output_array_data(%x %y %z)
	# Инициализация переменных на стэке
	addi sp sp -12
	sw a2 8(sp)
	sw a1 4(sp)
	mv a0 %x
	mv a1 %y
	mv a2 %z
	sw a0 (sp)
	
	mv a0 a2
	li a7 4
	ecall
	
	# Проверка на пустоту массива и вывод соответсвующего сообщения в случае пустоты
	bnez a1 end_if_arr_empty
	if_array_empty:
		la a0 empty_array_case
		li a7 4
		ecall
		j end_output_loop
	end_if_arr_empty:
	
	li t1 0		# Счетчик для цикла ввода массива
	lw t2 0(sp)	# Адресс массива
	
	#for (t1 = 0; t1 < a1; t1++) {...}
	for_output_loop:
	# Вывод элементов массива в консоль
	lw a0 0(t2)
	li a7 1
	ecall
	
	next	# Макрос вывода пробела в консоль

	addi t2 t2 4
	addi t1 t1 1
	blt t1 a1 for_output_loop
	end_output_loop:
	
	# Сохранение возвращаемых значений, воссатновление сохраненых на стеке значений
	lw a2 8(sp)
	lw a1 4(sp)
	lw a0 (sp)
	addi sp sp 12
.end_macro 

# Макрос для выброса исключения и завершения работы программы в случае недопустимого размера массива
# Параметры:
# %x - Адрес строки с сообщением об исключении
.macro throw_ArraySizeOutOfRangeException
	# Вывод сообщения с исключение в консоль и MessageDialog среды RARS
	la a0 ArraySizeOutOfRangeException
	li a1 0
	li a7 55
	ecall
	
	li a7 4
	ecall
	
	li a7 10
	ecall
.end_macro

# Макрос для вывода символа в консоль
.macro print_char(%x)
   li a7, 11
   li a0, %x
   ecall
.end_macro

# Макрос для вывода символа переноса строки в консоль
.macro newline
   print_char('\n')
.end_macro

.macro next
	la a0 nxt
	li a7 4
	ecall
.end_macro
	

# Макрос для завершения работы программы
.macro exitProgram
    li a7, 10
    ecall
.end_macro

# Макрос для формирования набора тестовых массивов
# Параметры:
# a0 - указатель на массив, где будут хранится указатели на тестовые массивы
# a1 - указатель на массив, где будут хранится размеры тестовых массивов
# Возвращаемое значение: 
# a0 - указатель на массив, заполненый указателями на тестовые массивы с данными
# a1 - указатель на массив, заполненый размерами тестовых массивов
# a2 - количество тестовых массивов (т.е размер массивов a0 и a1)
.macro create_test_arrays(%x %y)
	# Инициализация переменных на стэке
	mv a0 %x
	mv a1 %y
	addi sp sp -8
	sw a0 4(sp)
	sw a1 (sp)
	
	li t1 0		# Переменная для записи чисел в тестовые массивы
	li t0 0		# Переменная для хранения указателя на текущий тестовый массив
	li t3 0		# Переменная для хранения размера тестового массива
	
	#Создание 1 тестовго массива
	la t0 test_array_1
	sw t0 (a0)
	addi a0 a0 4
	li t3 6
	sw t3 (a1)
	addi a1 a1 4
	
	li t1 6
	sw t1 (t0)
	addi t0 t0 4
	li t1 5
	sw t1 (t0)
	addi t0 t0 4
	li t1 4
	sw t1 (t0)
	addi t0 t0 4
	li t1 3
	sw t1 (t0)
	addi t0 t0 4
	li t1 2
	sw t1 (t0)
	addi t0 t0 4
	li t1 1
	sw t1 (t0)
	addi t0 t0 4
	
	# Создание 2 тестовго массива
	la t0 test_array_2
	sw t0 (a0)
	addi a0 a0 4
	li t3 10
	sw t3 (a1)
	addi a1 a1 4
	
	li t1 10
	sw t1 (t0)
	addi t0 t0 4
	li t1 9
	sw t1 (t0)
	addi t0 t0 4
	li t1 12
	sw t1 (t0)
	addi t0 t0 4
	li t1 8
	sw t1 (t0)
	addi t0 t0 4
	li t1 5
	sw t1 (t0)
	addi t0 t0 4
	li t1 4
	sw t1 (t0)
	addi t0 t0 4
	li t1 3
	sw t1 (t0)
	addi t0 t0 4
	li t1 12
	sw t1 (t0)
	addi t0 t0 4
	li t1 13
	sw t1 (t0)
	addi t0 t0 4
	li t1 -100
	sw t1 (t0)
	addi t0 t0 4
	
	# Создание 3 тестовго массива
	la t0 test_array_3
	sw t0 (a0)
	addi a0 a0 4
	li t3 8
	sw t3 (a1)
	addi a1 a1 4
	
	li t1 1
	sw t1 (t0)
	addi t0 t0 4
	li t1 2
	sw t1 (t0)
	addi t0 t0 4
	li t1 3
	sw t1 (t0)
	addi t0 t0 4
	li t1 4
	sw t1 (t0)
	addi t0 t0 4
	li t1 5
	sw t1 (t0)
	addi t0 t0 4
	li t1 6
	sw t1 (t0)
	addi t0 t0 4
	li t1 7
	sw t1 (t0)
	addi t0 t0 4
	li t1 8
	sw t1 (t0)
	addi t0 t0 4
	
	
	# Создание 4 тестовго массива
	la t0 test_array_4
	sw t0 (a0)
	addi a0 a0 4
	li t3 6
	sw t3 (a1)
	addi a1 a1 4
	
	li t1 2
	sw t1 (t0)
	addi t0 t0 4
	li t1 2
	sw t1 (t0)
	addi t0 t0 4
	li t1 2
	sw t1 (t0)
	addi t0 t0 4
	li t1 2
	sw t1 (t0)
	addi t0 t0 4
	li t1 2
	sw t1 (t0)
	addi t0 t0 4
	li t1 1
	sw t1 (t0)
	addi t0 t0 4
	
	# Создание 5 тестовго массива
	la t0 test_array_5
	sw t0 (a0)
	addi a0 a0 4
	li t3 2
	sw t3 (a1)
	addi a1 a1 4
	
	li t1 9
	sw t1 (t0)
	addi t0 t0 4
	li t1 10
	sw t1 (t0)
	addi t0 t0 4
	
	# Создание 6 тестовго массива
	la t0 test_array_6
	sw t0 (a0)
	addi a0 a0 4
	li t3 1
	sw t3 (a1)
	addi a1 a1 4
	
	li t1 1111
	sw t1 (t0)
	addi t0 t0 4
	
	# Создание 7 тестовго массива
	la t0 test_array_7
	sw t0 (a0)
	addi a0 a0 4
	li t3 9
	sw t3 (a1)
	addi a1 a1 4
	
	li t1 3
	sw t1 (t0)
	addi t0 t0 4
	li t1 3
	sw t1 (t0)
	addi t0 t0 4
	li t1 5
	sw t1 (t0)
	addi t0 t0 4
	li t1 5
	sw t1 (t0)
	addi t0 t0 4
	li t1 2
	sw t1 (t0)
	addi t0 t0 4
	li t1 -99
	sw t1 (t0)
	addi t0 t0 4
	li t1 -101
	sw t1 (t0)
	addi t0 t0 4
	li t1 1001
	sw t1 (t0)
	addi t0 t0 4
	li t1 7
	sw t1 (t0)
	addi t0 t0 4
	
	
	# Сохранение возвращаемых значений, воссатновление сохраненых на стеке значений
	li a2 7
	lw a1 (sp)
	lw a0 4(sp)
	addi sp sp 8
.end_macro
