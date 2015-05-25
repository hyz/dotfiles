#include "task.h"
#include "mycurl.h"
#include "interaction.h"

#include <boost/foreach.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

const char* DailyTask::dailytask_data = "yx:interact:dailytask";
const char* BasicTask::basictask_data = "yx:interact:basictask";
const int Task::DAY_LEN = 24*60*60;
const int Task::HOUR_LEN = 60*60;

void Task::interact( UInt receiver_id, UInt deliver_id ) 
{
    const char* greetings[] = { "你好", "hi", "hello", "嗨", "[呵呵]", "[酷]", "[挤眼]", "[微笑]", 
        "在？", "在吗？", "好啊", "哈喽", "嘿嘿", "嘻嘻", "(*^__^*)", "[爱你]", "[呲牙大笑]" };

    int index = rand() % ( sizeof( greetings ) / sizeof( greetings[0] ) );
    int type = index % 2;
    interaction( receiver_id, deliver_id, type, greetings[index] );
}

bool Task::execute( time_t begin, time_t end, int sex )
{
    bool result = false;
    auto r = receivers( begin, end, sex );
    auto d = delivers( 2*r.size(), sex );
    if ( r.empty() || d.empty() ) { return result; }

    std::vector<UInt> dt( d.size(), 0 );
    std::sort( r.begin(), r.end() );
    std::sort( d.begin(), d.end() );
    auto it = std::set_difference( d.begin(), d.end(), r.begin(), r.end(), dt.begin() );
    dt.resize( it - dt.begin() );

    auto dtlen =  dt.size();
    auto rlen =  r.size();
    LOG_I<< "Sum of receivers:"<< rlen<< " sum of delivers:"<< dtlen;

    // LOG_I<< "r vector:"<< r; // test
    // LOG_I<< "dt vector:"<< dt; // test
    // UInt recv[] = { 1127075, 1125490 }; // test
    // UInt deleiver[] = { 10012, 10014, 10015, 10016 }; // test
    // r.assign( recv, recv + sizeof( recv ) / sizeof( recv[0] ) ); // test
    // dt.assign( deleiver, deleiver + sizeof( deleiver ) / sizeof( deleiver[0] ) ); // test
    // dtlen =  dt.size(); //test
    // rlen =  r.size(); //test

    if ( rlen && dtlen ) {
        BOOST_FOREACH( const auto i, r ) {
            auto rd = rand() % dtlen;
            interact( i, dt[rd] );
        }

        result = true;
    }

    return result;
}

void DailyTask::execute( const boost::system::error_code& ec )
{
    if ( !ec ) {
        time_t now = time( NULL );
        LOG_I<<"DailyTask::executing"; //test
        Task::execute( now-3*Task::DAY_LEN, now-2*Task::DAY_LEN, 0 );
        Task::execute( now-3*Task::DAY_LEN, now-2*Task::DAY_LEN, 1 );

        Task::execute( now-2*Task::DAY_LEN, now-1*Task::DAY_LEN, 0 );
        Task::execute( now-2*Task::DAY_LEN, now-1*Task::DAY_LEN, 1 );
    }

    set_expire();
    start( boost::bind( &DailyTask::execute, this, _1 ) );
}

void BasicTask::execute( const boost::system::error_code& ec )
{
    if ( !ec ) {
        end_ = time( NULL ) - Task::HOUR_LEN/2;
        LOG_I<<"BasicTask::executing"; //test
        bool flag = Task::execute( begin_, end_, 0 );
        flag = Task::execute( begin_, end_, 1 ) ? true : flag;
        if ( flag ) { begin_ = end_; }
    }

    set_expire();
    start( boost::bind( &BasicTask::execute, this, _1 ) );
}

std::vector<UInt> Task::receivers( time_t begin, time_t end, int sex )
{
     return UsrMgr::instance().getnewuser( begin, end, sex );
}

std::vector<UInt> Task::delivers( int num, int sex )
{
     return UsrMgr::instance().getactiveuser( num, sex );
}

void Task::start( boost::function<void ( const boost::system::error_code& )> f )
{
    timer_.expires_at( boost::posix_time::from_time_t( expire_ ) );
    timer_.async_wait( f );
}

void DailyTask::set_expire()
{
    time_t tomorrow_begin = ( time( NULL )/Task::DAY_LEN + 1  ) * Task::DAY_LEN;
    expire_ = tomorrow_begin + rand() %Task::DAY_LEN;

    auto val = boost::lexical_cast<std::string>( expire_ );
    redis::command( "SET", dailytask_data, val );
}

void BasicTask::set_expire()
{
    time_t next_hour_begin = ( time( NULL )/HOUR_LEN + 1  ) * HOUR_LEN;
    expire_ = next_hour_begin + rand() %( HOUR_LEN/2 );

    auto val = boost::lexical_cast<std::string>( expire_ ) + " " +  boost::lexical_cast<std::string>( begin_ );
    redis::command( "SET", basictask_data, val );
}

void DailyTask::init()
{
    time_t e = ( time( NULL )/Task::DAY_LEN )*Task::DAY_LEN + rand()%Task::DAY_LEN;
    auto reply = redis::command( "GET", dailytask_data );
    if ( reply && reply->type == REDIS_REPLY_STRING ) {
        expire_ = atoi( reply->str );
        LOG_I<< "expire:"<< expire_;
    }

    expire_ = expire_>e ? e : e;
}

void BasicTask::init()
{
    time_t b = time( NULL ) - Task::DAY_LEN;
    time_t e = ( time( NULL )/HOUR_LEN ) * HOUR_LEN + rand() %HOUR_LEN;
    auto reply = redis::command( "GET", basictask_data );
    if ( reply && reply->type == REDIS_REPLY_STRING ) {
        std::istringstream iss( reply->str );
        iss >>expire_ >>begin_;
        LOG_I<< "expire:"<< expire_<< "begin:"<< begin_;
    }

    begin_ = begin_>b ? begin_ : b;
    expire_ = expire_>e ? e : e;
}
