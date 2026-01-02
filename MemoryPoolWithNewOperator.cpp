/*
  Implement Pool using new operator
*/
template<typename T>
class MemoryPool {
  //Can replace both of these using ds in sharedPointerMinimal.cpp also
  std::vector<T*> m_pool; // Stores available memory for allocation
  std::vector<void*> m_toFree; // Stores allocations to delete on Dtor

  size_t m_reallocSize{0}; // Stores reallocation size if pool runs out of mememory
  size_t m_numAllocated{0}; // Store the number of T for which memory is allocated

public:

  explicit MemoryPool(size_t nAllocations, size_t nReAllocSize = 1<<8) {
    m_reallocSize = std::max<size_t>(nReAllocSize, 1);
    m_pool.reserve(nAllocations);
    if (nAllocations == 0) {
      return;
    }
    void* rawBytes = allocateRaw(nAllocations);
    addToPool(rawBytes, nAllocations);
  }
  
  //Rule of 5 : Disable Copy / Move
  ~MemoryPool() {
    // When Dtor calls user needs to ensure that dealloc is called on each alloc, otherwise there will be a memory leak
    for (void* toFree : m_toFree) {
      ::operator delete[](toFree, std::align_val_t(alignof(T)));
    }
  }
  MemoryPool(const MemoryPool&) = delete;
  MemoryPool& operator=(const MemoryPool&) = delete;
  // Rule of 5 end

  void* allocateRaw(size_t nAllocations) {
    // Get Raw Bytes with Alignment
    return ::operator new[](nAllocations * sizeof(T), std::align_val_t(alignof(T)));
  }

  // Adds memory available to pool
  void addToPool(void* rawBytesAllocatedPtr, size_t nAllocations) {
    T* startAddress = reinterpret_cast<T*>(rawBytesAllocatedPtr);
    for(size_t i = 0; i < nAllocations; i++) {
      m_pool.push_back(startAddress + i);
    }
    m_toFree.push_back(rawBytesAllocatedPtr);
    m_numAllocated += nAllocations;
  }

  // Resizes pool
  void resize(size_t nAllocations) {
    void* rawBytes = allocateRaw(nAllocations);
    addToPool(rawBytes, nAllocations);
  }

  // Returns address to construct object
  // this can leak if user does not handle throw on construction
  T* alloc() {
    // Resize
    if (m_pool.empty()) {
      resize(m_reallocSize);
    }
    // Get Allocation
    T* allocatePtr = m_pool.back();
    // Remove from pool
    m_pool.pop_back(); 
    return allocatePtr;
  }

  // Dealocates object and frees up memory in pool : Call this in Dtor of T
  void dealloc(T* ptr) {
    ptr->~T();
    m_pool.push_back(ptr);
  }

  // Function to construct T in place
  /*
    Note that Ctor can throw , to prevent that add: 
      static_assert(std::is_nothrow_constructible_v<T, ArgsT...>, "MemoryPool::make requires nothrow construction");
  */
  template<typename... ArgsT>
  T* make(ArgsT&&... args) {
    // if construction throws we have a leak
    static_assert(std::is_nothrow_constructible_v<T, ArgsT...>, "MemoryPool::make requires nothrow construction");
    return new (alloc()) T(std::forward<ArgsT>(args)...);
  }

  // Getters
  size_t allocated() const {
    return m_numAllocated;
  }

  size_t available() const {
    return m_pool.size();
  }

  // Setters
  void adjustReallocSize(size_t size) {
    m_reallocSize = (size == 0 ? 1 : size);
  }
};

int main() {
  MemoryPool<int> pool{2};
  int* p1 = pool.make(125);
  int* p2 = new (pool.alloc()) int(54);
  assert(*p1 == 125);
  assert(*p2 == 54);
  pool.dealloc(p1);
  int* p3 = pool.make(456);
  assert(*p3 == 456);
  pool.dealloc(p2);
  pool.dealloc(p3);

  struct A {
    A() noexcept { std::cout << "Constucting\n"; }
    ~A() { std::cout << "Destructing\n"; }
  };
  MemoryPool<A> pool1{1};
  A* p4 = pool1.make();
  pool1.dealloc(p4);

}

