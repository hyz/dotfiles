#ifndef __initialze_h__
#define __initialze_h__

#include <boost/noncopyable.hpp>

#include <string>

#include "dbc.h"

struct initializer : boost::noncopyable
{
    //sql::initializer dbc_;

    initializer( const std::string& fn="moon.conf" );
    ~initializer() {}

};

#endif
