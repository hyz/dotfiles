#include "apple_push.h"

boost::asio::ssl::context& ios_push_client::wrapssl::init_context(boost::asio::ssl::context& ctx, 
        const config& conf)
{
    LOG_I << "channel:"<< conf.ch<<" certfile:"<<conf.cfn<<" keyfile:"<<conf.kfn;
    ctx.set_password_callback(boost::bind(&ios_push_client::config::getpwd, &conf, _1, _2));

    ctx.use_certificate_file(conf.cfn, boost::asio::ssl::context::pem);
    ctx.use_private_key_file(conf.kfn, boost::asio::ssl::context::pem);

    return ctx;
}

std::string ios_push_client::unhex(const std::string & hs) 
{
    std::string buf;
    const char* s = hs.data();
    for (unsigned int x = 0; x+1 < hs.length(); x += 2)
    {   
        char h = s[x];
        char l = s[x+1];
        h -= (h <= '9' ? '0': 'a'-10);
        l -= (l <= '9' ? '0': 'a'-10);
        buf.push_back( char((h << 4) | l) );
    }
    return buf;
}

template <typename Int> std::string& ios_push_client::pkint(std::string & pkg, Int x)
{
    Int i;
    switch (sizeof(Int)) {
        case sizeof(uint32_t): i = htonl(x); break;
        case sizeof(uint16_t): i = htons(x); break;
        default: i = x; break;
    }
    char *pc = (char*)&i;
    pkg.insert(pkg.end(), &pc[0], &pc[sizeof(Int)]);
    return pkg;
}

std::string& ios_push_client::pkstr(std::string & pkg, const std::string & s)
{
    uint16_t len = s.length();
    len = htons(len);
    char *pc = (char*)&len;
    pkg.insert(pkg.end(), &pc[0], &pc[sizeof(uint16_t)]);
    return (pkg += s);
}

std::string ios_push_client::apack(std::string const & tok, int id, std::string const & aps)
{
    BOOST_ASSERT(tok.length() == 64);

    std::string buf; //(len, 0);
    if ( 64 != tok.length() ) {
        LOG_I<<"devtok length invalid "<< tok<<" channel:"<<conf_.ch;
        return buf;
    }

    pkstr(pkstr(pkint(pkint(pkint( buf
            , uint8_t(1)) // command
            , uint32_t(id)) // /* provider preference ordered ID */
            , uint32_t(time(NULL)+(60*60*6))) // /* expiry date network order */
            , unhex(tok)) // binary 32bytes device token
            , aps)
            ;
    return buf;
}

void ios_push_client::write_msg()
{
    if ( stage_ != stage::idle ) { 
        LOG_I<< "ch" << conf_.ch<< "stage" << stage_;
        return; 
    }

    const char* palert = "{\"aps\":{\"alert\":\"%1%\",\"badge\":%2%,\"sound\":\"default\"}}";
    // UInt id;
    std::string devtok;
    push_mgr::s_push_msg m;
    UInt num = 1;

    if ( !push_mgr::instance().get_one_msg( conf_.ch, devtok, m, num) ) { return; }

    LOG_I <<"channel:"<< conf_.ch << m << num;
    stage_ = stage::writing;
    buf_= apack( devtok, m.msgid, str( boost::format( palert ) %m.content %num));
    boost::asio::async_write(ssl_->socket()
            , boost::asio::buffer(buf_)
            , boost::bind(&this_type::handle_write, this, 
                boost::asio::placeholders::error, m));
}

