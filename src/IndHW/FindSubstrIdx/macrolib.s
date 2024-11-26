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


#------------------------------------------------
# Макрос-обертка над попдрограммой ввода файла и  загрузки данных из файла
.macro load_and_input_from_file_func(%pathB, %pathS, %dataB, %dataS)
.data
input_load_file_path: .asciz "Input path to the file to load data from: "
.text
	mv a0 %pathB
	mv a1 %pathS
	
	#print_string(input_load_file_path)
	# a0 - адрес буффера для хранения пути к файлу
	# a1 - максимальный размер пути к файлу
	#str_get(a0, a1)		# Вызов макроса записи пути к файлу в соответсвующий буффер
	str_get_dialog(input_load_file_path, %pathB, %pathS)
	
	mv a2 %dataB
	mv a3 %dataS
	jal load_from_file
.end_macro
#------------------------------------------------

#------------------------------------------------
# Макрос-обертка над попдрограммой загрузки данных в файл
.macro upload_to_file_func(%pathB, %pathS, %dataB, %dataS)
.text
	mv a0 %pathB
	mv a1 %pathS
	mv a2 %dataB
	mv a3 %dataS
	jal upload_to_file
.end_macro
#------------------------------------------------

#------------------------------------------------
# Макрос-обертка над попдрограммой загрузки данных в файл и ввода пути к файлу
.macro upload_to_file_input_func(%pathB, %pathS, %dataB, %dataS)
.data
input_upload_file_path: .asciz "\nInput path to the file to write data to: "
.text
	mv a0 %pathB
	mv a1 %pathS
	
	#print_string(input_upload_file_path)
	# a0 - адрес буффера для хранения пути к файлу
	# a1 - максимальный размер пути к файлу
	#str_get(a0, a1)		# Вызов макроса записи пути к файлу в соответсвующий буффер
	str_get_dialog(input_upload_file_path, %pathB, %pathS)
	
	mv a2 %dataB
	mv a3 %dataS
	jal upload_to_file
.end_macro
#------------------------------------------------

#------------------------------------------------
# Макрос-обертка над попдрограммой для поиска индексов первого символа для всех вхождений подстроки в строку
.macro find_substr_idx_func(%data, %subtr)
	mv a0 %data
	mv a1 %subtr
	jal find_substr_idx
.end_macro
#------------------------------------------------


#------------------------------------------------
# Макрос-обертка над попдрограммой вывода резульатов в консоль
.macro upload_to_console_func(%arr, %size)
.data
idx_output: .asciz "\nFound indexes of first char of substring:\n"
.text
	print_string(idx_output)
	mv a0 %arr
	mv a1 %size
	jal upload_to_console
.end_macro
#------------------------------------------------

#------------------------------------------------
# Макрос-обертка над попдрограммой для перевода массива 32-битных чисел в строку
.macro convert_array_to_string_func(%arr, %size, %buff)
	mv a0 %arr
	mv a1 %size
	mv a2 %buff
	jal convert_array_to_string
.end_macro
#------------------------------------------------

# Макрос для выбора пользователем способа вывода полученных данных в файл или консоль
# Выходные данные:
# a0 - 0 при выводе в консоль, 1 при выводе в файл
.macro chose_output()
.data
choose_output_impl: .asciz "Would you like to additionaly output found indexes to the console?"
.text
	stack_push_w(t0)
	
	while_bad_char_loop:
		li a7 50
		la a0 choose_output_impl
		ecall
		
		li t0 1
		beqz a0 if_y
		beq a0 t0 if_n
		j if_bad_char	
		if_n:
			li a0 1
			j end_bch_loop
		if_y:
			li a0 0
			j end_bch_loop
			
		if_bad_char:
		
		j while_bad_char_loop	
	end_bch_loop:
	
	stack_pop_w(t0)
.end_macro


.macro strlen(%str)
    stack_push_w(t0)
    stack_push_w(t1)
    li      t0 0
    mv a0 %str
loop_len:
    lb      t1 (a0)
    beqz    t1 end_loop_len
    addi    t0 t0 1
    addi    a0 a0 1
    b       loop_len
end_loop_len:
    mv      a0 t0
    stack_pop_w(t1)
    stack_pop_w(t0)
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

.macro next_space()
	print_char(';')
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

# Макрос для ввода подстроки для поиска в тексте
.macro input_substring(%strbuf, %size)
.data
input_substring_text: .asciz "Input substring to search for: "
.text
	#print_string(input_substring_text)
	str_get_dialog(input_substring_text, %strbuf, %size)
.end_macro

