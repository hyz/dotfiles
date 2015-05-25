#include "myconfig.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <iostream>
#include <boost/asio/streambuf.hpp>
#include <boost/thread/mutex.hpp>

#include "log.h"

using namespace std;
using namespace boost;

void printlog(const char*fn, int line, const char*fmt, ...)
{
    int num;
    va_list ap;
    char log_arry[200] ={0};
    num = sprintf(log_arry, "[%s:%d] ", fn, line);
    va_start(ap, fmt);
    vsprintf(log_arry+num, fmt, ap);
    va_end(ap);
    printf("%s", log_arry);
}

ostream& operator<<(ostream& out, const vector<string>& v)
{
    BOOST_FOREACH(const string& s, v)
        out << s << " ";
    return out;
}

std::ostream& operator<<(std::ostream& out, const std::pair<std::string,std::string>& p)
{
    return out << p.first << "," << p.second;
}

namespace logging {

std::ostream *ostream_ptr_ = 0;
static boost::asio::streambuf *logsbuf_ = 0;

//boost::reference_wrapper<std::ostream> clog_(std::clog);
// static boost::mutex mutex_;
// static bool syslog_ = false;
// static std::ostream* clog_ = &std::clog;

ostream_fwd::ostream_fwd(int pri, const char* fn, int ln)
{
    pri_ = pri;
    ln_ = ln;
    fn_ = fn;
    // mutex_.lock();
}

ostream_fwd::~ostream_fwd()
{
    // std::clog << static_cast<void*>(std::clog.rdbuf()) <<":"<< static_cast<void*>(&logsbuf_) << std::endl;
    if (logsbuf_)
    {
        asio::streambuf::const_buffers_type bufs = logsbuf_->data();
        asio::streambuf::const_buffers_type::const_iterator i = bufs.begin();
        if (i != bufs.end())
        {
            size_t n = 0;
            for (; i != bufs.end(); ++i)
            {
                const void *p = asio::detail::buffer_cast_helper(*i);
                size_t len = asio::detail::buffer_size_helper(*i);

                ::syslog(pri_, "%.*s #%s:%d", (int)len, (char*)p, fn_, ln_);
                n += len;
            }
            logsbuf_->consume(n);
        }
    }
    else
    {
        struct tm tm;

        time_t ss = time(0);
        localtime_r(&ss, &tm);

        char ts[32] = {0};
        strftime(ts,sizeof(ts)-1, "%T", &tm);

        (*ostream_ptr_) <<" #"<< fn_ <<":"<<ln_<<" "<< ts <<"\n";
    }

    // mutex_.unlock();
}

void setup(std::ostream * outs, int opt, int facility)
{
    if (outs)
    {
        logsbuf_ = 0;
        ostream_ptr_ = outs;
    }
    else
    {
        static boost::asio::streambuf logsbuf_s;
        static std::ostream slog_s(&logsbuf_s);
        openlog(0, opt, facility);
        logsbuf_ = &logsbuf_s;
        ostream_ptr_ = &slog_s;
    }

    // static asio::streambuf outbuf; cout.rdbuf(&outbuf);
    //boost::reference_wrapper<std::ostream> 
    //clog_ = boost::ref(slog_);

    // syslog_ = true;
    // ostream_fwd::clog_ = &slog;
}

} // namespace

std::ostream& operator<<(std::ostream& out, boost::asio::const_buffers_1 const & bufs)
{
    BOOST_FOREACH(boost::asio::const_buffer const& buf, bufs)
        out << buf;
    return out;
}

