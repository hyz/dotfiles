#include "myconfig.h"
#include <boost/asio/placeholders.hpp>
#include "mail.h"
#include "dbc.h"

using namespace std;
using namespace boost;

static std::string mail_from_ = "notification@kklink.com(月下)";
static std::string mail_auth_user_ = "notification@kklink.com";
static std::string mail_auth_pass_ = "yunsunno3";
static std::string mail_certs_ = "etc/moon.d";

std::ostream& operator<<(std::ostream& out, const email & m)
{
    return out << m.id() << "/" << m.rcpt() << "/{" << m.subject()<<"}";
}

void mailer::timed(boost::system::error_code const & ec)
{
    if (ec == boost::asio::error::operation_aborted)
        return;

    const char* fmts = "env MAILRC=/dev/null"
        " smtp=smtps://smtp.exmail.qq.com:465"
        " smtp-auth=login"
        " smtp-auth-user=%3%"
        " smtp-auth-password=\"%4%\""
        " nss-config-dir=%6%"
        " ssl-verify=ignore"
        " from=\"%5%\""
        " mail -v -n -s \"%2%\" %1%"
        "";

    string cmd = str(format(fmts)
            % mail_.rcpt() % mail_.subject()
            % mail_auth_user_ % mail_auth_pass_ % mail_from_ % mail_certs_);

    int exst = 0;

    FILE* fp = popen(cmd.c_str(), "w");
    if (fp)
    {
        fwrite(mail_.content().data(), mail_.content().size(), 1, fp);
        exst = pclose(fp);
        ++ntry_;
    }

    // LOG_I << "mail: " << mail_ << ": " << ntry_ << " " << exst;
    ::syslog(0, "%d/{%s} %d %d #%s %d", mail_.id(), mail_.subject().c_str(), exst, ntry_, __FUNCTION__, __LINE__);

    if (exst == 0 || ntry_ > 3)
    {
        sql::exec(format("DELETE FROM mailq where id=%1% LIMIT 1") % mail_.id());
        ntry_ = 0;

        deadline_.expires_at(boost::posix_time::pos_infin);
        io_service_.post(boost::bind(&mailer::check, this));
    }
    else
    {
        deadline_.expires_from_now(boost::posix_time::seconds(3));
    }

    deadline_.async_wait(boost::bind(&mailer::timed, this, asio::placeholders::error));
}

void mailer::check()
{
    if (deadline_.expires_at() != boost::posix_time::pos_infin)
        return;

    sql::datas datas("SELECT id,rcpt,subject,content FROM mailq LIMIT 1");
    sql::datas::row_type row = datas.next();
    if (row)
    {
        const char* id = row.at(0);
        const char* rcpt = row.at(1);
        const char* subject = row.at(2);
        const char* content = row.at(3);

        mail_ = email(lexical_cast<int>(id), rcpt, subject, content);
        ntry_ = 0;
        ::syslog(0, "%d/{%s} #%s %d", mail_.id(), mail_.subject().c_str(), __FUNCTION__, __LINE__);

        deadline_.expires_from_now(boost::posix_time::seconds(0));
        deadline_.async_wait(boost::bind(&mailer::timed, this, asio::placeholders::error));
    }
}

mailer::mailer()
    : deadline_(io_service_)
{
    ntry_ = 0;
}

mailer::~mailer()
{
    io_service_.stop();
    thread_.join();

    if (this == instance(0,0))
    {
        instance(1,0);
    }
}

void mailer::thread_start()
{
    deadline_.expires_at(boost::posix_time::pos_infin);
    deadline_.async_wait(boost::bind(&mailer::timed, this, asio::placeholders::error));

    io_service_.post(boost::bind(&mailer::check, this));

    thread_ = boost::thread(boost::bind(&boost::asio::io_service::run, &io_service_)) ;
}

//int send(const std::string & mail, const std::string & subject, const std::string & content);
void mailer::add(email m)
{
    sql::exec(format("INSERT INTO mailq(id,rcpt,subject,content) VALUES(%1%,'%2%','%3%','%4%')")
            % m.id() % m.rcpt() % sql::db_escape(m.subject()) % sql::db_escape(m.content()));

    io_service_.post(boost::bind(&mailer::check, this));
}

void mailer::send(const email & m)
{
    email tmp(m);
    io_service_.post(boost::bind(&mailer::add, this, tmp));
}

email::email()
{
    id_ = 0;
}

email::email(const std::string & rcpt) //, const std::string & subject, const std::string & content)
    : rcpt_(rcpt)//, subject_(subject), content_(content)
{
extern int mailq_idx();
extern int bindMail_idx();
extern int mailPwd_idx();

    static int idx = std::max(mailq_idx(), std::max(mailPwd_idx(), bindMail_idx()));
    id_ = idx++;
}

email::email(int id, const std::string & rcpt, const std::string & subject, const std::string & content)
    : rcpt_(rcpt), subject_(subject), content_(content)
{
    id_ = id;
}

void email::fill(const std::string & subject, const std::string & content)
{
    subject_ = subject;
    content_ = content;
}

// void mailto(const std::string & mail, const std::string & subject, const std::string & content);

mailer* mailer::instance(bool set, mailer* p)
{
    static mailer *inst = 0;
    if (set)
    {
        mailer *old = inst;
        inst = p;
        return old;
    }

    BOOST_ASSERT(inst != 0);
    return inst;
}

mailer::initializer::initializer(const boost::property_tree::ptree& ini)
{
    mail_auth_user_ = ini.get<string>("user", mail_auth_user_);
    mail_auth_pass_ = ini.get<string>("pass", mail_auth_pass_);
    mail_from_ = ini.get<string>("from", mail_from_);
    mail_certs_ = ini.get<string>("certs", mail_certs_);

    LOG_I << "mailer:init"
        <<" "<< mail_auth_user_
        <<" "<< mail_auth_pass_
        <<" "<< mail_from_
        <<" "<< mail_certs_
        ;

    mailer::instance(1, &mailer_);
    mailer_.thread_start();
}

mailer::initializer::~initializer()
{
    LOG_I << __FILE__ << __LINE__;
}

