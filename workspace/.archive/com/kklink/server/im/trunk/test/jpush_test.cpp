#include <unistd.h>
#include <string>
#include <iostream>


#include <curl/curl.h>

using namespace std;

const char* push_url = "https://api.jpush.cn/v3/push";
const char* content = "{\"platform\":\"all\",\"audience\":\"all\",\"notification\":{\"alert\":\"Hi,JPush!\"}}";
const char* key_passwd = "dc1f9d132aa65de36288899c:6a19705754d25e5f98eee98b";

size_t discard(char *ptr, size_t size, size_t nmemb, void *hf)
{
        return nmemb;
}

void loop_request()
{
    int ec = 0;
    int count = 8;
       
    CURL* curl = NULL;
    do {
        if ( NULL == curl ) {
            cout<< "init"<<endl;
            if ( (curl = curl_easy_init()) == 0) {
                usleep(500);
                continue;
            }

            cout<< "set url"<<endl;
            curl_easy_setopt(curl, CURLOPT_URL, push_url);
            curl_easy_setopt(curl, CURLOPT_POST, 1); 
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, false);
            curl_easy_setopt(curl, CURLOPT_USERPWD, key_passwd); 
            curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30); 
            curl_easy_setopt(curl, CURLOPT_HEADER, 0); 
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, NULL); 
        }

        cout<< "set content"<<endl;
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, content);

        cout<< "perform content"<<endl;
        ec = curl_easy_perform(curl);
        if (ec) { 
            cout<<"ec:"<<ec<<endl;
            curl_easy_cleanup(curl);
            curl = NULL;
        }

    } while (ec || --count);

    if ( NULL != curl )  {
        curl_easy_cleanup(curl);
    }
}

int main(int argc, char* argv[])
{
    loop_request();

    return 0;
}
