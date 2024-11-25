.include "../macrolib.s"

.global convert_array_to_string

# Подпрограмма для перевода массива 32-битных чисел в строку
# Входные параметры:
# a0 - адресс начала области памяти с массивом 32-битных чисел
# a1 - размер массива
# a2 - адрес начала буффера для временной записи строкового представления числа
# Выходные параметры:
# a0 - адрес области памяти в куче с итоговой строкой
# a1 - размер итоговой строки в байтах
.text
convert_array_to_string:
	stack_push_w(ra)
	stack_push_w(s1)
	stack_push_w(s2)
	stack_push_w(s3)
	stack_push_w(s4)
	stack_push_w(s5)
	stack_push_w(s6)
	stack_push_w(a5)
	stack_push_w(a3)
	
	mv s2 a2	# адрес начала буффера для временных переводов
	mv s5 zero	# адрес выделенной на хипе памяти
	mv a3 zero	# счетчик иттераций
	mv s6 a1	# размер входного массива
	mv s4 a0	# адрес массива 32-битных чисел
	mv s1 zero	# размер итоговой строки в байтах
	mv s3 zero	# количество текущих записанных в буффер байт
	mv t1 zero	# временная переменная для хранения числа из массива
	mv a5 zero	# адрес выделенной на хипе памяти
	
	# for(a3 = 0; a3 < s6; t0++)
	for_array_convert_loop:
		lw t1 (s4)
				
		# Вызов подпрограммы конвертации числа в строку
		mv a0 t1
		mv a1 s2
		jal convert_int_to_string
			
		mv s3 a0
		add s1 s1 s3
		allocate(s3)
		
		bnez s5 if_nfar
		if_first_allocate_arr:
			mv s5 a0
			mv a5 a0
		if_nfar:
		
		strncpy(s5, s2 ,s3)
		add s5 s5 s3
		
		li t1 59
		sb t1 (s5)
		addi s5 s5 1
		addi s1 s1 1
		
		addi s4 s4 4
		addi a3 a3 1
		blt a3 s6 for_array_convert_loop
	end_for_array_convert_loop:
	
	addi s5 s5 1
	addi s1 s1 1
	sb zero 0(s5)
	
	
	mv a1 s1
	mv a0 a5
	stack_pop_w(a3)
	stack_pop_w(a5)
	stack_pop_w(s6)
	stack_pop_w(s5)
	stack_pop_w(s4)
	stack_pop_w(s3)
	stack_pop_w(s2)
	stack_pop_w(s1)
	stack_pop_w(ra)
	jalr zero 0(ra)
