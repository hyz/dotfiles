#ifndef __JOSN_H__
#define __JOSN_H__

#include <stdio.h>
#include <ostream>
#include <sstream>
#include <string>
#include <vector>
//#include <list>
#include <stack>
//#include <stdexcept>
// #include <map>
#include <boost/container/flat_map.hpp>
// #include <boost/assign/list_inserter.hpp>
#include <boost/range.hpp>
#include <boost/variant.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/noncopyable.hpp>
#include <boost/none.hpp>
#include <boost/type_traits/is_enum.hpp>
#include <boost/type_traits/is_floating_point.hpp>
#include <boost/type_traits/is_integral.hpp>
#include <boost/core/enable_if.hpp>
#include <boost/foreach.hpp>
#include <boost/optional/optional.hpp>

#include "myerror.h"

namespace json {
// using boost::assign::insert;

// using boost::none;
typedef boost::none_t null_t;
typedef bool bool_t;
typedef double float_t;
typedef int64_t int_t;
typedef std::string string;

struct object;
struct array;
struct tag_begin {};
struct tag_end {};

typedef boost::variant<int_t,double,bool,null_t,string,array,object> variant;

namespace types {
    typedef json::null_t null_t;
    typedef json::bool_t bool_t;
    typedef json::int_t int_t;
    typedef json::float_t float_t;
    typedef json::string string;
    typedef json::object object;
    typedef json::array array;
    typedef json::variant variant;
} // namespace types

struct error : ::myerror {};
struct error_key : error {};
struct error_index : error {};
struct error_value : error {};
struct error_decode : error {};

typedef boost::error_info<struct json_key,std::string> errinfo_key;
typedef boost::error_info<struct json_index,size_t> errinfo_index;
typedef boost::error_info<struct json_index,std::string> errinfo_json;

template< typename T > void assign(variant& var, T const & val
    /*, typename boost::disable_if< boost::mpl::or_< boost::is_integral<T> , boost::is_enum<T> > >::type* = 0*/);
//template< typename T > void assign(variant& var, T const & val , typename boost::enable_if< boost::mpl::or_< boost::is_integral<T> , boost::is_enum<T> > >::type* = 0);

typedef std::vector<json::variant> array_impl_t;
struct array : array_impl_t
{
    typedef size_t index_type;

    template <typename T> iterator insert(iterator p, T const& v);

    template <typename T> iterator push_back(T const& x)
    {
        return this->insert(end(), x);
    }

    template <typename T> T& at(size_t x); // = delete // *Not* use this
};

typedef boost::container::flat_map<std::string, json::variant> object_impl_t;
struct object : object_impl_t
{
    typedef std::string index_type;

    bool rename(const std::string & fr, const std::string & to);

    bool exist(const std::string& k) const { return find(k) != end(); }

