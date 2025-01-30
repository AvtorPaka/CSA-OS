#!/bin/bash

# Скрипт для определения является ли введенное пользователем число простым

# Функция для нахождения [sqrt(num)] + 1
get_range_limits() {
    local num=$1
    local sqrt=$(echo "scale=3; sqrt($num)" | bc)
    local floored=$(awk "BEGIN {print int($sqrt)}")
    (( floored++ ))
    echo $floored
}

# Функция, проверяющая, является ли число простым
is_simple() {
    local num=$1
    local upper_limit=$(get_range_limits $num)

    local devider=2
    local simple=0
    while [ $devider -lt $upper_limit ]; do
        if (( $num % $devider == 0 )); then
            simple=1
            break
        fi
        (( devider++ ))
    done

    echo $simple
}

# Функция-entrypoint скрипта, инициализирует переменные,
# производит вызов функций, выводит результат и завершает работу
boot_script() {
    local num

    echo "Enter number: "
    read num

    if [ $num -lt 0 ]; then
        echo "Number must be >= 0"
        return 1
    fi

    local simple=$(is_simple $num)
    if (( $simple == 1 )); then
        echo "Number $num isn't simple"
    else
        echo "Number $num is simple"
    fi
    return 0
}

boot_script