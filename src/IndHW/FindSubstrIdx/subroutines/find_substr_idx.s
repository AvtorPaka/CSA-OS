.include "../macrolib.s"

.global find_substr_idx


# Подпрограмма для поиска индексов первого символа для всех вхождений подстроки в строку
# Входные парамтры:
# a0 - область памяти, где расположена строка(текст) , где происходит поиск
# a1 - область памяти, где расположена подстрока для поиска вхождений
# Выходные параметры:
# a0 - указатель на начало области памяти в куче, где расположены нужные индексы (массив)
# a1 - размер полученного массива
.text
find_substr_idx:
	stack_push_w(ra)
	stack_push_w(s1)
	stack_push_w(s2)
	stack_push_w(s3)
	stack_push_w(s4)
	
	mv s1 a1	# Сатический указатель на начало подстроки
	mv t2 a1	# Динамический указатель на начало подстроки (меняеся в цикле)
	mv t3 a0	# Динамический указатель на начало текста
	
	strlen(a1)
	mv t1 a0	# Длинна подстроки	
	
	mv a3 zero	# Текущий индекс текста
	mv t4 zero	# Регистр для хранения текущего символа подстроки
	mv s3 zero	# Регистр для хранения текущего символа текста
	li t5 -1	# Регистр для хранения потенциального индекса начала подстроки
	mv t6 zero	# Длинна текущей части потенциальной подстроки
	mv s4 zero	# Адрес динамической памяти, куда будут записываться индексы
	mv a1 zero	# Счетчик размера массива
	mv a4 zero
	
	# while (t3 != 0x00)
	for_idx_search_loop:
		lb s3 (t3)
		
		beqz s3 end_for_idx_search_loop
		if_not_end:
			lb t4 (t2)
			
			bne s3 t4 if_chars_not_eq
			if_chars_eq:
				addi t6 t6 1
				
				bgez t5 if_not_first
				if_first:
					mv t5 a3
				if_not_first:
				
				bne t6 t1 if_not_substring
				if_substring:
					addi a1 a1 1
					
					li t0 4
					allocate(t0)
					bnez s4 if_not_fa
					if_first_alloc:
						mv s4 a0
						mv a4 a0
					if_not_fa:
					sw t5 (s4)
					addi s4 s4 4
					
					
					mv t6 zero
					li t5 -1
					mv t2 s1
							
				if_not_substring:
				
				addi t2 t2 1
				j end_if_eq
			if_chars_not_eq:
				mv t2 s1
				li t5 -1
				mv t6 zero
			end_if_eq:
			
			
		if_end:
		
		addi a3 a3 1
		addi t3 t3 1
		j for_idx_search_loop
	end_for_idx_search_loop:
	
	mv a0 a4
	stack_pop_w(s4)
	stack_pop_w(s3)
	stack_pop_w(s2)
	stack_pop_w(s1)
	stack_pop_w(ra)
	jalr zero 0(ra)
