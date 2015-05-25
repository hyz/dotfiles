#ifndef __SERVICE_H__
#define __SERVICE_H__

#include <boost/property_tree/ptree.hpp>
#include <boost/function.hpp>

#include <vector>
#include <utility>

#include "singleton.h"
#include "ioctx.h"
#include "json.h"
#include "dbc.h"

typedef boost::function<int (io_context&, json::object const&, json::object& )> Pfun;

enum ERRORS{
    ILLEGAL_CMD=901, INVALID_CMD, 
    ILLEGAL_SID=905, INVALID_UID, 
    INVALID_AUTH=911, INVALID_APPID
};

struct server_setting
{
    int ims_port;
    // std::string ims_host;
    std::string ims_ipv4;

    int bs_port;
    int conf_port;

    server_setting(const boost::property_tree::ptree& ini);
};

struct public_msg_setting
{
    public_msg_setting(): new_id(0), old_id(0), isload(false){}
    UInt new_id;
    UInt old_id;
    bool isload;
    static const char* old_id_key;
};

struct service_mgr : singleton<service_mgr>
{
    service_mgr(server_setting const & setting);

public:
    int dispatch(io_context& ctx, json::object const&, int);

    void regist_service(int cmd, Pfun func);

    int imc_port() const { return net_.ims_port; }

    std::string imc_ip() const { return net_.ims_ipv4; }

    UInt max_public_msg_id();
    UInt old_public_msg_id();
    void set_max_public_msg_id( UInt id ) { if ( pms_.isload ) { pms_.new_id = id ; } }
    void set_old_public_msg_id( UInt id ) 
    { 
        BOOST_ASSERT(pms_.isload);
        // if ( pms_.isload ) { }
        redis::command( "SET", public_msg_setting::old_id_key, id ); 
        pms_.old_id = id ;
    }

private:
    std::vector<Pfun> funcs_;
    server_setting net_;

    void get_public_msg_setting();
    public_msg_setting pms_;
};

extern void bs_initialize(service_mgr&);
extern void ims_initialize(service_mgr&);

#endif

