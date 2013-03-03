#include <stdexcept>
#include <cstring>
#include <string>
#include <vector>
#include <algorithm>
#include <iostream>

struct jsexinput
{
    const char* beg;
    const char* end;
    mutable const char* cur;

    jsexinput(const char* b, const char* e) { beg = b; end = e; cur = beg; }
};

struct jsexnode
{
    virtual void jsex_kv(const std::string& k, jsexinput& jin) {}
    virtual ~jsexnode() {}
};

jsexinput& operator>>(jsexinput& jin, jsexnode& a);

struct jsexnull {};

struct jsexdictnull : jsexnode
{
    virtual void jsex_kv(const std::string& k, jsexinput& jin) {
        jin >> *this;
    }
};

struct jsexc : jsexinput, std::invalid_argument
{
    jsexc(const jsexinput& jin)
        : jsexinput(jin), std::invalid_argument(std::string(jin.cur,jin.end))
    {}
};

#define JSEX(ji) do{ throw jsexc(ji); }while(0)

jsexinput& jsex_skips(jsexinput& jin)
{
    for (; jin.cur != jin.end; ++jin.cur)
        if (!std::isspace(*jin.cur))
            break;
    return jin;
}

inline int jsex_peekc(jsexinput& jin)
{
    jsex_skips(jin);
    return *jin.cur;
}

inline int jsex_getc(jsexinput& jin)
{
    jsex_skips(jin);
    if (jin.cur >= jin.end)
        JSEX(jin);
    return *jin.cur++;
}

char* jsex_getc_n(jsexinput& jin, char a[], int n)
{
    // jsex_skips(jin);
    if (jin.cur + n > jin.end)
        JSEX(jin);
    std::memcpy(a, jin.cur, n);
    jin.cur += n;
    return a;
}

jsexinput& operator>>(jsexinput& jin, std::string& s)
{
    int q;
    switch ( (q = jsex_getc(jin))) {
        case '"': case '\'': break;
        default: JSEX(jin);
    }

    const char *e = jin.cur;
    while (e < jin.end && *e != q)
        ++e;
    if (e == jin.end)
        JSEX(jin);
    s.assign(jin.cur, e);
    jin.cur = e+1;

    return jin;
}

jsexinput& operator>>(jsexinput& jin, int& i)
{
    switch (jsex_peekc(jin))
    {
        case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9': case '-': case '+':
            {
                const char* e = jin.cur;
                while (e < jin.end && std::isdigit(*e))
                    ++e;
                i = atoi(jin.cur);
                jin.cur = e;
            }
            break;
        default:
            JSEX(jin);
    }
    return jin;
}

jsexinput& operator>>(jsexinput& jin, bool& b)
{
    switch (jsex_getc(jin))
    {
        case 't': case 'T':
            {
                char a[3];
                jsex_getc_n(jin, a, 3);
                if (memcmp("rue", a, 3))
                    JSEX(jin);
            }
            break;
        case 'f': case 'F':
            {
                char a[4];
                jsex_getc_n(jin, a, 4);
                if (memcmp("alse", a, 4))
                    JSEX(jin);
            }
            break;
        default:
            JSEX(jin);
    }

    return jin;
}

template <typename T>
jsexinput& operator>>(jsexinput& jin, std::vector<T>& v)
{
    if (jsex_getc(jin) == '[')
    {
        if (jsex_peekc(jin) == ']')
        {
            ++jin.cur;
            return jin;
        }
        int c;
        do {
            T x;
            jin >> x;
            v.push_back(x);
        } while ( (c = jsex_getc(jin)) == ',');
        if (c != ']')
            JSEX(jin);
    }

    return jin;
}

jsexinput& operator>>(jsexinput& jin, const jsexnull& nul);

jsexinput& operator>>(jsexinput& jin, jsexnode& a)
{
    if (jsex_getc(jin) != '{')
        JSEX(jin);
    if (jsex_peekc(jin) == '}')
        { ++jin.cur; return jin; }

    int c;
    do {
        std::string k;
        jin >> k;
        if (jsex_getc(jin) != ':')
            JSEX(jin);

        const char *cur = jin.cur;
        a.jsex_kv(k, jin);

        if (cur == jin.cur)
            jin >> jsexnull();

    } while ( (c=jsex_getc(jin)) == ',');

    if (c != '}')
        JSEX(jin);

    return jin;
}

jsexinput& operator>>(jsexinput& jin, const jsexnull& nul)
{
    switch (jsex_peekc(jin))
    {
        case 't': case 'T': case 'f': case 'F':
            { bool b; jin >> b; }
            break;
        case '"':
            { std::string x; jin >> x; }
            break;
        case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9': case '-': case '+':
            { int x; jin >> x; }
            break;
        case '[':
            { std::vector<jsexnull> x; jin >> x; }
            break;
        case '{':
            { jsexdictnull x; jin >> x; }
            break;
        default:
            JSEX(jin);
    }

    return jin;
}

// { 'int': 1, "str": "s", "vec" :[1,2,3], "b" : { "int" : 9, 'str': '""'} }
struct A : jsexnode
{
    int aint;
    std::string astr;
    std::vector<int> vec;

    struct B : jsexnode
    {
        int bint;
        std::string bstr;

        virtual void jsex_kv(const std::string& k, jsexinput& jin)
        {
            if (k == "int")
                jin >> bint;
            else if (k == "str")
                jin >> bstr;
        }
    } b;

    virtual void jsex_kv(const std::string& k, jsexinput& jin)
    {
        if (k == "int")
            jin >> aint;
        else if (k == "str")
            jin >> astr;
        else if (k == "b")
            jin >> b;
        else if (k == "vec")
            jin >> vec;
    }
};

int main(int ac, char *const av[])
{
    std::string line;
    while (std::getline(std::cin, line))
    {
        jsexinput jin(&line[0], &line[0] + line.size());

        A a;
        jin >> a;

        std::cout.write(jin.beg, jin.cur - jin.beg);
        std::cout << "\n";
    }

    return 0;
}

