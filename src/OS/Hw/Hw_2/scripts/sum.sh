#!/bin/bash

# Скрипт для вычисления суммы чисел от 1 до N

# Функция для вычисления суммы чисел от 1 до N
sum_numbers() {
    local num=$1
    local sum=0

    # Цикл для вычисления суммы
    for (( i=1; i<=num; i++ )); do
        (( sum += i ))
    done

    echo $sum
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


    local sum_num=$(sum_numbers $num)
    echo "Sum of numbers from 1 to $num: $sum_num"
    return 0
}

boot_script