    template <typename T>
        std::pair<iterator,bool> emplace(std::string const& k, T const& v);
};

template <typename T>
array::iterator array::insert(iterator p, T const& v)
{
    auto it = array_impl_t::insert(p, value_type());
    json::assign(*it, v);
    return it;
}

template <typename T>
std::pair<object::iterator,bool> object::emplace(std::string const& k, T const& v)
{
    auto p = object_impl_t::insert( std::make_pair(k, variant()) );
    if (p.second)
        assign(p.first->second, v);
    return p;
}

struct object_inserter
{
    template <typename T> object_inserter& operator()(std::string const &k, T const& v)
    {
        jo_.emplace(k, v);
        return *this;
    }
    object_inserter(object & jo) : jo_(jo) {}
    object & jo_;
};

inline object_inserter insert(object & jo) { return object_inserter(jo); }

// template< typename T >
// void assign(variant& var, T const & val
//     , typename boost::disable_if< boost::mpl::or_< boost::is_integral<T> , boost::is_enum<T> > >::type*)
// {
//     var = val;
// }


//template< typename T >
//boost::optional<T &> const get(variant & var)
//{
//    if (T * p = boost::get<T>(&var))
//        return boost::optional<T &>(*p);
//    return boost::optional<T &>();
//}
//
//template< typename T >
//boost::optional<T const&> const get(variant const& var)
//{
//    if (T const* p = boost::get<T>(&var))
//        return boost::optional<T const&>(*p);
//    return boost::optional<T const&>();
//}

//template< typename T >
//boost::optional<T &> get(object & jo, std::string const& k)
//{
//    boost::optional<T &> ret;
//    auto it = jo.find(k);
//    if (it != jo.end())
//        ret = get<T>(it->second);
//    return ret;
//}

template <class T, class Enable = void> 
struct traits {
    typedef T value_type;
    typedef T const& return_type;
    typedef boost::optional<T const&> optional_type;
};

template <class T>
struct traits<T, typename boost::enable_if< boost::mpl::or_< boost::is_integral<T> , boost::is_enum<T> > >::type>
{
    typedef json::int_t value_type;
    typedef T return_type;
    typedef boost::optional<T> optional_type;
};

//template <class T> class traits<T, typename enable_if<is_float<T> >::type> { };

template< typename T >
void assign(variant& var, T const & val
    /*, typename boost::enable_if< boost::mpl::or_< boost::is_integral<T> , boost::is_enum<T> > >::type**/)
{
    typedef typename traits<T>::value_type value_type;
    var = value_type(val);
}

template <typename T>
typename traits<T>::optional_type const as(variant const& var
    /*, typename boost::disable_if< boost::mpl::or_< boost::is_integral<T> , boost::is_enum<T> > >::type* = 0*/)
{
    typedef typename traits<T>::value_type value_type;
    typedef typename traits<T>::optional_type optional_type;
    if (auto ov = boost::get<value_type>(&var))
        return optional_type(*ov);
    return optional_type();
}

//template <typename T>
//typename traits<T>::optional_type const as(variant const& var
//    , typename boost::enable_if< boost::mpl::or_< boost::is_integral<T> , boost::is_enum<T> > >::type* = 0)
//{
//    if (auto ov = boost::get<int_t>(&var))
//        return typename traits<T>::optional_type(*ov);
//    return typename traits<T>::optional_type();
//}

template <typename T, typename K>
typename traits<T>::optional_type const as(object const& jo, K const& k)
{
    auto i = jo.find(k);
    if (i == jo.end())
        return typename traits<T>::optional_type();
    return as<T>(i->second);
}

template <typename T>
typename traits<T>::optional_type const as(array const& ja, size_t x)
{
    typedef typename traits<T>::value_type value_type;
    typedef typename traits<T>::optional_type optional_type;
    if (x >= ja.size())
        BOOST_THROW_EXCEPTION( json::error_index() << errinfo_index(x) );
    return as<T>(ja[x]);
}

// template< typename T >
// boost::optional<T const&> as(array const& ja, size_t x
//     , typename boost::disable_if< boost::mpl::or_< boost::is_integral<T> , boost::is_enum<T> > >::type* = 0)
// {
//     return get<T>(ja, x);
// }
// 
// template< typename T >
// boost::optional<T> as(array const& ja, size_t x
//     , typename boost::enable_if< boost::mpl::or_< boost::is_integral<T> , boost::is_enum<T> > >::type* = 0)
// {
//     if (auto const* ov = get<json::int_t>(&ja, x))
//         return boost::optional<T>(*ov);
//     return boost::optional<T>();
// }

template <typename T>
typename traits<T>::return_type const value(variant const& var)
{
    typedef typename traits<T>::value_type value_type;
    typedef typename traits<T>::return_type return_type;
    value_type const* p = boost::get<value_type>(&var);
    if (!p)
        BOOST_THROW_EXCEPTION( json::error_value() );
    return return_type(*p);
}

template <typename T, typename K>
typename traits<T>::return_type value(object const& jo, K const& k)
{
    auto i = jo.find(k);
    if (i == jo.end())
        BOOST_THROW_EXCEPTION( json::error_key() << errinfo_key(k) );

    typename traits<T>::optional_type opt = as<T>(i->second);
    if (!opt)
        BOOST_THROW_EXCEPTION( json::error_value() << errinfo_key(k) );

    return opt.value();
}

template <typename T>
typename traits<T>::return_type value(array const& ja, size_t x)
{
    if (x >= ja.size())
        BOOST_THROW_EXCEPTION( json::error_index() << errinfo_index(x) );

    typename traits<T>::optional_type opt = as<T>(ja[x]);
    if (!opt)
        BOOST_THROW_EXCEPTION( json::error_value() << errinfo_index(x) );

    return opt.value();
}

//template< typename T >
//T const * get(object const * jo, std::string const& k)
//{
//    auto it = jo->find(k);
//    if (it == jo->end())
//        return 0;
//    return get<T>(&it->second);
//}
//
//template< typename T >
//T const & get(object const & jo, std::string const& k)
//{
//    T const* p = get<T>(&jo, k);
//    if (!p)
//        BOOST_THROW_EXCEPTION( json::error_key() << errinfo_key(k) );
//    return *p;
//}
//
//template< typename T >
//T const * get(array const * ja, size_t x)
//{
//    if (x >= ja->size())
//        return 0; // BOOST_THROW_EXCEPTION( json::error_index() << errinfo_index(x) );
//    return get<T>(&(ja->operator[](x)));
//}
//
//template< typename T >
//T const & get(array const & ja, size_t x)
//{
//    T const* p = get<T>(&ja, x);
//    if (!p)
//        BOOST_THROW_EXCEPTION( json::error_value() << errinfo_index(x) );
//    return *p;
//}

extern bool _decode(variant& result, char const* beg, char const* end);

template <typename T,typename Iter>
boost::optional<T> decode(Iter beg, Iter end)
{
    BOOST_ASSERT(beg < end);
    ((void)(end-1));

    variant var;
    if (! _decode(var, &*beg, &*end))
    {
        BOOST_THROW_EXCEPTION( json::error_decode() << errinfo_json(std::string(beg,end)) );
    }

    return boost::optional<T>( boost::move(boost::get<T>(var)) );
}

template <typename T,typename Rng>
boost::optional<T> decode(Rng const& rng)
{
    return decode<T>(boost::begin(rng), boost::end(rng));
}

template <typename T, typename Ch>
boost::optional<T> decode(Ch const* c_str, char=Ch())
{
    return decode<T>(c_str, c_str + std::strlen(c_str));
}
template <typename T, typename Ch>
boost::optional<T> decode(Ch* c_str, char=Ch())
{
    return decode<T>(static_cast<Ch const*>(c_str));
}

std::string encode(json::array const & jv);
std::string encode(json::object const & jv);
std::string encode(json::variant const & jv);

} // namespace json

