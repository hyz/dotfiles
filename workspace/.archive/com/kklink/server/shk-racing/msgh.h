#ifndef MSGH_H__
#define MSGH_H__

#include "ioctx.h"

void msgh_player(io_context& ctx, int, std::string const& jv);
void msgh_gamebs(io_context& ctx, int, std::string const& jv);

// void msgh_main(io_context& ctx, int, std::string const& jv);
// void msgh_regist(int cmd, message_handler_t fp);
// void msgh_bs_set_authcode(std::string const& ac);

#endif // MSGH_H__

