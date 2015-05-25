#ifndef GLOBALS_H__
#define GLOBALS_H__

#include <boost/function.hpp>
#include <boost/signals2/signal.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/deadline_timer.hpp>
#include "singleton.h"
#include "myerror.h"
#include "log.h"

typedef unsigned int UInt;

struct globals : singleton<globals>
{
    boost::asio::io_service io_service;

    globals() {}
};

//#define emit(sig, ...) globals::instance().io_service.post(boost::bind(boost::ref(sig), __VA_ARGS__))
#define emit(sig, ...) sig(__VA_ARGS__)

struct emit_helper
{
    boost::function<void ()> fn_;

    template <typename Sig, typename ...Args>
    emit_helper(Sig& sig, Args ... params)
        : fn_( boost::bind(&emit_helper::fx, &sig, params...) ) // : fn_( boost::bind(boost::ref(sig), std::forward<Args>(params)...) )
    {}

    void operator()() const
    {
        try {
            fn_();
        } catch (myerror const& ex) {
            LOG << ex;
        } catch (std::exception const& ex) {
            LOG << "signal2" << ex.what();
        }
    }

    template <typename Sig, typename ...Args>
    static void fx(Sig *sig, Args ... params)
    {
        (*sig)(params ...);
    }
};

//template <typename Sig, typename ...Args>
//void emit(Sig& sig, Args const & ... params)
//{
//    globals::instance().io_service.post( emit_helper(sig, params ...) );
//    //globals::instance().io_service.post( boost::bind(boost::ref(sig), std::forward<Args>(params)...));
//}

struct simple_timer_loop
{
    boost::asio::deadline_timer *timer;
    UInt interval;
    boost::function<void ()> fn_;

    simple_timer_loop(boost::asio::deadline_timer& t, UInt inv, boost::function<void ()> f)
        : timer(&t), interval(inv), fn_(f)
    {}

    void operator()(boost::system::error_code ec)
    {
        if (ec) return;
        fn_();
        wait(interval);
    }

    void wait(UInt interval)
    {
        timer->expires_from_now(boost::posix_time::seconds(interval));
        timer->async_wait(*this);
    }
};

template <typename ...Args>
inline void stloop_start(boost::asio::deadline_timer& t, Args const & ... params)
{
    simple_timer_loop stl(t, params ...);
    stl(boost::system::error_code());
}

// template <typename ...Args> void log(Args && ... params)
// {
//     globals::instance().logger(std::forward<Args>(params)...);
// }

#endif // GLOBALS_H__

