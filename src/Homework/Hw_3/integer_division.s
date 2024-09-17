.data
input_dividend: .asciz "Enter dividend: "
input_divisor: .asciz "Enter the divisor: "
output_quotient: .asciz "\nCalculated quotient: "
output_remainder: .asciz "\nCalculated remainder: "
DevideByZeroException: .asciz "\nDevideByZeroException -> The divisor cannot be zero"

.text
main:	
	#Ввод делимого в регистр t0
	la a0 input_dividend
	li a7 4
	ecall
	li a7 5
	ecall
	mv t0 a0
	
	#Ввод делителя в регистр t1
	la a0 input_divisor
	li a7 4
	ecall 
	li a7 5
	ecall
	mv t1 a0
	
	#Вызов функции проверки аргументов
	jal ra, check_args
	
	#Перезапись в t0, t1 модулей хранимых чисел и сохранение их знаков в регистры t2(для t0), t3(для t1)
	mv t6 t0
	jal ra, get_abs_store_sign
	mv t0 t6
	mv t2 t5
	
	mv t6 t1
	jal ra, get_abs_store_sign
	mv t1 t6
	mv t3 t5
	
	#Вызываем метод подсчёта частного и остатка по модулю чисел
	jal ra, calculate
	
	#Восстановление знака остатка (s1) в зависимости от знака в t2
	bgtz t2 end_if_dividend_neg
	if_dividend_neg:
	mv t6 s1
	jal t4, change_number_sign
	mv s1 t6
	end_if_dividend_neg:
	
	#Восстановление знака частного (s0) в зависимости от знаков в t2 и t3
	add t2 t2 t3
	
	bnez t2 end_if_diff_sign
	if_different_sign:
	mv t6 s0
	jal t4, change_number_sign
	mv s0 t6
	end_if_diff_sign:
	
	#Вывод результата в консоль и завершение программы
	j output_calculations
	
#Метод нахождения целого частного и остатка
calculate:
	#Цикл while(t0 >= t1) {t0 -= t1; s0++}
	blt t0 t1 end_while 
	
	while:
	sub t0 t0 t1
	addi s0 s0 1		#Считаем целое частное без знака в s0
	blt t0 t1 end_while
	j while
	end_while:
	mv s1 t0	#Сохраняем остаток без знака в s1
	jalr zero, 0(ra)

#Метод для получения модуля числа в регистре t6 и сохранения его знака в регистр t5
get_abs_store_sign:

	if_negative:
	bgtz t6 if_positive
	li t5 -1
	jal t4, change_number_sign
	mv t4, zero
	j end_if_sign
	
	if_positive:
	li t5 1
	
	end_if_sign:
	jalr zero, 0(ra)
	
#Метод для смены знака числа из регистра t6 на противоположный
change_number_sign:
	xori t6, t6, -1
	addi t6 t6 1
	jalr zero, 0(t4)

#Метод проверки делителя(t1) на 0, делимого(t0) на 0 (досрочноe завершение с ответом 0,0)
check_args:
	beqz t1 devided_by_zero_exception
	beqz t0 output_calculations
	jalr zero, 0(ra)

	
#Метод выброса исключения при делении на 0
devided_by_zero_exception:
	la a0 DevideByZeroException
	li a1 0
	
	li a7 55
	ecall
	
	li a7 4
	ecall
	j exit_program

#Метод для вывода полученных значений(хранимых в регистрах s0, s1) в консоль
output_calculations:

	#Вывод частного (s0)
	la a0 output_quotient
	li a7 4
	ecall
	mv a0 s0
	li a7 1
	ecall
	
	#Вывод остатка (s1)
	la a0 output_remainder
	li a7 4
	ecall
	mv a0 s1
	li a7 1
	ecall
	
	j exit_program

#Метод завершения программы
exit_program:	
	#Очистка использованных регистров
	mv t0 zero
	mv t1 zero
	mv t2 zero
	mv t3 zero
	mv t4 zero
	mv t5 zero
	mv t6 zero
	mv s0 zero
	mv s1 zero
	mv a0 zero
	
	li a7 10
	ecall
