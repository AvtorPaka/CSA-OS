.eqv BUFFER_SIZE 512
.eqv INTTOSTR_BUFF_SIZE 16

.data
data_buffer: .space BUFFER_SIZE
int_str_buffer: .space INTTOSTR_BUFF_SIZE
input_paths_array: .space 28
output_paths_array: .space 28
test_substr_array: .space 28
output_cur_substr: .asciz "Test substring: "
output_test_num: .asciz "Test number: "
output_test_input_file: .asciz "Test input file: "
output_test_output_file: .asciz "Test output file: "
test_delimetr: .asciz "-------------------------------------|"

.include "../macrolib.s"

.global main

.text
main:
	# Вызов макроса формирования тестовых данных
	initialize_tests_data(input_paths_array, test_substr_array, output_paths_array)
	
	mv s11 zero	# Счетчик иттераций по массивам тестовых данных
	li s10 5	# Количество тестов
	mv s9 zero	# Временная переменная для хранения строки текущего пути к input файлу
	mv s8 zero 	# Временная переменная для хранения текущей подстроки для теста
	mv s7 zero	# Временная переменная для хранения строки текущего пути к output файлу
	la s6 input_paths_array
	la s5 output_paths_array
	la s4 test_substr_array
	li s3 256
	mv s2 zero 	# Переменная для хранения записанных данных на хипе
	mv s1 zero	# Переменная для хранения адреса памяти с массивом индексов / адреса памяти с строкой-массивом
	mv s0 zero	# Переменная для хранения размера полученного массива индексов / размера строки-массива в байтах
	
	print_string(test_delimetr)
	newline()
	# for (s11 = 0; s11 < s10; ++s11) {...}
	for_test_loop:
		lw s9 (s6)
		lw s8 (s4)
		lw s7 (s5)
		
		#Вывод инфорации о текущем тесте в консоль
		print_string(output_test_num)
		mv a0 s11
		addi a0 a0 1
		li a7 1
		ecall
		newline()
		print_string(output_test_input_file)
		mv a0 s9
		li a7 4
		ecall
		newline()
		print_string(output_cur_substr)
		mv a0 s8
		li a7 4
		ecall
		newline()
		print_string(output_test_output_file)
		mv a0 s7
		li a7 4
		ecall
		newline()
		print_string(test_delimetr)
		newline()
		
		# Вызов подпрограммы (обернутой в макрос) записи данных файла в память для теста
		la a2 data_buffer
		li a3 BUFFER_SIZE
		load_from_file_func(s9, s3, a2, a3)
		# Возвращаемое значение:
		mv s2 a0	# адрес начала области памяти данных на хипе
		
		# Вызов подпрограммы (обернутой в макрос) для поиска индексов
		find_substr_idx_func(s2, s8)
		# Возвращаемые значения:
		mv s1 a0	# адрес массива индексов
		mv s0 a1	# размер массива индексов
		
		blez s0 if_empty_array
		if_not_empty_array:
			
			# Вызов подпрограммы (обернутой в макрос) перевода массива 32-битных чисел-индексов в строку
			la a2 int_str_buffer
			convert_array_to_string_func(s1, s0, a2)
			# Возвращаемые значения:
			mv s1 a0	# адрес начала строки-массива
			mv s0 a1	# размер строки-массива в байтах
			addi s0 s0 -1	# убираем null-char для записи в файл
			
			# Вызов подпрограммы (обернутой в макрос) записи полученных данных в виде строки в файл
			upload_to_file_func(s7, s3, s1, s0)
		
		if_empty_array:
		
		addi s5 s5 4
		addi s4 s4 4
		addi s6 s6 4
		addi s11 s11 1
		blt s11 s10 for_test_loop
	end_for_test_loop:
	
	exit_program()
