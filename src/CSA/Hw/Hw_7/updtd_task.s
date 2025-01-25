# !!! The program must be executed at a reduced speed.
# !!! Approximately 30 inst/sec.
.include "macrolib.s"

.global main

.text
main:
        lui     t6 0xffff0          # база MMIO (старшие адреса)
        mv      t5 zero             # счётчик
        mv      t4 zero             # предыдущее значение
        li      t2 20               # Число нажатий до завершения программы
        li	s7 0xffff0010
        li	s1 0x80
        sb 	s1 0x10(t6)
        
loop:
        mv      t1 zero             # общий результат сканирования
        
        li      t0 1                # первый ряд
        sb      t0 0x12(t6)         # сканируем
        lb      t0 0x14(t6)         # забираем результат
        
        or      t1 t1 t0            # добавляем биты в общий результат
        
        li      t0 2                # второй ряд
        sb      t0 0x12(t6)
        lb      t0 0x14(t6)
        or      t1 t1 t0
        
        li      t0 4                # третий ряд
        sb      t0 0x12(t6)
        lb      t0 0x14(t6)
        or      t1 t1 t0
        
        li      t0 8                # четвёртый ряд
        sb      t0 0x12(t6)
        lb      t0 0x14(t6)
        or      t1 t1 t0
        
        beq     t1 t4 same
        
        
        beqz t1 if_zero
        if_t1_not_zero:
        	map_sfs(t1)
        	dls_print_int(a6, s7)
        	addi    t5 t5 1             # счётчик
        if_zero:   
        
        mv      t4 t1    
same:   ble     t5 t2 loop

        exit_program
