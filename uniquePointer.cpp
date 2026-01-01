

/*
Implement unique ptr with suport for deleters
*/

template<typename T>
struct DefaultDelete {
    void operator() (T* ptr) const {
        delete ptr;
    }
};

template<typename T>
struct DefaultDelete<T[]> {
    void operator() (T* ptr) const {
        delete []ptr;
    }
};

template<typename T, typename Deleter = DefaultDelete<T>>
class UniquePtr : private Deleter {
private:
    T* m_ptr{nullptr};
    
public:
    using Base = Deleter;

    UniquePtr() = default;
    // Explicit Ctor
    UniquePtr(std::nullptr_t) noexcept : Base{}, m_ptr{nullptr} {}
    explicit UniquePtr(T* ptr) : Base{}, m_ptr{ptr} {}
    template<typename U, 
             typename = std::enable_if_t<std::is_constructible_v<Base, U&&>>>
    explicit UniquePtr(T* ptr, U&& deleter) : Base{std::forward<U>(deleter)}, m_ptr{ptr} {}

    // deleter functions
    Base& getDeleter() {
        return static_cast<Base&>(*this);
    }
    const Base& getDeleter() const {
        return static_cast<const Base&>(*this);
    }

    // Rule of 5
    UniquePtr(const UniquePtr& other) = delete;
    UniquePtr& operator=(const UniquePtr& other) = delete;
    ~UniquePtr() {
        if (m_ptr) getDeleter()(m_ptr);
    }
    UniquePtr(UniquePtr&& other) noexcept : Base{std::move(other.getDeleter())}, m_ptr{other.m_ptr} {
        other.m_ptr = nullptr;
    }
    UniquePtr& operator=(UniquePtr&& other) noexcept {
        if (this != &other) {
            reset(other.release());
            getDeleter() = std::move(other.getDeleter());
        }
        return *this;
    }
    // Rule of 5 end

    // reset ownership
    void reset(T* ptr = nullptr) {
        if (m_ptr != ptr) {
            if (m_ptr) getDeleter()(m_ptr);
            m_ptr = ptr;
        }
    }
    // release ownership
    T* release() {
        T* save = m_ptr;
        m_ptr = nullptr;
        return save;
    }
    // get
    T* get() const {
        return m_ptr;
    }

    //nullptr assign
    UniquePtr& operator=(std::nullptr_t) noexcept {
        reset();
        return *this;
    }


    // operators
    T& operator*() {
        return *m_ptr;
    }
    const T& operator*() const {
        return *m_ptr;
    }
    T* operator->() {
        return m_ptr;
    }
    const T* operator->() const {
        return m_ptr;
    }

    // bool must be explicit
    explicit operator bool() const {
        return m_ptr != nullptr;
    }

    friend bool operator==(const UniquePtr& f,  std::nullptr_t) {
        return f.m_ptr == nullptr;
    }
    friend bool operator==(std::nullptr_t, const UniquePtr& f) {
        return f.m_ptr == nullptr;
    }
    friend bool operator!=(const UniquePtr& f,  std::nullptr_t) {
        return f.m_ptr != nullptr;
    }
    friend bool operator!=(std::nullptr_t, const UniquePtr& f) {
        return f.m_ptr != nullptr;
    }
    template<typename X, typename Y>
    friend bool operator==(const UniquePtr& f,  const UniquePtr<X, Y> &s) {
        return f.m_ptr == s.m_ptr;
    }
    template<typename X, typename Y>
    friend bool operator!=(const UniquePtr& f,  const UniquePtr<X, Y> &s) {
        return f.m_ptr != s.m_ptr;
    }
};
// Specialization for array
template<typename T,  typename Deleter>
class UniquePtr<T[], Deleter> : private Deleter  {
private:
    T* m_ptr{nullptr};
    
public:
    using Base = Deleter;

    UniquePtr() = default;
    // Explicit Ctor
    UniquePtr(std::nullptr_t) noexcept : Base{}, m_ptr{nullptr} {}
    explicit UniquePtr(T* ptr) : Base{}, m_ptr{ptr} {}
    template<typename U, 
             typename = std::enable_if_t<std::is_constructible_v<Base, U&&>>>
    explicit UniquePtr(T* ptr, U&& deleter) : Base{std::forward<U>(deleter)}, m_ptr{ptr} {}

    // deleter functions
    Base& getDeleter() {
        return static_cast<Base&>(*this);
    }
    const Base& getDeleter() const {
        return static_cast<const Base&>(*this);
    }

    // Rule of 5
    UniquePtr(const UniquePtr& other) = delete;
    UniquePtr& operator=(const UniquePtr& other) = delete;
    ~UniquePtr() {
        if (m_ptr) getDeleter()(m_ptr);
    }
    UniquePtr(UniquePtr&& other) noexcept : Base{std::move(other.getDeleter())}, m_ptr{other.m_ptr} {
        other.m_ptr = nullptr;
    }
    UniquePtr& operator=(UniquePtr&& other) noexcept {
        if (this != &other) {
            reset(other.release());
            getDeleter() = std::move(other.getDeleter());
        }
        return *this;
    }
    // Rule of 5 end

    // reset ownership
    void reset(T* ptr = nullptr) {
        if (m_ptr != ptr) {
            if (m_ptr) getDeleter()(m_ptr);
            m_ptr = ptr;
        }
    }
    // release ownership
    T* release() {
        T* save = m_ptr;
        m_ptr = nullptr;
        return save;
    }
    // get
    T* get() const {
        return m_ptr;
    }

    // nullptr assign
    UniquePtr& operator=(std::nullptr_t) noexcept {
        reset();
        return *this;
    }

    // operators
    T& operator[](size_t idx) {
        return m_ptr[idx];
    }
    const T& operator[](size_t idx) const {
        return m_ptr[idx];
    }
    // bool must be explicit
    explicit operator bool() const {
        return m_ptr != nullptr;
    }

    friend bool operator==(const UniquePtr& f,  std::nullptr_t) {
        return f.m_ptr == nullptr;
    }
    friend bool operator==(std::nullptr_t, const UniquePtr& f) {
        return f.m_ptr == nullptr;
    }
    friend bool operator!=(const UniquePtr& f,  std::nullptr_t) {
        return f.m_ptr != nullptr;
    }
    friend bool operator!=(std::nullptr_t, const UniquePtr& f) {
        return f.m_ptr != nullptr;
    }
    template<typename X, typename Y>
    friend bool operator==(const UniquePtr& f,  const UniquePtr<X[], Y> &s) {
        return f.m_ptr == s.m_ptr;
    }
    template<typename X, typename Y>
    friend bool operator!=(const UniquePtr& f,  const UniquePtr<X[], Y> &s) {
        return f.m_ptr != s.m_ptr;
    }
};
// “Size is irrelevant here; we want full ownership or not at all. Size means half ownership.”
template<typename T, size_t N, typename Deleter>
class UniquePtr<T[N], Deleter> {
    static_assert(false, "UniquePtr<T[N], Deleter> does not make sense, we want full ownership, N might mean half ownership.");
};

int main() {
    struct A {
        int x{200};
    };
    UniquePtr<A> ptr{ new A() };
    std::cout << ptr->x << '\n';

    UniquePtr<A[]> ptr2{ new A[10] };
    std::cout << ptr2[0].x << '\n';
}