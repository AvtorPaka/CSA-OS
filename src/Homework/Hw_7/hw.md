# АВС ДЗ №7

## Демченко Георгий Павлович, БПИ-235

## 0. Program goal

### Main

* Develop a subroutine that outputs a digit transmitted via register a0 to the digital block indicator. The address (left or right) of the indicator is specified in register a1. If the number in register a0 exceeds a hexadecimal digit, then only the lower 4 digits are taken into account and an additional period is output.

* To demonstrate, write a program that calls this subroutine, which in a cycle with a delay (system call sleep), every second outputs the next value in a cycle first to one, and then to the other indicator.

### Optional

* Modify the keyboard program so that when the corresponding key is pressed, it displays only the pressed digit in the right indicator on the digital indicator. If there is no pressed key, only a dot is displayed on the digital indicator.

### Local launch in RARS

**Since all the assembler files necessary to run the main and test programs are located in one directory, you should use the following execution settings in the RARS environment**

## 1. Documentation | [dls_show_nums.s](https://github.com/AvtorPaka/CSA_RISC-V/tree/master/src/Homework/Hw_7/dls_show_num.s)

![rars-boot-options](img/rars_boot_settings.png)


|  **Subroutine** | **Purpose**  |  **Passed parameters** | **Return value** |
| ---------- | -------------- |  ------------ | ------------ |
| **dls_show_num**  |  Output of hexadecimal digit to the DLS digital block indicator | **a0** - hexadecimal digit <br> **a1** - memory address of the left (0xffff0011) or right (0xffff0010) indicator of the DLS digital block  | **Нет** |

* **If the number in register a0 exceeds a hexadecimal digit, then only the lower 4 digits are taken into account and an additional dot is output.**

* **If a negative number is passed, its sign changes to positive**


## 2. Examples of program ([main.s](https://github.com/AvtorPaka/CSA_RISC-V/tree/master/src/Homework/Hw_7/main.s)) execution

**[Video](https://disk.yandex.ru/i/_en8ePRV9pRLtw) example**

**Needed assemblys to execute :**
- **macrolib.s**
- **dls_show_num.s**

## 3.(Optional) Updated program ([updtd_task.s](https://github.com/AvtorPaka/CSA_RISC-V/tree/master/src/Homework/Hw_7/updtd_task.s)) execution

**[Video](https://disk.yandex.ru/i/ucmxiKi3a-UPRg) example**

**Needed assemblys to execute :**
- **macrolib.s**
- **dls_show_num.s**
