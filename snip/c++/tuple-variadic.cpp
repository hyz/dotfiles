// #include <cstddef>
// #include <tuple>
// #include <type_traits>
// #include <utility>
// 
// template<size_t N>
// struct Apply {
//     template<typename F, typename T, typename... A>
//     static inline auto apply(F && f, T && t, A &&... a) {
//         return Apply<N-1>::apply(std::forward<F>(f), std::forward<T>(t), std::get<N-1>(std::forward<T>(t)), std::forward<A>(a)...);
//     }
// };
// 
// template<>
// struct Apply<0> {
//     template<typename F, typename T, typename... A>
//     static inline auto apply(F && f, T &&, A &&... a) {
//         return std::forward<F>(f)(std::forward<A>(a)...);
//     }
// };
// 
// template<typename F, typename T>
// inline auto apply(F && f, T && t) {
//     return Apply< std::tuple_size< std::decay_t<T>
//       >::value>::apply(std::forward<F>(f), std::forward<T>(t));
// }


#include <cstddef>
#include <tuple>
#include <type_traits>
#include <utility>

template<size_t N>
struct applyF_ {
    template<typename F, typename T, typename... A>
    static inline auto Fw(F && f, T && t, A &&... a) -> decltype(
        applyF_<N-1>::Fw(std::forward<F>(f), std::forward<T>(t), std::get<N-1>(std::forward<T>(t)), std::forward<A>(a)...))
    {
return  applyF_<N-1>::Fw(std::forward<F>(f), std::forward<T>(t), std::get<N-1>(std::forward<T>(t)), std::forward<A>(a)...);
    }
};

template<>
struct applyF_<0> {
    template<typename F, typename T, typename... A>
    static inline auto Fw(F && f, T &&, A &&... a) -> decltype(
                std::forward<F>(f)(std::forward<A>(a)...))
    {
        //return  std::forward<F>(f)(std::forward<A>(a)...);
        return  (f)(std::forward<A>(a)...);
    }
};

template<typename F, typename T>
inline auto apply(F && f, T && t) -> decltype(
        applyF_<std::tuple_size<typename std::decay<T>::type>::value>::Fw(std::forward<F>(f), std::forward<T>(t)))
{
return  applyF_<std::tuple_size<typename std::decay<T>::type>::value>::Fw(std::forward<F>(f), std::forward<T>(t));
}

#include <iostream>
#include <boost/core/ref.hpp>

struct Foo
{
    void operator()(int x, const char* y) //const
    {
        std::cout << x <<" "<< y;
    }
    void operator()(int x, std::tuple<int,char const*> & t) //const
    {
        std::cout << x <<" ";
        apply(*this, t);
    }
};

int main()
{
    auto t2 = std::make_tuple(11, "foo");
    auto t22 = std::make_tuple(11, std::make_tuple(22, "bar"));
    Foo foo;
    apply(foo, t22); puts("");
    apply(Foo(), t22); puts("");
    //apply(Foo(), t2); puts("");
    // apply(Foo(), t22); puts("");
}