void ios_push_mgr::init(boost::asio::io_service& io_service, const boost::property_tree::ptree& ini)
{
    boost::filesystem::path path( ini.get<std::string>("path","/etc/yxim") );
    std::string cf = ini.get<std::string>("certfile", "PushChatCert.pem");
    std::string kf = ini.get<std::string>("keyfile", "PushChatKey.pem");

    ios_push_client::config enter_cfg;
    std::string enter_host = ini.get<std::string>("enter_host", "gateway.push.apple.com");
    std::string enter_port = ini.get<std::string>("enter_port", "2195");

    std::string enter_bundle = "enterprise";
    enter_cfg.cfn = (path / enter_bundle / cf).string();
    enter_cfg.kfn = (path / enter_bundle / kf).string();
    enter_cfg.passwd = ini.get<std::string>("enter_password","abc123");
    enter_cfg.ch = push_mgr::IOS_ENTERPRISE;
    // enter_cfg.ep = resolv( io_service, enter_host, enter_port );
    enter_cfg.host_port = make_pair( enter_host, enter_port );

    ios_push_mgr::enter_bid = ini.get<std::string>("enter_bid", ios_push_mgr::enter_bid);

    LOG_I << enter_host << enter_port << path << enter_cfg.cfn << enter_cfg.kfn 
        << enter_cfg.passwd << ios_push_mgr::enter_bid;

    ios_push_client::config rel_cfg;
    std::string rel_host = ini.get<std::string>("rel_host", "gateway.push.apple.com");
    std::string rel_port = ini.get<std::string>("rel_port", "2195");

    std::string rel_bundle = "appstore";
#ifdef IOS_DEBUG
    rel_bundle = "sandbox";
    rel_host = ini.get<std::string>("rel_debug_host", "gateway.sandbox.push.apple.com");
    LOG << "setting ios debug.....";
#endif

    rel_cfg.cfn = (path / rel_bundle / cf).string();
    rel_cfg.kfn = (path / rel_bundle / kf).string();
    rel_cfg.passwd = ini.get<std::string>("rel_password","abc123");
    rel_cfg.ch = push_mgr::IOS_RELEASE;
    // rel_cfg.ep = resolv( io_service, rel_host, rel_port );
    rel_cfg.host_port = make_pair( rel_host, rel_port );

    ios_push_mgr::rel_bid = ini.get<std::string>("rel_bid", ios_push_mgr::rel_bid);

    LOG_I << rel_host << rel_port << path << rel_cfg.cfn << rel_cfg.kfn 
        << rel_cfg.passwd << ios_push_mgr::rel_bid;

    ios_push_client::config et_cfg;
    std::string et_host = ini.get<std::string>("et_host", "gateway.push.apple.com");
    std::string et_port = ini.get<std::string>("et_port", "2195");

    std::string et_bundle = "et";
    et_cfg.cfn = ( path / et_bundle / cf ).string();
    et_cfg.kfn = ( path / et_bundle / kf ).string();
    et_cfg.passwd = ini.get<std::string>("et_password","abc123");
    et_cfg.ch = push_mgr::IOS_ET;
    // et_cfg.ep = resolv( io_service, enter_host, enter_port );
    et_cfg.host_port = make_pair( et_host, et_port );

    ios_push_mgr::et_bid = ini.get<std::string>("et_bid", ios_push_mgr::et_bid);

    LOG_I << et_host << et_port << path << et_cfg.cfn << et_cfg.kfn 
        << et_cfg.passwd << ios_push_mgr::et_bid;

    ios_push_mgr::inst( &io_service, &enter_cfg, &rel_cfg, &et_cfg );
}

boost::asio::ip::tcp::endpoint ios_push_mgr::resolv(boost::asio::io_service & io_service, 
        const std::string& h, const std::string& p)
{
    using boost::asio::ip::tcp;
    tcp::resolver resolver(io_service);
    tcp::resolver::query query(h, p);
    tcp::resolver::iterator i = resolver.resolve(query);
    return *i;
}

ios_push_mgr& ios_push_mgr::inst( boost::asio::io_service* io_service, 
        const ios_push_client::config* enter, const ios_push_client::config* rel, 
        const ios_push_client::config* et )
{
    static ios_push_mgr instance( *io_service, *enter, *rel, *et );

    return instance;
}

std::string ios_push_mgr::enter_bid = "com.kkli.KKMoon";
std::string ios_push_mgr::rel_bid = "com.kklink.newMyMoon";
std::string ios_push_mgr::et_bid = "com.kklink.et";
