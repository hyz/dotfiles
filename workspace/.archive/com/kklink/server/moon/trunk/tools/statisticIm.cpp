#include<string>
#include<fstream>
#include<iostream>
#include<boost/format.hpp>

struct clientRecord
{
    clientRecord(int u=0,int beg=0, int end=0, int bg=0, int op=0)
    {
        uid=u;
        begin = beg;
        end = end;
        bg = bg;
        close_operator = op;
    }
    int uid;
    long begin;
    long end;
    int bg;
    int close_operator;
    friend std::ifstream& operator>>( std::ifstream& in, clientRecord& rec );
};

std::ifstream& operator>>( std::ifstream& in, clientRecord& rec )
{
    in>>rec.uid>>rec.begin>>rec.end>>rec.bg>>rec.close_operator;
    return in;
}

std::string statistic_client_Record( int uid )
{
    time_t first_conn = 0;
    time_t last_close = 0;
    time_t online_static = 0;
    int bg_count = 0;
    int client_close_count = 0;
    int server_close_count = 0;
    struct tm tm;
    time_t now = time(0);
    gmtime_r(&now, &tm);
    std::string filename = (boost::format("/tmp/yx-socket-closed.%1%-%2%")%tm.tm_mon%tm.tm_mday).str();
    std::cout << filename<<std::endl;
    std::ifstream file(filename.c_str(), std::ifstream::in);

    while ( file.good() ) {
        clientRecord rec;
        file >> rec;
        if ( rec.uid != uid ) {
            continue;
        }

        if ( 0 == first_conn ) {
            first_conn = rec.begin;
        }
        last_close = rec.end;

        if ( 1 == rec.bg ) {
            ++bg_count;
        }

        if ( 1 == rec.close_operator ) {
            ++client_close_count;
        } else {
            ++server_close_count;
        }

        online_static += (rec.end - rec.begin);
    }
    file.close();

    return (boost::format("userid:%1%, first_connection:%2%, last_close: %3%, online_static: %4%, "
            "background count: %5%, client close count:%6%, server close count:%7%")
        %uid %first_conn % last_close %online_static %bg_count %client_close_count 
        %server_close_count).str();
}

int main(int argc, char* argv[])
{
    if ( 2 != argc ) {
        std::cout<<" usage: exe userid"<<std::endl;
    }
    int uid = atoi(argv[1]);

    std::cout<<statistic_client_Record(uid)<<std::endl;

    return 0;
}
