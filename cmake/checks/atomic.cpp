#include <atomic>
#include <cstdint>

std::atomic<uint64_t> a(0);

int main() {
    a++;
    return a.load();
}
