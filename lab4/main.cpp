#include <iostream>
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <random>
#include <chrono>

using namespace std;

class Buffer {
public:
    Buffer(int capacity) : capacity(capacity) {}

    bool addRequest(int request) {
        lock_guard<mutex> lock(mtx);
        if (queue.size() < capacity) {
            queue.push(request);
            return true;
        }
        return false;
    }

    bool getRequest(int& request) {
        lock_guard<mutex> lock(mtx);
        if (!queue.empty()) {
            request = queue.front();
            queue.pop();
            return true;
        }
        return false;
    }

    bool isEmpty() {
        lock_guard<mutex> lock(mtx);
        return queue.empty();
    }

    bool isFull() {
        lock_guard<mutex> lock(mtx);
        return queue.size() == capacity;
    }

private:
    int capacity;
    queue<int> queue;
    mutex mtx;
};

class Channel {
public:
    Channel(int id, Buffer& inputBuffer, Buffer& outputBuffer)
        : id(id), inputBuffer(inputBuffer), outputBuffer(outputBuffer), isBusy(false) {}

    void processRequests() {
        while (true) {
            int request;
            {
                unique_lock<mutex> lock(mtx);
                cv.wait(lock, [this] { return !inputBuffer.isEmpty(); });
                if (inputBuffer.getRequest(request)) {
                    isBusy = true;
                    lock.unlock();
                    this_thread::sleep_for(chrono::milliseconds(rand() % 1000 + 500)); // Обработка заявки
                    cout << "Канал " << id << " обработал заявку " << request << endl;
                    outputBuffer.addRequest(request); // Перенос заявки в следующий буфер
                    isBusy = false;
                }
            }
        }
    }

    void notify() {
        cv.notify_one();
    }

    bool isChannelBusy() {
        return isBusy;
    }

private:
    int id;
    Buffer& inputBuffer;
    Buffer& outputBuffer;
    bool isBusy;
    mutex mtx;
    condition_variable cv;
};

class Step {
public:
    Step(int numChannels, int bufferCapacity) {
        inputBuffer = make_shared<Buffer>(bufferCapacity);
        outputBuffer = make_shared<Buffer>(bufferCapacity);
        for (int i = 0; i < numChannels; ++i) {
            channels.push_back(make_shared<Channel>(i + 1, *inputBuffer, *outputBuffer));
        }
    }

    void startProcessing() {
        for (auto& channel : channels) {
            threads.push_back(thread(&Channel::processRequests, channel));
        }
    }

    void addRequestToStep(int request) {
        while (true) {
            bool added = inputBuffer->addRequest(request);
            if (added) {
                // Уведомляем каналы о новом запросе
                for (auto& channel : channels) {
                    channel->notify();
                }
                break;
            }
            this_thread::sleep_for(chrono::milliseconds(100)); // Ожидание освобождения канала
        }
    }

    shared_ptr<Buffer> getOutputBuffer() {
        return outputBuffer;
    }

    void waitForCompletion() {
        for (auto& thread : threads) {
            if (thread.joinable()) {
                thread.join();
            }
        }
    }

private:
    vector<shared_ptr<Channel>> channels;
    shared_ptr<Buffer> inputBuffer;
    shared_ptr<Buffer> outputBuffer;
    vector<thread> threads;
};

class System {
public:
    System(int numSteps, int numChannels, int bufferCapacity)
        : numSteps(numSteps), numChannels(numChannels), bufferCapacity(bufferCapacity) {
        for (int i = 0; i < numSteps; ++i) {
            steps.push_back(make_shared<Step>(numChannels, bufferCapacity));
        }
    }

    void startSimulation(int numRequests) {
        for (int i = 0; i < numRequests; ++i) {
            int request = i + 1; // Запрос может быть просто номером
            steps[0]->addRequestToStep(request); // Добавление заявки в первую ступень
        }

        // Начинаем обработку заявок
        for (auto& step : steps) {
            step->startProcessing();
        }

        // Ожидаем завершения всех потоков
        for (auto& step : steps) {
            step->waitForCompletion();
        }
    }

private:
    int numSteps;
    int numChannels;
    int bufferCapacity;
    vector<shared_ptr<Step>> steps;
};

int main() {
    srand(time(nullptr));

    int numSteps = 3;
    int numChannels = 2;
    int bufferCapacity = 5;
    int numRequests = 10;

    System system(numSteps, numChannels, bufferCapacity);
    system.startSimulation(numRequests);

    return 0;
}
