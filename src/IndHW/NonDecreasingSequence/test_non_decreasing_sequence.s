.data
array_arr_pointers: .space 28
array_arr_sizes: .space 28
test_array_1: .space 24
test_array_2: .space 40
test_array_3: .space 32
test_array_4: .space 24
test_array_5: .space 8
test_array_6: .space 4
test_array_7: .space 36
array_b: .space 40
output_created_test_array: .asciz "\nTest array A:\n"
output_array_data: .asciz "\nCalculated array B:\n"
empty_array_case: .asciz "Array is empty."
new_line: .asciz "\n"
nxt: .asciz " "

.text
_start:	

	# Вызов одпрограммы для формирования набора тестовых массивов
					# Передаваемые параметры
	la a0 array_arr_pointers	# a0 - указатель на массив, где будут хранится указатели на тестовые массивы
	la a1 array_arr_sizes		# a1 - указатель на массив, где будут хранится размеры тестовых массивов
	jal ra create_test_arrays
	
	# Возвращаемое значение: 
	mv s1 a0		# a0 - указатель на массив, заполненый указателями на тестовые массивы с данными
	mv s2 a1		# a1 - указатель на массив, заполненый размерами тестовых массивов
	mv s3 a2		# a2 - количество тестовых массивов (т.е размер массивов a0 и a1)
	
	li a3 0		# Счетчик итераций по масcиву указателей на тестовые массивы
	
	# for(a3 = 0; a3 < s3; ++a3) {...}
	for_test_loop:
	
		# Вызов подпрограммы вывывода элементов массива (в частности элементов тестовго массива A)
						# Передаваемые параметры:  
		lw a0 (s1)			# a0 - адрес тестовго массива A
		lw a1 (s2) 			# a1 - размер тестовго массива A
		la a2 output_created_test_array		# a2 - указатель на строку с сообщением вывода (строка для вывода тестового массива A)
		jal ra func_output_array_data

		# Возвращаемое значение: 
		# a0 - адресс переданного  массива
	
		# Вызов подпрограммы формирование массива B по массиву тестовому массиву
				# Передаваемые параметры:
		lw a0 (s1)	# a0 - адрес массива A (тестового)
		lw a1 (s2)	# a1 - размер массива A (тестовго)
		la a2 array_b	# a2 - адрес массива B, куда будет вестить запись
		
		jal ra create_b_array
		# Возвращаемое значение: 
		# a0 - адресс созданного массива B
		# a1 - размер полученного массива B
		
		# Вызов подпрограммы вывывода элементов массива (в частности элементов массива B)
		# Передаваемые параметры:  
		# a0 - адрес массива, данные которого надо вывести (в a0 уже расположен адресс массива B)
		# a1 - размер массива (в a1 уже расположен размер массива B)
		# a2 - указатель на строку с сообщением вывода (строка для вывода массива B)
		la a2 output_array_data
		jal ra func_output_array_data

		# Возвращаемое значение: 
		# a0 - адресс переданного  массива
		
		li a7 4
		la a0 new_line
		ecall
		
		addi a3 a3 1
		addi s1 s1 4
		addi s2 s2 4
		blt a3 s3 for_test_loop
	end_test_loop:
	
	j exit_program


# Подпрограмма для формирования набора тестовых массивов
# Параметры:
# a0 - указатель на массив, где будут хранится указатели на тестовые массивы
# a1 - указатель на массив, где будут хранится размеры тестовых массивов
# Возвращаемое значение: 
# a0 - указатель на массив, заполненый указателями на тестовые массивы с данными
# a1 - указатель на массив, заполненый размерами тестовых массивов
# a2 - количество тестовых массивов (т.е размер массивов a0 и a1)
create_test_arrays:
	# Инициализация переменных на стэке
	addi sp sp -12
	sw ra 8(sp)
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
	
	
	# Сохранение возвращаемых значений, воссатновление сохраненых на стеке значений и возврат указателя
	li a2 7
	lw a1 (sp)
	lw a0 4(sp)
	lw ra 8(sp)
	addi sp sp 12
	jalr zero 0(ra)


