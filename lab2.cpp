#include <windows.h>
#include <iostream>
#include <queue>
#include <string>
#include <sstream>
#include <vector>
#include <chrono>
#include <fstream>

using namespace std;
using namespace std::chrono;

CRITICAL_SECTION readMutex;  
CRITICAL_SECTION writeMutex; 
CONDITION_VARIABLE dataReady;
queue<string> readQueue;
queue<string> writeQueue;
bool doneReading = false;
bool doneProcessing = false;

void HandleError(const string& message) {
    cerr << message << ". Error: " << GetLastError() << endl;
    exit(EXIT_FAILURE);
}


int Sync() {
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

    std::cout << "Sync: " << duration.count() << " seconds." << std::endl;

    return 0;
}

DWORD WINAPI asyncRead(LPVOID lpParam) {
    auto params = reinterpret_cast<pair<HANDLE, DWORD>*>(lpParam);
    HANDLE hInputFile = params->first;
    DWORD bufferSize = params->second;
    delete params; 
    char* buffer = new char[bufferSize];
    OVERLAPPED overlappedRead = {0};
    overlappedRead.hEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
    DWORD offset = 0;
    while (true) {
        overlappedRead.Offset = offset;
        if (!ReadFile(hInputFile, buffer, bufferSize, nullptr, &overlappedRead)) {
            if (GetLastError() != ERROR_IO_PENDING) {
                HandleError("Failed to initiate async read");
            }
        }
        WaitForSingleObject(overlappedRead.hEvent, INFINITE);
        DWORD bytesRead = 0;
        if (!GetOverlappedResult(hInputFile, &overlappedRead, &bytesRead, FALSE) || bytesRead == 0) {
            break;
        }
        EnterCriticalSection(&readMutex);
        readQueue.push(string(buffer, bytesRead));
        offset += bytesRead;
        WakeConditionVariable(&dataReady);
        LeaveCriticalSection(&readMutex);
    }
    doneReading = true;
    WakeConditionVariable(&dataReady);
    CloseHandle(overlappedRead.hEvent);
    delete[] buffer;
    return 0;
}

DWORD WINAPI processData(LPVOID) {
    while (true) {
        EnterCriticalSection(&readMutex);  
        while (readQueue.empty() && !doneReading) {
            SleepConditionVariableCS(&dataReady, &readMutex, INFINITE);
        }
        if (readQueue.empty() && doneReading) {
            LeaveCriticalSection(&readMutex);
            break;
        }
        string chunk = readQueue.front();
        readQueue.pop();
        LeaveCriticalSection(&readMutex);
        istringstream inputStream(chunk);
        ostringstream outputStream;
        int number;
        while (inputStream >> number) {
            number += 10;
            outputStream << number << '\n';
        }
        EnterCriticalSection(&writeMutex);  
        writeQueue.push(outputStream.str());
        WakeConditionVariable(&dataReady);
        LeaveCriticalSection(&writeMutex);
    }

    doneProcessing = true;
    WakeConditionVariable(&dataReady);
    return 0;
}

DWORD WINAPI asyncWrite(LPVOID lpParam) {
    HANDLE hOutputFile = reinterpret_cast<HANDLE>(lpParam);
    OVERLAPPED overlappedWrite = {0};
    overlappedWrite.hEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
    DWORD offset = 0;
    while (true) {
        EnterCriticalSection(&writeMutex);  // Используем writeMutex
        while (writeQueue.empty() && !doneProcessing) {
            SleepConditionVariableCS(&dataReady, &writeMutex, INFINITE);
        }
        if (writeQueue.empty() && doneProcessing) {
            LeaveCriticalSection(&writeMutex);
            break;
        }
        string dataToWrite = writeQueue.front();
        writeQueue.pop();
        LeaveCriticalSection(&writeMutex);
        overlappedWrite.Offset = offset;
        if (!WriteFile(hOutputFile, dataToWrite.c_str(), dataToWrite.size(), nullptr, &overlappedWrite)) {
            if (GetLastError() != ERROR_IO_PENDING) {
                HandleError("Failed to initiate async write");
            }
        }
        WaitForSingleObject(overlappedWrite.hEvent, INFINITE);
        DWORD bytesWritten = 0;
        if (!GetOverlappedResult(hOutputFile, &overlappedWrite, &bytesWritten, FALSE)) {
            HandleError("Failed to get result of async write");
        }
        offset += bytesWritten;
    }
    CloseHandle(overlappedWrite.hEvent);
    return 0;
}

int main() {
    const string inputFilePath = "Z:\\Users\\andreitsyrkunov\\Desktop\\c++\\numbers.txt";
    const string outputFilePath = "Z:\\Users\\andreitsyrkunov\\Desktop\\c++\\result_numbers_buffer_";
    const vector<DWORD> bufferSizes = {20000};
    InitializeCriticalSection(&readMutex);   
    InitializeCriticalSection(&writeMutex);  
    InitializeConditionVariable(&dataReady);
    for (const auto& bufferSize : bufferSizes) {
        cout << "Testing with buffer size: " << bufferSize << " bytes" << endl;
        HANDLE hInputFile = CreateFile(inputFilePath.c_str(), GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, nullptr);
        if (hInputFile == INVALID_HANDLE_VALUE) {
            HandleError("Failed to open input file");
        }
        HANDLE hOutputFile = CreateFile((outputFilePath + to_string(bufferSize) + ".txt").c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_FLAG_OVERLAPPED, nullptr);
        if (hOutputFile == INVALID_HANDLE_VALUE) {
            HandleError("Failed to open output file");
        }
        auto start = high_resolution_clock::now();
        HANDLE hReaderThread = CreateThread(nullptr, 0, asyncRead, new pair<HANDLE, DWORD>(hInputFile, bufferSize), 0, nullptr);
        HANDLE hProcessorThread = CreateThread(nullptr, 0, processData, nullptr, 0, nullptr);
        HANDLE hWriterThread = CreateThread(nullptr, 0, asyncWrite, hOutputFile, 0, nullptr);
        WaitForSingleObject(hReaderThread, INFINITE);
        WaitForSingleObject(hProcessorThread, INFINITE);
        WaitForSingleObject(hWriterThread, INFINITE);
        CloseHandle(hReaderThread);
        CloseHandle(hProcessorThread);
        CloseHandle(hWriterThread);
        auto end = high_resolution_clock::now();
        duration<double> duration = end - start;
        CloseHandle(hInputFile);
        CloseHandle(hOutputFile);
        cout << "Async: " << duration.count() << " seconds." << endl << endl;
    }
    DeleteCriticalSection(&readMutex);   // Удаляем readMutex
    DeleteCriticalSection(&writeMutex);  // Удаляем writeMutex
    Sync();
    return 0;
}
