#include<iostream>
#include<string>

class String {
    static size_t constexpr SSO_SIZE = 16;  
    size_t m_size{};
    size_t m_cap{SSO_SIZE-1};
    char m_data[SSO_SIZE]{};
    bool isSmall() const {
        return m_cap < SSO_SIZE;
    }
    char* getHeapPtr() { 
        return *(reinterpret_cast<char**>(m_data));
    }
    const char* getHeapPtr() const { 
        return *(reinterpret_cast<const char* const *>(m_data));
    }
    char* alloc(size_t size_) {
        return new char[size_];
    }
    void dealloc() noexcept {
        if (!isSmall()) {
            delete []getHeapPtr();
            m_cap = SSO_SIZE-1;
        }
    }
public:
    String() = default;
    String(const char* p) {
        size_t len = std::strlen(p);
        // Use SSO
        if (len < SSO_SIZE) {
            std::memcpy(m_data, p, len+1);
            m_cap = SSO_SIZE - 1;
        }
        // Use Heap
        else {
            char* buffer = new char[len+1];
            std::memcpy(buffer, p, len+1);
            std::memcpy(m_data, &buffer, sizeof(char*));
            m_cap = len;
        }
        m_size = len;
    }
    String(char c) {
        m_data[0] = c;
        m_data[1] = 0;
        m_size = 1;
    }
    String(const String& other) {
        // Copy into static buffer
        if (other.m_size < SSO_SIZE) {
            std::memcpy(m_data, other.getCString(), other.m_size+1);
            m_cap = SSO_SIZE - 1;
        }
        // Copy into Heap
        else {
            char* buffer = new char[other.m_size+1];
            std::memcpy(buffer, other.getCString(), other.m_size+1);
            std::memcpy(m_data, &buffer, sizeof(char*));
            m_cap = other.m_size;
        }
        m_size = other.m_size;
    }
    String(String&& other) noexcept : m_size{other.m_size}, m_cap{other.m_cap} {
        std::memcpy(m_data, other.m_data, SSO_SIZE);
        other.m_size = 0;
        other.m_cap = SSO_SIZE-1;
        other.m_data[0] = '\0';
    }
    String& operator=(const String& other) {
        if (this != &other) {
            if (other.m_size > m_cap) {
                char* buffer = new char[other.m_size+1];
                std::memcpy(buffer, other.getCString(), other.m_size+1);        
                dealloc();
                std::memcpy(m_data, &buffer, sizeof(char*));
                m_cap = other.m_size;
            }   
            else {
                std::memcpy(getCString(), other.getCString(), other.m_size+1);
            }
            m_size = other.m_size;
        }
        return *this;
    }
    String& operator=(String&& other) noexcept {
        String tmp{std::move(other)};
        swap(*this, tmp);
        return *this;
    }
    ~String() noexcept {
        dealloc();
    }
    char* getCString() {
        return (isSmall() ? m_data : getHeapPtr());
    }
    const char* getCString() const {
        return (isSmall() ? m_data : getHeapPtr());
    }
    friend void swap(String& f, String& s) noexcept {
        using std::swap;
        swap(f.m_size, s.m_size);
        swap(f.m_cap, s.m_cap);
        char temp[SSO_SIZE];
        memcpy(temp, f.m_data, SSO_SIZE);
        memcpy(f.m_data, s.m_data, SSO_SIZE);
        memcpy(s.m_data, temp, SSO_SIZE);
    }
    size_t size() const {
        return m_size;
    }
    bool empty() const {
        return m_size == 0;
    }

    String& append(const String& other) {
        size_t newSize = other.m_size + m_size;
        if (newSize > m_cap) {
            char* buffer = new char[newSize+1];
            std::memcpy(buffer, getCString(), m_size);        
            std::memcpy(buffer+m_size, other.getCString(), other.m_size+1);        
            dealloc();
            std::memcpy(m_data, &buffer, sizeof(char*));
            m_cap = newSize;
        }   
        else {
            std::memcpy(getCString()+m_size, other.getCString(), other.m_size+1);
        }
        m_size = newSize;
        return *this;
    }

    String& operator+=(const String& other) {
        return append(other);
    }
};

int main() {
    String s{"Kaleem is a very good boy"};
    String t{"Kaleem on stack"};
    std::cout << s.getCString() << '\n';
    std::cout << t.getCString() << '\n';
    s = t;//copy = stack
    std::cout << s.getCString() << '\n';
    s = std::move(t);//move= stack
    std::cout << s.getCString() << '\n';
    std::cout << t.size() << '\n';
    String u{"i am kaleem ahmad khan, this is a string implementation"};
    s = u;//copy = heap
    std::cout << s.getCString() << '\n';
    s = std::move(u); // move = heap
    std::cout << s.getCString() << '\n';
    std::cout << u.size() << '\n';
    String v = String{"This"}; // move ctor
    std::cout << v.getCString() << '\n';
    String w{v}; // copy ctor
    std::cout << w.getCString() << '\n';
    String x = String{"This s a very big string, this iwll undergo SSO"}; // move ctor
    std::cout << x.getCString() << '\n';
    String y{x}; // copy ctor
    std::cout << y.getCString() << '\n';  

    y.append(x);
    std::cout << y.getCString() << '\n';
    y.append(y);
    std::cout << y.getCString() << '\n';

    String o{""};
    o += "kaleem";
    std::cout << o.getCString() << '\n';
    o += "is a good boy";
    std::cout << o.getCString() << '\n';

}