# Подпрограмма для создания массива B ,состоящего из неубывающих последовательноcтей элементов из переданного массива A
# В случае когда в массиве A 1 элемент - массив B состоит из этого элемента
# В остальных случаях массив B будет состоять из неубывающих последовательностей в массиве A
# Если в A отсутствуют неубывающие последовательности - массив B будет пустым
# Параметры:
# a0 - адрес массива A
# a1 - размер массива A
# a2 - адрес массива B, куда будет вестить запись
# Возвращаемое значение: 
# a0 - адресс созданного массива B
# a1 - размер полученного массива B
create_b_array:
	# Инициализация переменных на стэке
	addi sp sp -8
	sw ra 4(sp)
	sw a2 (sp)
	
	#Проверка размера массива на 1, если _size (a1) = 1, возвращение B = A.
	li t0 1
	bne a1 t0 end_if_size_1
	if_size_1:
		lw t6 (a0)
		sw t6 (a2)
		li t4 1
		j end_alg_loop
	end_if_size_1:
	
	mv t0 a0	# Указатель на предыдущий элемент массива A - A[t5 - 1]
	mv t1 a0	# Указатель на текущий элемент массива A - A[t5]
	addi t1 t1 4
	mv t2 a2	# Текущая позиция для вставки в массив B
	li t3 0		# Индекс потенциального начала подпоследовательности
	li t4 0		# Счетчик количества элементов в массиве B
	li t5 1		# Cчетчик для итераций по массиву A
	li t6 0		# Временное значение для доступа к элементам массива A
	li a6 0		# Временное значение для доступа к элементам массива A
	
	# for (t5 = 1; t5 < a1; t5++) {...}
	for_alg_loop:
	
	lw t6 (t0)
	lw a6 (t1)
	blt a6 t6 else_1
	if_1:	# if A[t5] >= A[t5-1]
		
		addi t5 t5 -1
		bne t5 t3 end_if_2
		if_2: # if t5 - 1 == t3
			lw t6 (t0)
			sw t6 (t2)
			addi t2 t2 4
			addi t4 t4 1
		end_if_2:
		addi t5 t5 1
		
		lw t6 (t1)
		sw t6 (t2)
		addi t2 t2 4
		
		addi t4 t4 1
		b end_1
	else_1:	# A[t5] < A[t5-1]
		mv t3 t5
	end_1:
	
	addi t5 t5 1
	addi t0 t0 4
	addi t1 t1 4
	blt t5 a1 for_alg_loop
	end_alg_loop:
	
	
	# Сохранение возвращаемых значений, воссатновление сохраненых на стеке значений и возврат указателя
	mv a1 t4
	lw a0 (sp)
	lw ra 4(sp)
	addi sp sp 8
	jalr zero 0(ra)


# Подпрограмма для вывода значение из массива в консоль (в частности полученного в результате работы программы массива B)
# Параметры:  
# a0 - адрес массива, данные которого надо вывести
# a1 - размер массива
# a2 - указатель на строку с сообщением вывода
# Возвращаемое значение: 
# a0 - адресс переданного  массива
func_output_array_data:
	# Инициализация переменных на стэке
	addi sp sp -8
	sw ra 4(sp)
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
	
	la a0 nxt
	li a7 4
	ecall
	
	addi t2 t2 4
	addi t1 t1 1
	blt t1 a1 for_output_loop
	end_output_loop:
	
	# Сохранение возвращаемых значений, воссатновление сохраненых на стеке значений и возврат указателя
	lw a0 (sp)
	lw ra 4(sp)
	addi sp sp 8
	jalr zero 0(ra)
	

# Метод для завершения работы программы
exit_program:
	li a7 10
	ecall
