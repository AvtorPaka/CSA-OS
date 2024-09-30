.data
choose_implementation_input: .asciz "Select the implementation to use.\nEnter 0 for a recursive implementation,\nfor an iterative implementation, enter any other number: "
output_result: .asciz "Max factorial argument(a0): "

.text
main:	
	jal ra input_choose_implementation						# Выбор пользователем варианта имплементации алгоритма
	
	beqz a0 if_recursive
	if_iterative:
		jal ra calculate_max_factorial_argument_iterative	# Вызов итеративной имплементации
		j end_if_implementation
	if_recursive:
		li s0 2147483647									# INT32_MAXVALUE
		li a2 1												# Изначальное значение для текущего факториала внутри вызова рекурсивной функции
		li a3 1												# Изначальное значение для текущего аргумента факториала внутри вызова рекурсивной функции
		addi sp sp -8
		sw a2 4(sp)
		sw a3 0(sp)
		jal ra calculate_max_factorial_argument_recursive	#Вызов рекурсивной имплементации
	end_if_implementation:
	
	j output_result_func


# Метод для посчёта максимального аргумента факториала int32_t иттеративным способом
calculate_max_factorial_argument_iterative:
	li s0 2147483647
	li t0 1													# Текущее значение факториала
	li t1 0													# [s0/t0] - целая часть множителя до переполнения
	li a0 1													# Текущий аргумент факториала
	
	#while (a0 <= t1) {... a0++}
	while:
	mul t0 t0 a0
	
	div t1 s0 t0
	addi a0 a0 1
	bleu a0 t1 no_overflow_iter
	overflow_iter:
		addi a0 a0 -1
		j end_while
	no_overflow_iter:
		j while
	end_while:
	
	jalr zero, 0(ra)


# Метод для посчёта максимального аргумента факториала int32_t рекурсивным способом 
calculate_max_factorial_argument_recursive:
	lw a2 4(sp)
	lw a3 0(sp)
	addi sp sp 8
	
	addi sp sp -4
	sw ra (sp)
	
	mul a2 a2 a3
	div t0 s0 a2
	addi a3 a3 1
	blt t0 a3 done_calculate_max_factorial_argument_recursive
	
	addi sp sp -8
	sw a2 4(sp)
	sw a3 0(sp)
	
	jal ra calculate_max_factorial_argument_recursive
	
	
done_calculate_max_factorial_argument_recursive:
	addi a0 a0 1
	addi sp sp -4
	sw zero (sp)
	addi sp sp 4
	lw ra (sp)
	addi sp sp 4
	jalr zero, 0(ra)

# Метод для выбора применяемой имплементации подсчёта аргумента
input_choose_implementation:
	la a0 choose_implementation_input
	li a7 4
	ecall
	
	li a7 5
	ecall
	
	jalr zero 0(ra)

# Метод для вывода результата работы программы (a0) в консоль эмулятора		
output_result_func:
	mv a1 a0

	la a0 output_result
	li a7 4
	ecall
	
	mv a0 a1
	li a7 1
	ecall
	
	j exit_program

# Метод для очистки используемых регистров и завершения программы
exit_program:
	#Очистка используемых регистров
	mv a1 zero
	mv t0 zero
	mv t1 zero
	mv s0 zero
	
	li a7 10
	ecall
