#include "util.h"
#include "log.h"
#include "mycurl.h"
#include "interaction.h"

#include <boost/format.hpp>

void interaction( UInt to, UInt fr, UInt type, const std::string& msg )
{
    const char* url = "http://192.168.10.245/im/interaction?from=%1%&to=%2%&type=%3%&msg=%4%";
    auto rp = php_get( ( boost::format( url ) %fr %to %type % urlencode( msg ) ).str() );
    LOG_I<< rp;
}
