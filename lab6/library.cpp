#include "library.h"
#include <regex>
#include <iostream>
#include <algorithm>

// Глобальные массивы для хранения чисел
constexpr size_t MAX_NUMBERS = 100;
int array1[MAX_NUMBERS] = {0};
int array2[MAX_NUMBERS] = {0};
size_t array1_size = 0;
size_t array2_size = 0;

void broadcast_message(const std::string& message, SOCKET sender, std::map<SOCKET, std::string>& clients, std::mutex& clients_mutex) {
    std::lock_guard<std::mutex> lock(clients_mutex);
    for (const auto& client : clients) {
        if (client.first != sender) {
            send(client.first, message.c_str(), message.size(), 0);
        }
    }
}
// Извлечение чисел из сообщения
void extract_numbers(const std::string& message, int* array, size_t& size) {
    std::regex number_regex(R"(\d+)");
    auto numbers_begin = std::sregex_iterator(message.begin(), message.end(), number_regex);
    auto numbers_end = std::sregex_iterator();

    for (auto it = numbers_begin; it != numbers_end && size < MAX_NUMBERS; ++it) {
        array[size++] = std::stoi(it->str());
    }
}

// Перемножение массивов с использованием ассемблера
void multiply_arrays(const int* array1, const int* array2, int* result, size_t size) {
    for (size_t i = 0; i < size; ++i) {
        asm(
            "movl %[val1], %%eax\n\t"   // Загрузить array1[i] в EAX
            "imull %[val2], %%eax\n\t" // Умножить на array2[i]
            "movl %%eax, %[res]\n\t"   // Сохранить результат в result[i]
            : [res] "=m"(result[i])    // Выходной операнд
            : [val1] "r"(array1[i]), [val2] "r"(array2[i]) // Входные операнды
            : "eax" // Используемый регистр
        );
    }
}

// Функция обработки отключения клиента
void process_disconnection(const std::string& client_name) {
    // Выводим сообщение о разрыве соединения
    std::cout << client_name << " disconnected.\n";
    array1[0] = 2;
    array2[0] = 3;
    std::cout << "Array1: ";
    for (size_t i = 0; i < 1; ++i) {
        std::cout << array1[i] << " ";
    }
    std::cout << "\n";

    std::cout << "Array2: ";
    for (size_t i = 0; i < 1; ++i) {
        std::cout << array2[i] << " ";
    }
    std::cout << "\n";

    // Перемножение массивов
    size_t min_size = std::min(array1_size, array2_size);
    int result[MAX_NUMBERS] = {0};
    multiply_arrays(array1, array2, result, 1);

    // Вывод результата
    std::cout << "Result of array multiplication:\n";
    for (size_t i = 0; i < 1; ++i) {
        std::cout << result[i] << " ";
    }
    std::cout << "\n";

    // Очистка массивов
    array1_size = 0;
    array2_size = 0;
}

// Модифицированная функция отправки сообщения конкретному клиенту
void send_to_client(const std::string& message, const std::string& target_name, std::map<SOCKET, std::string>& clients, std::mutex& clients_mutex) {
    std::lock_guard<std::mutex> lock(clients_mutex);

    // Вывод сообщения для отладки
    std::cout << "Message to " << target_name << ": " << message << "\n";

    // Извлечение чисел и добавление их в массив
    if (target_name == "Client1") {
        extract_numbers(message, array1, array1_size);
    } else if (target_name == "Client2") {
        extract_numbers(message, array2, array2_size);
    }

    for (const auto& client : clients) {
        if (client.second == target_name) {
            send(client.first, message.c_str(), message.size(), 0);
            return;
        }
    }
}
