#include <tuple>
#include <string>
#include <typeinfo>
#include <iostream>
#include <boost/static_assert.hpp>

template<typename Sequence, typename T> struct push_front;
template<template<typename...> class Sequence, typename T, typename ... Args>
struct push_front<Sequence<Args...>, T> {
    typedef Sequence<T, Args...> type;
};

template<typename Sequence> struct pop_front;
template<template<typename...> class Sequence, typename T, typename ... Args>
struct pop_front<Sequence<T, Args...>> {
    typedef Sequence<Args...> type;
};

template <typename Tp, typename... T> struct TVar_each;
template <typename Tp> struct TVar_each<Tp> { static void sf() {} };
template <typename Tp, typename A, typename... T>
struct TVar_each<Tp,A,T...> {
    BOOST_STATIC_ASSERT(std::tuple_size<Tp>::value==1+sizeof...(T));
    static void sf() {
        std::cout <<" "<< typeid(A).name();
        TVar_each<typename pop_front<Tp>::type,T...>::sf();
    }
};

template <typename Tp, typename... T> struct TVar_match;
template <typename Tp> struct TVar_match<Tp> : std::true_type {};
template <typename Tp, typename A, typename... T>
struct TVar_match<Tp,A,T...>
    : std::conditional<
        std::is_same<A,typename std::tuple_element<0,Tp>::type>::value
            , TVar_match<typename pop_front<Tp>::type,T...>
            , typename std::conditional<
                std::is_arithmetic<A>::value && std::is_convertible<A,typename std::tuple_element<0,Tp>::type>::value
                    , TVar_match<typename pop_front<Tp>::type,T...>
                    , std::false_type
                    >::type
            >::type {
    BOOST_STATIC_ASSERT(std::tuple_size<Tp>::value==1+sizeof...(T));
};

template <typename Tp, size_t N, size_t I=0> 
struct Tuple_each {
    static void sf(Tp tp) {
        std::cout <<" "<< typeid(typename std::tuple_element<I,Tp>::type).name();
        Tuple_each<Tp,N,I+1>::sf(tp); 
    }
};  
template <typename Tp, size_t N> struct Tuple_each<Tp,N,N> { static void sf(Tp) {} };

template<class... Types>
struct size_of {
    static const std::size_t value = sizeof...(Types);
};
// struct SS : std::string, std::string {};

template <typename...> struct Convertible ;
template <typename F, typename T>
struct Convertible<F,T> : std::conditional<
    std::is_arithmetic<T>::value
        , typename std::is_convertible<F,T>::type
        , std::false_type
    >::type
{};
template <typename F, typename T>
struct Convertible<std::pair<F,T>> : Convertible<F,T>
{};

template <typename...T> struct All ; // : std::false_type {};
template <typename...T>
struct All<std::tuple<T...>> : All<T...>
{};
template <> struct All<> : std::true_type
{};
template <typename A, typename...T> 
struct All<A,T...> : std::conditional<
    Convertible<A>::value //std::is_integral<A>::value
        , typename All<T...>::type
        , std::false_type
    >::type
{};
 
template<class... L>
struct Zip {
    template<class... R>
    struct With {
        BOOST_STATIC_ASSERT(sizeof...(L)==sizeof...(R));
        typedef std::tuple<std::pair<L,R>...> type;
    };
    template<class... R>
    struct With<std::tuple<R...>> : With<R...>
    {};
};
template<class... L> struct Zip<std::tuple<L...>> : Zip<L...> {};

int main()
{
    typedef std::tuple<bool,int,std::string> Tp0;

    typedef Zip<bool,int,std::string>::With<char,std::string,std::string>::type Zip0;
    typedef Zip<long,int,std::string>::With<Tp0>::type Zip1;
    typedef Zip<Tp0>::With<Tp0>::type Zip2;
    std::cout <<"All:"
        <<"\n\t"<< All<Zip0>::value
        <<"\n\t"<< All<Zip1>::value
        <<"\n\t"<< All<Zip2>::value
        <<"\n";

    std::cout << std::tuple_size<Tp0>::value
        << " " << typeid(Tp0).name()
        << "\n";
    typedef pop_front<Tp0>::type Tp1;
    std::cout << std::tuple_size<Tp1>::value
        << " " << typeid(Tp1).name()
        << "\n";

    typedef pop_front<Tp0>::type Tp1;
    typedef pop_front<Tp1>::type Tp2;
    typedef pop_front<Tp2>::type Tp3;
    //typedef pop_front<Tp3>::type Tp4;

    Tp0 t;
    std::cout << std::tuple_size<Tp1>::value << " tuple_size\n";
    std::cout << size_of<bool,int>::value << " size_of\n";
    std::cout << size_of<>::value << " size_of\n";

    std::cout<<"TVar_each:\n";
    TVar_each<Tp0, bool,int,std::string>::sf(); std::cout<<"\n";
    TVar_each<Tp1, int,std::string>::sf(); std::cout<<"\n";;
    TVar_each<Tp2, std::string>::sf(); std::cout<<"\n";;
    TVar_each<Tp3>::sf(); std::cout<<"\n";;

    std::cout<<"Tuple_each:\n";
    Tuple_each<Tp0,std::tuple_size<Tp0>::value>::sf(Tp0()); std::cout<<"\n";
    Tuple_each<Tp1,std::tuple_size<Tp1>::value>::sf(Tp1()); std::cout<<"\n";;
    Tuple_each<Tp2,std::tuple_size<Tp2>::value>::sf(Tp2()); std::cout<<"\n";;
    Tuple_each<Tp3,std::tuple_size<Tp3>::value>::sf(Tp3()); std::cout<<"\n";;

    std::cout<<"TVar_match:\n";
    std::cout << TVar_match<Tp0, bool,int,std::string>::value <<"\n";
    std::cout << TVar_match<Tp0, bool,bool,std::string>::value <<"\n";
    std::cout << TVar_match<Tp0, bool,unsigned int,std::string>::value <<"\n";
    std::cout << TVar_match<Tp0, bool,char*,std::string>::value <<"\n";

    //std::cout << sizeof(t) << "\n";
}

