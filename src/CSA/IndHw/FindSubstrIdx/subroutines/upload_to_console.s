.include "../macrolib.s"

.global upload_to_console

# Подпрограмма для вывода полученного массива индексов (харнящихся в 32-битном числовом формате) в консоль
# Входные параметры:
# a0 - адресс начала области памяти с массивом 32-битных чисел
# a1 - размер массива
# Выходные параметры: нет
.text
upload_to_console:
	stack_push_w(ra)
	stack_push_w(t0)
	stack_push_w(t1)
	stack_push_w(t2)
	
	mv t0 a0
	mv t1 zero
	mv t2 zero
	#for (t1 = 0; t < a1; t1++) {...}
	beqz a1 end_for_console_out_loop
	
	for_console_out_loop:
		lw t2 (t0)
		print_int(t2)
		next_space()
		
		addi t1 t1 1
		addi t0 t0 4
		bltu t1 a1 for_console_out_loop
	end_for_console_out_loop:
	
	stack_pop_w(t2)
	stack_pop_w(t1)
	stack_pop_w(t0)
	stack_pop_w(ra)
	jalr zero 0(ra)
