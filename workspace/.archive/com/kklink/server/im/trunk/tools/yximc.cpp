#include <cstdlib>
#include <deque>
#include <string>
#include <iostream>
#include <chrono>
#include <thread>
#include <boost/tuple/tuple.hpp>
#include <boost/exception/error_info.hpp>
#include <boost/exception/get_error_info.hpp>
#include <boost/exception/diagnostic_information.hpp>
#include <boost/ptr_container/ptr_map.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/regex.hpp>
#include <boost/asio.hpp>
#include <boost/algorithm/string.hpp>
//#include <boost/property_tree/ptree.hpp>
//#include <boost/property_tree/json_parser.hpp>
#include <boost/logic/tribool.hpp>
#include <boost/format.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/posix_time/posix_time_io.hpp>
#include "globals.h"
#include "json.h"
#include "log.h"

namespace io = boost::asio;
namespace ip = boost::asio::ip;
namespace placeholders = boost::asio::placeholders;

typedef std::deque<std::pair<std::string,std::string> > message_queue;

// static std::string mac_ = "00:11:22";
static ip::tcp::endpoint endpoint_;
static ip::tcp::endpoint http_endpoint_;

inline std::ostream & operator<<(std::ostream & out, boost::system::error_code const * ec)
{
    return out << *ec <<":"<< ec->message();
}

template <typename T>
void log_err(boost::system::error_code const & ec, T* thiz, int lino)
{
  if (ec == io::error::operation_aborted) {
    LOG << &ec << " err#" << lino;
  } else {
    LOG << *thiz <<" "<< &ec <<" err#" << lino;
  }
}

template <typename T>
inline io::const_buffers_1 const_buffer(T const & m) { return io::buffer(m); }

class yx_client
{
    enum struct stage {
        zero = 0
        , init
        , connecting
        , authorization
        , established
        , destroy
        , Max
    };
    stage stage_;
    time_t stage_tp_;
    int stage_nloop_;

public:
  yx_client(io::io_service& io_s, boost::tuple<UInt,std::string,std::string,std::string> user)
    : user_(user)
      , io_service_(io_s)
      , deadline_(io_s)
      , socket_(io_s)
  {
    stage_ = stage::zero;
    stage_nloop_ = 0;
    stage_tp_ = 0;
    // lg_.add_attribute("User", boost::log::attributes::make_constant(user_.first));
  }

  void start() //(tcp::endpoint const & endp) // void start(tcp::resolver::iterator endpoint_iterator)
  {
    if (stage_ != stage::zero)
        return;
    deadline_.expires_from_now(boost::posix_time::seconds(1));
    deadline_.async_wait(boost::bind(&yx_client::timed_loop, this, placeholders::error));
  }

  void close() // (boost::system::error_code ec)
  {
    LOG << *this << " close";
    io_service_.post([this]() {
                socket_.close();
                deadline_.cancel();
            });
  }

  bool is_open() const { return socket_.is_open(); }

private:
  struct http_t
  {
      http_t(io::io_service & io_s, std::string user, std::string pwd)
          : login_(std::make_pair(user,pwd))
          , socket_(io_s)
      {
          // LOG << login_.first << "  http_t@" << this;
      }
      ~http_t()
      {
          // LOG << *this << " ~" << this;
      }

      void start(ip::tcp::endpoint const &endp, boost::function<void (std::string)> fn)
      {
          // BOOST_LOG_FUNCTION();
          // LOG << *this << " connect " << endp;
          fn_ = fn;
          socket_.async_connect(endp, [this](boost::system::error_code ec) {
                      if (ec) {
                          log_err(ec, this, __LINE__);
                      } else {
                          // LOG << *this << " request";
                          sign_in();
                      }
                  });
      }

      void stop()
      {
          // LOG << *this << " stop";
          boost::system::error_code ec;
          socket_.close(ec);
      }

      void sign_in()
      {
          bufs_ = str(boost::format("GET /login"
                      "?systemVersion=1.0&deviceType=3&channel=3"
                      "&type=3&phone=%1%&password=%2% HTTP/1.1\r\n"
                      "Host: moon.kklink.com:9000\r\n"
                      "Connection: close\r\n"
                      "\r\n")
                  % login_.first % login_.second);
          io::async_write(socket_, io::buffer(bufs_),
                  [this](boost::system::error_code ec, size_t) {
                      if (ec) {
                          log_err(ec, this, __LINE__);
                      } else {
                          bufs_.clear();
                          read_response();
                      }
                  });
      }

