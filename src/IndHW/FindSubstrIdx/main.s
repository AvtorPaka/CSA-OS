.eqv BUFFER_SIZE 512
.eqv PATH_SIZE 256

.data
data_buffer: .space BUFFER_SIZE
path_buffer: .space PATH_SIZE
ping: .asciz "pong"


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
	

	li a7 4
	ecall
	

	exit_program()