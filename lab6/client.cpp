#include <winsock2.h>
#include <iostream>
#include <thread>
#include <string>

#pragma comment(lib, "ws2_32.lib")

#define SERVER_IP "127.0.0.1"
#define PORT 12345
#define BUFFER_SIZE 1024

void receive_messages(SOCKET client_socket) {
    char buffer[BUFFER_SIZE];
    while (true) {
        int recv_size = recv(client_socket, buffer, BUFFER_SIZE, 0);
        if (recv_size <= 0) {
            std::cout << "Disconnected from server.\n";
            closesocket(client_socket);
            exit(0);
        }
        buffer[recv_size] = '\0';
        std::cout << buffer << std::endl;
    }
}

int main() {
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    SOCKET client_socket = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
    server_addr.sin_port = htons(PORT);

    if (connect(client_socket, (sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
    std::cerr << "Failed to connect to server. Error: " << WSAGetLastError() << "\n";
    return 1;
    }

    std::cout << "Connected to server. Enter your name: ";
    std::string name;
    std::getline(std::cin, name);
    send(client_socket, name.c_str(), name.size(), 0);

    std::thread(receive_messages, client_socket).detach();

    while (true) {
        std::string input;
        std::getline(std::cin, input);
        send(client_socket, input.c_str(), input.size(), 0);
    }

    closesocket(client_socket);
    WSACleanup();
    return 0;
}
