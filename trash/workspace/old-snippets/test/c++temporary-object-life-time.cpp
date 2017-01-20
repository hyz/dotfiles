#include <iostream>

struct T
{
    ~T() { std::cout << this << "~T\n"; }
    int a;
};

T const & foo(T const & t) { std::cout << "foo\n"; return t; }
char const * bar(char const * t) { std::cout << "bar\n"; return t; }

int main()
{
    T const & t = foo(T());

    std::cout << "return\n";
    std::cout << &t << "\n";

    return 0;
}

