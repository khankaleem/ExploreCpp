#include <vector>
#include <condition_variable>
#include <mutex>

/*
    Multi Reader Multi Writer Queue
    This is a bounded Circular Queue
*/

class MRMWLockedQueue {
    std::vector<int> m_arr;
    size_t m_front{0};
    size_t m_rear{0};
    size_t m_size{0};

    std::mutex m_locker;
    std::condition_variable m_condv_not_full; // reader notifes writer on pop, writer waits for it
    std::condition_variable m_condv_not_empty; // writer notifes reader on push, reader waits for it

    using LockGuard = std::lock_guard<std::mutex>;
    using UniqueLock = std::unique_lock<std::mutex>;

public:
    MRMWLockedQueue(size_t capacity_) : m_size{capacity_+1} {
        if (capacity_ == 0) {
            throw std::invalid_argument("MRMWLockedQueue size cannot be 0!");
        }
        m_arr.resize(capacity_+1, 0);
    }

    MRMWLockedQueue(const MRMWLockedQueue&) = delete;
    MRMWLockedQueue& operator=(const MRMWLockedQueue&) = delete;

    // Writer Calls
    bool try_push(int val) {
        LockGuard guard{m_locker};
        if ( (m_rear + 1)%m_size == m_front ) { // full
            return false;
        }
        m_arr[m_rear] = val;
        m_rear = (m_rear + 1)%m_size;
        m_condv_not_empty.notify_one();
        return true;
    }
    void wait_and_push(int val) {
        UniqueLock lock{m_locker};
        // wait until queue is not full
        m_condv_not_full.wait(lock, [this]() { return ((m_rear + 1)%m_size != m_front); } ); 
        m_arr[m_rear] = val;
        m_rear = (m_rear + 1)%m_size;
        m_condv_not_empty.notify_one();
    }

    // Reader Calls
    std::pair<bool, int> try_pop() {
        LockGuard guard{m_locker};
        if ( m_rear == m_front ) { // empty
            return {false, 0};
        }
        int elt = m_arr[m_front];
        m_front = (m_front + 1)%m_size;
        m_condv_not_full.notify_one();
        return {true, elt};
    }
    int wait_and_pop() {
        UniqueLock lock{m_locker};
        // wait until queue is not empty
        m_condv_not_empty.wait(lock, [this]() { return (this->m_rear != this->m_front); } );
        int elt = m_arr[m_front];
        m_front = (m_front + 1)%m_size;
        m_condv_not_full.notify_one();
        return elt;
    }

};


int main() {
    MRMWLockedQueue q{100};
}

