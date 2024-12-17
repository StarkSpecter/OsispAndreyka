#include <windows.h>
#include <iostream>
#include <string>
#include <vector>

const int BUFFER_SIZE = 256;       // Размер одного буфера
const int NUM_BUFFERS = 3;         // Количество буферов
const char* SHARED_MEMORY_NAME = "Global\\SharedMemoryExample";
const char* MUTEX_NAME_PREFIX = "Global\\MutexBuffer";

struct SharedMemory {
    char buffers[NUM_BUFFERS][BUFFER_SIZE];
    bool bufferInUse[NUM_BUFFERS];
};

// Функция работы "пользовательского" процесса
void ProcessBuffer(int processID, SharedMemory* sharedMemory) {
    while (true) {
        for (int i = 0; i < NUM_BUFFERS; ++i) {
            // Имя мьютекса для текущего буфера
            std::string mutexName = MUTEX_NAME_PREFIX + std::to_string(i);
            HANDLE hMutex = OpenMutexA(SYNCHRONIZE, FALSE, mutexName.c_str());

            if (hMutex == nullptr) {
                std::cerr << "Ошибка открытия мьютекса для буфера " << i << "\n";
                return;
            }

            // Захват мьютекса для работы с буфером
            WaitForSingleObject(hMutex, INFINITE);

            if (!sharedMemory->bufferInUse[i]) {
                sharedMemory->bufferInUse[i] = true;  // Занимаем буфер
                std::string data = "Процесс " + std::to_string(processID) + " обрабатывает данные";
                strncpy(sharedMemory->buffers[i], data.c_str(), BUFFER_SIZE - 1);
                sharedMemory->buffers[i][BUFFER_SIZE - 1] = '\0'; // Терминатор строки

                std::cout << "Процесс " << processID << " записал данные в буфер " << i << ": " << sharedMemory->buffers[i] << "\n";
                sharedMemory->bufferInUse[i] = false; // Освобождаем буфер
            }

            // Освобождение мьютекса
            ReleaseMutex(hMutex);
            CloseHandle(hMutex);
            Sleep(1000);  // Задержка для имитации работы
        }
    }
}

int main() {
    // Создаем объект разделяемой памяти
    HANDLE hMapFile = CreateFileMappingA(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0, sizeof(SharedMemory), SHARED_MEMORY_NAME);
    if (hMapFile == nullptr) {
        std::cerr << "Ошибка создания разделяемой памяти\n";
        return 1;
    }

    // Маппинг разделяемой памяти в адресное пространство
    SharedMemory* sharedMemory = static_cast<SharedMemory*>(MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(SharedMemory)));
    if (sharedMemory == nullptr) {
        std::cerr << "Ошибка маппинга разделяемой памяти\n";
        CloseHandle(hMapFile);
        return 1;
    }

    // Инициализация буферов и мьютексов
    for (int i = 0; i < NUM_BUFFERS; ++i) {
        sharedMemory->bufferInUse[i] = false;
        std::string mutexName = MUTEX_NAME_PREFIX + std::to_string(i);
        HANDLE hMutex = CreateMutexA(nullptr, FALSE, mutexName.c_str());
        if (hMutex == nullptr) {
            std::cerr << "Ошибка создания мьютекса для буфера " << i << "\n";
            UnmapViewOfFile(sharedMemory);
            CloseHandle(hMapFile);
            return 1;
        }
        CloseHandle(hMutex);
    }

    // Запуск "пользовательских" процессов
    const int NUM_PROCESSES = 2;
    std::vector<HANDLE> processes;

    for (int i = 0; i < NUM_PROCESSES; ++i) {
        STARTUPINFOA si = { sizeof(STARTUPINFOA) };
        PROCESS_INFORMATION pi;
        std::string commandLine = "ProcessUser " + std::to_string(i); // Предположим, что отдельный исполняемый файл ProcessUser
        if (CreateProcessA(nullptr, commandLine.data(), nullptr, nullptr, FALSE, 0, nullptr, nullptr, &si, &pi)) {
            processes.push_back(pi.hProcess);
            CloseHandle(pi.hThread);
        } else {
            std::cerr << "Ошибка запуска процесса " << i << "\n";
        }
    }

    // Ожидание завершения всех процессов
    WaitForMultipleObjects(NUM_PROCESSES, processes.data(), TRUE, INFINITE);

    // Очистка ресурсов
    for (HANDLE process : processes) {
        CloseHandle(process);
    }
    UnmapViewOfFile(sharedMemory);
    CloseHandle(hMapFile);

    std::cout << "Главный процесс завершен.\n";
    return 0;
}
