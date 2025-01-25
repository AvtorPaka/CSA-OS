#------------------------------------------------
# Макрос-обертка над попдрограммой копирования символов строки
.macro strncpy(%dest, %source, %num)
	la a0 %dest
	la a1 %source
	mv a2 %num
	jal strncpy
.end_macro
#------------------------------------------------

# Макрос для упрощения выполнения тестов с локальными data строками
.macro testStrncpy(%source, %num)
	newline()
	newline()
	
	print_string(cur_test_string)
	print_string(%source)
	newline()
	print_string(cur_test_num)
	li a2 %num
	print_int(a2)
	newline()
	
	
	strncpy(destination_string_buffer, %source, a2)
	
	print_string(copied_string_ouput)
	print_string(destination_string_buffer)
.end_macro


# Макрос для вывода целого числа в консоль
#Параметры:
# %num - регистр, значение которого будет выведено в консоль
.macro print_int(%num)
	stack_push_w(a0)
	
	mv a0 %num
	li a7 1
	ecall
	
	stack_pop_w(a0)
.end_macro

# Макрос для вывода строки в консоль
#Параметры:
# %str - лейбл с строкой, значение которой будет выведено в консоль
.macro print_string(%str)
	stack_push_w(a0)
	
	la a0 %str
	li a7 4
	ecall
	
	stack_pop_w(a0)
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

# Макрос для завершения работы программы
.macro exit_program
	li a7, 10
	ecall
.end_macro

# Макрос для выброса исключения валидации параметра m функции и завершения программы
.macro throw_num_arg_validation_error()
.data
validate_error: .asciz "NumValidationException -> Num of chars to copy must be greater or equal to zero"
.text	
	la a0 validate_error
	li a1 1
	li a7 55
	ecall

	print_string(validate_error)
	
	exit_program
.end_macro

# Макрос для сохраенения 32-битного слова на стек
# Параметры:
# %x - регистр, значение которого сохранится на стек
.macro stack_push_w(%x)
	addi	sp sp -4
	sw	%x 0(sp)
.end_macro

# Макрос для снятия 32-битного слова со стека
# Параметры:
# %x - регистр, в который будет записано значение со стека
.macro stack_pop_w(%x)
	lw	%x (sp)
	addi	sp sp 4
.end_macro