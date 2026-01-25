
#include <iostream>
#include <memory>
#include <typeinfo>
#include <utility>
#include <type_traits>

class BadAnyCast : public std::exception {
public:
    const char* what() const noexcept override {
        return "MiniAny: bad_any_cast";
    }
};


class Any {

    struct VTable {
       void  (*m_deleter)(void*);
       void* (*m_clone)(const void*);
       const std::type_info& (*m_type)();
    };

    // static vtable for type T
    template<typename T>
    static constexpr VTable vtable_ = {
        [](void* ptr) -> void {
            delete static_cast<T*>(ptr);
        },           //destroyer cloning function
        [](const void* ptr) -> void* {
            return new T(* static_cast<const T*>(ptr));
        },
                         // cloning function
        []() -> const std::type_info& {
            return typeid(T);
        } //return type
    };


    void* m_ptr{};
    const VTable* m_vptr{};

public:
    Any() = default;

    template<typename T, 
             typename U =  std::decay_t<T>,
             typename = std::enable_if_t<!std::is_same_v<U, Any>>
            >
    Any(T&& val) {
      m_ptr = new U{std::forward<T>(val)};
      m_vptr = &vtable_<U>;
    }

    // Rule of 5
    ~Any() {
        reset();
    }
    Any(const Any& other) {
      if (other.m_ptr) {
        m_ptr = other.m_vptr->m_clone(other.m_ptr);
        m_vptr = other.m_vptr;
      }
    }
    Any& operator=(const Any& other) {
      if (this != &other) {
        // copy
        if (other.m_ptr) {
          void* newPtr = other.m_vptr->m_clone(other.m_ptr);
          reset();
          m_ptr = newPtr;
          m_vptr = other.m_vptr;
        }
        else {
            reset();
        }
      }
      return *this;
    }
    Any(Any&& other) noexcept : m_ptr{other.m_ptr}, m_vptr{other.m_vptr} {
        other.m_ptr = nullptr;
        other.m_vptr = nullptr;
    }  
    Any& operator=(Any&& other) noexcept {
      if (this != &other) {
        //release
        reset();
        // copy
        m_ptr = other.m_ptr;
        other.m_ptr = nullptr;
        m_vptr = other.m_vptr;
        other.m_vptr = nullptr;
      }
      return *this;
    }
    // Rule of 5 end

    void reset() {
        if (m_ptr && m_vptr) {
            m_vptr->m_deleter(m_ptr);
        }
        m_ptr = nullptr;
        m_vptr = nullptr;
    }

    template<typename T>
    T* castPtr() {
      if (m_ptr && m_vptr && m_vptr->m_type() == typeid(T)) {
        return static_cast<T*>(m_ptr);
      }
      return nullptr;
    }
    template<typename T>
    const T* castPtr() const {
      if (m_ptr && m_vptr && m_vptr->m_type() == typeid(T)) {
        return static_cast<const T*>(m_ptr);
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