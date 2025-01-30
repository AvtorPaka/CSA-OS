# OC ДЗ №2

### Демченко Георгий Павлович, БПИ-235

## 1. [odd_even.sh](https://github.com/AvtorPaka/CSA-OS/tree/master/src/OS/Hw/Hw_2/scripts/odd_even.sh)

**Скрипт для определения четности введенного пользователем числа**

```bash
#!/bin/bash

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
```

## 2. [is_simple.sh](https://github.com/AvtorPaka/CSA-OS/tree/master/src/OS/Hw/Hw_2/scripts/is_simple.sh)


**Скрипт для определения является ли введенное пользователем число простым**

```sh
#!/bin/bash

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
```

## 3. [counter.sh](https://github.com/AvtorPaka/CSA-OS/tree/master/src/OS/Hw/Hw_2/scripts/counter.sh)

**Скрипт для вывода номеров итераций в консоль**


```sh
#!/bin/bash

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
```


## 4. [factorial.sh](https://github.com/AvtorPaka/CSA-OS/tree/master/src/OS/Hw/Hw_2/scripts/factorial.sh)

**Скрипт для вычисления факториала числа**

```bash
#!/bin/bash

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
```


## 5. [sum.sh](https://github.com/AvtorPaka/CSA-OS/tree/master/src/OS/Hw/Hw_2/scripts/sum.sh)

**Скрипт для вычисления суммы чисел от 1 до N**

```bash
#!/bin/bash

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
```