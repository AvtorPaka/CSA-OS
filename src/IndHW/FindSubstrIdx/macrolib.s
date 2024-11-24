#------------------------------------------------
# Макрос-обертка над попдрограммой копирования символов строки
.macro strncpy(%dest, %source, %num)
	mv a0 %dest
	mv a1 %source
	mv a2 %num
	jal strncpy
.end_macro
#------------------------------------------------

#------------------------------------------------
# Макрос-обертка над попдрограммой загрузки данных из файла
.macro load_from_file_func(%pathB, %pathS, %dataB, %dataS)
	mv a0 %pathB
	mv a1 %pathS
	mv a2 %dataB
	mv a3 %dataS
	jal load_from_file
.end_macro
#------------------------------------------------

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

# Макрос для выброса исключения валидации параметра num функции strncpy и завершения программы
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

.macro throw_invalid_file_path_error()
.data
path_error: .asciz "InvalidFilePathException -> Unable to resolve file."
.text
	la a0 path_error
	li a1 1
	li a7 55
	ecall
	
	print_string(path_error)
	
	exit_program
.end_macro

.macro throw_read_error()
.data
read_error: .asciz "ReadError -> Unable to read from file."
.text
	la a0 read_error
	li a1 1
	li a7 55
	ecall
	
	print_string(read_error)
	
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

#-------------------------------------------------------------------------------
# Ввод строки в буфер заданного размера с заменой перевода строки нулем
# %strbuf - регистр с адерсом буфера
# %size - регистр с константой ограничивающей размер вводимой строки
.macro str_get(%strbuf, %size)
    stack_push_w(a0)
    mv      a0 %strbuf
    mv      a1 %size
    li      a7 8
    ecall
    stack_push_w(s0)
    stack_push_w(s1)
    stack_push_w(s2)
    li	s0 '\n'
    mv	s1	%strbuf
next:
    lb	s2  (s1)
    beq s0	s2	replace
    addi s1 s1 1
    b	next
replace:
    sb	zero (s1)
    stack_pop_w(s2)
    stack_pop_w(s1)
    stack_pop_w(s0)
    stack_pop_w(a0)
.end_macro

#-------------------------------------------------------------------------------
# Открытие файла для чтения, записи, дополнения
.eqv READ_ONLY	0	# Открыть для чтения
.eqv WRITE_ONLY	1	# Открыть для записи
.eqv APPEND	    9	# Открыть для добавления
.macro open_and_validate(%file_name, %opt)
    li   	a7 1024     	# Системный вызов открытия файла
    mv      a0 %file_name   # Имя открываемого файла
    li   	a1 %opt        	# Открыть для чтения (флаг = 0)
    ecall             		# Дескриптор файла в a0 или -1)
    
    stack_push_w(t1)
    li t1 -1
    bne a0 t1 end_if_invalid_path
    if_invalid_path:
    	throw_invalid_file_path_error()
    end_if_invalid_path:
    stack_pop_w(t1)
.end_macro

#-------------------------------------------------------------------------------
# Чтение информации из открытого файла
.macro read(%file_descriptor, %strbuf, %size)
    li   a7, 63       	# Системный вызов для чтения из файла
    mv   a0, %file_descriptor       # Дескриптор файла
    la   a1, %strbuf   	# Адрес буфера для читаемого текста
    li   a2, %size 		# Размер читаемой порции
    ecall             	# Чтение
.end_macro

#-------------------------------------------------------------------------------
# Чтение информации из открытого файла,
# когда адрес буфера в регистре
.macro read_addr_reg(%file_descriptor, %reg, %size)
    stack_push_w(a1)
    stack_push_w(a2)
    li   a7, 63       	# Системный вызов для чтения из файла
    mv   a0, %file_descriptor       # Дескриптор файла
    mv   a1, %reg   	# Адрес буфера для читаемого текста из регистра
    mv   a2, %size 		# Размер читаемой порции
    ecall             	# Чтение
    stack_pop_w(a2)
    stack_pop_w(a1)
.end_macro

#-------------------------------------------------------------------------------
# Закрытие файла
.macro close(%file_descriptor)
    li   a7, 57       # Системный вызов закрытия файла
    mv   a0, %file_descriptor  # Дескриптор файла
    ecall             # Закрытие файла
.end_macro

#-------------------------------------------------------------------------------
# Выделение области динамической памяти заданного размера
.macro allocate(%size)
    li a7, 9
    mv a0, %size	# Размер блока памяти
    ecall
.end_macro
