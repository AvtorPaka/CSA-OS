.include "macrolib.s"

.global calculate_series

# Подпрограмма для подсчета значения биномиальной функции с помощью степенного ряда (Бинома Ньютона)
# Параметры:
# a1 - степень биномиальной функции m
# fa1 - параметр биномиальной функции x
# Возвращаемое значение: 
# fa0 - посчитанное значение биномиальной функции (1 + x)^m
.text
calculate_series:
	stack_push_w(ra)
	stack_push_w(t4)
	stack_push_d(ft5)
	stack_push_d(ft6)
	
	create_double_value(fa7 0)
	li t4 0
	#for (t4 = 0; t4 <= a1; ++t4) {...}
	for_binomial_series_loop:
	
		#Вызов макроса подсчета биномиального коефицента - C из n по k
		# Передаваемые параметры:
		# Параметры:
		# a1 (%n) - n параметр биномиального коефицента
		# t4 (%k) - k параметр биномиального коефицента
		# ft5 (%fr) - fp регистр, куда будет записано посчитанное значение
		calculate_binomial_coef(a1 t4 ft5)
		
		#Вызов макроса посчета степени (натуральной) дробного числа двойной точности
		# Передаваемые параметры:
		# fa1 (%fbase) - число для возведения в степень
		# t4 (%exp) - показатель степени числа
		# ft6 (%fr) - fp регистр, куда будет записано значение степени
		pow(fa1 t4 ft6)
			
		fmul.d ft5 ft5 ft6
		fadd.d fa7 fa7 ft5
		
		addi t4 t4 1
		bleu t4 a1 for_binomial_series_loop
	end_for_binomial_series_loop:
	
	fmv.d fa0 fa7
	stack_pop_d(ft6)
	stack_pop_d(ft5)
	stack_pop_w(t4)
	stack_pop_w(ra)
	jalr zero 0(ra)