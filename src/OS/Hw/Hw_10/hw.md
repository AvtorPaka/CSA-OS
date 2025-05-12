# OC ДЗ №10

### Демченко Георгий Павлович, БПИ-235

### Сборка программы

```sh
cd Hw_10/client_receiver && make && cd ../sender_server && make
```

### Запуск программы

**Процесс-сервер (отправитель)**

```sh
cd sender_server/bin/ && ./server_sender <broadcast ip address> <broadcast port>
```

**Процесс-клиент (получатель)**

```sh
cd client_receiver/bin/ && ./client_receiver <broadcast port>
```