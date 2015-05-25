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
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/regex.hpp>
#include <boost/asio.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/logic/tribool.hpp>
#include <boost/format.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/posix_time/posix_time_io.hpp>

#if defined(__clang__) || !defined(USE_BOOST_LOG)
#define LOG if(1) std::clog <<"\n"<<time(0)<<" "<< std::flush
#define log_init() ((void)0)

#else // __clang__
#include <boost/utility/empty_deleter.hpp>
#include <boost/log/common.hpp>
#include <boost/log/sinks.hpp>
#include <boost/log/sources/logger.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/attributes.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/support/date_time.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>

BOOST_LOG_INLINE_GLOBAL_LOGGER_DEFAULT(logger_, boost::log::sources::logger)

// BOOST_LOG_ATTRIBUTE_KEYWORD(AK_chat_client, "Obj_chat_client", chat_client*)
BOOST_LOG_ATTRIBUTE_KEYWORD(timestamp_, "TimeStamp", boost::posix_time::ptime)
BOOST_LOG_ATTRIBUTE_KEYWORD(scope_, "Scope", boost::log::attributes::named_scope::value_type)

static void log_formatter(boost::log::record_view const& rec, boost::log::formatting_ostream& strm)
{
    using namespace boost::log;

    // char ts[12];
    // std::time_t t = std::time(NULL);
    // std::strftime(ts, sizeof(ts), "%H%M%S", std::localtime(&t));
    // strm << ts;
    // boost::posix_time::ptime now = boost::posix_time::second_clock::local_time();
    //strm << expressions::format_date_time< boost::posix_time::ptime >("TimeStamp", "%d.%H:%M:%S.%f");
    strm << extract(timestamp_, rec);
    strm << extract(scope_, rec);
    // strm << expressions::format_date_time(extract(timestamp_, rec), keywords::format = "%d.%H:%M:%S.%f");

    // strm << aux::default_attribute_names::timestamp();
    // attributes::local_clock clock;
    // strm << expressions::format_date_time< boost::posix_time::ptime >(clock.get_value(), "%d.%m.%Y %H:%M:%S.%f");

    auto & attrs = rec.attribute_values();
    auto it = attrs.find("Obj_chat_client") ; //(AK_chat_client);
    if (it != attrs.end())
    {
        strm <<" ";
        // logging::value_ref<chat_client*> val = logging::extract< chat_client* >(attr);
        // chat_client::log(strm, logging::extract< chat_client* >(attr).get());
    }

    // The same for the severity level.
    // The simplified syntax is possible if attribute keywords are used.
    // strm << "<" << rec[logging::trivial::severity] << "> ";

    // Finally, put the record message to the stream
    strm <<": "<< rec[expressions::smessage];
    //strm << expressions::format_named_scope(extract(scope_,rec), keywords::format = " %n:%l %f");
    // strm << logging::extract< unsigned int >("LineID", rec) << " ";
}

void log_init()
{
    using namespace boost::log;

    typedef sinks::synchronous_sink< sinks::text_ostream_backend > text_sink;
    boost::shared_ptr<text_sink> sink = boost::make_shared<text_sink>();

    boost::shared_ptr<std::ostream> clog(&std::clog, boost::empty_deleter());
    sink->locked_backend()->add_stream( clog );
    sink->locked_backend()->add_stream( boost::make_shared<std::ofstream>("moon.log") );
    sink->locked_backend()->auto_flush(true);

    // sink->set_formatter(&log_formatter);
    sink->set_formatter(
                expressions::stream
                    << expressions::format_date_time(timestamp_, "%d.%H:%M:%S.%f")
                    <<" "<< expressions::format_named_scope(scope_, keywords::format = "%n (%f:%l)")
                    <<": "<< expressions::message
            );

    auto pc = core::get();

    pc->add_sink(sink);
    pc->add_global_attribute("Scope", attributes::named_scope());
    pc->add_global_attribute("TimeStamp", attributes::local_clock());

    //pc->add_global_attribute(aux::default_attribute_names::line_id(), attributes::counter< unsigned int >(1));
    //pc->add_global_attribute(aux::default_attribute_names::timestamp(), attributes::local_clock());

    // add_common_attributes();
    //attributes::local_clock TimeStamp;
    //core::get()->add_global_attribute("TimeStamp", TimeStamp);

    // auto & strm = ::boost::log::trivial::logger::get();
    // strm.imbue(std::locale(strm.getloc(), new boost::posix_time::time_facet("%d-%b-%Y %H:%M:%S")));
}

