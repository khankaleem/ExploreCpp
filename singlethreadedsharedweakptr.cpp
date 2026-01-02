/*
Implement shared ptr
*/
template<typename T>
struct ControlBlock {
private:
  size_t m_strongCount{0};
  size_t m_weakCount{0};
  T* m_ptr{nullptr};
public:
  ControlBlock() = default;
  explicit ControlBlock(T* rawPtr) : m_ptr{rawPtr} {}
  void acquireStrong() {
    m_strongCount++;
  }
  void releaseStrong() {
    m_strongCount--;
    if (m_strongCount == 0) {
      delete m_ptr;
      m_ptr = nullptr;
    }
  }
  void acquireWeak() {
    m_weakCount++;
  }
  void releaseWeak() {
    m_weakCount--;
  }
  size_t getUsedCount() const {
    return m_strongCount;
  }
  size_t getWeakCount() const {
    return m_weakCount;
  }
  bool dead() const {
    return (m_strongCount == 0 && m_weakCount == 0);
  }
  T* get() const {
    return m_ptr;
  }
  // Rule of 5: 
  ~ControlBlock() {
    if (m_ptr != nullptr) {
      delete m_ptr;
    }
  }
  ControlBlock(const ControlBlock&) = delete;
  ControlBlock& operator=(const ControlBlock&) = delete;
  // Rule of 5 end
};

template<typename T>
class WeakPtr;

template<typename T>
class SharedPtr {
public:
  using DataType = T;
private:
  using ControlBlockT = ControlBlock<T>;
  ControlBlockT* m_controlBlockPtr{nullptr};

  // Ctor / getter for weak pointer: mark weakPtr as friend to access this
  template<typename U>
  friend class WeakPtr;

  explicit SharedPtr(ControlBlockT* controlBlockPtr) {
    assert(controlBlockPtr != nullptr);
    m_controlBlockPtr = controlBlockPtr;
    m_controlBlockPtr->acquireStrong();
  }
  ControlBlockT* getControlBlockPtr() const {
    return m_controlBlockPtr;
  }

  // Release Ownership
  void release() noexcept {
    if (m_controlBlockPtr == nullptr) return;
    m_controlBlockPtr->releaseStrong();
    if (m_controlBlockPtr->dead()) {
      delete m_controlBlockPtr;     
    }
    m_controlBlockPtr = nullptr;
  }


public:

  // Parametrized Ctors
  SharedPtr() = default;
  explicit SharedPtr(DataType* rawPtr) {
    if (rawPtr == nullptr) return;
    // Guarantee that resource is released if ctor throw
    try {
      m_controlBlockPtr = new ControlBlock(rawPtr);
      m_controlBlockPtr->acquireStrong();
    }
    catch(...) {
      delete rawPtr;
      throw; // Rethrow the original exception
    }
  }


  // Rule of 5
  
  // Acquire shared ownership
  SharedPtr(const SharedPtr& other) noexcept : m_controlBlockPtr{other.m_controlBlockPtr}  {
    if (m_controlBlockPtr != nullptr) {
      m_controlBlockPtr->acquireStrong();
    }
  }

  // Transfer ownership from other
  SharedPtr(SharedPtr&& other) noexcept : m_controlBlockPtr{other.m_controlBlockPtr} {
    other.m_controlBlockPtr = nullptr;
  }
  SharedPtr& operator=(const SharedPtr& other) noexcept {
    if (this == &other) {
      return *this;
    }
    // Release Ownership
    release();
    // Acquire shared ownership of other
    m_controlBlockPtr = other.m_controlBlockPtr;
    if (m_controlBlockPtr != nullptr) {
      m_controlBlockPtr->acquireStrong();
    }
    return *this;
  }
  SharedPtr& operator=(SharedPtr&& other) noexcept {
    if (this == &other) {
      return *this;
    }
    // Release Ownership
    release();
    // Transfer ownership from other
    m_controlBlockPtr = other.m_controlBlockPtr;
    other.m_controlBlockPtr = nullptr;
    return *this;
  }
  ~SharedPtr() {
    release();
  }
  // Rule of 5 end
  void reset(DataType* rawPtr = nullptr) {
    if (rawPtr == nullptr) {
      release();
      return;
    }
    // Allocate memory first, then release ownership as new can throw
    ControlBlockT* newControlBlockptr = nullptr;
    try {
      newControlBlockptr = new ControlBlockT(rawPtr);
    }
    catch(...) {
      delete rawPtr;
      throw;
    }
    release();
    m_controlBlockPtr = newControlBlockptr;
    m_controlBlockPtr->acquireStrong();
  }

