#include <arpa/inet.h>
#include <sstream>
#include <sstream>
#include <boost/scope_exit.hpp>
#include "service.h"
#include "message.h"
#include "log.h"
#include "msgh.h"

enum { Rspcode_invalid_cmd=902 };

inline bool success(int errc) // (std::pair<int,int>const & p)
{
    return errc==0; //p.first==0 && p.second==0;
}

void msgh_main(io_context& ctx, int cmd, json::object const& jv)
{
    auto res = service_mgr::instance().dispatch(ctx, jv, cmd);
    if (success(res))
        return;

    if (!ctx.is_closed())
        ctx.close(__LINE__,__FILE__);
}

void msgh_ims_default(io_context& ctx, int cmd, json::object const& jv)
{
    enum { IMC_cmd_99=99 };

    if (cmd != IMC_cmd_99)
    {
        json::object rsp;
        json::insert(rsp)
            ("cmd", cmd) ("error", Rspcode_invalid_cmd);
        ctx.send(rsp);
        ctx.close(__LINE__,__FILE__);
        return;
    }

    if (success(service_mgr::instance().dispatch(ctx, jv, cmd)))
    {
        ctx.new_handler(&msgh_main);
    }
    else if (!ctx.is_closed())
    {
        ctx.close(__LINE__,__FILE__);
    }
}

static std::string authcode_;

void msgh_bs_default(io_context& ctx, int cmd, json::object const& jv)
{
    BOOST_SCOPE_EXIT(&ctx) {
        ctx.close(__LINE__,__FILE__);
    } BOOST_SCOPE_EXIT_END

    if (!authcode_.empty())
    {
        auto opt = json::as<std::string>(jv,"auth");
        if (!opt || opt.value() != authcode_)
        {
            json::object rsp;
            json::insert(rsp) ("cmd", cmd) ("error", Rspcode_invalid_cmd);
            ctx.send(rsp);
            return;
        }
    }

    msgh_main(ctx, cmd, jv);
}

void msgh_bs_set_authcode(std::string const& ac)
{
    authcode_ = ac;
}

