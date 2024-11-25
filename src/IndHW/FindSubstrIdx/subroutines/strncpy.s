.include "../macrolib.s"

.global strncpy

# Подпрограмма для копирования первых num количества символов
# из одной области памяти (строки) в другую область памяти (строку)
# Входные параметры:
# a0 (destination) - область памяти, кула будет скопирована часть строки
# a1 (source) - область памяти (не нуль-терминированная строка), откуда будут скопированы num символов
# a2 (num) - 32-битное число, определяющее количество первых символов source для копирования в destination
# Выходные параметры:
# a0 - destination
# Поведение (согласно https://cplusplus.com/reference/cstring/strncpy/ )
#
# If the end of the source C string (which is signaled by a null-character) is found before num characters have been copied,
# destination is padded with zeros until a total of num characters have been written to it.
#
# No null-character is implicitly appended at the end of destination if source is longer than num.
# Thus, in this case, destination shall not be considered a null terminated C string (reading it as such would overflow).
#
# Дополнительно осуществленна проверка num на неотрицательность (в C num - unsigned)
.text
strncpy:
	stack_push_w(ra)
	stack_push_w(a0)
	stack_push_w(a1)
	stack_push_w(a2)

	
	bgez a2 end_if_invalid_number_arg
	if_invalid_number_arg:
		throw_num_arg_validation_error()
	end_if_invalid_number_arg:
	
	li t0 0	 # счетчик иттераций
	li t1 0  # флаг определяющий, закончилась ли строки source (был ли добавлен null-char к destination)
	li t2 0  # регистр для хранения текущего символа
	lw t3 8(sp) # регистр хранящий destination
	# for (t0 = 0; t0 < num; t0++) {...}
	bge t0 a2 end_for_loop
	for_loop:
		beqz t1 if_source_not_ended
		if_source_ened:
			li t2 0x30
			sb t2 (t3)
			addi t3 t3 1
			j end_if_source_ened
		if_source_not_ended:
			lb t2 (a1)
			sb t2 0(t3)
			addi t3 t3 1
			addi a1 a1 1
			
			bnez t2 end_if_cur_null
			if_cur_null:
				li t1 1
			end_if_cur_null:
				
		end_if_source_ened:
		
		addi t0 t0 1
		blt t0 a2 for_loop
	end_for_loop:
	
	#Добавление null-char в конец destination, если source больше чем num и num не достигнут
	beq t0 a2 end_if_not_terminated
	bnez t1 end_if_not_terminated
	if_not_terminated:
		li t2 0x00
		sb t2 (t3)
	end_if_not_terminated:
	

	stack_pop_w(a2)
	stack_pop_w(a1)
	stack_pop_w(a0)
	stack_pop_w(ra)
	jalr zero 0(ra)
