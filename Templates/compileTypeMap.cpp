#include<type_traits>
#include<cassert>

template<int>
struct Map;

template<>
struct Map<1024> {
    using type = double;
    static constexpr type value = 1.25;
};

template<>
struct Map<2048> {
    using type = char;
    static constexpr type value = 'a';
};

template<>
struct Map<0> {
    using type = nullptr_t;
    static constexpr type value = nullptr;
};

template<>
struct Map<128> {
    using type = const char*;
    static constexpr type value = "Kaleem";
};

template<>
struct Map<256> {
    using type = int;
    static constexpr type value = 200056;
};


int main() {
    static_assert(Map<256>::value == 200056);
}

