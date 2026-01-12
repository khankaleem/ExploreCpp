#include<vector>
#include<thread>
#include<atomic>
#include<cassert>

class SpinLock {
public:
    SpinLock() = default;
    void lock() {
        while(m_flag.test_and_set(std::memory_order_acquire)) {
            asm volatile("pause" ::: "memory");
        }
    }
    void unlock() {
        m_flag.clear(std::memory_order_release);
    }

    SpinLock(const SpinLock& other) = delete;
    SpinLock& operator=(const SpinLock& other) = delete;

private:
    std::atomic_flag m_flag = ATOMIC_FLAG_INIT;
};

int main() {
    SpinLock lock;
    int sum = 0;
    int n = 10000;
    std::vector<std::thread> v;
    for(int i = 0; i < 10; i++) {
        v.push_back(std::thread{
            [&sum, &lock, n]() {
                for(int i = 0; i < n+1; i++) {
                    lock.lock();
                    sum += i;
                    lock.unlock();
                }
            }});
    }
    for(int i = 0; i < 10; i++) {
        v[i].join();
    }
    assert( sum == (1LL * v.size() * (n * (n+1)))/2 );
}





