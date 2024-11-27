.include "../macrolib.s"

.global upload_to_file

# Подпрограмма для загрузки результата работы программы в файл
# Входные параметры:
# a0 - указатель на область памяти со строкой - путем к файлу
# a1 - максимальный размер буффера пути к файлу (256)
# a2 - адрес памяти на куче где хранится строка для записи в файл
# a3 - размер записываемой строки в байтах
.text
upload_to_file:
	stack_push_w(ra)
	stack_push_w(s1)
	
	open_and_validate(a0, WRITE_ONLY)
	# Возвращаемое значение:
	# a0 - дескриптор файла
	mv s1 a0
	
	# Системный вызов записи в файл
	li a7 64
	mv a0 s1
	mv a1 a2
	mv a2 a3
	ecall
	
	close (s1)
	
	stack_pop_w(s1)
	stack_pop_w(a0)
	jalr zero 0(ra)
