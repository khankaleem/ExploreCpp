template<typename T>
class FastResizableStack {
  static_assert(std::is_trivially_copyable_v<T> && std::is_trivially_destructible_v<T>, 
                "FastResizableStack only works for trivially copyable / destructible types");
private:
  T* m_arr{nullptr};
  size_t m_cap{0};
  // top is insertion point - 1 so init to max value which wraps arounds to 0 on increment
  size_t m_top{std::numeric_limits<size_t>::max()};
public:
  explicit FastResizableStack(size_t cap) : m_cap{cap ? cap : 1} {
    m_arr = reinterpret_cast<T*>(::operator new[](m_cap * sizeof(T), std::align_val_t(alignof(T))));
  }
  void push(T val) {
    if (m_top + 1 != m_cap) {
      m_arr[++m_top] = val; 
      return;
    }  
    T* newArr = reinterpret_cast<T*>(::operator new[]((m_cap<<1) * sizeof(T), std::align_val_t(alignof(T))));
    memcpy(newArr, m_arr, sizeof(T) * m_cap);
    m_cap = (m_cap << 1);
    ::operator delete[](m_arr, std::align_val_t(alignof(T)));
    m_arr = newArr;
    m_arr[++m_top] = val; 
  }
  // Pop will work regardless, correctness to be enured by user
  void pop() {
    m_top--;
  }
  // Top will return regardless, correctness to be enured by user
  T top() const {
    return m_arr[m_top];
  }
  bool empty() const {
    return m_top == std::numeric_limits<size_t>::max();
  }

  // Rule of 5
  ~FastResizableStack() {
    ::operator delete[](m_arr, std::align_val_t(alignof(T)));
  }
  FastResizableStack(const FastResizableStack&) = delete;
  FastResizableStack& operator=(const FastResizableStack&) = delete;
  // Rule of 5
};
