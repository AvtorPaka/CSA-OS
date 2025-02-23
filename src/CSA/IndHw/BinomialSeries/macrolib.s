.data
input_m_func_param: .asciz "Input m variable value for binomial function: "
invalid_m_func_param: .asciz "ValidationException.\nInvalid m variable value -> m must be >= 0"
undefined_function_value: .asciz "Undefined function result."
input_x_func_param: .asciz "Input x variable value for binomial function: "
output_function_result: .asciz "Function result: "
test_separator: .asciz "------------------------------------"
output_x_string: .asciz "Current x value: "
output_m_string: .asciz "Current m value: "

#Макрос для ввода аргументов биномиальной функции
#Параметры :
# %x - f регистр, куда будет записан x параметр при успешном вводе
# %m - регистр, куда будет записанн m параметр при успешном вводе
# Включает валидацию параметров, читайте описание макроса validate_function_variables
.macro input_function_parameters(%x %m)
	print_string(input_m_func_param)
	
	li a7 5
	ecall
	mv %m a0
	
	print_string(input_x_func_param)
	
	li a7 7
	ecall
	fmv.d %x fa0
	
	validate_function_variables(%x, %m)
.end_macro

#Макрос для валидации входных параметров
#Параметры :
# %x - параметр x биномиальной функции
# %m - параметр m биномиальной функции
# В случае недопустимых входных параметров (m < 0) программа завершается с ошибкой валидации
# В случае (m = 0, x = -1.00) программа завершается с undefined_function_value
# В случае (m = 0, x != -1.00) программа завершается с результатом функции 1.00
# В случае (m != 0, x = -1.00) програма завершается с результатом функции 0.00
# В ином случае ход программы продолжается
.macro validate_function_variables(%x, %m)
	# Проверка на валидность параметра m
	bgez %m valid_m
	invalid_m:
		throw_m_variable_validation_error()
	valid_m:
	
	
	create_double_value(ft0, -1)
	feq.d t1 %x ft0
	
	
	bnez %m end_if_m_zero
	if_m_zero:
		beqz t1 end_if_x_neg_1_sec
		if_x_neg_1_sec:				# m = 0 & x = -1.00
			print_string(undefined_function_value)
			exit_program
		end_if_x_neg_1_sec: 			# m = 0 & x != -1.00
			create_double_value(ft0, 1)
			print_function_result(ft0)
			exit_program
	end_if_m_zero:
		beqz t1 end_if_x_neg_1
		if_x_neg_1:				# m != 0 & x = -1.00
			create_double_value(ft0, 0)
			print_function_result(ft0)
			exit_program
		end_if_x_neg_1:				# m != 0 & x != -1.00
.end_macro

# Макрос для подсчета 64-битного факториала натурального числа
# Параметры:
# %num - параметр факториала
# %fr - fp регистр, куда будет записано значение факториала
.macro calculate_factotial(%num, %fr)
	stack_push_w(t2)
	stack_push_d(ft2)
	
	bnez %num if_num_not_zero
	if_num_zero:
		create_double_value(%fr 1)
		j end_if_num_zero
	if_num_not_zero:
		create_double_value(%fr 1)
		li t2 1
		
		#while (t2 <= num) {... t2++}
		while_factorial:
			fcvt.d.wu ft2 t2
			fmul.d %fr %fr ft2
			
			addi t2 t2 1
			bleu t2 %num while_factorial
		end_while_factorail:
				
	end_if_num_zero:
	
	stack_pop_d(ft2)
	stack_pop_w(t2)
.end_macro

#Макрос для подсчета биномиального коефицента - C из n по k
# Параметры:
# %n - n параметр биномиального коефицента
# %k - k параметр биномиального коефицента
# %fr - fp регистр, куда будет записано посчитанное значение
.macro calculate_binomial_coef(%n %k %fr)
	stack_push_w(t3)
	stack_push_d(ft3)
	stack_push_d(ft4)
	
	calculate_factotial(%n %fr)
	sub t3 %n %k
	calculate_factotial(%k ft3)
	calculate_factotial(t3 ft4)
	fmul.d ft3 ft3 ft4
	fdiv.d %fr %fr ft3
	
	stack_pop_d(ft4)
	stack_pop_d(ft3)
	stack_pop_w(t3)
.end_macro

# Макрос для подсчета степени (натуральной) дробного числа двойной точности
# Параметры:
# %fbase - число для возведения в степень
# %exp - показатель степени числа
# %fr - fp регистр, куда будет записано значение степени
.macro pow(%fbase %exp %fr)
	stack_push_w(t2)
	
	create_double_value(%fr 1)
	
	bnez %exp if_not_exp_zero
	if_exp_zero:
		j end_if_exp_zero
	if_not_exp_zero:
		li t2 0
		#while (t2 < %exp) {... t2++}
		while_pow_loop:
		fmul.d %fr %fr %fbase
		
		addi t2 t2 1
		blt t2 %exp while_pow_loop
		end_while_pow:
	end_if_exp_zero:
	
	stack_pop_w(t2)
.end_macro

