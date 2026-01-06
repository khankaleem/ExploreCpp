#include<type_traits>
#include<vector>
#include<set>
#include<cassert>

//overload add for insert / pushback
template<typename T, typename V>
auto add(T& container, const V& val) ->
    decltype(container.push_back(val), void()) {
    container.push_back(val);
}
template<typename T, typename V>
auto add(T& container, const V& val) ->
    decltype(container.insert(val), void()) {
    container.insert(val);
}


// has_foo function
template<typename C, typename Signature, typename = void>
struct has_foo : std::false_type {};

template<typename C, typename R, typename... ArgsT>
struct has_foo<C, R(ArgsT...), 
    std::void_t<
        std::enable_if_t<
           std::is_same_v<decltype(std::declval<C>().foo(std::declval<ArgsT>()...)), R>
        >
    >
> : std::true_type {};

int main() {
    std::vector<int> v;
    add(v, 100);
    assert(v[0] == 100);

    std::set<int> s;
    add(s, 20);
    assert(*s.begin() == 20);

    struct A {
        double foo(int) {return 0.0;}
    };
    static_assert(has_foo<A, double(int)>::value);

    struct B {
        double foo() {return 0.0;}
    };
    static_assert(!has_foo<B, double(int)>::value);

}
