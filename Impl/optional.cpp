#include <iostream>
#include <new>    
#include <utility> 
#include <stdexcept>

template<typename T>
class Optional {
    alignas(T) char m_storage[sizeof(T)];
    bool m_engaged{};
    T* getPtr() {
        return std::launder(reinterpret_cast<T*>(m_storage));
    }
    const T* getPtr() const {
        return std::launder(reinterpret_cast<const T*>(m_storage));
    }
public:
    Optional() = default;
    explicit Optional(T&& val) {
        m_engaged = true;
        new (m_storage) T(std::move(val));
    }
    explicit Optional(const T& val) {
        m_engaged = true;
        new (m_storage) T(val);
    }

    // Rule of 5
    Optional(const Optional& other) : m_engaged{other.m_engaged} {
        if (!m_engaged) { return; }
        new (m_storage) T(other.value());
    }
    Optional(Optional&& other) : m_engaged{other.m_engaged} {
        if (!m_engaged) { return; }
        new (m_storage) T(std::move(other.value()));
        // no need to call other.reset
    }
    Optional& operator=(const Optional& other) {
        if (this == &other) { return *this; }
        if (other.m_engaged) {
            if (m_engaged) {
                value() = other.value();
            }
            else {
                new (m_storage) T(other.value());
                m_engaged = true;
            }
        }
        else {
            reset();
        }
        return *this;
    }
    Optional& operator=(Optional&& other) {
        if (this == &other) { return *this; }
        if (other.m_engaged) {
            if (m_engaged) {
                value() = std::move(other.value());
            }
            else {
                new (m_storage) T(std::move(other.value()));
                m_engaged = true;
            }
        }
        else {
            reset();
        }
        return *this;
    }
    ~Optional() {
        reset();
    }

    void reset() {
        if (m_engaged) {
            getPtr()->~T();
            m_engaged = false;
        }
    }

    template<typename... ArgsT>
    T& emplace(ArgsT&&... args_) {
        reset();
        new (m_storage) T(std::forward<ArgsT>(args_)...);
        m_engaged = true;
        return value();
    }

    T& value() {
        if (!m_engaged) throw std::bad_optional_access{};
        return *(getPtr());
    }
    const T& value() const {
        if (!m_engaged) throw std::bad_optional_access{};
        return *(getPtr());
    }
    bool hasValue() const {
        return m_engaged;
    }

    explicit operator bool() const {
        return m_engaged;
    }
    T& operator*() {
        return value();
    }
    const T& operator*() const {
        return value();
    }
    T* operator->() {
        return getPtr();
    }
    const T* operator->() const {
        return getPtr();
    }
};


int main() {
    Optional<int> opt{500};
    Optional<int> opt1{800};
    std::cout << opt.value();
    opt.reset();
    opt = opt1;
    std::cout << opt.value();
}

