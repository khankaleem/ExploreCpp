#include <iostream>
#include <type_traits>
#include <utility>
#include<cassert>

/*
  Compile time check for existence of a function in a class
*/

// Primary template: All templates for has_foo must be specializaitions
template<typename ClassT, typename SignatureT>
struct has_foo;

template<typename ClassT, typename ReturnT, typename... ArgsT>
struct has_foo<ClassT, ReturnT(ArgsT...)> {
    /*
        Create two overloads, one more specialized, 
        if specialized overload results in an invalid expression
        compiler instanitates the fallback one

        To make SFINAE valid expressions must be dependent on the template parameter, 
        other compiler will hard instantiate
        std::declval<ClassT>().foo(std::declval<ArgsT>()...)), wont work
    */
    template<typename T>
    static constexpr auto check(int) ->  
        typename std::is_same<decltype(std::declval<T>().foo(std::declval<ArgsT>()...)), ReturnT>::type;

    // Generic check
    template<typename T>
    static constexpr std::false_type check(...);

public:
    using type = decltype(check<ClassT>(0));
    static constexpr bool value = type::value;
};

struct A {
    constexpr int foo(double, char){ return 2000; }
};

struct B {
    constexpr int foo() {return 100;}
};

int main() {
    int val = 0;
    if constexpr(has_foo<A, int(double, char)>::value) {
        A a{};
        val = a.foo(1.0, 'a'); 
        static_assert( a.foo(1.0, 'a') == 2000 );
    }
    
    if constexpr(!has_foo<B, int(double, char)>::value) {
        B b{};
        static_assert( b.foo() == 100 );
    } 


}
