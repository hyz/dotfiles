#include <boost/foreach.hpp>
#include "log.h"
#include "json.h"

template <typename return_pair, typename Parent, typename Object>
struct jsv_walker : boost::static_visitor<return_pair>
{
    Parent* parent_;
    Object const& obj_;
    typename Object::index_type const& idx_;

    jsv_walker(Parent* p, Object const& o, typename Object::index_type const& k)
        : obj_(o), idx_(k)
    { parent_ = p; }

    return_pair operator()(json::int_t x) { return parent_->access(obj_, idx_, x); }
    return_pair operator()(json::null_t x) { return parent_->access(obj_, idx_, x); }
    return_pair operator()(json::float_t x) { return parent_->access(obj_, idx_, x); }
    return_pair operator()(json::bool_t x) { return parent_->access(obj_, idx_, x); }
    return_pair operator()(json::string const& x) { return parent_->access(obj_, idx_, x); }

    return_pair operator()(json::object const& obj);
    return_pair operator()(json::array const& arr);
    // bool access(json::variant const& v) { return boost::apply_visitor(*this, v); }
};

template <typename return_pair, typename Parent, typename Object>
return_pair jsv_walker<return_pair, Parent, Object>::operator()(json::object const& obj)
{
    auto retp = parent_->access(obj_, idx_, obj, json::tag_begin());
    if (retp) {
        return retp;
    }
    BOOST_FOREACH(auto & p, obj) {
        jsv_walker<return_pair,Parent,json::object> jsv(parent_, obj, p.first);
        retp = boost::apply_visitor(jsv, p.second);
        if (retp) {
            return retp;
        }
    }
    return parent_->access(obj_, idx_, obj, json::tag_end());
}

template <typename return_pair, typename Parent, typename Object>
return_pair jsv_walker<return_pair, Parent, Object>::operator()(json::array const& arr)
{
    auto retp = parent_->access(obj_, idx_, arr, json::tag_begin());
    if (retp) {
        return retp;
    }
    size_t ix = 0;
    BOOST_FOREACH(auto & x, arr) {
        jsv_walker<return_pair,Parent,json::array> jsv(parent_, arr, ix);
        retp = boost::apply_visitor(jsv, x);
        if (retp) {
            return retp;
        }
        ++ix;
    }
    return parent_->access(obj_, idx_, arr, json::tag_end());
}

template <typename Op>
typename Op::return_type walk(json::object const& obj, Op & op)
{
    typedef typename Op::return_type return_t;
    jsv_walker<return_t,Op,json::object> jsv(&op, obj, std::string());
    return jsv(obj);
}

template <typename Ret>
struct access_helper
{
    typedef Ret return_type;

    template <typename T, typename K, typename V>
    return_type access(T const& p, K const& k, V const& v, json::tag_begin) const
    {
        LOG << "begin" << k;
        return return_type();
    }
    template <typename T, typename K, typename V>
    return_type access(T const& p, K const& k, V const& v, json::tag_end) const
    {
        LOG << "end" << k;
        return return_type();
    }
    template <typename T, typename K, typename V>
    bool access(T const& p, K const& k, V const& v) const
    {
        LOG << "value" << k << v << "ignore";
        return return_type();
    }
};

static bool yx_onoff = 0;
struct subs_finder2 : access_helper<bool>
{
    using access_helper<bool>::access;

    template <typename T, typename K>
    bool access(T const& p, K const& k, std::string const& v) const
    {
        bool stop = 0;
        LOG << "value" << k << v << yx_onoff;
        if (v == "yx") {
            stop = yx_onoff;
            yx_onoff = !yx_onoff;
        }
        return stop;
    }
};

void f00(json::object const& jv)
{
    LOG << "";
    subs_finder2 subs;
    auto res = walk(jv, subs);
    (void)res;
    //LOG << res.first;
}

//boost::iterator_range<std::string::const_iterator>
//    find_any(std::string const& txt, std::vector<std::string> const& keywords)
//    {
//        BOOST_FOREACH(auto const &k, keywords) {
//            auto r = boost::algorithm::find(txt, k);
//            if (!empty(r)) {
//                return r;
//            }
//        }
//        return make_iterator_range(std::string()); //iterator_range<string::const_iterator>();
//    }
//
//boost::iterator_range<std::string::const_iterator>
//    find_any(json::object const& jo, std::vector<std::string> const& keywords)
//    {
//        std::vector<std::string*> lis;
//        jo, lis;
//
//        BOOST_FOREACH(auto const &y, lis) {
//            auto r = find_any(y, keywords);
//            if (!empty(r)) {
//                return r;
//            }
//        }
//        return make_iterator_range(std::string()); //iterator_range<string::const_iterator>();
//    }
//
