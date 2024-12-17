#include <winsock2.h>
#include <iostream>
#include <thread>
#include <mutex>
#include <map>
#include <vector>
#include "library.h"

#pragma comment(lib, "ws2_32.lib")

#define PORT 12345
#define BUFFER_SIZE 1024

std::map<SOCKET, std::string> clients; 
std::mutex clients_mutex;

//void broadcast_message(const std::string& message, SOCKET sender) {
//    std::lock_guard<std::mutex> lock(clients_mutex);
//    for (const auto& client : clients) {
//        if (client.first != sender) {
//            send(client.first, message.c_str(), message.size(), 0);
//        }
//    }
//}

//void send_to_client(const std::string& message, const std::string& target_name) {
//    std::lock_guard<std::mutex> lock(clients_mutex);
//    for (const auto& client : clients) {
//        if (client.second == target_name) {
//            send(client.first, message.c_str(), message.size(), 0);
//            return;
//        }
//    }
//}

void handle_client(SOCKET client_socket) {
    char buffer[BUFFER_SIZE];
    std::string client_name;

    // Получение имени клиента
    int recv_size = recv(client_socket, buffer, BUFFER_SIZE, 0);
    if (recv_size > 0) {
        buffer[recv_size] = '\0';
        client_name = buffer;
        std::lock_guard<std::mutex> lock(clients_mutex);
        clients[client_socket] = client_name;
        std::cout << client_name << " connected.\n";
    }

    // Рассылка клиенту списка активных пользователей
    {
        std::lock_guard<std::mutex> lock(clients_mutex);
        std::string client_list = "Clients:";
        for (const auto& client : clients) {
            client_list += " " + client.second;
        }
        client_list += "\n";
        send(client_socket, client_list.c_str(), client_list.size(), 0);
    }

    while (true) {
        int recv_size = recv(client_socket, buffer, BUFFER_SIZE, 0);
        if (recv_size <= 0) {
            std::lock_guard<std::mutex> lock(clients_mutex);
            std::cout << client_name << " disconnected.\n";
            clients.erase(client_socket);
            closesocket(client_socket);
            break;
        }

        buffer[recv_size] = '\0';
        std::string message(buffer);
        std::cout << "Message from " << client_name << ": " << message << std::endl;

        if (message == "list") {
            // Отправка списка клиентов
            std::lock_guard<std::mutex> lock(clients_mutex);
            std::string client_list = "Clients:";
            for (const auto& client : clients) {
                client_list += " " + client.second;
            }
            client_list += "\n";
            send(client_socket, client_list.c_str(), client_list.size(), 0);
        } else if (message.rfind("msg ", 0) == 0) {
            // Отправка сообщения указанному клиенту или всем
            size_t space_pos = message.find(' ', 4);
            if (space_pos != std::string::npos) {
                std::string target = message.substr(4, space_pos - 4);
                std::string msg_body = client_name + ": " + message.substr(space_pos + 1);
                if (target == "all") {
                    broadcast_message(msg_body, client_socket);
                } else {
                    send_to_client(msg_body, target);
                }
            }
        }
    }
}

int main() {
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    SOCKET server_socket = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    bind(server_socket, (sockaddr*)&server_addr, sizeof(server_addr));
    listen(server_socket, SOMAXCONN);
    std::cout << "Server started on port " << PORT << "\n";

    while (true) {
        SOCKET client_socket = accept(server_socket, nullptr, nullptr);
        std::thread(handle_client, client_socket).detach();
    }

    closesocket(server_socket);
    WSACleanup();
    return 0;
}
