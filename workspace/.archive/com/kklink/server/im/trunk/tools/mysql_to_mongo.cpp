#include <boost/function.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/foreach.hpp>

#include <iostream>
#include <vector>

#include "mongo/client/dbclient.h"
#include "../dbc.h"
#include "../log.h"
#include "../json.h"


using namespace mongo;
using namespace std;

typedef boost::function<void (const string&, vector<BSONObj>&)> Sel_Func;

void get_public_message(const string& sql, vector<BSONObj>& out )
{
      // "select 'id', 'from', 'sid', 'content', 'ctime' from public_message";
    sql::datas datas( sql );
    while ( sql::datas::row_type row = datas.next() ) {
        int id = boost::lexical_cast<int>(row[0]) ;
        int uid = boost::lexical_cast<int>(row[1]);
        time_t ctime = boost::lexical_cast<time_t>(row[4]);
        BSONObj p = BSON( "id" << id << "from_uid" << uid << "sid" << row[2] 
                << "content" << row[3] << "ctime" << (unsigned int)ctime );

        out.push_back( p );
    }

    cout << "out size:"<<out.size();
}

boost::property_tree::ptree getconf(boost::filesystem::ifstream &cf, const char* sec)
{
    boost::property_tree::ptree ini;
    boost::property_tree::ini_parser::read_ini(cf, ini);
    if (sec)
    {
        boost::property_tree::ptree empty, ini;
        return ini.get_child(sec, empty);
    }
    return ini;
}

int mysql_to_mongo( const string& sql, Sel_Func func, const string& key )
{
   Status status = client::initialize();
   if ( !status.isOK() ) {
       std::cout << "failed to initialize the client driver: " << status.toString() << endl;
       return EXIT_FAILURE;
   }

   const char *port = "27017";

   boost::filesystem::ifstream confs("/home/lindu/moon2.0/yxim.conf");
   boost::property_tree::ptree conf = getconf(confs, 0);
   sql::dbc::init( sql::dbc::config(conf.get_child("mysql")) );

   logging::syslog(LOG_PID|LOG_CONS, 0);

   DBClientConnection c;
   c.connect(string("192.168.1.57:") + port); //"192.168.58.1");
   cout << "connected ok" << endl;

   vector<BSONObj> datas; 

   func( sql, datas);

   if ( !datas.empty() ) {
       c.insert( key, datas );
   } else { 
       cout << "datas empty"<< endl;
   }

   return EXIT_SUCCESS;
}

boost::shared_ptr<DBClientCursor> mongo_select2(const std::string& db_table, const Query& in)
{
    Status status = client::initialize();
    if ( !status.isOK() ) {
        std::cout << "failed to initialize the client driver: " << status.toString() << endl;
        return boost::shared_ptr<DBClientCursor>(); 
    }

    const char *port = "27017";

    DBClientConnection c;
    c.connect(string("192.168.1.57:") + port); //"192.168.58.1");
    boost::shared_ptr<DBClientCursor> cursor( c.query(db_table, BSONObj()));
    if (!cursor.get()) {
        LOG << "query failure";
        BOOST_THROW_EXCEPTION( error_query() );
    }

    return cursor;
}

void test_print()
{
    int buf[1024];
    memset(buf,0,sizeof(buf));

    void *p = calloc(sizeof(int),1024);
    free(p);
    p = 0;

    return;
}

int public_message_get2( const std::string& key )
{
    boost::format fmt("{\"id\":{$gt:%1%}}");

    auto out = mongo_select2( key, (fmt %0).str());

    if ( out ) {
        while ( out->more() ){
            BSONObj obj = out->next();
            if ( !obj.isValid() ) continue;

            LOG<< obj.toString();
            int id = obj.getIntField("id");
            string sid = obj.getStringField("sid");
            time_t ct = obj.getIntField("ctime");
            string data = obj.getStringField("content");

            json::object obj2;
            obj2.emplace("sid", sid);
            test_print();
            obj2.emplace("data", json::decode<json::object>(data).value());
            obj2.emplace("ctime", ct);

            cout<< obj2<<endl;
        }
    }

    return 0;
}

int main(int argc, char* argv[]) {
    int ret = EXIT_SUCCESS;
    try {
        string sql = "select id, from_uid, sid, content, UNIX_TIMESTAMP(ctime) from public_message";
        string key = "moonv2.public_messages";
        // ret = mysql_to_mongo(sql, get_public_message, key);
        ret = public_message_get2( key );
    }
    catch( DBException &e ) {
        cout << "caught " << e.what() << endl;
        ret = EXIT_FAILURE;
    }
    return ret;
}
