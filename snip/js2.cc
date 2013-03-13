#include <stdlib.h>
#include <stdexcept>
#include <cstring>
#include <string>
#include <vector>
#include <algorithm>
#include <iostream>

using std::find;

struct jsrange
{
    const char *begin;
    const char *end;
    jsrange(const char* b=0, const char *e=0) { begin = b; end = e; }

    inline bool operator==(const jsrange& l)
        { return (begin == l.begin && end == l.end); }
};

struct jsexc : std::invalid_argument
{
    const char *begin;
    const char *end;

    jsexc(const jsrange& r)
        : std::invalid_argument(std::string(r.begin,r.end))
    { begin = r.begin; end = r.end; }

    static void raise(const jsrange& r) { throw jsexc(r); }
};


inline bool empty(jsrange& r)
{
    return r.begin == r.end;
}

inline char front(jsrange& r)
{
    if (empty(r))
        jsexc::raise(r);
    return *r.begin;
}

inline char back(jsrange& r)
{
    if (empty(r))
        jsexc::raise(r);
    return *(r.end-1);
}

// struct jsexspaces : jsrange {}; jsinput& operator>>(jsinput& jis, jsexspaces& sps);

struct jsinput : jsrange
{
    jsinput(const jsrange& r) : jsrange(r.begin, r.end) {}
    jsinput() {}
};

struct jsnumber : jsrange
{ };

struct jsbool : jsrange
{ };

struct jsstring : jsrange
{ };

struct jsany : jsrange
{ };

// template <typename T> struct jsexarray : jsrange {};
// template <typename T> struct jsexdict : jsrange {};

jsinput& operator>>(jsinput& jis, const jsany& nul);

struct jsstruct
{
    virtual void jsex_kv(jsinput& jis, const std::string& k) = 0;
    virtual ~jsstruct() {}
};

jsinput& operator>>(jsinput& jis, jsstruct& a);

//struct jsexdictnull : jsstruct
//{
//    virtual void jsex_kv(jsinput& jis, const std::string& k) {
//        jis >> jsany();
//    }
//};

////////////////////
//
//
//

//jsinput& operator>>(jsinput& jis, jsexspaces& sps)
//{
//    sps.begin = find(jis.begin, jis.end, IsSpace());
//    sps.end = find(sps.begin, jis.end, NotSpace());
//    return jis;
//}

jsinput& jsextract(jsinput& jis, jsstring& ret)
{
    jis.begin = ret.begin = find(jis.begin, jis.end, NotSpace());

    switch (front(jis)) {
        case '"': case '\'':
            break;
        default: jsexc::raise(jis);
    }

    ret.end = ++jis.begin;
    do {
        if (front(jis) == front(ret))
            break;
    } while (++jis.begin < jis.end);

    if (empty(jis))
        jsexc::raise(jis);
    ret.end = ++jis.begin;

    return jis;
}

inline jsinput& operator>>(jsinput& jis, std::string& ret)
{
    jsstring s;
    jsextract(jis, s);
    ret.assign(s.begin+1, s.end-1);
    return jis;
}

jsinput& jsextract(jsinput& jis, jsnumber& ret)
{
    const char* sign = 0;
    const char* d = 0;
    const char* dot = 0;

    jis.begin = ret.begin = find(jis.begin, jis.end, NotSpace());

Nxt_Char:
    switch (front(jis))
    {
        case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
            if (!d)
                d = jis.begin;
            ++jis.begin;
            goto Nxt_Char;
        case '-': case '+':
            if (sign || d)
                jsexc::raise(jis);
            sign = jis.begin++;
            goto Nxt_Char;
        case '.':
            if (dot)
                jsexc::raise(jis);
            dot = jis.begin++;
            goto Nxt_Char;
        default:
            break;
    }
    if (!d || dot+1 == jis.begin)
        jsexc::raise(jis);
    ret.end = jis.begin;

    return jis;
}

template <typename T> jsinput& jsex_xint(jsinput& jis, T& x)
{
    jsnumber numb;
    jsextract(jis, numb);
    std::istringstream ins(numb.begin, numb.end);
    ins >> x;
    return jis;
}

inline jsinput& operator>>(jsinput& jis, int& i) { return jsex_xint(jis, i); }
inline jsinput& operator>>(jsinput& jis, unsigned int& i) { return jsex_xint(jis, i); }
inline jsinput& operator>>(jsinput& jis, short& i) { return jsex_xint(jis, i); }
inline jsinput& operator>>(jsinput& jis, unsigned short& i) { return jsex_xint(jis, i); }
inline jsinput& operator>>(jsinput& jis, long& i) { return jsex_xint(jis, i); }
inline jsinput& operator>>(jsinput& jis, unsigned long& i) { return jsex_xint(jis, i); }
inline jsinput& operator>>(jsinput& jis, unsigned char& i) { return jsex_xint(jis, i); }

//inline jsinput& operator>>(jsinput& jis, char& c)
//{
//    jsexspaces sp;
//    return jis >> sp >> c;
//}

inline bool is_starts(const char* p, const char* end, const char* s)
{
    while (p < end && *s && *p == *s)
        { ++p; ++s; }
    return (*s==0);
}

