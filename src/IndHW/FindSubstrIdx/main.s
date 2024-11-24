.eqv BUFFER_SIZE 512
.eqv PATH_SIZE 256
.eqv SUBSTRING_SIZE 256

.data
data_buffer: .space BUFFER_SIZE
path_buffer: .space PATH_SIZE
substr_buffer: .space SUBSTRING_SIZE


.include "macrolib.s"

.global main

.text
main:
	# Вызов подпрограммы (обернутой в макрос) для чтения данных из файла в память на куче с помощью скользящего окна
	# Передаваемые параметры:
	la a0 path_buffer	# %pathB (a0) - буффер для хранения пути к файлу
	li a1 PATH_SIZE		# %pathS (a1) - максимальный размер пути к файлу (размер буффера)
	la a2 data_buffer	# %dataB (a2) - буффер для хранения части текста при считывании
	li a3 BUFFER_SIZE	# %dataS (a3) - максимальный размер буффера данных (512)

	load_from_file_func(a0, a1, a2, a3)
	# Возвращаемое значение:
	# a0 - адрес памяти на хипе, куда были записаны данные
	
	mv s1 a0
	
	# Вызов макроса для ввода подстроки поиска
	# Передаваемые параметры:
	la a0 substr_buffer	# %strbuf (a0) - буффер, куда будет записана строка
	li a1 SUBSTRING_SIZE	# %size (a1) - максимальный размер буффера
	input_substring(a0, a1)
	# Возвращаемое значение: нет
		
	# Вызов макроса для поиска индексов первого символа для всех вхождений подстроки в строку
	# Передаваемые параметры:
	mv a0 s1		#
	la a1 substr_buffer	# 
	jal find_substr_idx
	# Возвращаемые значения:
	# a0 - адресс полученного массива индексов в куче
	# a1 - размер полученного массива

	mv s2 a0
	mv s3 a1
	
	mv a0 s1
	li a7 4
	ecall		
	exit_program()
