#This version made especially for testing with C++ (read hw.md)
#If you want to start it manually use integer_division.s

#!!! Be sure to enter full path to the buffer.bin file
.data
buffer:    .space 8
file_path: .asciz "{YOUR_PATH}/buffer.bin"

.text
main:	
	#Чтение произвольных чисел, записанных С++ в бинайрный файл и размещение в регистрах t0, t1
	li a7 1024
	la a0 file_path
	li a1 0
	ecall

	mv t0 a0

	li a7 63             
	mv a0 t0
	la a1 buffer
	li a2 8
	ecall
	
	li a7 57
	mv a0 t0
	ecall
	
	la t6 buffer
	
	lw t0 0(t6)
	lw t1 4(t6)
    	
	mv a0 zero
	mv a1 zero
	mv t6 zero
    	
	#Далее обычный ход программы
	jal ra, check_args
	
	mv t6 t0
	jal ra, get_abs_store_sign
	mv t0 t6
	mv t2 t5
	
	mv t6 t1
	jal ra, get_abs_store_sign
	mv t1 t6
	mv t3 t5
	
	jal ra, calculate
	
	bgtz t2 end_if_dividend_neg
	if_dividend_neg:
	mv t6 s1
	jal t4, change_number_sign
	mv s1 t6
	end_if_dividend_neg:
	
	add t2 t2 t3
	
	bnez t2 end_if_diff_sign
	if_different_sign:
	mv t6 s0
	jal t4, change_number_sign
	mv s0 t6
	end_if_diff_sign:
	
	j output_calculations
	
calculate:
	blt t0 t1 end_while 
	
	while:
	sub t0 t0 t1
	addi s0 s0 1		
	blt t0 t1 end_while
	j while
	end_while:
	mv s1 t0	
	jalr zero, 0(ra)

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
	
change_number_sign:
	xori t6, t6, -1
	addi t6 t6 1
	jalr zero, 0(t4)

check_args:
	beqz t1 devided_by_zero_exception
	beqz t0 output_calculations
	jalr zero, 0(ra)

	
devided_by_zero_exception:
	j exit_program

output_calculations:
	#Запись полученных чисел в конец бинарного файла
	li a7 1024
	la a0 file_path
	li a1 1
	ecall
	
	mv t5 a0
	
	la t6 buffer
	sw s0, 0(t6)
	sw s1, 4(t6)
	
	li a7 64             
	mv a0 t5
	la a1 buffer
	li a2 8
	ecall
	
	li a7, 57
	mv a0, t5
	ecall
	
	j exit_program

exit_program:	
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
	mv a1 zero
	
	li a7 10
	ecall
