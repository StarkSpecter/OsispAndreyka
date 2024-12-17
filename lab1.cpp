#include <windows.h>
#include <iostream>
#include <vector>
#include <chrono>

using namespace std;

const int M = 512;  // Размер матриц MxM
const int N = 8;    // Количество потоков (нитей)

// Матрица A и B
vector<vector<int>> A(M, vector<int>(M));
vector<vector<int>> B(M, vector<int>(M));
vector<vector<int>> C(M, vector<int>(M));  // Результат умножения

// Функция для умножения матриц (линейная версия)
void multiplyMatricesLinear() {
    for (int i = 0; i < M; ++i) {
        for (int j = 0; j < M; ++j) {
            C[i][j] = 0;
            for (int k = 0; k < M; ++k) {
                C[i][j] += A[i][k] * B[k][j];
            }
        }
    }
}

// Структура данных для аргументов потока
struct ThreadData {
    int startRow;
    int endRow;
};

// Функция для умножения матриц в многопоточном режиме
DWORD WINAPI multiplyMatricesParallel(LPVOID param) {
    ThreadData* data = (ThreadData*)param;
    for (int i = data->startRow; i < data->endRow; ++i) {
        for (int j = 0; j < M; ++j) {
            C[i][j] = 0;
            for (int k = 0; k < M; ++k) {
                C[i][j] += A[i][k] * B[k][j];
            }
        }
    }

    return 0;
}

// Функция для создания случайной матрицы
void generateMatrix(vector<vector<int>>& matrix) {
    for (int i = 0; i < M; ++i) {
        for (int j = 0; j < M; ++j) {
            matrix[i][j] = rand() % 10;
        }
    }
}

int main() {
    srand(GetTickCount());
    
    // Генерация матриц
    generateMatrix(A);
    generateMatrix(B);

    // Линейное умножение
    auto start = chrono::high_resolution_clock::now();
    multiplyMatricesLinear();
    auto end = chrono::high_resolution_clock::now();
    auto duration = chrono::duration_cast<chrono::milliseconds>(end - start);
    cout << "Линейное умножение: " << duration.count() << " }