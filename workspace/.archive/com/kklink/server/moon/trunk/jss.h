#ifndef JSS_H__
#define JSS_H__

#include <stdio.h>
#include <ostream>
#include <sstream>
#include <string>
#include <vector>
#include <list>
#include <stack>
//#include <stdexcept>
#include <boost/range.hpp>
#include <boost/variant.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/noncopyable.hpp>
#include <boost/none.hpp>

#include "myerror.h"
#include "libjson/json.h"

namespace json {

struct array;
struct object;
using std::string;
using boost::none_t;
using boost::none;

typedef boost::variant<int,unsigned int,long long int,double,bool,json::none_t,std::string, array, object> variant;

double _convert(variant & var, double *);
template <typename T> T _convert(variant & var, T*);

struct array : std::list<variant>
{
    // template <int N> array& operator()(const char (&s)[N]) { return (*this)(std::string(s)); }
    array& operator()(const char* s) { return put(std::string(s)); }
    template <typename T> array& operator()(const T& x) { return put(x); }
    template <typename T> array& put(const T& x) { push_back(x); return *this; }

    // template <typename T> T& at(unsigned int idx, T& x) { return _at(idx, &x); }
    // template <typename T> T& at(unsigned int idx) { return _at(idx, static_cast<T*>(0)); }

    template <typename T> const T& at(unsigned int idx) const 
    {
        return const_cast<array*>(this)->_at(idx, static_cast<T*>(0));
    }
    template <typename T> T at(unsigned int idx, const T& x) const 
    {
        return const_cast<array*>(this)->_at(idx, &x);
    }

    template <typename R> array(const R& r);

    array();

private:
    template <typename T> T& _at(unsigned int idx, T* x);
};

template <typename R> array::array(const R& r)
{
    typename R::const_iterator begin = boost::begin(r), end = boost::end(r);
    for ( ; begin != end; ++begin)
        (*this)(*begin);
}

struct object : std::list<std::pair<std::string, variant> >
{
    typedef std::list<std::pair<std::string, variant> > base_type;

    inline bool exist(const std::string& k) { return find(k) != end(); }

    iterator find(const std::string& k);

    const_iterator find(const std::string& k) const
    {
        return const_cast<object*>(this)->find(k);
    }

    // template <typename T> T& get(const std::string& k, T& defa) { return _get(k, &defa); }
    // template <typename T> T& get(std::string& k) { return _get(k, static_cast<T*>(0)); }

    template <typename T> T get(const std::string& k, const T& defa) const
    {
        return const_cast<object*>(this)->_get(k, const_cast<T*>(&defa));
    }
    template <typename T> const T get(const std::string& k) const
    {
        return const_cast<object*>(this)->_get(k, static_cast<T*>(0));
    }

    void erase(const std::string& k);
    using base_type::erase;

    template <typename T> void put(const std::string& k, const T& v)
    {
        (*this)(k, v);
    }

    iterator insert(const value_type& p);

    object& operator()(const value_type& p)
    {
        insert(p);
        return *this;
    }

    object& operator()(const std::string& k, const char* s)
    {
        return (*this)(k, std::string(s));
    }

    // template <int N> object& operator()(const std::string& k, char const (&s)[N]) { return (*this)(k, std::string(s)); }

    object& operator()(const std::string& k, const variant& v)
    {
        return (*this)(std::make_pair(k, v));
    }

    template <typename T>
    object& operator()(const std::string& k, const T& x)
    {
        return (*this)(std::make_pair(k, variant(x)));
    }

    template <typename R> object(const R& r);

    object() {}

    object & operator+=(const object & rhs);
    object operator+(const object & rhs) const;

    void rename(const std::string & fr, const std::string & to);

private:
    template <typename T> T _get(const std::string& k, T* defa);
};

template <typename R> object::object(const R& r)
{
    typename R::const_iterator begin = begin(r), end = end(r);
    for ( ; begin != end; ++begin)
        (*this)(begin->first, begin->second);
}

template <typename T> T object::_get(const std::string& k, T* defa)
{
    iterator i = find(k);
    if (i == end())
    {
        if (!defa)
            THROW_EX(EN_JsonName_NotFound);
        return *defa;
    }
    return _convert(i->second, static_cast<T*>(0));
}

template <typename T> T& array::_at(unsigned int idx, T* defa)
{
    //std::advance(i, idx);
    iterator i = begin();
    while (idx-- > 0 && (i != end()))
        ++i;
    if (i == end())
    {
        if (!defa)
            THROW_EX(EN_JsonName_NotFound);
        return *defa;
    }
    return _convert(*i, static_cast<T*>(0));
}

template <typename T> T _convert(variant & var, T*)
{
    T* t = boost::get<T>(&var);
    if (!t)
        THROW_EX(EN_JsonValue_TypeError);
    return *t;
}

template <typename T> std::vector<T> to_vector(const array & a)
{
    std::vector<T> v;
    for (array::const_iterator i = a.begin(); i != a.end(); ++i)
        v.push_back(boost::get<T>(*i));
    return v;
}

struct printer;
struct parser;

variant & decode(variant & var, std::istream& in);
variant & decode(variant & var, const std::string& s);

object decode(std::istream& in);
object decode(const std::string& s);
template <typename Iter> inline object decode(Iter beg, Iter end) { return decode(std::string(beg, end)); }

template <typename T,typename Input> T decode(Input& in)
{
    variant var;
    decode(var, in);
    return boost::get<T>(var);
}

std::ostream& encode(std::ostream& out, const object& jv);
std::ostream& encode(std::ostream& out, const array& jv);
std::ostream& encode(std::ostream& out, const variant& jv);

template <typename T>
std::string encode(T const& jv)
{
    std::ostringstream oss;
    encode(oss, jv);
    return oss.str();
}

struct printer : boost::static_visitor<> //, boost::noncopyable
{
    printer(std::ostream& os)
        : jp_(new_json_printer(os), delete_json_printer)
    {
    }
    ~printer()
    {
    }

