
#include<mutex>
#include<condition_variable>
#include<memory>
#include<queue>
#include<thread>
#include<cassert>
/*
Multiple Reader Multiple Writer Queue using mutex and condition variables
This is an Unbounded Queue
*/
template<typename T>
class MRMWLockedStdQueue {
    std::queue<T> m_queue;
    std::mutex m_lock;
    std::condition_variable m_cond;
    using LockGuard = std::lock_guard<std::mutex>;
public:
    MRMWLockedStdQueue() = default;

    /*
        Disable Copy and Move
    */
    MRMWLockedStdQueue(const MRMWLockedStdQueue&) = delete;
    MRMWLockedStdQueue& operator=(const MRMWLockedStdQueue&) = delete;

    // writer calls
    void push(const T& data) {
        LockGuard guard{m_lock};
        m_queue.push(data);
        m_cond.notify_one();// notify readers waiting
    }   
    void push(T&& data) {
        LockGuard guard{m_lock};
        m_queue.push(std::move(data));
        m_cond.notify_one();// notify readers waiting
    }


    //reader calls
    /*
        merge pop and top into one operation because:
         1. reader 1 calls top
         2. reader 2 calls top
         3. reader 1 calls pop
         4. reader 2 calls pop
         2nd element was never read by anyone

         since we are using std::queue and not our own structure, we need to acquire a copy  of the data
         we could have moved data from std::queue but if move ctor throws then we loose the object totally
         if copy throws thats fine, queue does not loose date

         also we cannot return by value, because again if copy / move throws we have lost the object

        basically if we are poping from the queue we need to ensure that the object reaches the user safely without exceptions
        
        for this we need to return either a pointer or take reference from user
        if we return pointer we cannot return raw because then user will have to free it, so we must return in an RAII wrapper

        we use both methods here
    */
    bool try_pop(T& val) {
        LockGuard guard{m_lock};
        if (m_queue.empty()) {
            return false;
        }
        if constexpr (std::is_nothrow_move_assignable_v<T>) {
            val = std::move(m_queue.front()); // Fast, safe move
        }
        else {
            val = m_queue.front(); // Fallback to copy for exception safety
        }
        // pop is safe now
        m_queue.pop();
        return true;
    }
    std::shared_ptr<T> try_pop() {
        LockGuard guard{m_lock};
        if (m_queue.empty()) {
            return std::shared_ptr<T>{};
        }
        // assignment
        std::shared_ptr<T> ret{std::make_shared<T>(std::move_if_noexcept(m_queue.front()))}; 
        // pop is safe now
        m_queue.pop();
        return ret;
    }
    bool wait_and_pop_until(T& val, std::chrono::milliseconds timeout) {
        std::unique_lock<std::mutex> lock{m_lock};
        // got to sleep until data is ready
        bool ready = m_cond.wait_for(
                                        lock,
                                        timeout,
                                        [this]() { return !m_queue.empty(); }
                                    );
        if (!ready) {
            return false;
        }
        if constexpr (std::is_nothrow_move_assignable_v<T>) {
            val = std::move(m_queue.front()); // Fast, safe move
        }
        else {
            val = m_queue.front(); // Fallback to copy for exception safety
        }
        // pop is safe now
        m_queue.pop();
        return true;
    }
   std::shared_ptr<T> wait_and_pop_until(std::chrono::milliseconds timeout) {
        std::unique_lock<std::mutex> lock{m_lock};
        // got to sleep until data is ready
        bool ready = m_cond.wait_for(
                                        lock,
                                        timeout,
                                        [this]() { return !m_queue.empty(); }
                                    );
        if (!ready) {
            return std::shared_ptr<T>{};
        }
        // assignment
        std::shared_ptr<T> ret{std::make_shared<T>(std::move_if_noexcept(m_queue.front()))}; 
        // pop is safe now
        m_queue.pop();
        return ret;
    }
    bool empty() {
        LockGuard guard{m_lock};
        return m_queue.empty(); // little to no use as even after check reader may find the quue empty on pop
    }
};


int main() {
    MRMWLockedStdQueue<int> q;
    int64_t c1 = 0, c2 = 0, c3 = 0, c4 = 0;
    std::thread r1{[&q, &c3]() {
        pthread_setname_np("Reader1");
        int front = 0;
        for(int i = 0; i < 100000; i++) {
            bool ret = q.try_pop(front);
            c3 -= (ret ? front : 0);
            std::this_thread::sleep_for(std::chrono::microseconds(3));
        }
    }};
    std::thread r2{[&q, &c4]() {
        pthread_setname_np("Reader2");
        std::shared_ptr<int> front{};
        for(int i = 0; i < 10000; i++) {
            front = q.wait_and_pop_until(std::chrono::milliseconds(3));
            c4 -= (front != nullptr ? *front.get() : 0);
            std::this_thread::sleep_for(std::chrono::microseconds(3));
        }
    }};
    std::thread w1{[&q, &c1]() {
    pthread_setname_np("Writer1");
        for(int i = 0; i < 10000; i++) {
            q.push(i);
            c1 += i;
            std::this_thread::sleep_for(std::chrono::microseconds(1));
        }
    }};
    std::thread w2{[&q, &c2]() {
        pthread_setname_np("Writer2");
        for(int i = 0; i < 10000; i++) {
            c2 += i;
            q.push(i);
            std::this_thread::sleep_for(std::chrono::microseconds(1));
        }
    }};
    w1.join();
    w2.join();
    r1.join();
    r2.join();
    assert(c1+c2+c3+c4 == 0);
}