  // get
  DataType* get() const {
    return (m_controlBlockPtr ? m_controlBlockPtr->get() : nullptr);
  }
  // get used count
  size_t getUsedCount() const {
    return (m_controlBlockPtr ? m_controlBlockPtr->getUsedCount() : 0);
  }
  size_t getWeakCount() const {
    return (m_controlBlockPtr ? m_controlBlockPtr->getWeakCount() : 0);
  }

  // Operators
  DataType& operator*() {
    return *get();
  }
  const DataType& operator*() const {
    return *get();
  }
  DataType* operator->() {
    return get();
  }
  const DataType* operator->() const {
    return get();
  }
  explicit operator bool() const {
    return get() != nullptr;
  }
};


template<typename T>
class WeakPtr {
private:
  using ControlBlockT = ControlBlock<T>;
  ControlBlockT* m_controlBlockPtr{nullptr};
public:
  using DataType = T;

  WeakPtr() = default;
  explicit WeakPtr(const SharedPtr<T>& sharedPtr) : 
      m_controlBlockPtr{sharedPtr.getControlBlockPtr()} {
    if (m_controlBlockPtr != nullptr) {
      m_controlBlockPtr->acquireWeak();
    }
  }

  // Rule of 5 start
  ~WeakPtr() {
    reset();
  }
  // Copy
  WeakPtr(const WeakPtr& other) : m_controlBlockPtr{other.m_controlBlockPtr} {
    if (m_controlBlockPtr != nullptr) {
      m_controlBlockPtr->acquireWeak();
    }
  }
  // Move
  WeakPtr(WeakPtr&& other) : m_controlBlockPtr{other.m_controlBlockPtr} {
    other.m_controlBlockPtr = nullptr;
  }
  // Copy Assign 
  WeakPtr& operator=(const WeakPtr& other) {
    if (this == &other) {
      return *this;
    }
    // Release
    reset();
    // Copy
    m_controlBlockPtr = other.m_controlBlockPtr;
    if (m_controlBlockPtr != nullptr) {
      m_controlBlockPtr->acquireWeak();
    }
    return *this;
  }
  WeakPtr& operator=(WeakPtr&& other) {
    if (this == &other) {
      return *this;
    }
    // Release
    reset();
    // Move
    m_controlBlockPtr = other.m_controlBlockPtr;
    // Invalidate other
    other.m_controlBlockPtr = nullptr;
    return *this;
  }
  // Rule of 5 end

  SharedPtr<T> lock() const {
    if (m_controlBlockPtr == nullptr || expired()) {
      return SharedPtr<T>{};
    }
    SharedPtr<T> ret{m_controlBlockPtr};
    return ret;
  }

  bool expired() const {
    return (getUsedCount() == 0);
  }

  size_t getUsedCount() const {
    return (m_controlBlockPtr ? m_controlBlockPtr->getUsedCount() : 0);
  }
  size_t getWeakCount() const {
    return (m_controlBlockPtr ? m_controlBlockPtr->getWeakCount() : 0);
  }

  DataType* get() const {
    return (m_controlBlockPtr ? m_controlBlockPtr->get() : nullptr);
  }

  void reset() {
    if(m_controlBlockPtr == nullptr) {
      return;
    }
    m_controlBlockPtr->releaseWeak();
    if (m_controlBlockPtr->dead()) {
      delete m_controlBlockPtr;
    }
    m_controlBlockPtr = nullptr;
  }
};

int main() {
  struct A {
    A() {
      std::cout << "Creating\n";
    }
    ~A() {
      std::cout << "Destroying\n";
    }
  };
  SharedPtr<A> s{new A()};
  assert(s.getUsedCount() == 1);
  SharedPtr<A> s2{s};
  assert(s2.getUsedCount() == 2);
  {
    SharedPtr<A> s3{s};
    assert(s3.getUsedCount() == 3);
  }
  SharedPtr<A> s4{std::move(s)};
  assert(s4.getUsedCount() == 2);
  s2.reset();
  assert(s4.getUsedCount() == 1);

  SharedPtr<A> copy(new A());
  assert(copy.getUsedCount() == 1);

  copy = s4;
  assert(s4.getUsedCount() == 2);

  SharedPtr<A> move(new A());
  assert(move.getUsedCount() == 1);
  move = std::move(s4);
  assert(move.getUsedCount() == 2);

  {
    WeakPtr<A> weak{move};
    assert(move.getUsedCount() == 2);
    assert(weak.getWeakCount() == 1);
    assert(weak.expired() == false);
  }

  WeakPtr<A> weak{move};
  WeakPtr<A> weak2{weak};
  assert(weak2.getWeakCount() == 2);
  WeakPtr<A> weak3{std::move(weak)};
  assert(weak3.getWeakCount() == 2);

  auto sp = weak3.lock();
  assert(weak2.getWeakCount() == 2);
  assert(move.getUsedCount() == 3);
  assert(sp.getUsedCount() == 3);
  weak3.reset();
  assert(sp.getWeakCount() == 1);

}
