#ifndef __mycurl_h__
#define __mycurl_h__

#include <string>
#include <vector>
#include <curl/curl.h>
#include <boost/lexical_cast.hpp>

class Curl_Handle
{
    private:
        CURL* curl_;
        std::string base_url_;
        std::vector<std::string> header_;

    public:
        static Curl_Handle& init( const std::string& url= "", 
                const std::vector<std::string>& header= std::vector<std::string>() );
        static Curl_Handle& inst( const std::string& url= "", 
                const std::vector<std::string>& header= std::vector<std::string>() );
        bool http_post( const std::string& msg );

    private:
        Curl_Handle( const std::string& url, const std::vector<std::string>& header )
        {
            base_url_ = url;
            header_ = header;

            for( auto it=header_.begin(); it!=header_.end(); ++it ) {
                std::cout<< *it;
            }

            curl_global_init( CURL_GLOBAL_ALL );
            curl_ = NULL;
        }

        ~Curl_Handle()
        {
            if ( curl_ ) {
                curl_easy_cleanup( curl_ );
            }

            curl_global_cleanup();
        }

        CURL* curl_init()
        {
            curl_ = curl_easy_init();
            if ( curl_ ) {
                curl_easy_setopt( curl_, CURLOPT_POST, 1 ); 
                curl_easy_setopt( curl_, CURLOPT_SSL_VERIFYPEER, false );
                curl_easy_setopt( curl_, CURLOPT_TIMEOUT, 30 ); 
                curl_easy_setopt( curl_, CURLOPT_HEADER, 0 ); 
                curl_easy_setopt( curl_, CURLOPT_WRITEFUNCTION, NULL ); 
            }

            return curl_;
        }

        std::string make_url( const std::string& url_extent = "" ) 
        {
            return base_url_ + url_extent;
        }

        curl_slist* make_head()
        {
            struct curl_slist *headers = NULL;
            for( auto it=header_.begin(); it!=header_.end(); ++it ) {
                headers = curl_slist_append( headers, it->c_str() );
            }

            return headers;
        }
};

inline bool transmit( const std::string& msg )
{
    return Curl_Handle::inst().http_post( msg );
}

#endif