    void operator()(long long int i) const
    {
        char tmp[24];
        int len = snprintf(tmp,sizeof(tmp), "%lld", i);
        json_print_raw(jp_.get(), JSON_INT, tmp, len);
    }
    void operator()(int i) const
    {
        char tmp[24];
        int len = snprintf(tmp,sizeof(tmp), "%d", i);
        json_print_raw(jp_.get(), JSON_INT, tmp, len);
    }
    void operator()(unsigned int i) const
    {
        char tmp[24];
        int len = snprintf(tmp,sizeof(tmp), "%u", i);
        json_print_raw(jp_.get(), JSON_INT, tmp, len);
    }

    void operator()(double s) const
    {
        char tmp[24];
        int len = snprintf(tmp,sizeof(tmp), "%.6f", s);
        json_print_raw(jp_.get(), JSON_FLOAT, tmp, len);
    }

    void operator()(bool b) const { json_print_raw(jp_.get(), b ? JSON_TRUE : JSON_FALSE, 0, 0); }
    void operator()(const std::string& s) const { json_print_raw(jp_.get(), JSON_STRING, s.data(), s.size()); }
    void operator()(none_t s) const { json_print_raw(jp_.get(), JSON_NULL, 0, 0); }

    void operator()(const array& jsa) const // { (*p_)(a); }
    {
        json_print_raw(jp_.get(), JSON_ARRAY_BEGIN, NULL, 0);
        for (array::const_iterator i = jsa.begin();
                i != jsa.end(); ++i)
        {
            apply_visitor(*this, *i);
        }
        json_print_raw(jp_.get(), JSON_ARRAY_END, NULL, 0);
    }

    void operator()(const object& jso) const //{ (*p_)(s); }
    {
        json_print_raw(jp_.get(), JSON_OBJECT_BEGIN, NULL, 0);
        for (object::const_iterator i = jso.begin();
                i != jso.end(); ++i)
        {
            json_print_raw(jp_.get(), JSON_KEY, i->first.data(), i->first.size());
            apply_visitor(*this, i->second);
        }
        json_print_raw(jp_.get(), JSON_OBJECT_END, NULL, 0);
    }

    void operator()(const variant& jv) const
    {
        boost::apply_visitor(*this, jv);
    }

private:
    static int _printer_callback(void *userdata, const char *s, uint32_t length)
    {
        std::ostream* os = static_cast<std::ostream*>(userdata);
        os->write(s,length);
        return 0;
    }

    static json_printer* new_json_printer(std::ostream& os)
    {
        json_printer* p = new json_printer;
        json_print_init(p, _printer_callback, &os);
        return p;
    }
    static void delete_json_printer(json_printer *p)
    {
        json_print_free(p);
        delete p;
    }

    boost::shared_ptr<json_printer> jp_;
};

struct parser : boost::noncopyable
{
    parser(variant& jsv)
        : jsv_(jsv)
    {
        json_parser_init(&jp_, 0, _parser_callback, this);
    }

    ~parser()
    {
        json_parser_free(&jp_);
    }

    bool decode(std::istream& in);

private:
    static int _parser_callback(void *userdata, int type, const char *data, uint32_t length);

    struct jsvalue_add;

    variant& jsv_;
    std::stack< std::pair<std::string,variant> > stack_;
    json_parser jp_;
};

struct parser::jsvalue_add : boost::static_visitor<> //, boost::noncopyable
{
    std::pair<std::string,variant>* jsv_;

    jsvalue_add(std::pair<std::string,variant>& jsv)
        : jsv_(&jsv)
    {}
    // jsvalue_add() : jsv_(0) {}

    void operator()(object& o) const
    {
        o.insert( *jsv_ );
    }
    void operator()(array& a) const
    {
        a.insert( a.end(), jsv_->second );
    }

    template <typename X> void operator()(X x) const { BOOST_ASSERT(false); }
};

} // namespace json


inline std::ostream& operator<<(std::ostream& out, const json::object& o)
{
    return json::encode(out, o);
}
inline std::ostream& operator<<(std::ostream& out, const json::array& o)
{
    return json::encode(out, o);
}


#endif

