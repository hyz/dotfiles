#include <stdlib.h>
#include <stdexcept>
#include <cstring>
#include <string>
#include <vector>
// #include <algorithm>
#include <iostream>

struct jsexins
{
    const char* beg;
    const char* end;
    mutable const char* cur;

    jsexins(const char* b=0, const char* e=0) { beg = b; end = e; cur = beg; }
};

struct jsexc : jsexins, std::invalid_argument
{
    jsexc(const jsexins& jis)
        : jsexins(jis), std::invalid_argument(std::string(jis.cur,jis.end))
    {}
};

struct jsexnode
{
    virtual void jsex_kv(jsexins& jis, const std::string& k) = 0;
    virtual ~jsexnode() {}
};
jsexins& operator>>(jsexins& jis, jsexnode& a);

struct jsexnull {};
jsexins& operator>>(jsexins& jis, const jsexnull& nul);

struct jsexdictnull : jsexnode
{
    virtual void jsex_kv(jsexins& jis, const std::string& k) {
        jis >> jsexnull();
    }
};

#define JSEX(ji) do{ throw jsexc(ji); }while(0)

const char* skips(const char *cur, const char* end)
{
    for (; cur != end; ++cur)
        if (!std::isspace(*cur))
            break;
    return cur;
}

// inline int jsex_peekc(jsexins& jis)
// {
//     jis.cur = skips(jis.cur, jis.end);
//     if (jis.cur >= jis.end)
//         JSEX(jis);
//     return *jis.cur;
// }

inline char jsex_peekc_skips(jsexins& jis)
{
    jis.cur = skips(jis.cur, jis.end);
    if (jis.cur >= jis.end)
        JSEX(jis);
    return *jis.cur;
}

inline char jsex_peekc(jsexins& jis)
{
    if (jis.cur >= jis.end)
        JSEX(jis);
    return *jis.cur;
}

inline char jsex_getc_skips(jsexins& jis)
{
    jis.cur = skips(jis.cur, jis.end);
    if (jis.cur >= jis.end)
        JSEX(jis);
    return *jis.cur++;
}

inline char jsex_getc(jsexins& jis)
{
    if (jis.cur >= jis.end)
        JSEX(jis);
    return *jis.cur++;
}

jsexins& operator>>(jsexins& jis, std::string& s)
{
    jis.cur = skips(jis.cur, jis.end);

    int q;
    switch ( (q = jsex_getc(jis))) {
        case '"': case '\'': break;
        default: JSEX(jis);
    }

    const char *p = jis.cur;
    while (p < jis.end && *p != q)
        ++p;
    if (p == jis.end)
        JSEX(jis);

    s.assign(jis.cur, p);
    jis.cur = p+1;

    return jis;
}

static long jsex_longint(jsexins& jis)
{
    jis.cur = skips(jis.cur, jis.end);

    const char* sign = 0;
    const char* d = jis.cur;
    const char* dot = 0;
    const char* p = jis.cur;

Nxt_Char:
    if (p < jis.end) switch (*p)
    {
        case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
            ++p;
            goto Nxt_Char;
        case '-': case '+':
            if (sign)
                JSEX(jis);
            sign = p++;
            d = p;
            goto Nxt_Char;
        case '.':
            if (dot)
                JSEX(jis);
            dot = p++;
            goto Nxt_Char;
        default:
            break;
    }
    if (d == p)
        JSEX(jis);
    jis.cur = p;

    if (!sign)
        sign = "+";

    return atoi(d) * ((int)',' - (int)(*sign));
}

template <typename T> jsexins& jsex_xint(jsexins& jis, T& x) { x = (T)jsex_longint(jis); return jis; }

inline jsexins& operator>>(jsexins& jis, int& i) { return jsex_xint(jis, i); }
inline jsexins& operator>>(jsexins& jis, unsigned int& i) { return jsex_xint(jis, i); }
inline jsexins& operator>>(jsexins& jis, short& i) { return jsex_xint(jis, i); }
inline jsexins& operator>>(jsexins& jis, unsigned short& i) { return jsex_xint(jis, i); }
inline jsexins& operator>>(jsexins& jis, long& i) { return jsex_xint(jis, i); }
inline jsexins& operator>>(jsexins& jis, unsigned long& i) { return jsex_xint(jis, i); }
inline jsexins& operator>>(jsexins& jis, unsigned char& i) { return jsex_xint(jis, i); }