      void read_response()
      {
          socket_.async_read_some(io::buffer(tmps_, sizeof(tmps_)),
                  [this](boost::system::error_code ec, size_t bytes) {
                      if (ec) {
                          log_err(ec, this, __LINE__);
                      } else {
                          bufs_.insert(bufs_.end(), &tmps_[0], &tmps_[bytes]);
                          std::string tok;
                          boost::logic::tribool b = parse_response_data(tok);
                          if (boost::logic::indeterminate(b)) {
                              read_response();
                          } else if (b) {
                              // BOOST_LOG_FUNCTION();
                              // LOG << *this << " end " << tok;
                              socket_.get_io_service().post( boost::bind(fn_, tok) );
                          } else {
                              LOG << *this << " fail";
                          }
                      }
                  });
      }

      boost::logic::tribool parse_response_data(std::string & token)
      {
          boost::iterator_range<std::string::iterator> eoh = boost::find_first(bufs_, "\r\n\r\n");
          if (boost::empty(eoh))
          {
              return boost::logic::indeterminate_keyword_t();
          }
          // LOG_I << io::const_buffer(bufs_.data(), eoh.end()-bufs_.begin());

          boost::iterator_range<std::string::iterator> cl = boost::ifind_first(bufs_, "content-Length");
          if (boost::empty(cl))
          {
              return false;
          }

          size_t clen = atoi(&cl.end()[2]);
          // LOG_I << "content-Length " << clen <<" "<< bufs_.size() - (eoh.end() - bufs_.begin());
          if (eoh.end() + clen > bufs_.end())
          {
              return boost::logic::indeterminate_keyword_t();
          }

          boost::iterator_range<std::string::iterator> body(eoh.end(), eoh.end() + clen);

          json::object js = json::decode<json::object>(body).value();
          std::string tok;
          tok = json::as<std::string>(js, "token").value();
          if (tok.empty())
              return false;
          token = tok;

          return true;
      }

      std::string bufs_;
      char tmps_[64];
      std::pair<std::string,std::string> login_;
      boost::function<void (std::string)> fn_;
      ip::tcp::socket socket_;

      friend std::ostream & operator<<(std::ostream & out, http_t const & c);
  };

  void timed_loop(boost::system::error_code ec)
  {
      if (ec == io::error::operation_aborted)
          return;
      stage old_stage = stage_;

      switch (stage_) {
      case stage::zero:
            if (!boost::empty(boost::get<3>(user_))) {
                stage_ = stage::connecting;
                break;
            }
            stage_ = stage::init;
      case stage::init:
          if (!http_) {
              if (n_http_c_ > 16) {
                  stage_tp_ = time(0);
                  break;
              }
              ++n_http_c_;
          } else if (stage_tp_ + 10 > time(0)) {
              break;
          }

          if (++stage_nloop_ > 10) {
              stage_ = stage::destroy;
              --n_http_c_;
              break;
          }
          stage_tp_ = time(0);
          http_ = std::make_shared<http_t>(io_service_, boost::get<1>(user_), boost::get<2>(user_));
          http_->start(http_endpoint_, [this] (std::string tok) {
                      boost::get<3>(user_) = tok;
                      stage_switch(stage::connecting);
                      --n_http_c_;
                      // LOG << *this << " " << tok <<" init done.";
                  });
          break;
      case stage::connecting:
          if (http_)
              http_.reset();
          if (stage_tp_ + 3 > time(0))
              break;
          ++stage_nloop_;
          stage_tp_ = time(0);
          {
              socket_.close(ec);
              socket_.open(ip::tcp::v4());
              socket_.async_connect(endpoint_, [this](boost::system::error_code ec) {
                  if (ec) {
                      log_err(ec, this, __LINE__);
                  } else {
                    stage_switch( stage::authorization );
                  }
              });
          }
          break;
      case stage::authorization:
          if (boost::get<3>(user_).empty()) {
              stage_ = stage::destroy;
              break;
          }
          auth();
          do_read_size();
          break;
      case stage::established:
          if (stage_tp_ + 55 < time(0)) {
              hello();
              ++stage_nloop_;
              stage_tp_ = time(0);
          }
          break;
      default:
          stage_ = stage::destroy;
          break;
      }

      if (stage_ != old_stage) // (|| time(0) - stage_tp_ > 20)
          LOG << *this <<" loop "<< n_http_c_;

      if (stage_ == stage::destroy) {
          io_service_.post([this]() {
                      clients_ptr->erase(user_.get<0>());
                  });
          return;
      }

      deadline_.expires_from_now(boost::posix_time::seconds(2));
      deadline_.async_wait(boost::bind(&yx_client::timed_loop, this, placeholders::error));
  }

  void stage_switch(stage st)
  {
      stage_ = st;
      stage_nloop_ = 0;
      stage_tp_ = (0);
  }