#-------------------------------------------------------------------------------
# Ввод строки в буфер заданного размера с заменой перевода строки нулем
# %strbuf - регистр с адерсом буфера
# %size - регистр с 32-битным числом, ограничивающим размер вводимой строки
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
# Ввод строки в буфер заданного размера через диалговое окно с заменой перевода строки нулем
# %dialogString - лейб строки для вывода пользователем в окно
# %strbuf - регистр с адерсом буфера
# %size - регистр с 32-битным числом, ограничивающим размер вводимой строки
.macro str_get_dialog(%dialogString, %strbuf, %size)
    stack_push_w(a0)
    stack_push_w(a1)
    stack_push_w(a2)
    stack_push_w(s0)
    stack_push_w(s1)
    stack_push_w(s2)
    mv s1 %strbuf
    mv s2 %size
    
    while_cancel_or_empty:
	    la      a0 %dialogString
	    mv      a1 s1
	    mv	    a2 s2
	    li      a7 54
	    ecall
	    
	    bnez a1 while_cancel_or_empty
    end_while_coe:
        
    li	s0 '\n'
next:
    lb	s2  (s1)
    beq s0	s2	replace
    addi s1 s1 1
    b next
replace:
    sb	zero (s1)
    
    stack_pop_w(s2)
    stack_pop_w(s1)
    stack_pop_w(s0)
    stack_pop_w(a2)
    stack_pop_w(a1)
    stack_pop_w(a0)
.end_macro

#-------------------------------------------------------------------------------
# Открытие файла для чтения, записи, дополнения
.eqv READ_ONLY	0	# Открыть для чтения
.eqv WRITE_ONLY	1	# Открыть для записи
.eqv APPEND	    9	# Открыть для добавления
.macro open_and_validate(%file_name, %opt)
    stack_push_w(a1)
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
    stack_pop_w(a1)
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
    mv a0, %size
    ecall
.end_macro

# Макрос для формирования набора тестовых данных
# Входные параметры:
# %inputArray - адрес начала области памяти для записи указателей на входные пути файлов
# %substrArray - адрес начала области памяти для записи подстрок поиска для тестов
# %outputArray - адрес начала области памяти для записи указателей на выходные пути файлов
# Выходные параметры: нет
.macro initialize_tests_data(%inputArray, %substrArray, %outputArray)
.data
test_input_path_1: .asciz "data/input/1.txt"
test_input_path_2: .asciz "data/input/2.txt"
test_input_path_3: .asciz "data/input/3.txt"
test_input_path_4: .asciz "data/input/4.txt"
test_input_path_5: .asciz "data/input/5.txt"
test_substr_1: .asciz "test"
test_substr_2: .asciz "талант"
test_substr_3: .asciz "а"
test_substr_4: .asciz "jtof"
test_substr_5: .asciz "Navy SEAL"
test_output_path_1: .asciz "data/output/1.txt"
test_output_path_2: .asciz "data/output/2.txt"
test_output_path_3: .asciz "data/output/3.txt"
test_output_path_4: .asciz "data/output/4.txt"
test_output_path_5: .asciz "data/output/5.txt"
.text
	stack_push_w(s1)
	stack_push_w(s2)
	stack_push_w(s3)
	stack_push_w(s4)
	stack_push_w(s5)
	la s1 %inputArray
	la s2 %substrArray
	la s3 %outputArray
	
	# Первый тест
	la s4 test_input_path_1
	sw s4 (s1)
	addi s1 s1 4
	la s4 test_substr_1
	sw s4 (s2)
	addi s2 s2 4
	la s4 test_output_path_1
	sw s4 (s3)
	addi s3 s3 4
	
	# Второй тест
	la s4 test_input_path_2
	sw s4 (s1)
	addi s1 s1 4
	la s4 test_substr_2
	sw s4 (s2)
	addi s2 s2 4
	la s4 test_output_path_2
	sw s4 (s3)
	addi s3 s3 4
	
	# Третий тест
	la s4 test_input_path_3
	sw s4 (s1)
	addi s1 s1 4
	la s4 test_substr_3
	sw s4 (s2)
	addi s2 s2 4
	la s4 test_output_path_3
	sw s4 (s3)
	addi s3 s3 4
	
	# Четвертый тест
	la s4 test_input_path_4
	sw s4 (s1)
	addi s1 s1 4
	la s4 test_substr_4
	sw s4 (s2)
	addi s2 s2 4
	la s4 test_output_path_4
	sw s4 (s3)
	addi s3 s3 4
	
	# Пятый тест
	la s4 test_input_path_5
	sw s4 (s1)
	la s4 test_substr_5
	sw s4 (s2)
	la s4 test_output_path_5
	sw s4 (s3)
	
	
	stack_pop_w(s5)
	stack_pop_w(s4)
	stack_pop_w(s3)
	stack_pop_w(s2)
	stack_pop_w(s1)
.end_macro
