// (C) Copyright 2008 CodeRage, LLC (turkanis at coderage dot com)
// (C) Copyright 2005-2007 Jonathan Turkanis
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt.)

// See http://www.boost.org/libs/iostreams for documentation.

#include <syslog.h>
#include <cassert>
#include <vector>
#include <string>
#include <boost/lexical_cast.hpp>

#include <iostream>

struct syslogger // io::stream<string_sink>
{
    syslogger() // : io::stream<string_sink>(*this)
    {
        bytes_count_ = 0;
        lins_.resize(1);
    }

    template <typename T> syslogger & operator<<(T const & l)
    {
        lins_.push_back( boost::lexical_cast<std::string>(l) );
        bytes_count_ += lins_.back().size() + 1;
        return *this;
    }

    void commit(bool sink_syslog, int linum, const char *fn, const char *fname)
    {
        std::string tmps;
        tmps.reserve((lins_.front().size() + bytes_count_ + 4) & 0x0ffffc);
        std::clog << tmps.capacity() << " capacity\n";

        for (auto i = lins_.begin(); i != lins_.end(); ++i)
        {
            tmps += *i;
            tmps += ' ';
        }
        std::clog << tmps.size() << " size\n";
        BOOST_ASSERT(tmps.size() < tmps.capacity());

        // ::syslog(0, "%.*s #%s:%d", (int)tmps.size(), tmps.c_str(), fn, linum);
        std::clog << tmps << "#"<<linum<<":"<<fn;

        lins_.resize(1);
        bytes_count_ = 0;
    }

    template <typename T>
    void tag_s(T const & t)
    {
        std::string & ts = lins_.front();
        if (!ts.empty())
            ts += ' ';
        ts += boost::lexical_cast<std::string>(t);
    }

    unsigned int bytes_count_;
    std::vector<std::string> lins_;
};

template <typename T,int Y> struct flipbool_helper : T
{
    mutable int xbool_;

    template <typename ...A> flipbool_helper(A... a) : T(a...) { xbool_=Y; }
    flipbool_helper() { xbool_=Y; }

    bool get_and_flip() const
    {
        bool b = xbool_;
        xbool_ = !b;
        return b;
    }
};

#define LOG for(flipbool_helper<syslogger,1> l; (l).get_and_flip(); (l).commit(sink_syslog_, __LINE__,__FUNCTION__,__FILE__)) (l)
#define LOG_L(l) for(int y=1; y--; (l).commit(sink_syslog_, __LINE__,__FUNCTION__,__FILE__)) (l)

struct X {};

std::ostream & operator<<(std::ostream & out, X const &)
{
    return out << "12345X";
}

int main()
{
    LOG << sizeof(syslogger);
    LOG << sizeof(std::string);
    // LOG << sizeof(io::stream<string_sink>);
    LOG << "";
    // LOG << std::endl;
    LOG << "";
    LOG << sizeof(std::clog) <<" "<< sizeof(std::ostream);

    syslogger slog;
    slog.tag_s("hello");
    slog.tag_s("world");

    LOG_L(slog) << "f";
    LOG_L(slog) << "fo";
    LOG_L(slog) << "foo";
    LOG_L(slog) << "foo bar";

    LOG_L(slog) << X();
}

