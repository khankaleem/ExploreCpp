
#include <iostream>
#include <memory>
#include <typeinfo>
#include <utility>

class BadAnyCast : public std::exception {
public:
    const char* what() const noexcept override {
        return "MiniAny: bad_any_cast";
    }
};

class Any {
  class StorageBase {
    public:
      virtual ~StorageBase() = default;
      virtual std::unique_ptr<StorageBase> clone() const = 0;
      virtual const std::type_info& type() const = 0;
  };

  template<typename ElemT>
  class StorageImpl : public StorageBase {
    public:
      ElemT m_val;
      StorageImpl(const ElemT& val_) : m_val{val_}{}
      StorageImpl(ElemT&& val_) : m_val{std::move(val_)}{}
      std::unique_ptr<StorageBase> clone() const override {
        return std::make_unique<StorageImpl<ElemT>>(m_val);
      }
      const std::type_info& type() const override {
        return typeid(ElemT);
      }
  };
  std::unique_ptr<StorageBase> m_ptr{};

  public:
    Any() = default;
  
    template<typename T>
    Any(const T& val) {
      using U = std::decay_t<T>; // decay to remove references
      m_ptr = std::make_unique<StorageImpl<U>>(val);
    }
    template<typename T>
    Any(T&& val) {
      using U = std::decay_t<T>; // decay to remove references
      m_ptr = std::make_unique<StorageImpl<U>>(std::forward<T>(val));
    }
  
    // Rule of 5
    ~Any() = default;
    Any(Any&& other) = default;
    Any& operator=(Any&& other) = default;
    Any(const Any& other) {
      if (other.m_ptr) {
        m_ptr = other.m_ptr->clone();
      }
    }
    Any& operator=(const Any& other) {
      if (this != &other) {
        if (other.m_ptr) {
          m_ptr = (other.m_ptr ? other.m_ptr->clone() : nullptr);
        } else {
            m_ptr.reset();
        }
      }
      return *this;
    }
    // Rule of 5 end

    template<typename T>
    T* castPtr() {
      if (m_ptr && m_ptr->type() == typeid(T)) {
        return &static_cast<StorageImpl<T>*>(m_ptr.get())->m_val;
      }
      return nullptr;
    }
    template<typename T>
    const T* castPtr() const {
      if (m_ptr && m_ptr->type() == typeid(T)) {
        return &((static_cast<StorageImpl<T>*>(m_ptr.get()))->m_val);
      }
      return nullptr;
    }
};


// Any&& : returns by value (cannot return references safely)
template<typename T>
T anyCast(Any&& obj) {
  static_assert(!std::is_reference_v<T>,
                "anyCast(Any&&) cannot return a reference");
  using U = std::remove_reference_t<T>;
  U* ret = obj.castPtr<U>();
  if (ret == nullptr) {
    throw BadAnyCast{};
  }
  return static_cast<T>(std::move(*ret));
}

template<typename T>
T anyCast(Any& obj) {
  using U = std::remove_reference_t<T>;
  U* ret = obj.castPtr<U>();
  if (ret == nullptr) {
    throw BadAnyCast{};
  }
  return static_cast<T>(*ret);
}

template<typename T>
T anyCast(const Any& obj) {
  using U = std::remove_reference_t<T>;
  const U* ret = obj.castPtr<U>();
  if (!ret) throw BadAnyCast{};
  return static_cast<T>(*ret); 
}

template<typename T>
T* anyCast(Any* obj) noexcept {
  return (obj ? obj->castPtr<T>() : nullptr);
}

int main() {
  Any any_{1234};
  std::cout << anyCast<int>(any_) << '\n';
  any_ = 2.5;
  std::cout << anyCast<double>(any_) << '\n';
}