  void do_socket_error(boost::system::error_code ec, int lino)
  {
      log_err(ec, this, __LINE__);
      if (ec == io::error::operation_aborted)
          return;
      if (stage_ == stage::established) {
          stage_switch(stage::connecting);
      }
  }

  void do_read_size()
  {
    // std::cout << "do read size " << sizeof(read_size_) << "\n";
      io::async_read(socket_, io::buffer(&read_size_.s[0], sizeof(read_size_)),
        [this](boost::system::error_code ec, std::size_t /*length*/) {
          if (ec) {
            do_socket_error(ec, __LINE__);
          } else {
            do_read_data();
          }
        });
  }

  void do_read_data()
  {
    UInt siz = ntohl(read_size_.len);
    LOG << siz;
    read_msg_.resize( siz );
    io::async_read(socket_, io::buffer(const_cast<char*>(read_msg_.data()), read_msg_.size()),
        [this](boost::system::error_code ec, std::size_t /*length*/) {
            if (ec) {
                do_socket_error(ec, __LINE__);
            } else {
                LOG << read_msg_;
                json::object msg = json::decode<json::object>(read_msg_).value_or(json::object());
                auto ocmd = json::as<UInt>(msg,"cmd");
                if (ocmd) {
                  json::object body = json::as<json::object>(msg,"body").value_or(json::object());
                  process(ocmd.value(), body);
                }
                do_read_size();
            }
        });
  }

  void process(UInt cmd, const json::object & body)
  {
      LOG << cmd;
    switch (cmd)
    {
        case 200:
          ack( json::as<UInt>(body,"msgid").value_or(0), json::as<std::string>(body,"sid").value_or("") );
          break;
        case 99:
          if (stage_ == stage::authorization) {
              stage_switch( stage::established );
          }
          break;
    }
  }

  void ack(UInt sequence, std::string const& sid)
  {
    if (sequence == 0)
      return;

      // LOG << *this <<" ack "<< seqn;
    json::object req, body;
    json::insert(body)
        ("msgid", sequence)
        ("sid", sid)
        ("dest", boost::get<0>(user_))
        ;
    json::insert(req)
        ("version", std::string("1.0.0"))
        ("cmd", 95)
        ("body", body)
        ;
    std::string reqs = json::encode(req);

    decltype(read_size_) u;
    u.len = htonl(static_cast<UInt>(reqs.size()));
    this->write(std::string(&u.s[0],sizeof(u)) + reqs
            , "ack " + boost::lexical_cast<std::string>(sequence));
  }

  void auth()
  {
    json::object req, body;
    json::insert(body)
        ("appid", std::string("yx"))
        ("uid", boost::get<0>(user_))
        ("token", boost::get<3>(user_))
        ;
    json::insert(req)
        ("version", std::string("1.0.0"))
        ("cmd", 99)
        ("body", body)
        ;

    std::string data = json::encode(req);
    decltype(read_size_) u;
    u.len = htonl(data.size());

    this->write(std::string(&u.s[0],sizeof(u)) + data, "99");
  }
  void hello()
  {
    json::object req;
    json::insert(req)
        ("version", std::string("1.0.0"))
        ("cmd", 91)
        ;

    std::string data = json::encode(req);
    decltype(read_size_) u;
    u.len = htonl(data.size());

    this->write(std::string(&u.s[0],sizeof(u)) + data, "91");
  }

  void write(const std::string& msg, std::string const & hint)
  {
        // LOG << *this << " queue " << hint;
    io_service_.post(
        [this, msg, hint]() {
          bool write_in_progress = !write_msgs_.empty();
          write_msgs_.push_back( make_pair(msg, hint) );
          if (!write_in_progress)
            do_write();
        });
  }

  void do_write()
  {
    // std::cout << "do write " << write_msgs_.front() << "\n";
      io::async_write(socket_,
            const_buffer(write_msgs_.front().first), // io::buffer(write_msgs_.front().data(), write_msgs_.front().length()),
        [this](boost::system::error_code ec, std::size_t /*length*/) {
          if (ec) {
              log_err(ec, this, __LINE__);
              // close(ec); // socket_.close();
          } else {
            // stage_tp_ = time(0) + 55;
            std::string const & hint = write_msgs_.front().second;
            LOG << *this << " send " << hint;
            write_msgs_.pop_front();
            if (!write_msgs_.empty())
              do_write();
          }
        });
  }

private:
  boost::tuple<UInt,std::string,std::string,std::string> user_;
  // std::string token_;

  union { uint32_t len; char s[sizeof(uint32_t)]; } read_size_;
  std::string read_msg_;
  message_queue write_msgs_;

  io::io_service & io_service_;
  io::deadline_timer deadline_;
  ip::tcp::socket socket_;

  std::shared_ptr<http_t> http_;

