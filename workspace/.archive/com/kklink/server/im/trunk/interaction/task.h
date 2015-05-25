#ifndef __task_h__
#define __task_h__

#include "dbc.h"
#include "log.h"
#include "users.h"

#include <vector>
#include <boost/function.hpp>
#include <boost/asio.hpp>

class Task
{
    public:
        Task( boost::asio::io_service& io_serv ) : timer_( io_serv ) {}
        static const int DAY_LEN;
        static const int HOUR_LEN;
        virtual void init() = 0;
        virtual void set_expire() = 0;
    protected:
        time_t expire_;
        boost::asio::deadline_timer timer_;

        std::vector<UInt> receivers( time_t begin, time_t end, int sex );
        std::vector<UInt> delivers( int num, int sex );
        bool execute( time_t begin, time_t end, int sex );
        void interact( UInt receiver_id, UInt deliver_id );
        void start( boost::function<void ( const boost::system::error_code& )> f );
};

class DailyTask : public Task
{
    public:
        DailyTask( boost::asio::io_service& io_serv ) : Task( io_serv ) {
            init();
            start( boost::bind( &DailyTask::execute, this, _1 ) );
        }
        void execute( const boost::system::error_code& ec );
    private:
        void init();
        void set_expire();
    private:
        static const char* dailytask_data;
};

class BasicTask : public Task
{
    public:
        BasicTask( boost::asio::io_service& io_serv ) : Task( io_serv ) {
            init();
            start( boost::bind( &BasicTask::execute, this, _1 ) );
        }
        void execute( const boost::system::error_code& ec );
    private:
        void init();
        void set_expire();
    private:
        time_t begin_, end_;
        static const char* basictask_data;
};

#endif
