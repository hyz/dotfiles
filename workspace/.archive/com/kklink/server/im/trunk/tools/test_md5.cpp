#include <openssl/md5.h>
#include <stdint.h>
#include <curl/curl.h>
#include <stdio.h>
#include <iostream>
#include <string>
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>

using namespace std;

string low_case_32_md5( const string& src )
{
    string dst;

    char tmp[3]={'\0'};
    unsigned char out16[16]={'\0'};
    MD5( reinterpret_cast<const unsigned char*>(src.c_str()), src.length(), out16 );

    for ( int i = 0; i < 16; ++i ) {
        sprintf(tmp,"%2.2x",out16[i]);
        dst += tmp;
    }

    return dst;
}

struct XG_push
{
    string make_msg( const uint32_t id, const string& content, const string& title="月下" )
    {
        char const* msg_form = "{\"title\":\"%1%\",\"content\":\"%2%\",\"builder_id\":%3%,\"n_id\":0,\"ring\":1,\"vibrate\":1,\"clearable\":1,\"action\":{\"action_type\": 1,\"browser\": {\"url\": \"\",\"confirm\": 1},\"activity\":\"月下\",\"intent\": \"xxx\"}}";
        return (boost::format( msg_form ) %title %content %id ).str();
    }


    string create_url()
    {
        return "http://" + domain_path_;
    }

    string md5_params( const string& param_string ) 
    {
        return low_case_32_md5( "POST" + domain_path_ + param_string );
    }

    string src_param_string( const string& token, const uint32_t id, const string& content, const string& now )
    {
        return "access_id=" + access_id_ + "device_token=" + token 
            + "message=" + make_msg( id, content )
            +"message_type=1" + "timestamp=" + now + secret_key_;

    }

    string param_string( const string& token, const uint32_t id, const string& content )
    {
        string now = boost::lexical_cast<string>( time(NULL) );
        return "access_id=" + access_id_ + "&device_token=" + token 
            + "&message=" + make_msg( id, content )
            + "&message_type=1" + "&sign=" + md5_params( src_param_string( token, id, content, now )) 
            + "&timestamp=" + now;
    }

    private:
    static string domain_path_;
    static string access_id_;
    static string secret_key_;
};

string XG_push::domain_path_ = "openapi.xg.qq.com/v2/push/single_device";
string XG_push::access_id_ = "2100046570";
string XG_push::secret_key_ = "4baf03f3f68b44bbba04bf21f389ad43";

int main( int argc, char **argv )
{

    CURL* curl = NULL;

    if ( NULL == (curl = curl_easy_init())) {
        cout << "failed to curl_easy_init";
        exit(0);
    }

    XG_push xg;
    string push_url = xg.create_url();
    cout<< "set url: "<< push_url << endl;
    curl_easy_setopt( curl, CURLOPT_URL, push_url.c_str() );
    curl_easy_setopt(curl, CURLOPT_POST, 1);
    curl_easy_setopt(curl, CURLOPT_HEADER, 0);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, NULL);

    string pmsg = xg.param_string( "49ed9d07f57587ae4d42bde78e8f4e557bada792", 12323, "curl测试" );

    cout << "content: " << pmsg << endl;
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, pmsg.c_str());
    cout<< "result: " << curl_easy_perform(curl) << endl;

    return 0;
}