#define LOG BOOST_LOG(logger_::get())
#endif

using boost::asio::ip::tcp;
namespace asio = boost::asio;
namespace placeholders = boost::asio::placeholders;

typedef std::deque<std::pair<std::string,std::string> > message_queue;

// static std::string mac_ = "00:11:22";
static std::string spot_groupid_; // = "KK1007A";
// static std::string chat_groupid_; // = "KK1007A";
static tcp::endpoint http_endpoint_;
static tcp::endpoint im_endpoint_;


namespace json {
    using boost::property_tree::ptree;

    std::string encode(boost::property_tree::ptree const & pt)
    {
      std::ostringstream oss;
      boost::property_tree::json_parser::write_json(oss, pt, false);
      return oss.str();
    }

    template <typename I> ptree decode(I begin, I end)
    {
      ptree pt;
      std::istringstream iss(std::string(begin, end));
      boost::property_tree::json_parser::read_json(iss, pt);
      return pt;
    }
}

inline std::ostream & operator<<(std::ostream & out, boost::system::error_code const * ec)
{
    return out << *ec <<":"<< ec->message();
}


template <typename T>
void log_err(boost::system::error_code const & ec, T* thiz, int lino)
{
  if (ec == boost::asio::error::operation_aborted) {
    LOG << &ec << " err#" << lino;
  } else {
    LOG << *thiz <<" "<< &ec <<" err#" << lino;
  }
}

template <typename T>
inline boost::asio::const_buffers_1 const_buffer(T const & m) { return boost::asio::buffer(m); }

class chat_client
{
    // boost::log::source::logger lg_;

    enum struct stage {
        zero = 0
        , init
        , connecting
        , established
        , destroy
        , Max
    };
    stage stage_;
    time_t stage_tp_;
    int n_repeat_;

public:
  chat_client(asio::io_service& io_service, boost::tuple<std::string,std::string,std::string> user)
    : user_(user)
      , io_service_(io_service)
      , deadline_(io_service)
      , socket_(io_service)
  {
    stage_ = stage::zero;
    n_repeat_ = 0;
    stage_tp_ = 0;
    // lg_.add_attribute("User", boost::log::attributes::make_constant(user_.first));
  }

