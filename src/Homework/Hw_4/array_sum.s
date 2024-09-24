.data
array: .space 40
enter_size: .asciz "Input array size (1-10): "
enter_array_nums: .asciz "Input array data:\n"
ArraySizeOutOfRangeException: .asciz "ArraySizeOutOfRangeException -> array size must be 1<= _size <= 10"
SumOverflowExpecption: .asciz "SumOverflowExpecption -> Sum of array elements overflows 32-bits signed integer. Calculation stops.\n"
output_sum: .asciz "Sum of elements in array: "
output_even: .asciz "Number of even values: "
output_odd: .asciz "Number of odd values: "
exception_sum: .asciz "Sum of elements in array until overflow: "
exception_cnt: .asciz "Number of summed elemets in array unitl overflow: "
nxt: .asciz "\n"

.text
main:	
	#Ввод размера массива
	la a0 enter_size
	li a7 4
	ecall
	li a7 5
	ecall
	mv t0 a0
	
	jal ra check_array_size			# Проверка введенной длинны массива
	jal ra fill_array 				# Запись чисел в память(массив)
	jal ra calculate_sum			# Подсчет суммы элементов массива
	
	beqz s2 throw_sum_overflow_exception	#Выборос исключения и соотвествующей информации при переполнении
	j f_ouput_sum							#Вывод полученных значений

#Метод для вычисления суммы элементов в массиве (памяти)
#Подсчёт
calculate_sum:
	li s0 2147483647		# Верхняя граница int32_t
	li s1 -2147483648		# Нижняя граница int32_t
	li s2 1					# Флаг для переполнения (нужен для дальнейшей итерации по массиву и подсчёта чет/нечет чисел)
	li t3 0					# Счетчик кол-ва нечётных чисел
	li t4 0					# Кол-во просумированных элементов
	li s3 0					# Сумма элементов массива
	la t6 array	
	li t2 0	
	
	# for (int t2 = 0; t2 < t0; ++t2) {...}
	for_sum_loop:
	lw t1 0(t6)
	
	#Проверка на переполнение и суммирование в случае непереполнения
	beqz s2 end_cont_sum
	continue_sum:
		#Внутренняя проверка на переполнение если добавить текущее число - t1
		blez s3 sum_negative
		#Проверка на верхнюю границу
		sum_positive:
			mv s6 s0
			sub s6 s6 s3
		
			ble t1 s6 end_if_ov_p
			if_overflow_positive:
				addi s2 s2 -1
			end_if_ov_p:
			mv s6 zero
			j end_sum_diff
		
		#Проверка на нижнюю границу
		sum_negative:
			mv s6 s1
			sub s6 s6 s3
			
			bge t1 s6 end_if_ov_neg
			if_overflow_negative:
				addi s2 s2 -1
			end_if_ov_neg:
			mv s6 zero
			j end_sum_diff

		end_sum_diff:
		
		#Суммирование в случае если переполнения не случилось
		#Иначе сохраняем кол-во просуммированных элементов и поднимаем флаг переполнения
		bnez s2 end_oc
		overfloaw_case:
			mv t4 t2
			j end_cont_sum
		end_oc:
			add s3 s3 t1
		
	end_cont_sum:
	
	#Подсчет кол-ва четных/нечетных чисел
	andi s7 t1 1
	beq zero s7 even
	odd:
	addi t3 t3 1
	even:
	
	addi t6 t6 4
	addi t2 t2 1
	blt t2 t0 for_sum_loop
	end_sum_for_loop:
	
	jalr zero 0(ra)

#Метод записи введеных в консоль значений в массив
fill_array:
	la a0 enter_array_nums
	li a7 4
	ecall
	
	mv t2 zero
	la t6 array
	
	# for (int t2 = 0; t2 < t0; ++t2) {...}
	for_loop:
	addi t2 t2 1
	
	#Ввод числа в регистр t1
	li a7 5
	ecall
	mv t1 a0
	
	#Запись числа в память(массив)
	sw t1, 0(t6)
	addi t6 t6 4
	blt t2 t0 for_loop
	end_loop:
	
	mv t1 zero
	jalr zero, 0(ra)
	
#Метод проверки размера массива на соответствия условиям
check_array_size:
	blez t0 throw_out_of_range_exception
	li t1 10
	bgt t0 t1 throw_out_of_range_exception
	mv t1 zero
	
	jalr zero, 0(ra)

#Метод выброса исключения при переполнении суммы чисел в массиве и вывода соответствующей информации
throw_sum_overflow_exception:
	la a0 SumOverflowExpecption
	li a1 0
	li a7 55
	ecall
	
	li a7 4
	ecall
	
	la a0 exception_sum
	li a7 4
	ecall 
	
	mv a0 s3
	li a7 1
	ecall
	
	la a0 nxt
	li a7 4
	ecall
	
	la a0 exception_cnt
	li a7 4
	ecall
	
	mv a0 t4
	li a7 1
	ecall
	
	la a0 nxt
	li a7 4
	ecall
	
	j output_odd_even

# Метод для вывода полученной суммы	
f_ouput_sum:
	la a0 output_sum
	li a7 4
	ecall 
	
	mv a0 s3
	li a7 1
	ecall
	
	la a0 nxt
	li a7 4
	ecall
	
	j output_odd_even

#Метод для вывода кол-ва четных и нечетных чисел в массиве
output_odd_even:
	la a0 output_odd
	li a7 4
	ecall 
	
	mv a0 t3
	li a7 1
	ecall
	
	la a0 nxt
	li a7 4
	ecall
	
	sub t3 t0 t3
	
	la a0 output_even
	li a7 4
	ecall 
	
	mv a0 t3
	li a7 1
	ecall
	
	j exit_program

#Метод для проброса исключения при введении неверного размера массива
throw_out_of_range_exception:
	la a0 ArraySizeOutOfRangeException
	li a1 0
	li a7 55
	ecall
	
	li a7 4
	ecall
	j exit_program

#Метод завершения работы программы
exit_program:
	#Очистка использованных регистров
	mv ra zero
	mv t0 zero
	mv t1 zero
	mv t2 zero
	mv t3 zero
	mv t6 zero
	mv s0 zero
	mv s1 zero
	mv s2 zero
	mv s3 zero
	mv s7 zero
	mv a0 zero
	
	li a7 10
	ecall
