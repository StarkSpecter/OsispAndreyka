#include "library.h"

void broadcast_message(const std::string& message, SOCKET sender, std::map<SOCKET, std::string>& clients, std::mutex& clients_mutex) {
    std::lock_guard<std::mutex> lock(clients_mutex);
    for (const auto& client : clients) {
        if (client.first != sender) {
            send(client.first, message.c_str(), message.size(), 0);
        }
    }
}

void send_to_client(const std::string& message, const std::string& target_name, std::map<SOCKET, std::string>& clients, std::mutex& clients_mutex) {
    std::lock_guard<std::mutex> lock(clients_mutex);
    std::cout << "Hello world!" << "\n";
    for (const auto& client : clients) {
        if (client.second == target_name) {
            send(client.first, message.c_str(), message.size(), 0);
            return;
        }
    }
}
