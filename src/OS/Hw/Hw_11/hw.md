# OC ДЗ №11

**Демченко Георгий Павлович, БПИ-235**

### Сборка программы

```sh
cd Hw_11 && make
```

### Запуск программы

```sh
cd bin && ./max_link_open_depth
```

### Пример исполнения

```
Created temporary directory: /tmp/symlink_depth_test
Changed working directory to: /tmp/symlink_depth_test
Created link: link_0 -> initial
Created link: link_1 -> link_0
Created link: link_2 -> link_1
Created link: link_3 -> link_2
Created link: link_4 -> link_3
Created link: link_5 -> link_4
Created link: link_6 -> link_5
Created link: link_7 -> link_6
Created link: link_8 -> link_7
Created link: link_9 -> link_8
Created link: link_10 -> link_9
Created link: link_11 -> link_10
Created link: link_12 -> link_11
Created link: link_13 -> link_12
Created link: link_14 -> link_13
Created link: link_15 -> link_14
Created link: link_16 -> link_15
Created link: link_17 -> link_16
Created link: link_18 -> link_17
Created link: link_19 -> link_18
Created link: link_20 -> link_19
Created link: link_21 -> link_20
Created link: link_22 -> link_21
Created link: link_23 -> link_22
Created link: link_24 -> link_23
Created link: link_25 -> link_24
Created link: link_26 -> link_25
Created link: link_27 -> link_26
Created link: link_28 -> link_27
Created link: link_29 -> link_28
Created link: link_30 -> link_29
Created link: link_31 -> link_30
Created link: link_32 -> link_31
Reached maximum symbolic link depth.
Failed to open 'link_32' 
Maximum recursion depth for symbolic links: 32
Temporary directory /tmp/symlink_depth_test removed.
```