  void start(tcp::endpoint const & endp) // void start(tcp::resolver::iterator endpoint_iterator)
  {
    if (stage_ != stage::zero)
        return;
    endp_ = endp;
    deadline_.expires_from_now(boost::posix_time::seconds(1));
    deadline_.async_wait(boost::bind(&chat_client::timed_loop, this, placeholders::error));
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
      http_t(asio::io_service & io_s, std::string user, std::string pwd)
          : login_(std::make_pair(user,pwd))
          , socket_(io_s)
      {
          // LOG << login_.first << "  http_t@" << this;
      }
      ~http_t()
      {
          // LOG << *this << " ~" << this;
      }

      void start(tcp::endpoint const &endp, boost::function<void (std::string)> fn)
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
          asio::async_write(socket_, asio::buffer(bufs_),
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
          socket_.async_read_some(asio::buffer(tmps_, sizeof(tmps_)),
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
          // LOG_I << boost::asio::const_buffer(bufs_.data(), eoh.end()-bufs_.begin());

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

          json::ptree js = json::decode(body.begin(), body.end());
          std::string tok;
          tok = js.get<std::string>("token", tok);
          if (tok.empty())
              return false;
          token = tok;

          return true;
      }

      std::string bufs_;
      char tmps_[64];
      std::pair<std::string,std::string> login_;
      boost::function<void (std::string)> fn_;
      tcp::socket socket_;

      friend std::ostream & operator<<(std::ostream & out, http_t const & c);
  };

  void timed_loop(boost::system::error_code ec)
  {
      if (ec == boost::asio::error::operation_aborted)
          return;
      stage old_stage = stage_;

      switch (stage_) {
      case stage::zero:
            if (!boost::empty(boost::get<2>(user_))) {
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

          if (++n_repeat_ > 10) {
              stage_ = stage::destroy;
              --n_http_c_;
              break;
          }
          stage_tp_ = time(0);
          http_ = std::make_shared<http_t>(io_service_, boost::get<0>(user_), boost::get<1>(user_));
          http_->start(endp_, [this] (std::string tok) {
                  boost::get<2>(user_) = tok;
                      stage_ = stage::connecting;
                      n_repeat_ = 0;
                      stage_tp_ = 0;
                      --n_http_c_;
                      // LOG << *this << " " << tok <<" init done.";
                  });
          break;
      case stage::connecting:
          if (http_)
              http_.reset();
          if (stage_tp_ + 6 > time(0))
              break;
          ++n_repeat_;
          stage_tp_ = time(0);
          {
              // tcp::endpoint endp(endp_.address(), import_);
              // LOG << endp;
              socket_.close(ec);
              socket_.open(tcp::v4());
              socket_.async_connect(im_endpoint_, [this](boost::system::error_code ec) {
                  if (ec) {
                      log_err(ec, this, __LINE__);
                  } else {
                    stage_ = stage::established;
                    n_repeat_ = 0;
                    stage_tp_ = 0;

                    do_read_size();
                  }
              });
          }
          break;
      case stage::established:
          if (boost::get<2>(user_).empty()) {
              stage_ = stage::destroy;
              break;
          }
          if (stage_tp_ + 55 > time(0))
              break;
          ++n_repeat_;
          stage_tp_ = time(0);
          hello();
          break;
      default:
          stage_ = stage::destroy;
          break;
      }
      if (stage_ != old_stage) // (|| time(0) - stage_tp_ > 20)
          LOG << *this <<" loop "<< n_http_c_;

      if (stage_ == stage::destroy) {
          io_service_.post([this]() {
                      std::string k = user_.get<0>();
                      clients_ptr->erase(k);
                  });
          return;
      }

      deadline_.expires_from_now(boost::posix_time::seconds(2));
      deadline_.async_wait(boost::bind(&chat_client::timed_loop, this, placeholders::error));
  }

  void do_socket_error(boost::system::error_code ec, int lino)
  {
      log_err(ec, this, __LINE__);
      if (ec == boost::asio::error::operation_aborted)
          return;
      if (stage_ == stage::established) {
          stage_ = stage::connecting;
          n_repeat_ = 0;
          stage_tp_ = time(0);
      }
  }

  void do_read_size()
  {
    // std::cout << "do read size " << sizeof(read_size_) << "\n";
    asio::async_read(socket_, asio::buffer(&read_size_.s[0], sizeof(read_size_)),
        [this](boost::system::error_code ec, std::size_t /*length*/) {
          if (!ec) {
            read_size_.h[0] = ntohs(read_size_.h[0]);
            read_size_.h[1] = ntohs(read_size_.h[1]);
            read_msg_.resize( read_size_.h[0] + read_size_.h[1] );
            do_read_data();
          } else {
            do_socket_error(ec, __LINE__);
          }
        });
  }

  void do_read_data()
  {
    asio::async_read(socket_, asio::buffer(const_cast<char*>(read_msg_.data()), read_msg_.size()),
        [this](boost::system::error_code ec, std::size_t /*length*/) {
          if (ec) {
              do_socket_error(ec, __LINE__);
          } else {
            json::ptree head = json::decode(&read_msg_[0], &read_msg_[read_size_.h[0]]);
            ack( head.get<int>("sequence") );

            if (!boost::starts_with(head.get<std::string>("method"), "data/")) {
              json::ptree body = json::decode(&read_msg_[read_size_.h[0]], &read_msg_[read_msg_.size()]);
              process(head, body);
            }

            do_read_size();
          }
        });
  }

  void process(json::ptree const & head, const json::ptree & body)
  {
      const std::string & meth = head.get<std::string>("method");
      unsigned int sequence = head.get<int>("sequence");
      if (meth == "chat/text") {
          LOG << *this <<" "<< sequence <<" "<< meth <<" "<< body.get<std::string>("sid");
          ;
      } else {
          LOG << *this <<" "<< sequence <<" "<< meth;
          if (meth == "chat/in-spot") {
              spot_groupid_ = body.get<std::string>("content");
          } else if (meth == "chat/out-spot") {
              spot_groupid_ = std::string();
          } else if (meth == "warn/other-login") {
              LOG << *this <<" "<< boost::get<2>(user_) <<" err# "<< json::encode(body);
              boost::get<2>(user_).clear();
          }
      }
  }

  void ack(unsigned int sequence)
  {
    if (sequence == 0)
      return;

      // LOG << *this <<" ack "<< seqn;
    json::ptree head;
    head.put("token", boost::get<2>(user_));
    head.put("method", "ack");
    head.put("sequence", sequence);

    std::string heads = boost::replace_all_regex_copy(json::encode(head)
        , boost::regex("\"sequence\":\"([0-9]+?)\""), std::string("\"sequence\":$1"));

    // time_t t = time(0);
    // std::cout << "<<< "<< heads <<" "<< ctime(&t) << std::endl;

    decltype(read_size_) u;
    u.h[0] = htons(static_cast<short>(heads.size()));
    u.h[1] = htons(2);
    this->write(std::string(&u.s[0],sizeof(u)) + heads + "{}"
            , "ack " + boost::lexical_cast<std::string>(sequence));
  }

  void hello()
  {
    json::ptree head, body;
    head.put("method", "hello");
    head.put("token", boost::get<2>(user_));

    if (!spot_groupid_.empty()) {
        body.put("spotsid",spot_groupid_);
    // if (!chat_groupid_.empty())
        body.put("usingsid",spot_groupid_);
    }

    std::string heads = json::encode(head);
    std::string bodys = json::encode(body);
    decltype(read_size_) u;
    u.h[0] = htons(static_cast<short>(heads.size()));
    u.h[1] = htons(static_cast<short>(bodys.size()));

    this->write(std::string(&u.s[0],sizeof(u)) + heads + bodys, "hello");
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
    asio::async_write(socket_,
            const_buffer(write_msgs_.front().first), // asio::buffer(write_msgs_.front().data(), write_msgs_.front().length()),
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
  boost::tuple<std::string,std::string,std::string> user_;
  // std::string token_;

  union { uint16_t h[2]; char s[4]; uint32_t len; } read_size_;
  std::string read_msg_;
  message_queue write_msgs_;

  asio::io_service & io_service_;
  tcp::endpoint endp_;
  asio::deadline_timer deadline_;
  tcp::socket socket_;

  std::shared_ptr<http_t> http_;

  friend std::ostream & operator<<(std::ostream & out, chat_client const & c);
  friend std::ostream & operator<<(std::ostream & out, http_t const & c);

  static int n_http_c_;
public:
  bool operator<(chat_client const & r) const { return user_.get<0>() < r.user_.get<0>(); }

  static boost::ptr_map<std::string, chat_client> *clients_ptr;
};

std::ostream & operator<< (std::ostream & out, chat_client const & c)
{
    const char * stags[int(chat_client::stage::Max)] = {
        "zero"
        , "init"
        , "connecting"
        , "established"
        , "err# destroy"
    };
    out << c.user_.get<0>();
    if (c.stage_ == chat_client::stage::established)
        out <<"/"<< c.socket_.local_endpoint() <<"/"<< boost::get<2>(c.user_);
    return out <<"/"<< c.n_repeat_ <<"/"<< stags[int(c.stage_)] <<"/"<< c.stage_tp_;
}

std::ostream & operator<<(std::ostream & out, chat_client::http_t const & c)
{
    return out << c.login_.first;
}


boost::ptr_map<std::string, chat_client> *chat_client::clients_ptr;
int chat_client::n_http_c_ = 0;

void read_user(boost::ptr_map<std::string, chat_client> & clients, asio::io_service & io_service
        , std::string const & fn )
{
    std::vector<boost::ptr_map<std::string, chat_client>::iterator> vec;

    std::ifstream fin(fn.c_str());
    std::string line;
    while (std::getline(fin, line))
    {
        boost::regex e("^(\\d+)\\s+(\\S+)\\s+(\\S+)(\\s+(\\S+))?$");
        boost::smatch what;
        if (boost::regex_match(line, what, e))
        {
            LOG << what;
            std::string tok = boost::trim_copy(what[4].str());
            auto usr = boost::make_tuple(what[2], what[3], tok);
            std::auto_ptr<chat_client> ptr( new chat_client(io_service, usr) );
            auto i = clients.insert(what[2], ptr);
            if (i.second)
                vec.push_back(i.first);
        }
    }

    LOG << "n-client " << clients.size();

    for (auto i : vec)
        i->second->start(http_endpoint_);
}

#include <boost/program_options.hpp>
namespace po = boost::program_options;

std::string args(asio::io_service & io_service, int ac, char * const av[])
{
    std::string host_ = "127.0.0.1";
    std::string imhost_ = "127.0.0.1";
    std::string port_ = "9000";
    std::string import_ = "9900";
  std::string fn; // std::pair<std::string,std::string> user;

  po::options_description opt_desc("Options");
  opt_desc.add_options()
    ("help", "display this help and exit")
    ("host", po::value<std::string>(&host_)->default_value(host_), "host to connect")
    ("port", po::value<std::string>(&port_)->default_value(port_), "http port")
    ("imhost", po::value<std::string>(&imhost_)->default_value(imhost_), "imhost")
    ("import", po::value<std::string>(&import_)->default_value(import_), "import")
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

    tcp::resolver resolver(io_service);
    tcp::resolver::iterator iter;

    iter = resolver.resolve({ host_, port_ });
    http_endpoint_ = *iter;
    iter = resolver.resolve({ imhost_, import_ });
    im_endpoint_ = *iter;

    LOG << http_endpoint_ <<" "<< im_endpoint_;

  return fn;
}

int main(int ac, char* const av[])
{
  try
  {
      log_init();

    asio::io_service io_service;
    boost::ptr_map<std::string, chat_client> clients;

    read_user(clients, io_service, args(io_service, ac, av)); // emplace_back
    chat_client::clients_ptr = &clients;

    // c.hello();

    // std::thread t([&io_service](){ io_service.run(); });
    // while (c.is_open())
    //{
        // c.hello();
        // std::this_thread::sleep_for(std::chrono::seconds(30));
    //}

    // char line[chat_message::max_body_length + 1];
    // while (std::cin.getline(line, chat_message::max_body_length + 1))
    // {
    //   chat_message msg;
    //   msg.body_length(std::strlen(line));
    //   std::memcpy(msg.body(), line, msg.body_length());
    //   msg.encode_header();
    //   c.write(msg);
    // }

    // boost::system::error_code ec;
    // c.close(ec);
    //
    // t.join();

    io_service.run();

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

