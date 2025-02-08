#include <chrono>
#include <thread>

void sleep(int ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}