#Макрос для вывода результатов работы биномиальной функции функции
# Параметры
# %fnum - десятичное число двойной точности, резултат функции
.macro print_function_result(%fnum)
	print_string(output_function_result)
	print_double_value(%fnum)
.end_macro

#Макрос для создания дробного числа двоичной точности(double) из целого числа
#Параметры:
# %fr - fp регистр, куда будет записано число
# %num - число, которое нужно преобразовать и записать
.macro create_double_value(%fr %num)
	stack_push_w(t0)
	
	li t0 %num
	fcvt.d.w %fr t0
	
	stack_pop_w(t0)
.end_macro

# Макрос для вывода дробного числа двоичной точности в консоль
#Параметры:
# %fr - fp регистр, значение которого будет выведено в консоль
.macro print_double_value(%fr)
	stack_push_d(fa0)

	fmv.d fa0 %fr
	li a7 3
	ecall
	
	stack_pop_d(fa0)
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

# Макрос для выброса исключения валидации параметра m функции и завершения программы
.macro throw_m_variable_validation_error()
	la a0 invalid_m_func_param
	li a1 1
	li a7 55
	ecall
	
	li a7 4
	ecall
	
	exit_program
.end_macro

# Макрос для вывода символа в консоль
#Параметры:
# %x - символ для вывода в консоль
.macro print_char(%x)
	stack_push_w(a0)
	
	li a7, 11
	li a0, %x
	ecall
	
	stack_pop_w(a0)
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

# Макрос для сохраенения 64-битного слова на стек
# Параметры:
# %fx - fp регистр, значение которого сохранится на стек
.macro stack_push_d(%fx)
	addi	sp sp -8
	fsd	%fx 0(sp)
.end_macro

# Макрос для снятия 64-битного слова со стека
# Параметры:
# %fx - fp регистр, в который будет записано значение со стека
.macro stack_pop_d(%fx)
	fld	%fx (sp)
	addi	sp sp 8
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

# Макрос для вывода разделителя в тестах
.macro print_test_separator()
	newline()
	print_string(test_separator)
	newline()
.end_macro

# Макрос для вывода параметров функции в тестах
# Параметры:
# %num - m параметр функции, целое число
# %fnum - x параметр функции, дробное число двоичной точности
.macro print_test_params(%num %fnum)
	print_string(output_x_string)
	print_double_value(%fnum)
	newline()
	print_string(output_m_string)
	print_int(%num)
.end_macro

#Макрос для генерации набора тестовых данных
#Параметры: 
# %arrx - ссылка на массив, куда будут записаны x параметры тестовых данных
# %arrm - ссылка на массив, куда будут записаны m параметры тестовых данных 
# %size - регистр, куда будет записан размер массивов тестовых данных
.macro generate_test_data(%arrx %arrm %size)
.data
test_x_1: .double 1.00
test_x_2: .double -2.00
test_x_3: .double 54.124
test_x_4: .double 11.78
test_x_5: .double 2.5
test_x_6: .double 1786.444
test_x_7: .double -57.22
.text
	stack_push_w(%arrx)
	stack_push_w(%arrm)
	stack_push_w(t3)
	stack_push_w(t2)
	stack_push_w(t1)
	stack_push_d(ft0)
	
	mv t1 %arrx
	mv t2 %arrm
	
	# Первый тестовый набор
	fld ft0 test_x_1 t3
	fsd ft0 0(t1)
	
	li t3 14
	sw t3 0(t2)
	
	addi t1 t1 8
	addi t2 t2 4
	
	# Второй тестовый набор
	fld ft0 test_x_2 t3
	fsd ft0 0(t1)
	
	li t3 19
	sw t3 0(t2)
	
	addi t1 t1 8
	addi t2 t2 4
	
	# Третий тестовый набор
	fld ft0 test_x_3 t3
	fsd ft0 0(t1)
	
	li t3 3
	sw t3 0(t2)
	
	addi t1 t1 8
	addi t2 t2 4
	
	# Четвертый тестовый набор
	fld ft0 test_x_4 t3
	fsd ft0 0(t1)
	
	li t3 5
	sw t3 0(t2)
	
	addi t1 t1 8
	addi t2 t2 4
	
	# Пятый тестовый набор
	fld ft0 test_x_5 t3
	fsd ft0 0(t1)
	
	li t3 9
	sw t3 0(t2)
	
	addi t1 t1 8
	addi t2 t2 4
	
	# Шестой тестовый набор
	fld ft0 test_x_6 t3
	fsd ft0 0(t1)
	
	li t3 2
	sw t3 0(t2)
	
	addi t1 t1 8
	addi t2 t2 4
	
	# Седьмой тестовый набор
	fld ft0 test_x_7 t3
	fsd ft0 0(t1)
	
	li t3 3
	sw t3 0(t2)

	
	li %size 7
	stack_pop_d(ft0)
	stack_pop_w(t1)
	stack_pop_w(t2)
	stack_pop_w(t3)
	stack_pop_w(%arrm)
	stack_pop_w(%arrx)
.end_macro