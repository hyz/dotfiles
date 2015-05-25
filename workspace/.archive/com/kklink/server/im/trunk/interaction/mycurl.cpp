#include <iostream>

#include "mycurl.h"
#include "log.h"

const std::string php_header[] = {
    "Host: gray_v4.api.moon.kklink.com",
    "X-API-VERSION: v4.0"
};

size_t curl_callback( void *buffer, size_t size, size_t nmemb, void *user_p )
{
    std::string& us = *( (std::string*)( user_p ) );
    us += std::string( static_cast<const char*>( buffer ), size*nmemb );
    return size*nmemb;
}

json::object php_get( const std::string&url )
{
    json::object ret;
    LOG_I<< url;
    auto resp = CurlHandle::instance().http_get( url, curl_callback, 
            std::vector<std::string>( php_header, php_header+sizeof(php_header)/sizeof( php_header[0] ) ) );
    LOG_I<< resp.first<< resp.second;

    if ( resp.first ) {
        ret = json::decode<json::object>( resp.second ).value();
    }

    return  ret;
}

json::object php_post( const std::string&url, const std::string& data )
{
    json::object ret;
    LOG_I<< url;
    LOG_I<< data;
    auto resp = CurlHandle::instance().http_post( url, data, curl_callback, 
            std::vector<std::string>( php_header, php_header+sizeof(php_header)/sizeof( php_header[0] ) ) );

    LOG_I<< resp.first<< resp.second;

    if ( resp.first ) {
        ret = json::decode<json::object>( resp.second ).value();
    }

    return  ret;
}

std::pair<bool,std::string> CurlHandle::http_post( const std::string& url, const std::string& data, 
        ProcessData pfunc, const std::vector<std::string>& header )
{
    bool result = false;
    std::string response;
    for ( int i=0; i<3; ++i ) {
        if ( !curl_ ) { 
            curl_init(); 
            if ( curl_ ) { curl_easy_setopt( curl_, CURLOPT_POST, 1 ); }
        }

        if ( curl_ ) {
            curl_easy_setopt( curl_, CURLOPT_URL, url.c_str() );
            curl_easy_setopt( curl_, CURLOPT_HTTPHEADER, make_head( header ) );
            if ( !data.empty() ) {
                curl_easy_setopt( curl_, CURLOPT_POSTFIELDS, data.c_str() );
            }
            if ( NULL != pfunc ) {
                curl_easy_setopt( curl_, CURLOPT_WRITEFUNCTION, pfunc );
                curl_easy_setopt( curl_, CURLOPT_WRITEDATA, &response );
            }

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

    LOG_I<< response;
    return make_pair( result, response );
}

std::pair<bool,std::string> CurlHandle::http_get( const std::string& url, 
        ProcessData pfunc, const std::vector<std::string>& header )
{
    bool result = false;
    std::string response;
    for ( int i=0; i<3; ++i ) {
        if ( !curl_ ) { curl_init(); }

        if ( curl_ ) {
            curl_easy_setopt( curl_, CURLOPT_URL, url.c_str() );
            curl_easy_setopt( curl_, CURLOPT_HTTPHEADER, make_head( header ) );
            if ( NULL != pfunc ) {
                curl_easy_setopt( curl_, CURLOPT_WRITEFUNCTION, pfunc );
                curl_easy_setopt( curl_, CURLOPT_WRITEDATA, &response );
            }

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

    return make_pair( result, response );
}
