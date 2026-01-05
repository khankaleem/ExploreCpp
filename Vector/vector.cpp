/*
Pending : Trivial types Fast Path / Allocator / exception safety of new / Iterator / Testing
*/
template<typename T>
class Vector {
private:
    T* m_arr{nullptr};
    size_t m_cap{0};
    size_t m_size{0};

    void* allocateRaw(size_t size) {
        // Allocate raw bytes, set alignment as well while allocating raw bytes
        return ::operator new[](size * sizeof(T), std::align_val_t(alignof(T)));
    }
    void increaseCapacity(size_t newCap) {
        assert(newCap >= m_cap);
        // Allocate new memory
        T* newArrPtr = reinterpret_cast<T*>(allocateRaw(newCap));
        // Move
        for (size_t i = 0; i < m_size; i++) { 
            new (newArrPtr + i) T(std::move_if_noexcept(m_arr[i]));
        }
        // Destroy
        for (size_t i = 0; i < m_size; i++) {
            (m_arr + i)->~T();
        }
        ::operator delete[](m_arr, std::align_val_t(alignof(T)));  
        // Save
        m_arr = newArrPtr;
        m_cap = newCap;
    }
    void reset() noexcept {
        for (size_t i = 0; i < m_size; i++) {
            (m_arr + i)->~T();
        }
        ::operator delete[](m_arr, std::align_val_t(alignof(T)));  
        m_size = 0;
        m_cap = 0;  
        m_arr = nullptr;
    }
public:
    Vector() = default;

    // Parametrized Ctors
    Vector(size_t size, const T& defaultValue) {
        if (size == 0) return;
        // Allocate
        m_arr = reinterpret_cast<T*>(allocateRaw(size));
        // Copy Construct T
        for (size_t i = 0; i < size; i++) {
            new (m_arr + i) T(defaultValue);
        }
        m_size = size;
        m_cap = size;
    }
    Vector(size_t size) {
        if (size == 0) return;
        // Increase capacity
        m_arr = reinterpret_cast<T*>(allocateRaw(size));
        // Default Construct T
        for (size_t i = 0; i < size; i++) {
            new (m_arr + i) T();
        }
        m_size = size;
        m_cap = size;
    }

    // Parametrized Ctors End

    //..... Rule of 5 Start .....
    ~Vector() {
        reset();
    }

    // Copying does not copy the capacity, only elements are assured to be copied
    Vector(const Vector& other) {
        if (other.m_size == 0) return;
        m_arr = reinterpret_cast<T*>(allocateRaw(other.m_size));
        for (size_t i = 0; i < other.m_size; i++) {
            new (m_arr + i) T(other.m_arr[i]);
        }
        m_size = other.m_size;
        m_cap = m_size;
    }

    Vector(Vector&& other) noexcept {
        // Move
        m_arr = other.m_arr;
        m_cap = other.m_cap;
        m_size = other.m_size;
        // Invalidate other 
        other.m_arr = nullptr;
        other.m_size = 0;
        other.m_cap = 0;
    }

    // Assigning does not copy the capacity, only elements are assured to be copied
    Vector& operator=(const Vector&other) {
        if (this == &other) return *this;
        if (other.m_size >= m_size) {
            if (other.m_size > m_cap) {
                increaseCapacity(other.m_size);
            }
            for (size_t i = 0; i < m_size; i++) {
                m_arr[i] = other.m_arr[i];
            }
            for (size_t i = m_size; i < other.m_size; i++) {
                new (m_arr + i) T(other.m_arr[i]);
            }
        }
        else {
            for (size_t i = other.m_size; i < m_size; i++) {
                (m_arr + i)->~T();
            }
            for (size_t i = 0; i < other.m_size; i++) {
                m_arr[i] = other.m_arr[i];
            }
        }
        m_size = other.m_size;
        return *this;
    }

