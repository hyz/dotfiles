#ifndef _LOG_H_
#define _LOG_H_

#include <syslog.h>
#include <set>
#include <vector>
#include <ostream>
#include <boost/ref.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/foreach.hpp>

void printlog(const char*fn, int line, const char*fmt, ...);
#define LOG(fmt, ...) printlog(__FILE__,__LINE__,fmt, ##__VA_ARGS__)

template <typename T>
std::ostream& operator<<(std::ostream& out, const std::vector<T>& v)
{
    BOOST_FOREACH(T const& x, v)
        out << x << " ";
    return out;
}

template <typename T>
std::ostream& operator<<(std::ostream& out, const std::set<T>& v)
{
    BOOST_FOREACH(T const& x, v)
        out << x << " ";
    return out;
}

std::ostream& operator<<(std::ostream& out, const std::pair<std::string,std::string>& p);

inline std::ostream& operator<<(std::ostream& out, boost::asio::const_buffer const & buf)
{
    out.write(boost::asio::buffer_cast<char const*>(buf), boost::asio::buffer_size(buf));
    return out;
}
std::ostream& operator<<(std::ostream& out, boost::asio::const_buffers_1 const & bufs);

namespace logging {

extern std::ostream * ostream_ptr_;

struct ostream_fwd
{
    explicit ostream_fwd(int pri, const char* fn, int ln);

    ~ostream_fwd();

    const ostream_fwd& operator<<(std::ostream& (*pf)(std::ostream&)) const
    {
        *ostream_ptr_ << pf;
        return *this;
    }

    template <typename T> const ostream_fwd& operator<<(const T& x) const
    {
        *ostream_ptr_ << x;
        return *this;
    }

    unsigned short pri_;
    unsigned short ln_;
    const char* fn_;
};

void setup(std::ostream* outs, int opt, int facility);

// inline ostream_fwd debug()   { return ostream_fwd(); }
// inline ostream_fwd info()    { return ostream_fwd(); }
// inline ostream_fwd warning() { return ostream_fwd(); }
// inline ostream_fwd error()   { return ostream_fwd(); }

} // namespace

//#define LOG_D (logging::ostream_fwd(LOG_USER|LOG_DEBUG, __FUNCTION__,__LINE__))
#define LOG_I (logging::ostream_fwd(LOG_USER|LOG_INFO, __FUNCTION__,__LINE__))
#define LOG_W (logging::ostream_fwd(LOG_USER|LOG_WARNING, __FUNCTION__,__LINE__))
#define LOG_E (logging::ostream_fwd(LOG_USER|LOG_ERR, __FUNCTION__,__LINE__))


#endif

