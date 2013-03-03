#include <stdexcept>
#include <cstring>
#include <string>
#include <vector>
#include <algorithm>
#include <iostream>

struct jsexins;

struct jsexnode
{
    virtual void jsex_kv(jsexins& jis, const std::string& k) = 0;
    virtual ~jsexnode() {}
};

struct jsexins
{
    const char* beg;
    const char* end;
    mutable const char* cur;

    jsexins(const char* b=0, const char* e=0) { beg = b; end = e; cur = beg; }
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

struct jsexc : jsexins, std::invalid_argument
{
    jsexc(const jsexins& jis)
        : jsexins(jis), std::invalid_argument(std::string(jis.cur,jis.end))
    {}
};

#define JSEX(ji) do{ throw jsexc(ji); }while(0)

jsexins& jsex_skips(jsexins& jis)
{
    for (; jis.cur != jis.end; ++jis.cur)
        if (!std::isspace(*jis.cur))
            break;
    return jis;
}

inline int jsex_peekc(jsexins& jis)
{
    jsex_skips(jis);
    return *jis.cur;
}

inline int jsex_getc(jsexins& jis)
{
    jsex_skips(jis);
    if (jis.cur >= jis.end)
        JSEX(jis);
    return *jis.cur++;
}

char* jsex_getc_n(jsexins& jis, char a[], int n)
{
    // jsex_skips(jis);
    if (jis.cur + n > jis.end)
        JSEX(jis);
    std::memcpy(a, jis.cur, n);
    jis.cur += n;
    return a;
}

jsexins& operator>>(jsexins& jis, std::string& s)
{
    int q;
    switch ( (q = jsex_getc(jis))) {
        case '"': case '\'': break;
        default: JSEX(jis);
    }

    const char *e = jis.cur;
    while (e < jis.end && *e != q)
        ++e;
    if (e == jis.end)
        JSEX(jis);
    s.assign(jis.cur, e);
    jis.cur = e+1;

    return jis;
}

static long jsex_longint(jsexins& jis)
{
    jsex_skips(jis);

    const char* sign = 0;
    const char* d = jis.cur;
    const char* dot = 0;
    const char* p = jis.cur;

_Next:
    if (p < jis.end) switch (*p)
    {
        case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
            ++p;
            goto _Next;
        case '-': case '+':
            if (sign)
                JSEX(jis);
            sign = p++;
            d = p;
            goto _Next;
        case '.':
            if (dot)
                JSEX(jis);
            dot = p++;
            goto _Next;
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
// inline jsexins& operator>>(jsexins& jis, char& i) { return jsex_xint(jis, i); }
inline jsexins& operator>>(jsexins& jis, unsigned char& i) { return jsex_xint(jis, i); }

jsexins& operator>>(jsexins& jis, bool& b)
{
    switch (jsex_getc(jis))
    {
        case 't': case 'T':
            {
                char a[3];
                jsex_getc_n(jis, a, 3);
                if (memcmp("rue", a, 3))
                    JSEX(jis);
            }
            break;
        case 'f': case 'F':
            {
                char a[4];
                jsex_getc_n(jis, a, 4);
                if (memcmp("alse", a, 4))
                    JSEX(jis);
            }
            break;
        default:
            JSEX(jis);
    }

    return jis;
}

template <typename T>
jsexins& operator>>(jsexins& jis, std::vector<T>& v)
{
    if (jsex_getc(jis) == '[')
    {
        if (jsex_peekc(jis) == ']')
        {
            ++jis.cur;
            return jis;
        }
        int c;
        do {
            T x;
            jis >> x;
            v.push_back(x);
        } while ( (c = jsex_getc(jis)) == ',');
        if (c != ']')
            JSEX(jis);
    }

    return jis;
}

jsexins& operator>>(jsexins& jis, jsexnode& a)
{
    if (jsex_getc(jis) != '{')
        JSEX(jis);
    if (jsex_peekc(jis) == '}')
        { ++jis.cur; return jis; }

    int c;
    do {
        std::string k;
        jis >> k;
        if (jsex_getc(jis) != ':')
            JSEX(jis);

        const char *cur = jis.cur;
        a.jsex_kv(jis, k);

        if (cur == jis.cur)
            jis >> jsexnull();

    } while ( (c=jsex_getc(jis)) == ',');

    if (c != '}')
        JSEX(jis);

    return jis;
}

jsexins& operator>>(jsexins& jis, const jsexnull& nul)
{
    switch (jsex_peekc(jis))
    {
        case 't': case 'T': case 'f': case 'F':
            { bool b; jis >> b; }
            break;
        case '"':
            { std::string x; jis >> x; }
            break;
        case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9': case '-': case '+':
            { int x; jis >> x; }
            break;
        case '[':
            { std::vector<jsexnull> x; jis >> x; }
            break;
        case '{':
            { jsexdictnull x; jis >> x; }
            break;
        default:
            JSEX(jis);
    }

    return jis;
}

// { 'int': 1, "str": "s", "vec" :[1,2,3], "b" : { "int" : 9, 'str': '""'}, "": False }
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

