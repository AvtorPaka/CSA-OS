.eqv BUFFER_SIZE 512

.data
source_string_buffer: .space BUFFER_SIZE
destination_string_buffer: .space BUFFER_SIZE
input_string: .asciz "Input string to copy: "
check_input_string: .asciz "Given string: "
input_copy_num: .asciz "Input num of chars to copy: "
copied_string_ouput: .asciz "Copied string: "
cur_test_string: .asciz "Test string to copy: "
cur_test_num: .asciz "Number of chars to copy: "
data_tests: .asciz "Asciz data tests:"
empty_test_string: .asciz ""
hello_test_string: .asciz "Hello copied world!"
tricky_test_string: .asciz "I'm happy and grateful that i'm not a copy"
small_test_string: .asciz "I to large for current buffer file. Just kidding"



.include "macrolib.s"

.global main

.text
main:	
	# Ввод строки для копирования
	print_string(input_string)
	la a0 source_string_buffer
	li a1 BUFFER_SIZE
	li a7 8
	ecall
	
	#Ввод количества символов для копирования
	print_string(input_copy_num)
	li a7 5
	ecall
	mv a2 a0
	
	#Вызов функции strncpy c полученными параметрами
	# Передаваемые параметры:
	strncpy(destination_string_buffer, source_string_buffer, a2)
	
	print_string(copied_string_ouput)
	print_string(destination_string_buffer)
	
	#Тесты с предоставленными данными
	newline()
	print_string(data_tests)
	
	testStrncpy(empty_test_string, 5)
	testStrncpy(hello_test_string, 12)
	testStrncpy(tricky_test_string, 22)
	testStrncpy(small_test_string, 120)
	
	
	exit_program()
