#!/bin/bash

# Скрипт для вычисления факториала числа

# Функция для вычисления факториала числа
factorial() {
    local num=$1
    local result=1

    for (( i=1; i<=num; i++ )); do
        (( result *= i ))
    done

    echo $result
}

# Функция-entrypoint скрипта, инициализирует переменные,
# производит вызов функций, выводит результат и завершает работу
boot_script() {
    local num

    echo "Enter number to calculate its factorial: "
    read num

    if [ $num -lt 0 ]; then
        echo "Number must be >= 0"
        return 1
    fi


    local fact_num=$(factorial $num)
    echo "Factorial of $num: $fact_num"
    return 0
}

boot_script