jsinput& jsextract(jsinput& jis, jsbool& ret)
{
    jis.begin = ret.begin = find(jis.begin, jis.end, NotSpace());
    switch (front(jis))
    {
        case 't': case 'T':
            if (!is_starts(jis.begin+1, jis.end, "rue"))
                jsexc::raise(jis);
            jis.begin += 4;
            break;
        case 'f': case 'F':
            if (!is_starts(jis.begin+1, jis.end, "alse"))
                jsexc::raise(jis);
            jis.begin += 5;
            break;
        default:
            jsexc::raise(jis);
    }
    ret.end = jis.begin;

    return jis;
}

jsinput& operator>>(jsinput& jis, bool& ret)
{
    jsbool b;
    jsextract(jis, b);
    ret = (b.end - b.begin == 4);
    return jis;
}

template <>
struct jsvector : jsarray_base
{
    virtual void element(jsinput& jis) { jsany a; jis >> a; }
};

template <typename C>
struct jsvector : jsarray_base
{
    C *vec_;
    jsvector(C* a) { vec_ = a; }

    virtual void element(jsinput& jis)
    {
        vec_->resize(vec_->size() + 1);
        jis >> vec_->back();
    }
};

jsinput& jsextract(jsinput& jis, jsarray_base& ret)
{
    jis.begin = ret.begin = find(jis.begin, jis.end, NotSpace());

    if (front(jis) != '[')
        jsexc::raise(jis);

    ret.end = find(jis.begin + 1, jis.end, NotSpace());
    if (ret.end < jis.end)
        ++ret.end;
    if (back(ret) == ']')
    {
        jis.begin = ret.end;
        return jis;
    }

    do {
        ++jis.begin;
        jis = ret.element(jis);
        jis.begin = find(jis.begin, jis.end, NotSpace());
    } while (front(jis) == ',');

    if (front(jis) != ']')
        jsexc::raise(jis);
    ret.end = ++jis.begin;

    return jis;
}

template <typename T>
jsinput& operator>>(jsinput& jis, std::vector<T>& v)
{
    jsvector<std::vector<T> > vec(v);
    return jsextract(jis, vec);
}

struct jsstruct_range : jsrange
{
    jsstruct *node;
    jsstruct_range(jsstruct *a) { node = a; }
};

jsinput& jsextract(jsinput& jis, jsstruct_range& ret)
{
    jis.begin = ret.begin = find(jis.begin, jis.end, NotSpace());

    if (front(jis) != '{')
        jsexc::raise(jis);

    ret.end = find(jis.begin + 1, jis.end, NotSpace());
    if (ret.end < jis.end)
        ++ret.end();
    if (back(ret) == '}')
    {
        jis.begin = ret.end;
        return jis;
    }

    do {
        jsstring k;
        jsextract(jis, k);

        // jsspaces sp; char chr; jis >> sp >> chr >> sp;
        jis.begin = find(jis.begin, jis.end, NotSpace());
        if (front(jis) != ':')
            jsexc::raise(jis);
        jis.begin = find(jis.begin+1, jis.end, NotSpace());

        jsrange v = jis;
        if (ret.node)
            ret.node->jsex_kv(jis, std::string(k.begin, k.end));
        if (v == jis)
        {
            jsany a;
            jsextract(jis, a);
        }

        jis.begin = find(jis.begin, jis.end, NotSpace());
    } while (front(jis) == ',');

    if (front(jis) != '}')
        jsexc::raise(jis);
    ret.end = ++jis.begin;

    return jis;
}

jsinput& operator>>(jsinput& jis, jsstruct& a)
{
    jsstruct_range jn(&a);
    return jsextract(jis, jn);
}

jsinput& operator>>(jsinput& jis, jsany& nul)
{
    jis.begin = nul.begin = find(jis.begin, jis.end, NotSpace());
    switch (front(jis))
    {
        case 't': case 'T': case 'f': case 'F':
            { jsbool b; jsextract(jis, b); }
            break;
        case '"': case '\'':
            { jsstring x; jsextract(jis, x); }
            break;
        case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
        case '-': case '+':
            { jsnumber x; jsextract(jis, x); }
            break;
        case '[':
            { jsvector<> a; jsextract(jis, a); }
            break;
        case '{':
            { jsstruct_range a(0); jsextract(jis, a); }
            break;
        default:
            jsexc::raise(jis);
    }
    nul.end = jis.begin;

    return jis;
}

// { 'int': 1, "str": "s", 'foo':"foo", "vec" :[1,2,3], "b" : { "int" : 9, 'str': '""'}, "": False }
struct A : jsstruct
{
    short aint;
    std::string astr;
    std::vector<int> vec;

    struct B : jsstruct
    {
        short bint;
        std::string bstr;

        virtual void jsex_kv(jsinput& jis, const std::string& k)
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

    virtual void jsex_kv(jsinput& jis, const std::string& k)
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
        jsinput jis(&line[0], &line[0] + line.size());

        A a;
        jis >> a;
        a.print();

        std::cout.write(jis.begin, jis.end - jis.begin);
        std::cout << "\n";
    }

    return 0;
}