    Vector& operator=(Vector&& other) noexcept {
        if (this == &other) return *this;
        // Release Resources
        reset();
        // Move
        m_arr = other.m_arr;
        m_cap = other.m_cap;
        m_size = other.m_size;
        // Invalidate other 
        other.m_arr = nullptr;
        other.m_size = 0;
        other.m_cap = 0;
        return *this;
    }
    //..... Rule of 5  End ..... 
    
    void resize(size_t newSize) {
        if (m_size < newSize) {
            if (m_cap < newSize) {
                increaseCapacity(newSize);
            }
            for(size_t i = m_size; i < newSize; i++) {
                new (m_arr + i) T();
            }
            m_size = newSize;
        }
        else if (m_size > newSize) {
            // reduce to m_size elements
            for(size_t i = newSize; i < m_size; i++) {
                (m_arr + i)->~T();
            }
            m_size = newSize;
        }
    }
    void resize(size_t newSize, const T& defaultValue) {
        if (m_size < newSize) {
            if (m_cap < newSize) {
                increaseCapacity(newSize);
            }
            for(size_t i = m_size; i < newSize; i++) {
                new (m_arr + i) T(defaultValue);
            }
            m_size = newSize;
        }
        else if (m_size > newSize) {
            // reduce to m_size elements
            for(size_t i = newSize; i < m_size; i++) {
                (m_arr + i)->~T();
            }
            m_size = newSize;
        }
    }
    void shrinkToFit() {
        if (m_cap == 0) {
            return;
        }
        if (m_size == 0) {
            reset();
            return;
        }
        if (m_size == m_cap) {
            return;
        }
        T* newArrayPtr = reinterpret_cast<T*>(allocateRaw(m_size));
        for (size_t i = 0; i < m_size; i++) {
            new (newArrayPtr+i) T(std::move_if_noexcept(m_arr[i]));
        }
    
        for (size_t i = 0; i < m_size; ++i) {
            (m_arr + i)->~T();
        }
        ::operator delete[](m_arr, std::align_val_t(alignof(T)));
        m_cap = m_size;
        m_arr = newArrayPtr;
    }
    void reserve(size_t capacity) {
        if (m_cap >= capacity) return;
        increaseCapacity(capacity);
    }

    // Appending functions start...
    void push_back(const T& val) {
        if (m_size == m_cap) {
            increaseCapacity(m_cap ? m_cap << 1 : 1);
        }
        new (m_arr + m_size) T(val);
        m_size++;
    }
    void push_back(T&& val) {
        if (m_size == m_cap) {
            increaseCapacity(m_cap ? m_cap << 1 : 1);
        }
        new (m_arr + m_size) T(std::move(val));
        m_size++;
    }
    template<typename... ArgsT>
    void emplace_back(ArgsT&&... args) {
        if (m_size == m_cap) {
            increaseCapacity(m_cap ? m_cap << 1 : 1);
        }
        new (m_arr + m_size) T(std::forward<ArgsT>(args)...);
        m_size++;
    }
    // Appending functions end...

    // Back
    const T& back() const {
        return m_arr[m_size - 1];
    }

    // at
    T& at(size_t idx) {
        if (idx < m_size) {
            return m_arr[idx];
        }
        throw std::out_of_range{"Array Index Out of bounds"};
    }
    const T& at(size_t idx) const {
        if (idx < m_size) {
            return m_arr[idx];
        }
        throw std::out_of_range{"Array Index Out of bounds"};
    }

    // Operator []
    T& operator[](size_t idx) {
        return m_arr[idx];
    }
    const T& operator[](size_t idx) const {
        return m_arr[idx];
    }

    // Clear : Retrieve capacity but destroy elements
    void clear() {
        for (size_t i = 0; i < m_size; i++) {
            (m_arr + i)->~T();
        }
        m_size = 0;
    }

    bool empty() const {
        return m_size == 0;
    }
    size_t capacity() const { return m_cap; }
    size_t size() const { return m_size; }
};

int main() {
    Vector<int> x;
    x.push_back(100);
    std::cout << x[0];
}
