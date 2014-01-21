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

struct mylogs // : std::string , io::stream<string_sink>
{
    mylogs() // : io::stream<string_sink>(*this)
    {
        ntotal_ = 0;
        lins_.resize(1);
    }
    // ~mylogs() { commit(); }

    template <typename T> mylogs & operator<<(T const & l)
    {
        lins_.push_back( boost::lexical_cast<std::string>(l) );
        ntotal_ += lins_.back().size() + 1;
        return *this;
    }

    void commit(int linum, const char *fn, const char *fname)
    {
        std::string tmps;
        tmps.reserve((lins_.front().size() + ntotal_ + 4) & 0x0ffffc);
        std::cout << tmps.capacity() << " capacity\n";

        for (auto i = lins_.begin(); i != lins_.end(); ++i)
        {
            tmps += *i;
            tmps += ' ';
        }
        std::cout << tmps.size() << " size\n";
        BOOST_ASSERT(tmps.size() < tmps.capacity());

        char tmpa[128];
        snprintf(tmpa, sizeof(tmpa), "#%d:%s\n", linum, fn);

        // syslog(, "%s%s", tmps.c_str(), tmpa);
        std::cout << tmps << tmpa;

        ntotal_ = 0;
        lins_.resize(1);
    }

    void tag_(std::string const & t)
    {
        std::string & ts = lins_.front();
        if (!ts.empty())
            ts += ' ';
        ts += t;
    }

    mutable bool xbool_, _;
    unsigned short ntotal_;
    std::vector<std::string> lins_;

    bool yes1() const
    {
        bool b = xbool_;
        xbool_ = 0;
        return b;
    }
};

#define LOG for(mylogs l; (l).yes1(); (l).commit(__LINE__,__FUNCTION__,__FILE__)) (l)
#define LOGR(l) for(int y=1; y--; (l).commit(__LINE__,__FUNCTION__,__FILE__)) (l)

struct X {};

std::ostream & operator<<(std::ostream & out, X const &)
{
    return out << "12345X";
}

int main()
{
    LOG << sizeof(mylogs);
    LOG << sizeof(std::string);
    // LOG << sizeof(io::stream<string_sink>);
    LOG << "";
    // LOG << std::endl;
    LOG << "";
    LOG << sizeof(std::cout) <<" "<< sizeof(std::ostream);

    mylogs logs;
    logs.tag_("hello");
    logs.tag_("world");

    LOGR(logs) << "f";
    LOGR(logs) << "fo";
    LOGR(logs) << "foo";
    LOGR(logs) << "foo bar";

    LOGR(logs) << X();
}

