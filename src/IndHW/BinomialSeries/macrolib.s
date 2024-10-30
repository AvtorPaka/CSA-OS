.data
input_m_func_param: .asciz "Input m variable value for binomial function: "
invalid_m_func_param: .asciz "ValidationException.\nInvalid m variable value -> m must be >= 0"
undefined_function_value: .asciz "Undefined function result."
input_x_func_param: .asciz "Input x variable value for binomial function: "
output_function_result: .asciz "Function result: "
nxt: .asciz " "

#Макрос для ввода аргументов биномиальной функции
#Параметры :
# %x - f регистр, куда будет записан x параметр при успешном вводе
# %m - регистр, куда будет записанн m параметр при успешном вводе
# Включает валидацию параметров, считайте описание макроса validate_function_variables
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

#Макрос для вывода результатов работы биномиальной функции функции
# Параметры
# %fnum - десятичное число двойной точности, резултат функции
.macro print_function_result(%fnum)
	print_string(output_function_result)
	print_double_value(%fnum)
.end_macro

#Макрос для создания дробного числа двоичной точности(double) из целого числа
#Параметры:
# %fr - f регистр, куда будет записано число
# %num - число, которое нужно преобразовать и записать
.macro create_double_value(%fr %num)
	stack_push_w(t0)
	
	li t0 %num
	fcvt.d.w %fr t0
	
	stack_pop_w(t0)
.end_macro

# Макрос для вывода дробного числа двоичной точности в консоль
.macro print_double_value(%fr)
	stack_push_d(fa0)

	fmv.d fa0 %fr
	li a7 3
	ecall
	
	stack_pop_d(fa0)
.end_macro

# Макрос для вывода строки в консоль
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

# Макрос для вывода символа пробела в консоль
.macro next
	la a0 nxt
	li a7 4
	ecall
.end_macro
	
# Макрос для завершения работы программы
.macro exit_program
	li a7, 10
	ecall
.end_macro

# Макрос для сохраенения 64-битного слова на стек
.macro stack_push_d(%fx)
	addi	sp sp -8
	fsd	%fx 0(sp)
.end_macro

# Макрос для снятия 64-битного слова со стека
.macro stack_pop_d(%fx)
	fld	%fx (sp)
	addi	sp sp 8
.end_macro

# Макрос для сохраенения 32-битного слова на стек
.macro stack_push_w(%x)
	addi	sp sp -4
	sw	%x 0(sp)
.end_macro

# Макрос для снятия 32-битного слова со стека
.macro stack_pop_w(%x)
	lw	%x (sp)
	addi	sp sp 4
.end_macro