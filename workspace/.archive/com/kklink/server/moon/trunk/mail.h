#ifndef MAIL_H__
#define MAIL_H__

// #define BOOST_ASIO_ENABLE_HANDLER_TRACKING 1

#include <stdio.h>
#include <string>
#include <boost/system/error_code.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <boost/thread.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/format.hpp>

struct email
{
    email();
    email(const std::string & rcpt);
    email(int id, const std::string & rcpt, const std::string & subject, const std::string & content);

    void fill(const std::string & subject, const std::string & content);

    int id() const { return id_; }
    const std::string& rcpt() const { return rcpt_; }
    const std::string& subject() const { return subject_; }
    const std::string& content() const { return content_; }

private:
    int id_;
    std::string rcpt_;
    std::string subject_;
    std::string content_;
};

struct mailer
{
    ~mailer();
    mailer();

    void thread_start();

    void send(const email & m);

    boost::asio::io_service& get_io_service() { return io_service_; }

public:
    static mailer& instance() { return *instance(0,0); }
    static mailer* instance(bool set, mailer* p=0);

private:
    void check();
    void add(email m);
    void timed(boost::system::error_code const & ec);

    email mail_;
    int ntry_;

    boost::asio::io_service io_service_;
    boost::asio::deadline_timer deadline_;
    boost::thread thread_;

public:
    struct initializer;
};

struct mailer::initializer : boost::noncopyable
{
    initializer(const boost::property_tree::ptree& ini);
    ~initializer();
    mailer mailer_;
};

std::ostream& operator<<(std::ostream& out, const email & m);

#endif // MAIL_H__

