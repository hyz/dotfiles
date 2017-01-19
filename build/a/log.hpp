#ifndef LOG_HPP__
#define LOG_HPP__

#include <boost/config.hpp>

#ifdef BOOST_WINDOWS

#define GOOGLE_GLOG_DLL_DECL
#define GLOG_NO_ABBREVIATED_SEVERITIES
#include "glog/logging.h"

#ifdef NDEBUG
#  pragma comment(lib,"glog/libglog_static.lib")
#else
#  pragma comment(lib,"glog/libglog_static_d.lib")
#endif

#else
#  include "glog/logging.h"
#endif

struct logging_init
{
    logging_init(int ac, char* const av[]) {
        google::InitGoogleLogging(av[0]);

        FLAGS_log_dir = "/tmp/_a";
        FLAGS_alsologtostderr = 1;
        //FLAGS_stderrthreshold = ;
        FLAGS_stop_logging_if_full_disk = true;
        // FLAGS_minloglevel ;

        //google::SetLogDestination(google::ERROR, "err");  
        //google::SetStderrLogging(google::GLOG_INFO);

        //google::ParseCommandLineFlags(&ac, &av, true);

        LOG(INFO) << FLAGS_log_dir;
    }
    ~logging_init() {
        LOG(ERROR) << "Shutdown";
        google::ShutdownGoogleLogging();
    }

    void samples() {
      //LOG(DEBUG)   << "debug"   << __FUNCTION__;
        LOG(INFO)    << "INFO"    << __FUNCTION__;
        LOG(WARNING) << "WARNING" << __FUNCTION__;
        LOG(ERROR)   << "ERROR"   << __FUNCTION__;
    }
    void fatal() {
        LOG(FATAL) << "FATAL" << __FUNCTION__;
    }
};

//#include "spdlog/spdlog.h"
//auto console = spdlog::stdout_color_mt("stdout");
//console->info("Welcome to spdlog!");
//console->error("Some error message with arg{}..", 1);

#endif // LOG_HPP__


