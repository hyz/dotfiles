// #include "myconfig.h"
#include <boost/range.hpp>
#include <iostream>
#include <sstream>
#include "libjson/json.h"
#include "myerror.h"
#include "json.h"

using namespace std;
using namespace boost;

namespace json {

bool object::rename(const std::string & fr, const std::string & to)
{
    json::variant tmp;
    {
        auto i = find(fr);
        if (i == end())
            return false;

        std::swap(tmp, i->second);
        erase(i);
    }

    insert( std::make_pair(to, tmp) );
    return true;
}

struct parser : boost::noncopyable
{
    parser(json::variant& jsv)
        : jsv_(jsv)
    {
        json_parser_init(&jp_, 0, _parser_callback, this);
    }

    ~parser()
    {
        json_parser_free(&jp_);
    }

    bool operator()(char const* beg, char const* end)
    {
        BOOST_ASSERT(stack_.empty());
        BOOST_ASSERT(beg < end);
        if (beg >= end) return false;

        do
        {
            if (json_parser_string(&jp_, beg, end - beg, NULL))
                return false;
        }
        while (!stack_.empty());

        return true;
    }

private:
    static int _parser_callback(void *userdata, int type, const char *data, uint32_t length);

    struct jsvalue_add;

    json::variant& jsv_;
    std::stack< std::pair<std::string,json::variant> > stack_;
    json_parser jp_;
};

struct parser::jsvalue_add : boost::static_visitor<> //, boost::noncopyable
{
    std::pair<std::string,json::variant>* jsv_;

    jsvalue_add(std::pair<std::string,json::variant>& jsv)
        : jsv_(&jsv)
    {}
    // jsvalue_add() : jsv_(0) {}

    void operator()(json::object& jo) const
    {
        jo.insert( *jsv_ );
    }
    void operator()(json::array& ja) const
    {
        ja.insert( ja.end(), jsv_->second );
    }

    template <typename X> void operator()(X x) const { BOOST_ASSERT(false); }
};

int parser::_parser_callback(void *userdata, int type, const char *data, uint32_t length)
{
    parser* self = static_cast<parser*>(userdata);

    std::pair<std::string,json::variant> result;
    json::variant& val = result.second;
    switch (type)
    {
    case JSON_KEY: //???
        BOOST_ASSERT(!self->stack_.empty());
        self->stack_.top().first = std::string(data,length);
        return 0;
    case JSON_ARRAY_BEGIN:
        val = json::array();
        self->stack_.push( make_pair(std::string(), val) );
        return 0;
    case JSON_OBJECT_BEGIN:
        val = json::object();
        self->stack_.push( make_pair(std::string(), val) );
        return 0;

    case JSON_ARRAY_END:
    case JSON_OBJECT_END:
        swap(val, self->stack_.top().second);
        self->stack_.pop();
        break;
    case JSON_INT:
        val = static_cast<json::int_t>( atoi(data) );
        break;
    case JSON_FLOAT:
        val = static_cast<double>(atof(data));
        break;
    case JSON_STRING:
        val = json::string(data, length);
        break;
    case JSON_TRUE:
        val = true;
        break;
    case JSON_FALSE:
        val = false;
        break;
    case JSON_NULL:
        val = json::none;
        break;

    default:
        BOOST_ASSERT(false);
        return 0;
    }

    if (self->stack_.empty())
    {
        swap(result.second, self->jsv_); // DONE
    }
    else
    {
        swap(result.first, self->stack_.top().first);
        jsvalue_add add(result);
        auto & tmp = self->stack_.top().second;
        apply_visitor(add, tmp);
        //apply_visitor(add, self->stack_.top().second);
    }

    return 0;
}

bool _decode(json::variant& result, char const* beg, char const* end)
{
    parser parse(result);
    return parse(beg, end);
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

    void operator()(json::int_t i) const
    {
        char tmp[24];
        int len = snprintf(tmp,sizeof(tmp), "%ld", i);
        json_print_raw(jp_.get(), JSON_INT, tmp, len);
    }

    void operator()(double s) const
    {
        char tmp[24];
        int len = snprintf(tmp,sizeof(tmp), "%.5f", s);
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

    void operator()(const object& jo) const //{ (*p_)(s); }
    {
        json_print_raw(jp_.get(), JSON_OBJECT_BEGIN, NULL, 0);
        for (object::const_iterator i = jo.begin();
                i != jo.end(); ++i)
        {
            json_print_raw(jp_.get(), JSON_KEY, i->first.data(), i->first.size());
            apply_visitor(*this, i->second);
        }
        json_print_raw(jp_.get(), JSON_OBJECT_END, NULL, 0);
    }

    void operator()(const json::variant& jv) const
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

std::string encode(const array& jv)
{
    std::ostringstream oss;
    printer p(oss);
    p(jv);
    return oss.str();
}

std::string encode(const object& jv)
{
    std::ostringstream oss;
    printer p(oss);
    p(jv);
    return oss.str();
}

std::string encode(const json::variant& jv)
{
    std::ostringstream oss;
    printer p(oss);
    p(jv);
    return oss.str();
}

} // namespace json

// template <typename T>
// struct StatSaver
// {
//     T value;
//     T *addr;
//     StatSaver(T& obj)
//     {
//         addr = &obj;
//         value = obj;
//     }
//     ~StatSaver() { *addr = value; }
// };

