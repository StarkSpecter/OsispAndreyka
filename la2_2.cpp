#include <windows.h>
#include <iostream>
#include <string>
#include <sstream>
#include <chrono>  // Для измерения времени
#include <vector>
#include <fstream>

void HandleError(const std::string& message) {
    std::cerr << message << ". Error: " << GetLastError() << std::endl;
    exit(EXIT_FAILURE);
}

int main() {
    const std::string inputFilePath = "Z:\\Users\\andreitsyrkunov\\Desktop\\c++\\numbers.txt";
    const std::string outputFilePath = "Z:\\Users\\andreitsyrkunov\\Desktop\\c++\\output_numbers.txt";

    // Измерение времени выполнения
    auto start = std::chrono::high_resolution_clock::now();

    // Чтение данных из файла
    std::ifstream inputFile(inputFilePath);
    if (!inputFile.is_open()) {
        HandleError("Failed to open input file");
    }

    std::vector<int> numbers;
    int number;
    while (inputFile >> number) {
        numbers.push_back(number);
    }
    inputFile.close();

    // Обработка данных (прибавление 10 к числам)
    for (auto& num : numbers) {
        num += 10;
    }

    // Запись данных в выходной файл
    std::ofstream outputFile(outputFilePath);
    if (!outputFile.is_open()) {
        HandleError("Failed to open output file");
    }

    for (const auto& num : numbers) {
        outputFile << num << '\n';
    }
    outputFile.close();

    // Завершение времени выполнения
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> duration = end - start;

    std::cout << "Data processing completed with traditional reading, processing, and writing." << std::endl;
    std::cout << "Time taken: " << duration.count() << " seconds." << std::endl;

    return 0;
}
