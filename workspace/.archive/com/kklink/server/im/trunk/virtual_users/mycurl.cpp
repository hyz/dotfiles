#include <iostream>

#include "mycurl.h"
#include "log.h"

Curl_Handle& Curl_Handle::init( const std::string& url, const std::vector<std::string>& header )
{
    return inst( url, header );
}

Curl_Handle& Curl_Handle::inst( const std::string& url, const std::vector<std::string>& header )
{
    static Curl_Handle handle_( url, header );
    return handle_;
}

bool Curl_Handle::http_post( const std::string& msg )
{
    bool result = false;
    for ( int i=0; i<3; ++i ) {
        if ( !curl_ ) { curl_init(); }

        if ( curl_ ) {
            std::string url = make_url();
            LOG_I<< "url:"<< url<< "message:"<< msg;
            curl_easy_setopt( curl_, CURLOPT_URL, url.c_str() );
            curl_easy_setopt( curl_, CURLOPT_HTTPHEADER, make_head() );
            curl_easy_setopt( curl_, CURLOPT_POSTFIELDS, msg.c_str() );

            int ec = curl_easy_perform( curl_ );
            if ( 0 == ec ) {
                result = true;
                break;
            } else { 
                LOG_I<< "try:"<< i<< "Send Error:"<< ec;
                curl_easy_cleanup( curl_ );
                curl_ = NULL;
            }
        }

        if ( 0 != i ) { sleep( 1 ); }
    }

    return result;
}
