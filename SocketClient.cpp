#define WIN32_LEAN_AND_MEAN

#include <iostream>
#include <string>
#include <windows.h>
#include <WinSock2.h>
#include <WS2tcpip.h>

using namespace std;

// Функция для очистки ресурсов и закрытия сокета
void cleanup(SOCKET& ConnectSocket, ADDRINFO* addrResult) {
    // Закрываем клиентский сокет, если он действительный
    if (ConnectSocket != INVALID_SOCKET) {
        closesocket(ConnectSocket);
        ConnectSocket = INVALID_SOCKET;
    }

    // Освобождаем память, выделенную для структуры addrinfo
    if (addrResult != NULL) {
        freeaddrinfo(addrResult);
        addrResult = NULL;
    }

    // Завершаем работу с библиотекой Winsock
    WSACleanup();
}

int main() {
    WSADATA wsaData;
    ADDRINFO hints;
    ADDRINFO* addrResult = NULL;
    SOCKET ConnectSocket = INVALID_SOCKET;

    const char* sendBuffer = "Client test message";
    char recvBuffer[512];

    int result;

    // Инициализация сокета
    result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        cout << "WSAStartup failed, result = " << result << endl;
        return 1;
    }

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    // Получение информации об адресе сервера
    string serverIP;
    cout << "Enter server IP: ";
    cin >> serverIP;

    result = getaddrinfo(serverIP.c_str(), "8888", &hints, &addrResult);
    if (result != 0) {
        cout << "getaddrinfo failed, result = " << result << endl;
        cleanup(ConnectSocket, addrResult);
        return 1;
    }

    // Попытка подключения к серверу
    for (ADDRINFO* addr = addrResult; addr != NULL; addr = addr->ai_next) {
        // Создание сокета для подключения
        ConnectSocket = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
        if (ConnectSocket == INVALID_SOCKET) {
            cout << "Socket creation failed" << endl;
            cleanup(ConnectSocket, addrResult);
            return 1;
        }

        // Подключение к серверу
        result = connect(ConnectSocket, addr->ai_addr, (int)addr->ai_addrlen);
        if (result == SOCKET_ERROR) {
            cout << "Server connection failed, trying next address..." << endl;
            closesocket(ConnectSocket);
            ConnectSocket = INVALID_SOCKET;
            continue;
        }

        // Если подключение успешно, то выходим из цикла
        break;
    }

    // Проверяем, удалось ли подключиться к серверу
    if (ConnectSocket == INVALID_SOCKET) {
        cout << "Failed to connect to the server" << endl;
        cleanup(ConnectSocket, addrResult);
        return 1;
    }

    // Отправка данных серверу
    result = send(ConnectSocket, sendBuffer, (int)strlen(sendBuffer), 0);
    if (result == SOCKET_ERROR) {
        cout << "Send failed, result = " << result << endl;
        cleanup(ConnectSocket, addrResult);
        return 1;
    }

    cout << "Bytes sent: " << result << " bytes" << endl;

    // Завершение отправки данных
    result = shutdown(ConnectSocket, SD_SEND);
    if (result == SOCKET_ERROR) {
        cout << "shutdown failed, result = " << result << endl;
        cleanup(ConnectSocket, addrResult);
        return 1;
    }

    do {
        ZeroMemory(recvBuffer, sizeof(recvBuffer)); // Очистка буфера приема перед каждым приемом данных
        // Прием данных от сервера
        result = recv(ConnectSocket, recvBuffer, 512, 0);
        if (result > 0) {
            cout << "Received " << result << " bytes" << endl;
            cout << "Received data: " << recvBuffer << endl;
        }
        else if (result == 0) {
            cout << "Connection closed" << endl;
        }
        else {
            cout << "recv failed with error" << endl;
        }
    } while (result > 0);

    // Закрытие сокета и освобождение ресурсов
    cleanup(ConnectSocket, addrResult);

    // Ожидание ввода пользователя перед закрытием консоли
    system("pause");

    return 0;
}