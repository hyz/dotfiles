#ifndef _MYERROR_H_
#define _MYERROR_H_

#include <string>
#include <ostream>
#include <fstream>
#include <utility>
#include <boost/tuple/tuple.hpp>
#include <boost/exception/error_info.hpp>
#include <boost/exception/get_error_info.hpp>
#include <boost/exception/diagnostic_information.hpp>
#include <boost/exception/all.hpp>
#include <boost/system/error_code.hpp>

// #define my_error_N(e) THROW_EX(e)
// #define my_error_0() (json::object()("errno", 0))

struct my_exception
        : public virtual std::exception
        , public virtual boost::exception
{
    boost::system::error_code ec_;

    explicit my_exception(const boost::system::error_code & ec);
    explicit my_exception(int n, const boost::system::error_category & cat);

    const boost::system::error_code & error_code() const { return ec_; }

    ~my_exception() throw ()
        {}
    char const * what() const throw() { return ec_.category().name(); }
};

inline std::ostream& operator<<(std::ostream& out, const my_exception& e)
    { return out << e.error_code(); }

// const boost::system::error_category & myerror_category();
struct Error_category : boost::system::error_category
{
    struct code_string { int first; const char * second; };

    std::string message( int c ) const;
    const char * name() const BOOST_NOEXCEPT { return node_->name; }

    bool operator==(const Error_category & rhs) const { return this==&rhs; }

    template <typename T> static T & inst()
    {
        static T inst_;
        return inst_;
    }

protected:
    Error_category();

    template <typename This>
    static void Init(This *self, code_string *beg, code_string *end, const char* name="");

private:
    struct Node
    {
        const char* name;
        code_string *begin, *end;
        Node *next;
        Node(const char* nm, Node *nd, code_string *b, code_string *e);
    };

    Node *node_;
};

template <typename This>
void Error_category::Init(This *self, code_string *beg, code_string *end, const char* name)
{
    static Node nd(name, self->node_, beg, end);
    self->node_ = &nd;
}

#define Error_Category myerror_category
#define MYTHROW(En,ECAT) mythrow(__LINE__,__FILE__, boost::system::error_code(En, Error_category::inst<ECAT>()))
#define THROW_EX(En) mythrow(__LINE__,__FILE__, boost::system::error_code(En, Error_category::inst<Error_Category>()))

typedef boost::tuple<boost::errinfo_at_line,boost::errinfo_file_name> errinfo_pos;
// typedef boost::errinfo_errno errinfo_ec;
// typedef boost::error_info<struct tag_error_info_sysec, boost::system::error_code> errinfo_ec;

void mythrow(int ln, const char* fn, const boost::system::error_code & ec);

// << errinfo_ec(boost::system::error_code(c, myerror_category()))

enum myerror_category_en
{
    EN_HTTPRequestPath_NotFound = 115,
    EN_HTTPParam_NotFound,
    EN_HTTPHeader_NotFound,

    EN_JsonName_NotFound = 125,
    EN_JsonValue_TypeError,

    EN_DBConnect_Fail = 135,
    EN_SQL_Fail,

    EN_Server_InternalError = 500,
};

struct myerror_category : Error_category
{
    myerror_category();
};

namespace boost { namespace system {
    template<> struct is_error_code_enum<myerror_category_en>
    {
        static const bool value = true;
    };
} }

//inline boost::system::error_code make_error_code(myerror_category_en e)
//{ return boost::system::error_code(e, error_category::inst<myerror_category>()); }

#endif
