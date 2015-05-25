#include <boost/timer/timer.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ip/udp.hpp>
#include <boost/format.hpp>
#include "util.h"
#include "json.h"
#include "globals.h"
#include "service.h"
#include "myerror.h"
#include "log.h"

using namespace std;

enum class error_code {
    ok = 0,

    version_incompactable = 900,
    illformat = 901,
    invalid_command = 902,

    value_not_found = 905,
    value_format = 906,

    unknown = 920
};


template <typename C> inline Pfun get_fun(int idx, C const & funcs_)
{
    if (idx < 0 || idx > funcs_.size())
        return Pfun();
    return funcs_[idx];
}

int service_mgr::dispatch(io_context& ctx, json::object const& in, int cmd)
{
    AUTO_CPU_TIMER(str(boost::format("service_mgr:dispatch(%1%)") % cmd));
    error_code errc = error_code::ok;

    try {
        auto funct = get_fun(cmd, this->funcs_);
        if ( !funct ) {
            LOG << "INVALID_CMD:" << cmd;
            errc = error_code::invalid_command;
            goto Pos_ret_;
        }

        json::object out_body;
        json::object tmp_in_body;
        json::object const& in_body = json::as<json::object>(in,"body").value_or(tmp_in_body);

        int result = funct(ctx, in_body, out_body );
        if (!ctx.is_closed()) {
            out_body.emplace("result", result);
            {
                boost::optional<UInt> tag = json::as<UInt>(in_body,"tag");
                if (tag) { // (boost::optional<UInt> t = json::as<UInt>(in_body,"tag"))
                    out_body.emplace("tag", tag.value()) ;
                }
            }

            json::object out;
            json::insert(out)
                ("cmd", cmd) ("error", 0) ("body", out_body)
                ;

            ctx.send(out);

        } else {
            LOG << cmd << result << "is closed";
        }
        return 0;

    } catch (json::error_value const& e) {
        LOG << e;
        errc = error_code::value_format;
    } catch (json::error_key const& e) {
        LOG << e;
        errc = error_code::value_not_found;
    } catch (json::error const& e) {
        LOG << e;
        errc = error_code::value_not_found;

    } catch (myerror const& e) {
        LOG << e;
        errc = error_code::unknown;
    } catch (boost::exception const& e) {
        LOG << e;
        errc = error_code::unknown;
    } catch (std::exception const& e) {
        LOG << "=except:" << e.what();
        errc = error_code::unknown;
    }

Pos_ret_:
    if (ctx.is_closed()) {
        LOG << cmd << int(errc) << "is closed 2";
        return int(errc);
    }
    LOG << cmd << "ret" << int(errc);

    {
        json::object out;
        json::insert(out)
            ("cmd", cmd) ("error", errc)
            ;
        ctx.send(out);
    }
    ctx.close(__LINE__,__FILE__);

    return int(errc);
}

void service_mgr::regist_service(int cmd, Pfun func) 
{
    if (funcs_.size() <= size_t(cmd))
        funcs_.resize(cmd+1);
    funcs_[cmd] = func;
}

static std::string primary_ipa_v4()
{
    namespace ip = boost::asio::ip;

    try {
        boost::asio::io_service & io_s = globals::instance().io_service;

        ip::udp::resolver resolver(io_s);
        ip::udp::resolver::query query(ip::udp::v4(), "8.8.8.8", "");
        ip::udp::resolver::iterator iter = resolver.resolve(query);
        ip::udp::endpoint ep = *iter;

        ip::udp::socket socket(io_s);
        socket.connect(ep);
        ip::udp::endpoint endp = socket.local_endpoint();
        return endp.address().to_v4().to_string();

    } catch (std::exception const& e) {
        LOG << "Exception" << e.what();
    }

    return std::string();
}

static std::string resolv(std::string const& h)
{
    namespace ip = boost::asio::ip;

    auto & io_s = globals::instance().io_service;

    ip::tcp::resolver resolver(io_s);
    ip::tcp::resolver::query query(h, "");
    ip::tcp::endpoint endp = *resolver.resolve(query);
    return endp.address().to_v4().to_string();
}

server_setting::server_setting(const boost::property_tree::ptree& ini)
{
    auto ims = ini.get_child("ims");
    ims_port = ims.get<int>("port", default_ims_port);

    std::string host = ims.get<string>("host", std::string());
    std::string proxy_host = ims.get<std::string>("proxy_host", std::string());

    if (proxy_host.empty())
    {
        if (host.empty() || host == "0") {
            ims_ipv4 = primary_ipa_v4();
            if (ims_ipv4.empty()) {
                char const* h = "ims.kklink.com";
                ims_ipv4 = resolv(h);
                LOG << h << ims_ipv4;
            }

        } else {
            ims_ipv4 = resolv(host);
        }
    }
    else
    {
        ims_ipv4 = resolv(proxy_host);
    }
    LOG << ims_ipv4 <<"host"<< host <<"proxy"<< proxy_host;

    auto bs = ini.get_child("bs");
    bs_port = ini.get<int>("port", default_bs_port);

    auto admin = ini.get_child("admin");
    conf_port = admin.get<int>("port", default_admin_port);
}

service_mgr::service_mgr(server_setting const & setting)
    : net_(setting)
{
    bs_initialize(*this);
    ims_initialize(*this);

    //((void)bs_reg);
    //((void)imc_reg);
}

const char* public_msg_setting::old_id_key = "public_message_bid";

UInt service_mgr::max_public_msg_id()
{
    if ( !pms_.isload ) {
        get_public_msg_setting();
    }

    return pms_.new_id;
}

UInt service_mgr::old_public_msg_id()
{
    if ( !pms_.isload ) {
        get_public_msg_setting();
    }

    return pms_.old_id;
}

void service_mgr::get_public_msg_setting()
{
    sql::datas datas("select max(id) from public_message");
    if ( sql::datas::row_type row = datas.next() ) {
        pms_.new_id = boost::lexical_cast<UInt>( row.at(0,"0") );
    }

    auto reply = redis::command( "GET",  public_msg_setting::old_id_key );
    if (reply && reply->type == REDIS_REPLY_STRING) {
        pms_.old_id = boost::lexical_cast<UInt>(reply->str);
    }


    LOG_I <<"max(id) of public_message is:"<< pms_.new_id<<" id of public_message has been read is:"<< pms_.old_id;

    pms_.isload = true;
}