  friend std::ostream & operator<<(std::ostream & out, yx_client const & c);
  friend std::ostream & operator<<(std::ostream & out, http_t const & c);

  static int n_http_c_;
public:
  bool operator<(yx_client const & r) const { return user_.get<0>() < r.user_.get<0>(); }

  static boost::ptr_map<UInt, yx_client> *clients_ptr;
};

std::ostream & operator<< (std::ostream & out, yx_client const & c)
{
    const char * stags[int(yx_client::stage::Max)] = {
        "zero"
        , "init"
        , "connecting"
        , "authorization"
        , "established"
        , "destroy"
    };
    out << c.user_.get<0>();
    if (c.stage_ == yx_client::stage::established)
        out <<"/"<< c.socket_.local_endpoint(); // <<"/"<< boost::get<3>(c.user_);
    return out <<"/"<< c.stage_nloop_ <<"/"<< stags[int(c.stage_)] <<"/"<< c.stage_tp_;
}

std::ostream & operator<<(std::ostream & out, yx_client::http_t const & c)
{
    return out << c.login_.first;
}

////
boost::ptr_map<UInt, yx_client> *yx_client::clients_ptr;
int yx_client::n_http_c_ = 0;

void read_user(boost::ptr_map<UInt, yx_client> & clients, io::io_service & io_s
        , std::string const & fn )
{
    std::vector<boost::ptr_map<UInt, yx_client>::iterator> vec;

    boost::filesystem::ifstream fin(fn);
    std::string line;
    while (std::getline(fin, line))
    {
        boost::regex e("^(\\d+)\\s+(\\S+)\\s+(\\S+)(\\s+(\\S+))?$");
        boost::smatch what;
        if (boost::regex_match(line, what, e))
        {
            LOG << what;
            UInt uid = boost::lexical_cast<UInt>(what[1]);
            std::string tok = boost::trim_copy(what[4].str());
            auto usr = boost::make_tuple(uid, what[2], what[3], tok);
            std::auto_ptr<yx_client> ptr( new yx_client(io_s, usr) );
            auto i = clients.insert(uid, ptr);
            if (i.second)
                vec.push_back(i.first);
        }
    }

    LOG << "n-client " << clients.size();

    for (auto i : vec)
        i->second->start();
}

//signals_.async_wait(&instance::signal);

//void instance::signal(const boost::system::error_code& er, int sig)
//{
//    G->signals_.async_wait(&instance::signal);
//}

#include <boost/program_options.hpp>
namespace po = boost::program_options;

std::string args(io::io_service & io_s, int ac, char * const av[])
{
    std::string host_ = "127.0.0.1";
    std::string port_ = "10011";
    // std::string imhost_ = "127.0.0.1"; std::string import_ = "9900";
  std::string fn; // std::pair<std::string,std::string> user;

  po::options_description opt_desc("Options");
  opt_desc.add_options()
    ("help", "display this help and exit")
    ("host", po::value<std::string>(&host_)->default_value(host_), "host to connect")
    ("port", po::value<std::string>(&port_)->default_value(port_), "service port")
    //("imhost", po::value<std::string>(&imhost_)->default_value(imhost_), "imhost")
    //("import", po::value<std::string>(&import_)->default_value(import_), "import")
    // ("mac", po::value<std::string>(&mac_)->default_value(mac_), "the macaddress")
    // ("chat", po::value<std::string>(&chat_groupid_)->default_value(chat_groupid_), "chat group-id")
    ("file,f", po::value<std::string>(&fn)->required(), "login user list in file")
    ;
  po::positional_options_description pos_desc;
  pos_desc.add("file", 1);

  po::variables_map vm;
  po::store(po::command_line_parser(ac, av)
      .options(opt_desc)
      .positional(pos_desc)
      .run(), vm);
  if (vm.count("help"))
  {
    std::cout << "Usage: a.out [options] <users-file>\n"
      << opt_desc;
    exit(0);
  }

  po::notify(vm);

  ip::tcp::resolver resolver(io_s);
  ip::tcp::resolver::iterator iter;

    iter = resolver.resolve({ host_, port_ });
    endpoint_ = *iter;

    LOG << endpoint_; // <<" "<< im_endpoint_;

  return fn;
}

int main(int ac, char* const av[])
{
  try
  {
      io::io_service io_s;
    boost::ptr_map<UInt, yx_client> clients;

    read_user(clients, io_s, args(io_s, ac, av)); // emplace_back
    yx_client::clients_ptr = &clients;

    io_s.run();

    std::cout << "bye.\n";
  }
  catch (boost::exception const & e)
  {
    std::cerr << "Exception: " << diagnostic_information(e) << "\n";
  }
  catch (std::exception const & e)
  {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}

