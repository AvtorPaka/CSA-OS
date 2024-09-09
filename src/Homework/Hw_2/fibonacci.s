main:                   #Формат инструкции:
    mv   t0, zero       # Псевдокоманда, расскладывается в add : R-type 
    li   t1, 1          # Псевдокманда, расскалдывается в addi : I-type

    li   a7, 5          # Псевдокманда, расскалдывается в addi : I-type 
    ecall               # I-type
    mv   t3, a0         # Псевдокоманда, расскладывается в add : R-type  
fib:
    beqz t3, finish     # Псевдокоманда, расскладыватся в beq : B-type
    add  t2, t1, t0     # R-type
    mv   t0, t1         # Псевдокоманда, расскладывается в add : R-type 
    mv   t1, t2         # Псевдокоманда, расскладывается в add : R-type 
    addi t3, t3, -1     # I-type
    j    fib            # Псевдокоманда, расскладывается в jal : J-type
finish:
    li   a7, 1          # Псевдокманда, расскалдывается в addi : I-type
    mv   a0, t0         # Псевдокоманда, расскладывается в add : R-type 
    ecall               # I-type