inline std::ostream& operator<<(std::ostream& out, json::object const& o)
{
    return out << json::encode(o);
}

inline std::ostream& operator<<(std::ostream& out, json::array const& o)
{
    return out << json::encode(o);
}

namespace json {

template <typename Access, typename Object>
struct jsv_visitor : boost::static_visitor<typename Access::result_type>
{
    typedef typename Access::result_type result_type;
    Access* access_;
    Object const& obj_;
    typename Object::index_type const& idx_;

    jsv_visitor(Access* p, Object const& o, typename Object::index_type const& k)
        : obj_(o), idx_(k)
    { access_ = p; }

    result_type operator()(json::int_t x) { return access_->access(obj_, idx_, x); }
    result_type operator()(json::null_t x) { return access_->access(obj_, idx_, x); }
    result_type operator()(json::float_t x) { return access_->access(obj_, idx_, x); }
    result_type operator()(json::bool_t x) { return access_->access(obj_, idx_, x); }
    result_type operator()(json::string const& x) { return access_->access(obj_, idx_, x); }

    result_type operator()(json::object const& obj);
    result_type operator()(json::array const& arr);
    // bool access(json::variant const& v) { return boost::apply_visitor(*this, v); }
};

template <typename Access>
typename Access::result_type jsv_walk(json::object const& obj, Access & aces)
{
    typedef typename Access::result_type result_type;
    BOOST_FOREACH(auto & p, obj) {
        jsv_visitor<Access,json::object> jsv(&aces, obj, p.first);
        if (result_type ret = boost::apply_visitor(jsv, p.second)) {
            return ret;
        }
    }
    return result_type();
}

template <typename Access>
typename Access::result_type jsv_walk(json::array const& arr, Access & aces)
{
    typedef typename Access::result_type result_type;
    size_t ix = 0;
    BOOST_FOREACH(auto & x, arr) {
        jsv_visitor<Access,json::array> jsv(&aces, arr, ix);
        if (auto ret = boost::apply_visitor(jsv, x)) {
            return ret;
        }
        ++ix;
    }
    return result_type();
}

template <typename Access, typename Object>
auto jsv_visitor<Access, Object>::operator()(json::object const& obj) -> result_type
{
    if (auto ret = access_->access(obj_, idx_, obj, json::tag_begin())) {
        return ret;
    }
    if (auto ret = json::jsv_walk(obj, *(this->access_))) {
        return ret;
    }
    return access_->access(obj_, idx_, obj, json::tag_end());
}

template <typename Access, typename Object>
auto jsv_visitor<Access, Object>::operator()(json::array const& arr) -> result_type
{
    if (auto ret = access_->access(obj_, idx_, arr, json::tag_begin())) {
        return ret;
    }
    if (auto ret = json::jsv_walk(arr, *(this->access_))) {
        return ret;
    }
    return access_->access(obj_, idx_, arr, json::tag_end());
}

////////////////////////// ////////////////////////// //////////////////////////

template <typename RetType>
struct access_dummy_helper
{
    typedef RetType result_type;

    template <typename T, typename K, typename V>
    result_type access(T const& p, K const& k, V const& v, json::tag_begin) const
    {
        // LOG << "begin" << k;
        return result_type();
    }
    template <typename T, typename K, typename V>
    result_type access(T const& p, K const& k, V const& v, json::tag_end) const
    {
        // LOG << "end" << k;
        return result_type();
    }
    template <typename T, typename K, typename V>
    result_type access(T const& p, K const& k, V const& v) const
    {
        // LOG << "value" << k << v << "ignore";
        return result_type();
    }
};

} //namespace json

#endif

