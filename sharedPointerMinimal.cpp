

/*
Implement shared ptr
pending: weak ptr / thread safety / alloc

check: singlethreadedsharedweakptr.cpp for single threaded shared weak ptr
*/



template<typename T>
class SharedPtr {
public:
  using DataType = T;
private:
  struct ControlBlock {
    size_t refCount{1};
    void increment() {
      refCount++;
    }
    void decrement() {
      refCount--;
    }
    size_t getRefCount() const {
      return refCount;
    }
  };
  DataType* m_ptr{nullptr};
  ControlBlock* m_controlBlockPtr{nullptr};

  // Release Ownership
  void release() noexcept {
    if (m_controlBlockPtr == nullptr) return;
    m_controlBlockPtr->decrement();
    if (m_controlBlockPtr->getRefCount() == 0) {
      delete m_ptr;
      delete m_controlBlockPtr;     
    }
    m_ptr = nullptr;
    m_controlBlockPtr = nullptr;
  }

public:

  // Parametrized Ctors
  SharedPtr() = default;
  explicit SharedPtr(DataType* rawPtr) : m_ptr{rawPtr} {
    if (rawPtr == nullptr) return;
    // Guarantee that resource is released if ctor throw
    try {
      m_controlBlockPtr = new ControlBlock;
    }
    catch(...) {
      delete rawPtr;
      m_ptr = nullptr;
      throw; // rethrow the original exception
    }
  }

  // Rule of 5
  
  // Acquire shared ownership
  SharedPtr(const SharedPtr& other) noexcept : 
  m_ptr{other.m_ptr}, m_controlBlockPtr{other.m_controlBlockPtr}  {
    if (m_controlBlockPtr != nullptr) {
      m_controlBlockPtr->increment();
    }
  }

  // Transfer ownership from other
  SharedPtr(SharedPtr&& other) noexcept :
    m_ptr{other.m_ptr}, m_controlBlockPtr{other.m_controlBlockPtr} {
    other.m_ptr = nullptr;
    other.m_controlBlockPtr = nullptr;
  }
  SharedPtr& operator=(const SharedPtr& other) noexcept {
    if (this == &other) {
      return *this;
    }
    // Release Ownership
    release();
    // Acquire shared ownership of other
    m_ptr = other.m_ptr;
    m_controlBlockPtr = other.m_controlBlockPtr;
    if (m_controlBlockPtr != nullptr) {
      m_controlBlockPtr->increment();
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
    m_ptr = other.m_ptr;
    m_controlBlockPtr = other.m_controlBlockPtr;
    other.m_ptr = nullptr;
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
    ControlBlock* newControlBlockptr = nullptr;
    try {
      newControlBlockptr = new ControlBlock;
    }
    catch(...) {
      delete rawPtr;
      throw;
    }
    release();
    m_ptr = rawPtr;
    m_controlBlockPtr = newControlBlockptr;
  }

  // get used count
  size_t getUsedCount() const {
    return (m_controlBlockPtr ? m_controlBlockPtr->getRefCount() : 0);
  }
  // get
  DataType* get() const {
    return m_ptr;
  }

  // Operators
  DataType& operator*() {
    return *m_ptr;
  }
  const DataType& operator*() const {
    return *m_ptr;
  }
  DataType* operator->() {
    return m_ptr;
  }
  const DataType* operator->() const {
    return m_ptr;
  }
  explicit operator bool() const {
    return m_ptr != nullptr;
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
}
