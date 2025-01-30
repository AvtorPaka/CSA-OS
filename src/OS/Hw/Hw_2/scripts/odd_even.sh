#!/bin/bash

# Скрипт для определения четности введенного пользователем числа

# Функция для определения четности числа
# возвращаемое значение: 0 - четное, 1 - нечетное
is_even(){
    if (( $1 % 2 == 0 )); then
        echo 0
    else
        echo 1
    fi
}

# Функция-entrypoint скрипта, инициализирует переменные,
# производит вызов функций, выводит результат и завершает работу
boot_script() {
    local num

    echo "Enter number: "
    read num

    local even=$(is_even $num)
    local state
    if (( $even == 0 )); then
        state="even"
    else
        state="odd"
    fi

    echo "Number $num is $state"
    return 0
}

boot_script