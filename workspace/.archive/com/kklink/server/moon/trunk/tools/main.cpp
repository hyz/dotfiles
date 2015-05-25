#include <iostream>
#include <boost/format.hpp>

#include "dbc.h"
using namespace std;
using namespace boost;
int main()
{
    const char * dbcfg = "[default]\n"
        "host=127.0.0.1\n"
        "port=3306\n"
        "user=lindu\n"
        "password=lindu12345\n"
        "db=lindu_test\n"
        ;

    istringstream ins(dbcfg);
    db(&ins);
    Datas_row qq(db().connect());
    qq.exec("select * from feedbacks");
    qq.exec("insert into feedbacks values(\"13816775985\",\"太烂了哥们\")", true);

    {
        Datas_row qq(db().connect());
        qq.exec("SELECT user_name,sex,birthday as bir from users");

        Datas_row qq2(db().connect());
        qq2.exec( "SELECT user_name,sex,birthday as bir from users");
    }

    Datas_row q(db().connect());
    q.exec("SELECT user_name,sex,birthday as bir,user_phone from users");

    Datas_row q2(db().connect());
    q2.exec("SELECT user_name,sex,birthday as bir,user_phone from users");

    Datas_row q3(db().connect());
    q3.exec("SELECT whisper_id,user_id,user_name,user_phone,birthday,sex,img_path,"
            "recv_atti,msg_type,COUNT(*) AS unread,send_atti,rec_time FROM letters_view WHERE "
            "lover_phone=18677503698 GROUP BY user_phone"
            " ORDER BY rec_time DESC");

    while (q3.next())
        clog << format("phone %s\n") % q3.get<string>("user_phone");
    //while (q2.next())
    //    clog << format("phone %s\n") % q2.get<string>("user_phone");
    // vector<string> q2v(q2.begin(), q2.end());
    // clog << q2v << endl;

    for (Datas_row::iterator it = q.begin();
            it != q.end(); ++it)
        clog << format("phone %s\n") % it->get<string>("user_phone");

    clog << endl;
    for (Datas_row::iterator it = q.begin();
            it != q.end(); ++it)
        clog << format("phone %s\n") % it->get<string>("user_phone");

    return 0;
}