inline jsexins& operator>>(jsexins& jis, char& c)
{
    c = jsex_getc_skips(jis);
    return jis;
}

inline bool is_starts(const char* p, const char* end, const char* s)
{
    while (p < end && *s && *p == *s)
        ++p, ++s;
    return (*s==0);
}

jsexins& operator>>(jsexins& jis, bool& ret)
{
    switch (jsex_getc_skips(jis))
    {
        case 't': case 'T':
            if (!is_starts(jis.cur, jis.end, "rue"))
                JSEX(jis);
            jis.cur += 3;
            ret = true;
            break;
        case 'f': case 'F':
            if (!is_starts(jis.cur, jis.end, "alse"))
                JSEX(jis);
            jis.cur += 4;
            ret = false;
            break;
        default:
            JSEX(jis);
    }

    return jis;
}

template <typename T>
jsexins& operator>>(jsexins& jis, std::vector<T>& v)
{
    if (jsex_getc_skips(jis) != '[')
        JSEX(jis);
    if (jsex_peekc_skips(jis) == ']')
        { ++jis.cur; return jis; }

    char chr;
        do {
            T x;
            jis >> x;
            v.push_back(x);
    } while ( (chr = jsex_getc_skips(jis)) == ',');

    if (chr != ']')
            JSEX(jis);

    return jis;
}

jsexins& operator>>(jsexins& jis, jsexnode& a)
{
    if (jsex_getc_skips(jis) != '{')
        JSEX(jis);
    if (jsex_peekc_skips(jis) == '}')
        { ++jis.cur; return jis; }

    char chr ;
    do {
        std::string k;
        jis >> k;
        if (jsex_getc_skips(jis) != ':')
            JSEX(jis);

        const char *cur = jis.cur;
        a.jsex_kv(jis, k);

        if (cur == jis.cur)
            jis >> jsexnull();

    } while ( (chr = jsex_getc_skips(jis)) == ',');

    if (chr  != '}')
        JSEX(jis);

    return jis;
}

jsexins& operator>>(jsexins& jis, const jsexnull& nul)
{
    switch (jsex_peekc_skips(jis))
    {
        case 't': case 'T': case 'f': case 'F':
            { bool b; jis >> b; }
            break;
        case '"':
            { std::string x; jis >> x; }
            break;
        case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9': case '-': case '+':
            { long x; jis >> x; }
            break;
        case '[':
            { std::vector<jsexnull> v; jis >> v; }
            break;
        case '{':
            { jsexdictnull x; jis >> x; }
            break;
        default:
            JSEX(jis);
    }

    return jis;
}

// { 'int': 1, "str": "s", 'foo':"foo", "vec" :[1,2,3], "b" : { "int" : 9, 'str': '""'}, "": False }
struct A : jsexnode
{
    short aint;
    std::string astr;
    std::vector<int> vec;

    struct B : jsexnode
    {
        short bint;
        std::string bstr;

        virtual void jsex_kv(jsexins& jis, const std::string& k)
        {
            if (k == "int")
                jis >> bint;
            else if (k == "str")
                jis >> bstr;
        }
        void print()
        {
            std::cout << "bint=" << bint << "\n"
                << "bstr=" << bstr << "\n"
                ;
        }
    } b;

    void print()
    {
        std::cout << "aint=" << aint << "\n"
            << "astr=" << astr << "\n"
            << "vec=[" << vec[0] << ",...," << vec[vec.size()-1] << "]\n"
            ;
        b.print();
    }

    virtual void jsex_kv(jsexins& jis, const std::string& k)
    {
        if (k == "int")
            jis >> aint;
        else if (k == "str")
            jis >> astr;
        else if (k == "b")
            jis >> b;
        else if (k == "vec")
            jis >> vec;
    }
};

int main(int ac, char *const av[])
{
    std::string line;
    while (std::getline(std::cin, line))
    {
        jsexins jis(&line[0], &line[0] + line.size());

        A a;
        jis >> a;
        a.print();

        std::cout.write(jis.cur, jis.end - jis.cur);
        std::cout << "\n";
    }

    return 0;
}

