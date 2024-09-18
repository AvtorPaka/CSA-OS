# АВС ДЗ №3

## Демченко Георгий Павлович, БПИ-235

## Documentation | [integer_division.s](https://github.com/AvtorPaka/CSA_RISC-V/tree/master/src/Homework/Hw_3/integer_division.s)

### 0. Program goal

* Program in RARS assembler that performs integer division for 32-bit signed integers using subtraction, branching, and loops.

* The remainder of division is calculated according to the rules used when performing the operation of calculating the remainder (%) in the C/C++ programming languages.

### 1. Registers usage

| **Register** |  **Purpose**  |
| --------- | ------------------ |
| ra |  Storing caller CP for **check_args**, **get_abs_store_sign**, **calculate** functions |
| a0 |  Storing values for/from syscals |
| a7  | Storing syscals types |
| t0  | Storing the dividend -> Storing dividend absolute value |
| t1  |  Storing the divisor -> Storing divisor absolute value |
| t2  |  Storing sign of dividend after calculating its absolute value (-1 or 1) |
| t3  | Storing sign of divisor after calculating its absolute value (-1 or 1)  |
| t4 | Storing caller CP for **change_number_sign** function |
| t5 | Temporary storage for number sign in **get_abs_store_sign** function (t5 -> t2 for t0, t5 -> t3 for t1) |
| t6 | Parametr for **get_abs_store_sign**, **change_number_sign** functions (t0, t1 will be passed there) |
| s0  | Storing calculated quotient  |
| s1  |  Storing calculated remainder |

### 2. Functions usage

| **Function** |  **Usage**  |
| --------- | ------------------ |
| **calculate**  |  Calculates quotient and remainder for absolute values of **t0** (dividend) and **t1** (divisor) and stores them in **(s0, s1)**  |
| **get_abs_store_sign** |  Calculate absolute value of int in **t6** and stores it in **t6**, stores it sign (-1 or 1) in **t5** |
| **change_number_sign**  |  Changes the sign of int in **t6** and stores calculated number in **t6** |
| **check_args**  | Check if **t0** (dividend) is 0 -> output 0 0 <br/> Check if **t1** (divisor) is 0 -> throws DevideByZeroException |
| **devided_by_zero_exception**  | Throws  DevideByZeroException in Rars window and console |
| **output_calculations**  | Ouput **s0** (quotient) and **s1** (remainder) in console  |
| **exit_program**  |  Clear values in registers and exit program |

## Autotesting with C++

### You can find [example](https://disk.yandex.ru/i/3sbJotb__i3cVA) video of running tests

### test.cpp execution steps

- Upload 2 32bit integers in **buffer.bin** file
- Call **bootstrap_test.sh** script which runs **integer_division_testing_version.s**
    - Asm progream loads these 2 integers from **buffer.bin** in t0, t1 registrs
    - Calculates quotient and remainder
    - Upload calculated values in **buffer.bin**
- Load calculated quotient and remainder from **buffer.bin**
- Check that the resulting numbers match those calculated using C++
- Execution continues if the test passes and crashes otherwise

### In order to execute test.cpp by yourself you must change:

1. **boostrap_tets.sh**
```sh
java -jar {YOUR_PATH}/rars1_6.jar {YOUR_PATH}/integer_division_testing_version.s
```

2. **integer_division_testing_version.s**

```assembly
.data
buffer:    .space 8
file_path: .asciz "{YOUR_PATH}/buffer.bin"
```

3. **test.cpp**

```cpp
const char* boostrap_path = "{YOUR_PATH}/boostrap_test.sh";
std::string buffer_path = "{YOUR_PATH}/buffer.bin";
```