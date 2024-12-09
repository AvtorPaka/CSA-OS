.eqv LEFT_DLS_NB 0xffff0011
.eqv RIGHT_DLS_NB 0xffff0010

.include "macrolib.s"

.global main

.text
main:
	li s0 LEFT_DLS_NB
	li s1 RIGHT_DLS_NB
	
	mv t1 zero
	li t2 32
	#for (t1 = 0; t1 < t2; t1++) {...}
	for_loop:
		dls_print_int(t1, s0)
		sleep(500)
		dls_print_int(t1, s1)
		sleep(500)
		
		addi t1 t1 1 
		blt t1 t2 for_loop
	end_for_loop:
	
	dls_print_int(s2, s1)
	
	exit_program()
