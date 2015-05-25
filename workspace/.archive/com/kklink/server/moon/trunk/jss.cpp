#include "myconfig.h"
#include <boost/range.hpp>
#include <iostream>
#include <sstream>
#include "myerror.h"
#include "jss.h"

using namespace std;
using namespace boost;

namespace json {

array::array() {}

object::iterator object::insert(const object::value_type& p)
{
    iterator i = find(p.first);
    if (i == end())
    {
        i = this->base_type::insert(end(), p);
    }
    else
    {
        i->second = p.second;
    }
    return i;
}

void object::rename(const std::string & fr, const std::string & to)
{
    if (find(to) == end())
    {
        iterator i = find(fr);
        if (i != end())
        {
            i->first = to;
        }
    }
}

void object::erase(const std::string& k)
{
    iterator i = find(k);
    if (i != end())
        erase(i);
}

object::iterator object::find(const std::string& k)
{
    iterator i = begin();
    for (; i != end(); ++i)
        if (i->first == k)
            break;
    return i;
}

object object::operator+(const object & rhs) const
{
    object ret = *this;
    return ret += rhs;
}

object & object::operator+=(const object & rhs)
{
    for (const_iterator i = rhs.begin(); i != rhs.end(); ++i)
        this->insert(*i);
    return *this;
}

double _convert(variant & var, double*)
{
    double* d = boost::get<double>(&var);
    if (!d)
    {
        return double(_convert<int>(var, static_cast<int*>(0)));
    }
    return *d;
}

bool parser::decode(std::istream& in)
{
    BOOST_ASSERT(stack_.empty());
    std::string tmp(1024,'\0');
    do
    {
        if (!in)
            return false;

        int len = in.readsome(const_cast<char*>(tmp.data()), tmp.size());
        if (len > 0)
        {
            if (json_parser_string(&jp_, tmp.data(), len, NULL))
                return false;
        }
    }
    while (!stack_.empty());

    return true;
}

int parser::_parser_callback(void *userdata, int type, const char *data, uint32_t length)
{
    parser* self = static_cast<parser*>(userdata);

    std::pair<std::string,variant> result;
    variant& val = result.second;
    switch (type)
    {
    case JSON_KEY: //???
        BOOST_ASSERT(!self->stack_.empty());
        self->stack_.top().first = std::string(data,length);
        return 0;
    case JSON_ARRAY_BEGIN:
        val = array();
        self->stack_.push( make_pair(std::string(), val) );
        return 0;
    case JSON_OBJECT_BEGIN:
        val = object();
        self->stack_.push( make_pair(std::string(), val) );
        return 0;

    case JSON_ARRAY_END:
    case JSON_OBJECT_END:
        swap(val, self->stack_.top().second);
        self->stack_.pop();
        break;
    case JSON_INT:
        val = (int)atoi(data);
        break;
    case JSON_FLOAT:
        val = (float)atof(data);
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
        apply_visitor(add, self->stack_.top().second);
    }

    return 0;
}

variant & decode(variant& result, std::istream& in)
{
    parser parser(result);
    if (!parser.decode(in))
    {
        THROW_EX(EN_JsonName_NotFound);
    }
    return result;
}

variant & decode(variant& result, const std::string& s)
{
    istringstream in(s);
    return decode(result, in);
}

object decode(std::istream& in)
{
    variant var;
    decode(var, in);
    return boost::get<object>(var);
}

object decode(const std::string& s)
{
    std::istringstream in(s);
    return decode(in);
}

std::ostream& encode(std::ostream& out, const array& jv)
{
    printer p(out);
    p(jv);
    return out;
}

std::ostream& encode(std::ostream& out, const object& jv)
{
    printer p(out);
    p(jv);
    return out;
}

std::ostream& encode(std::ostream& out, const variant& jv)
{
    printer p(out);
    p(jv);
    return out;
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

