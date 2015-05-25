#ifndef __mycurl_h__
#define __mycurl_h__

#include <string>
#include <vector>
#include <curl/curl.h>
#include <boost/lexical_cast.hpp>

#include "json.h"
#include "singleton.h"

class CurlHandle : public singleton<CurlHandle>
{
    public:
        CurlHandle() { 
            curl_global_init( CURL_GLOBAL_ALL );
            curl_ = NULL;
        }
        ~CurlHandle() {
            if ( curl_ ) { curl_easy_cleanup( curl_ ); }
            curl_global_cleanup();
        }
        typedef size_t (*ProcessData)(void *buffer, size_t size, size_t nmemb, void *user_p);
        std::pair<bool,std::string> http_post( const std::string& url, const std::string& data="", 
                ProcessData pfunc=NULL, const std::vector<std::string>& header=std::vector<std::string>() );
        std::pair<bool,std::string> http_get( const std::string& url, 
                ProcessData pfunc=NULL, const std::vector<std::string>& header=std::vector<std::string>() );

    private:
        CURL* curl_;
    private:
        // CurlHandle() { 
        //     curl_global_init( CURL_GLOBAL_ALL );
        //     curl_ = NULL;
        // }

        // ~CurlHandle() {
        //     if ( curl_ ) { curl_easy_cleanup( curl_ ); }
        //     curl_global_cleanup();
        // }

        CURL* curl_init() {
            curl_ = curl_easy_init();
            if ( curl_ ) {
                curl_easy_setopt( curl_, CURLOPT_SSL_VERIFYPEER, false );
                curl_easy_setopt( curl_, CURLOPT_TIMEOUT, 30 ); 
                curl_easy_setopt( curl_, CURLOPT_HEADER, 0 ); 
                curl_easy_setopt( curl_, CURLOPT_WRITEFUNCTION, NULL ); 
            }

            return curl_;
        }

        curl_slist* make_head( const std::vector<std::string>& header ) {
            struct curl_slist *headers = NULL;
            for( auto it=header.begin(); it!=header.end(); ++it ) {
                headers = curl_slist_append( headers, it->c_str() );
            }

            return headers;
        }
};

json::object php_get( const std::string&url );
json::object php_post( const std::string&url, const std::string& d );
#endif
