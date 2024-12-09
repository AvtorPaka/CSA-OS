.global dls_show_num

.include "macrolib.s"

# Подпрограмма для вывода шестандцатеричной цифры на индикатор цифрового блока DLS
# Входные параметры:
# a0 - шестнадцетиричная цифра
# a1 - адрес память левого (0xffff0011) или правого (0xffff0010) индикатора цифрового блока DLS
# Выходные параметры: нет
# Поведение: 
# Если число в регистре a0 превышает шестнадцатиричную цифру,
# то учитывается только младшие 4 разряда и выводится дополнительная точка
# Если передано отрицательное число, то его знак меняется на положительный
.text
dls_show_num:
	stack_push_w(ra)
	stack_push_w(a0)
	stack_push_w(a1)
	stack_push_w(s1)
	stack_push_w(s2)
	stack_push_w(t1)
	stack_push_w(t2)
	stack_push_w(t3)
	mv s1 a0		# Регистр для хранения переданного числа
	mv s2 a1		# Регистр для хранения переданного адреса памяти цифрового блока
	mv t3 zero		# Флаг для переполнения (когда число не является шестнадцетиричной цифрой)
	mv t2 zero		# Регистр для временного хранения разрядов отображающихся в число на индикаторе
	
	bgez s1 end_if_negative
	if_negative:
		neg s1 s1
	end_if_negative:
	
	li t1 15		# Регистр для временных манипуляций
	ble s1 t1 if_not_hex_end
	if_not_hex_num:
		li t1 0xF
		and s1 s1 t1
		li t3 1 
	if_not_hex_end:
	
			
	bnez s1 end_if_0
	if_0:
		beqz t3 if_not_ovefloated
		if_ovefloated:
			li t2 0xBF
			j end_all
		if_not_ovefloated:
			li t2 0x3F
			j end_all
	end_if_0:
	
	li t1 1
	bne s1 t1 end_if_1
	if_1:	
		beqz t3 if_not_ovefloated_1
		if_ovefloated_1:
			li t2 0x86
			j end_all
		if_not_ovefloated_1:
			li t2 0x6
			j end_all
	end_if_1:
	
	li t1 2
	bne s1 t1 end_if_2
	if_2:	
		beqz t3 if_not_ovefloated_2
		if_ovefloated_2:
			li t2 0xdb
			j end_all
		if_not_ovefloated_2:
			li t2 0x5b
			j end_all
	end_if_2:
	
	li t1 3
	bne s1 t1 end_if_3
	if_3:
		beqz t3 if_not_ovefloated_3
		if_ovefloated_3:
			li t2 0xCF
			j end_all
		if_not_ovefloated_3:
			li t2 0x4F
			j end_all
	end_if_3:
	
	li t1 4
	bne s1 t1 end_if_4
	if_4:
		beqz t3 if_not_ovefloated_4
		if_ovefloated_4:
			li t2 0xE6
			j end_all
		if_not_ovefloated_4:
			li t2 0x66
			j end_all
	end_if_4:
	
	li t1 5
	bne s1 t1 end_if_5
	if_5:
		beqz t3 if_not_ovefloated_5
		if_ovefloated_5:
			li t2 0xED
			j end_all
		if_not_ovefloated_5:
			li t2 0x6D
			j end_all
	end_if_5:
	
	li t1 6
	bne s1 t1 end_if_6
	if_6:
		beqz t3 if_not_ovefloated_6
		if_ovefloated_6:
			li t2 0xFD
			j end_all
		if_not_ovefloated_6:
			li t2 0x7D
			j end_all
	end_if_6:
	
	li t1 7
	bne s1 t1 end_if_7
	if_7:
		beqz t3 if_not_ovefloated7
		if_ovefloated7:
			li t2 0x87
			j end_all
		if_not_ovefloated7:
			li t2 0x07
			j end_all
	end_if_7:
	
	li t1 8
	bne s1 t1 end_if_8
	if_8:
		beqz t3 if_not_ovefloated8
		if_ovefloated8:
			li t2 0xFF
			j end_all
		if_not_ovefloated8:
			li t2 0x7F
			j end_all
	end_if_8:
	
	li t1 9
	bne s1 t1 end_if_9
	if_9:
		beqz t3 if_not_ovefloated9
		if_ovefloated9:
			li t2 0xEF
			j end_all
		if_not_ovefloated9:
			li t2 0x6F
			j end_all
	end_if_9:
	
	li t1 10
	bne s1 t1 end_if_10
	if_10:
		beqz t3 if_not_ovefloated10
		if_ovefloated10:
			li t2 0xF7
			j end_all
		if_not_ovefloated10:
			li t2 0x77
			j end_all
	end_if_10:
	
	li t1 11
	bne s1 t1 end_if_11
	if_11:
		beqz t3 if_not_ovefloated11
		if_ovefloated11:
			li t2 0xFC
			j end_all
		if_not_ovefloated11:
			li t2 0x7C
			j end_all
	end_if_11:
	
	li t1 12
	bne s1 t1 end_if_12
	if_12:
		beqz t3 if_not_ovefloated12
		if_ovefloated12:
			li t2 0xB9
			j end_all
		if_not_ovefloated12:
			li t2 0x39
			j end_all
	end_if_12:
	
	li t1 13
	bne s1 t1 end_if_13
	if_13:
		beqz t3 if_not_ovefloated13
		if_ovefloated13:
			li t2 0xDE
			j end_all
		if_not_ovefloated13:
			li t2 0x5E
			j end_all
	end_if_13:
	
	li t1 14
	bne s1 t1 end_if_14
	if_14:
		beqz t3 if_not_ovefloated14
		if_ovefloated14:
			li t2 0xF9
			j end_all
		if_not_ovefloated14:
			li t2 0x79
			j end_all
	end_if_14:
	
	li t1 15
	bne s1 t1 end_if_15
	if_15:
		beqz t3 if_not_ovefloated15
		if_ovefloated15:
			li t2 0xF1
			j end_all
		if_not_ovefloated15:
			li t2 0x71
			j end_all
	end_if_15:
	
	end_all:
	sb t2 0(s2)
	
	
	stack_pop_w(t3)
	stack_pop_w(t2)
	stack_pop_w(t1)
	stack_pop_w(s2)
	stack_pop_w(s1)
	stack_pop_w(a1)
	stack_pop_w(a0)
	stack_pop_w(ra)
	jalr zero 0(ra)
