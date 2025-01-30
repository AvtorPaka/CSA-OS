#!/bin/bash

# Скрипт для вывода номеров итераций в консоль

# Функция для вывода номеров итераций в консоль
count() {
    local num=$1;

    local counter=1;
    (( num++ ))
    while [ $counter -lt $num ]; do
        echo "Counter: $counter"
        (( counter++ ))
    done
}

# Функция-entrypoint скрипта, инициализирует переменные,
# производит вызов функций, выводит результат и завершает работу
boot_script() {
    local num

    echo "Enter number of iterations to count: "
    read num

    count $num
    return 0
}

boot_script