#ifndef MSGH_H__
#define MSGH_H__

#include "ioctx.h"

void msgh_main(io_context& ctx, int, json::object const& jv);

void msgh_ims_default(io_context& ctx, int, json::object const& jv);
void msgh_bs_default(io_context& ctx, int, json::object const& jv);

void msgh_regist(int cmd, message_handler_t fp);

void msgh_bs_set_authcode(std::string const& ac);

#endif // MSGH_H__

