#include<atomic>
#include<thread>
#include<cassert>
#include<iostream>
/*
    Single Producer Single Consumer Lock Free Low Latency Queue
*/
template<typename T, size_t SIZE = (1<<10)>
class SPSClockFree {
    static_assert( SIZE > 1 && !(SIZE & (SIZE-1)), "Size should be a power of two greater than 1" );

    T* m_ptr{nullptr};
     // front : where read will take place
     // align with 64 to avoid false sharing
    alignas(64) std::atomic<uint64_t> m_front{0};
     // rear : where next write will take place
     // align with 64 to avoid false sharing
    alignas(64) std::atomic<uint64_t> m_rear{0};
    /* front == rear -> queue is empty
       (rear + 1)%SIZE = front -> queue is full
       (rear + 1)%SIZE is same as (rear + 1) & (SIZE - 1) for SIZE = 2^x
       1 slot will be free 
    */
public:
    SPSClockFree() {
        // allocate memory
        m_ptr = reinterpret_cast<T*>(::operator new[](SIZE * sizeof(T), std::align_val_t(alignof(T))));
        // if T has a default CTOR we can optimize 
        // this by constructing all object befre ahand, and only moveing / copying in push
        // and not detroying in pop
    }

    // Rule of 5
    ~SPSClockFree() {
        auto rear = m_rear.load(std::memory_order_relaxed);
        auto front = m_front.load(std::memory_order_relaxed);
        while(front != rear) {
            (m_ptr + front)->~T();
            front = (front + 1) & (SIZE - 1);
        }
        ::operator delete[](m_ptr,  std::align_val_t(alignof(T)));
    }
    SPSClockFree(const SPSClockFree&) = delete;
    SPSClockFree& operator=(const SPSClockFree&) = delete;
    // Rule of 5 end

    /*
        writer must be sure that slot is available to write
           : pop/top - push need to form a happens before relationship  

        reader must be sure that data is written
           : push - pop/top need to form a happens before relationship
    */
    // writer calls
    bool try_push(const T& val) {
        // synchronize with reader to ensure that data is read if queue is not full
        auto rear = m_rear.load(std::memory_order_relaxed);
        auto front = m_front.load(std::memory_order_acquire);
        if (((rear + 1) & (SIZE - 1)) == front) {
            return false;
        }
        new (m_ptr + rear) T(val); // copy construct
        // data is succsessfully written - release the rear index for reader to synchronize with
        m_rear.store( (rear + 1) & (SIZE - 1), std::memory_order_release ); 
        return true;
    }
    bool try_push(T&& val) {
        // synchronize with reader to ensure that data is read if queue is not full
        auto rear = m_rear.load(std::memory_order_relaxed);
        auto front = m_front.load(std::memory_order_acquire);
        if (((rear + 1) & (SIZE - 1)) == front) {
            return false;
        }
        new (m_ptr + rear) T(std::move(val)); // move construct
        // data is succsessfully written - release the rear index for reader to synchronize with
        m_rear.store( (rear + 1) & (SIZE - 1), std::memory_order_release ); 
        return true;
    }
    template<typename... ArgsT>
    bool try_emplace(ArgsT&&... args) {
        // synchronize with reader to ensure that data is read if queue is not full
        auto rear = m_rear.load(std::memory_order_relaxed);
        auto front = m_front.load(std::memory_order_acquire);
        if (((rear + 1) & (SIZE - 1)) == front) {
            return false;
        }
        new (m_ptr + rear) T(std::forward<ArgsT>(args)...); // in place construct
        // data is succsessfully written - release the rear index for reader to synchronize with
        m_rear.store( (rear + 1) & (SIZE - 1), std::memory_order_release ); 
        return true;
    }
    bool full() const {
        // synchronize with reader to ensure that data is read if queue is not full
        auto rear  = m_rear.load(std::memory_order_relaxed);
        auto front = m_front.load(std::memory_order_acquire);
        return (((rear + 1) & (SIZE - 1)) == front);
    }

    //reader calls
    T* top() {
        // synchronize with writer to ensure that data is written if queue is not empty
        auto rear = m_rear.load(std::memory_order_acquire);
        auto front = m_front.load(std::memory_order_relaxed);
        if (rear == front) { // empty
            return nullptr;
        }
        return (m_ptr + front);
    }
    const T* top() const {
        // synchronize with writer to ensure that data is written if queue is not empty
        auto rear = m_rear.load(std::memory_order_acquire);
        auto front = m_front.load(std::memory_order_relaxed);
        if (rear == front) { // empty
            return nullptr;
        }
        return (m_ptr + front);
    }
    bool try_pop() {
        // synchronize with writer to ensure that data is written if queue is not empty
        auto rear = m_rear.load(std::memory_order_acquire);
        auto front = m_front.load(std::memory_order_relaxed);
        if (rear == front) { // empty
            return false;
        }
        (m_ptr + front)->~T(); // Call Dtor
        // Data has been read succesfully
        // data is succsessfully read - release the front index for writer to synchronize with
        m_front.store((front + 1) & (SIZE - 1), std::memory_order_release);
        return true;   
    }
    bool empty() const {
        // synchronize with writer to ensure that data is written if queue is not empty
        auto rear = m_rear.load(std::memory_order_acquire);
        auto front = m_front.load(std::memory_order_relaxed);
        return (rear == front);
    }
};

int main() {

    SPSClockFree<int, 1<<5> q;
    int p = 0;
    int c = 0;
    std::thread r{
        [&q, &c]() {
            for(int i = 0; i < 1000; i++) {
                auto ptr = q.top();
                c += (ptr == nullptr ? 0 : *ptr);
                if (q.top()) {
                    q.try_pop();
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(2));
            }
        }
    };
    std::thread w{
        [&q, &p]() {
            for(int i = 1; i < 100; i++) {
                if ( q.try_push(i) ) {
                    p += i;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(3));
            }
        }
    };
    r.join();
    w.join();
    assert(p == c);

}
