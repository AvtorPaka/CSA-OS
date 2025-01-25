# Макрос-обертка над подпрограммой вывода шестандцатеричной цифры на индикатор цифрового блока DLS
.macro dls_print_int(%num, %addr)
	mv a0 %num
	mv a1 %addr
	jal dls_show_num
.end_macro


# Макрос для вывода строки в консоль
#Параметры:
# %str - лейбл с строкой, значение которой будет выведено в консоль
.macro print_string(%str)
	stack_push_w(a0)
	
	la a0 %str
	li a7 4
	ecall
	
	stack_pop_w(a0)
.end_macro

# Макрос для системного вызова засыпания программы в миллисекундах
.macro sleep(%x)
	li a0, %x
    li a7, 32
    ecall
.end_macro


.macro map_sfs(%snum)
	stack_push_w(s1)
	stack_push_w(t1)
	mv s1 %snum
	mv t1 zero
	
	li t1 17
	bne s1 t1 end_if_1
	if_0:
		mv a6 zero
		j end_all
	end_if_0:
	
	li t1 33
	bne s1 t1 end_if_1
	if_1:	
		li a6 1
		j end_all
	end_if_1:
	
	li t1 65
	bne s1 t1 end_if_2
	if_2:	
		li a6 2
		j end_all
	end_if_2:
	
	li t1 -127
	bne s1 t1 end_if_3
	if_3:
		li a6 3
		j end_all
	end_if_3:
	
	li t1 18
	bne s1 t1 end_if_4
	if_4:
		li a6 4
		j end_all
	end_if_4:
	
	li t1 34
	bne s1 t1 end_if_5
	if_5:
		li t2 5
		j end_all
	end_if_5:
	
	li t1 66
	bne s1 t1 end_if_6
	if_6:
		li a6 6
		j end_all
	end_if_6:
	
	li t1 -126
	bne s1 t1 end_if_7
	if_7:
		li a6 7
		j end_all
	end_if_7:
	
	li t1 20
	bne s1 t1 end_if_8
	if_8:
		li a6 8
		j end_all
	end_if_8:
	
	li t1 36
	bne s1 t1 end_if_9
	if_9:
		li a6 9
		j end_all
	end_if_9:
	
	li t1 68
	bne s1 t1 end_if_10
	if_10:
		li a6 10
		j end_all
	end_if_10:
	
	li t1 -124
	bne s1 t1 end_if_11
	if_11:
		li a6 11
		j end_all
	end_if_11:
	
	li t1 24
	bne s1 t1 end_if_12
	if_12:
		li a6 12
		j end_all
	end_if_12:
	
	li t1 40
	bne s1 t1 end_if_13
	if_13:
		li a6 13
		j end_all
	end_if_13:
	
	li t1 72
	bne s1 t1 end_if_14
	if_14:
		li a6 14
		j end_all
	end_if_14:
	
	li t1 -120
	bne s1 t1 end_if_15
	if_15:
		li a6 15
		j end_all
	end_if_15:
	
	end_all:
	
	stack_pop_w(t1)
	stack_pop_w(s1)
.end_macro


# Печать содержимого заданного регистра как двоичного целого
.macro print_int_bin (%x)
   stack_push_w(a0)
   stack_push_w(a7)
	mv a0, %x
	li a7, 35
	ecall
   stack_pop_w(a7)
   stack_pop_w(a0)
.end_macro

# Печать содержимого заданного регистра как шестнадцатиричного целого
.macro print_int_hex (%x)
   stack_push_w(a0)
   stack_push_w(a7)
	mv a0, %x
	li a7, 34
	ecall
   stack_pop_w(a7)
   stack_pop_w(a0)
.end_macro

# Макрос для вывода целого числа в консоль
#Параметры:
# %num - регистр, значение которого будет выведено в консоль
.macro print_int(%num)
	stack_push_w(a0)
	
	mv a0 %num
	li a7 1
	ecall
	
	stack_pop_w(a0)
.end_macro

# Макрос для вывода символа в консоль
.macro print_char(%x)
	stack_push_w(a0)
   	li a7, 11
   	li a0, %x
   	ecall
   	stack_pop_w(a0)
.end_macro

# Макрос для вывода символа переноса строки в консоль
.macro newline
	print_char('\n')
.end_macro

# Макрос для завершения работы программы
.macro exit_program
	li a7, 10
	ecall
.end_macro

# Макрос для сохраенения 32-битного слова на стек
# Параметры:
# %x - регистр, значение которого сохранится на стек
.macro stack_push_w(%x)
	addi	sp sp -4
	sw	%x 0(sp)
.end_macro

# Макрос для снятия 32-битного слова со стека
# Параметры:
# %x - регистр, в который будет записано значение со стека
.macro stack_pop_w(%x)
	lw	%x (sp)
	addi	sp sp 4
.end_macro
