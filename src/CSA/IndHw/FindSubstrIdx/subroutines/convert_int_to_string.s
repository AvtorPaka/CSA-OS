.include "../macrolib.s"

.global convert_int_to_string

# Подпрограмма для перевода 32-битного числа в не нуль-терминированую строку
# Входные параметры:
# a0 - число для перевода
# a1 - адрес начала буффера для записи строкового представления числа
# Выходные параметры:
# a0 - количество записанных в буффер байт
.text
convert_int_to_string:
	stack_push_w(ra)
	stack_push_w(s1)
	stack_push_w(s2)
	stack_push_w(s3)
	stack_push_w(s4)
	stack_push_w(s5)
	stack_push_w(s6)
	stack_push_w(t5)
	stack_push_w(t6)
	
	mv s1 a0	# Регистр для хранения числа
	mv s2 a1	# Регистр для хранения адреса начала буффера
	mv s3 a1	# Регистр для хранения и динамического изменения адреса буффера
	mv s5 a1	# Регистр для хранения и динамического изменения адреса буффера
	mv s4 zero	# Счетчик записанных байт

	li t5 10
	convert_loop:
    		rem t6, s1, t5
    		addi t6, t6, 48
    	
		sb t6, 0(s3)
		addi s4 s4 1
	
		addi s3, s3, 1
		div s1, s1, t5
		bnez s1, convert_loop 
	end_convert_loop:


	addi s3, s3, -1
	reverse_loop:
		bge s5, s3, end_reverse_loop
		
		lb t6, 0(s5)
		lb t5, 0(s3)
		sb t5, 0(s5)
		sb t6, 0(s3)
		
		addi s5, s5, 1
		addi s3, s3, -1
		j reverse_loop
	end_reverse_loop:
		
	mv a0 s4
	stack_pop_w(t6)
	stack_pop_w(t5)
	stack_pop_w(s6)
	stack_pop_w(s5)
	stack_pop_w(s4)
	stack_pop_w(s3)
	stack_pop_w(s2)
	stack_pop_w(s1)
	stack_pop_w(ra)
	jalr zero 0(ra)
