#include <cstdlib>
#include <cstring>
#include <iostream>
#include <boost/array.hpp>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/algorithm/string/find.hpp>
#include <boost/range/iterator_range.hpp>
#include <boost/program_options.hpp>
//#include <boost/filesystem/path.hpp>
//#include <boost/filesystem/fstream.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/format.hpp>
#include "myerror.h"
#include "log.h"

using boost::asio::ip::tcp;
namespace placeholders = boost::asio::placeholders;
namespace posix = boost::asio::posix;
static int sigN_ = 0;

namespace error {
    struct fatal : myerror { using myerror::myerror; };
    struct signal : myerror { using myerror::myerror; };
    struct output_not_exist : myerror { using myerror::myerror; };

    // typedef boost::error_info<struct I,size_t> info_int;
    // typedef boost::error_info<struct S,std::string> info_str;
    // typedef boost::error_info<struct E,boost::system::error_code> info_ec;
    template <typename T> inline boost::error_info<T,T> info(T const& x) { return boost::error_info<T,T>(x); }

} // namespace error

//template <typename T>
//inline std::ostream& operator<<(std::ostream& out, std::vector<T> const& v)
//{
//    typedef std::vector<T>::const_iterator iterator;
//    return out << boost::make_iterator_range<iterator>(v);
//}

static int exec_redir_aux(int ret[2], std::vector<char*>& v, std::string a)
{
    v.push_back( const_cast<char*>(a.c_str()) );

    LOG << "exec" << v;

    int inp[2], outp[2];
    pipe(inp); // ON_SCOPE_EXIT
    pipe(outp); // ON_SCOPE_EXIT

    pid_t pid = fork();
    if (pid < 0) {
        LOG << "fork fail" << pid;
        return pid;
    }

    if (pid == 0) // child
    {
        close(outp[1]);
        if (outp[0] != STDIN_FILENO) {
            dup2(outp[0], STDIN_FILENO);
            close(outp[0]);
        }
        close(inp[0]);
        if (inp[1] != STDOUT_FILENO) {
            dup2(inp[1], STDOUT_FILENO);
            close(inp[1]);
        }

        v.push_back(0);
        int errN;
        execvp(v[0], &v[1]);
        errN = errno;
        if (errN == EACCES) {
            LOG << "exec EACCES" << errN << strerror(errN) << v[0];
            std::swap(v[0], v[1]);
            execv("/bin/sh", &v[0]);
            errN = errno;
        }
        LOG << "exec fail" << errN << strerror(errN) << v[0];
        kill(getppid(), SIGUSR1);
        exit(errN);
    }

    ret[0] = inp[0];
    close(inp[1]);
    ret[1] = outp[1];
    close(outp[0]);
    return pid;
}

template <typename ...Args>
static int exec_redir_aux(int ret[2], std::vector<char*>& v, std::string a, Args... args)
{
    v.push_back( const_cast<char*>(a.c_str()) );
    return exec_redir_aux(ret, v, args...);
}

template <typename ...Args>
static int exec_redir(int ret[2], std::string fp, Args... args)
{
    std::vector<char*> v{ const_cast<char*>(fp.c_str()) };
    return exec_redir_aux(ret, v, args...);
}

template <typename InputObject, typename Tag>
struct Input : InputObject
{
    typedef InputObject base;
    boost::array<char,512> buf;

    template <typename ...Params>
    Input(boost::asio::io_service& io_s, Params... args)
        : InputObject(io_s, args...)
    {}
};

//template <typename OutputObject>
//struct Output : OutputObject
//{
//    // std::vector<char> buf;
//    template <typename ...Params>
//    Input(boost::asio::io_service& io_s, Params... args)
//        : OutputObject(io_s, args...)
//    {}
//};

typedef boost::reference_wrapper<tcp::socket> socket_ref;

//inline template <typename S> S& base(Input<S>& s) { return s; }
//inline template <typename S> S const& base(Input<S> const& s) { return s; }

struct Args
{
    int argc;
    char* const* argv;
    std::string host_; //("127.0.0.1");
    std::string port_; // = "9999";
    std::string proto_;
    std::vector<std::string> apps_;
    bool listen_;

    Args(int ac, char *const av[])
        : argc(ac), argv(av)
    {
        namespace opt = boost::program_options;

        opt::variables_map vm;
        opt::options_description opt_desc("Options");
        opt_desc.add_options()
            ("help", "show this")
            ("listen,l", "listen mode")
            ("host,h", opt::value<std::string>(&host_)->default_value("127.0.0.1"), "host")
            ("port,p", opt::value<std::string>(&port_)->default_value("9999"), "port")
            ("protocol,x", opt::value<std::string>(&proto_)->default_value("cat"), "protocol format/translate")
            ("apps", opt::value<std::vector<std::string> >(&apps_)->required(), "apps")
            ;
        opt::positional_options_description pos_desc;
        pos_desc.add("apps", -1);

        opt::store(opt::command_line_parser(argc, argv).options(opt_desc).positional(pos_desc).run(), vm);

        if (vm.count("help")) {
            std::cout << boost::format("Usage:\n  %1% [Options]\n") % argv[0]
                << opt_desc
                ;
            exit(0);
        }
        opt::notify(vm);
        listen_ = vm.count("listen");
    }
};

namespace placeholders = boost::asio::placeholders;

struct Main : boost::asio::io_service
{
#define DECL_TAG(X) struct X{ friend std::ostream& operator<<(std::ostream& out,X const& x) {return out<< #X;} }
    DECL_TAG(Soc); DECL_TAG(Dec); DECL_TAG(Enc); DECL_TAG(App);

    boost::asio::ip::tcp::acceptor acceptor_;

    Input<tcp::socket,Soc> socket_;
    posix::stream_descriptor dec_out_;
    Input<posix::stream_descriptor,Dec> dec_in_;
    std::vector<posix::stream_descriptor> outs_;
    std::vector<Input<posix::stream_descriptor,App>> ins_;
    std::vector<posix::stream_descriptor> enc_outs_; // posix::stream_descriptor enc_out_;
    std::vector<Input<posix::stream_descriptor,Enc>> enc_ins_; // Input<posix::stream_descriptor,Enc> enc_in_;

    int idx_;
    boost::asio::signal_set signals_;

    Main(Args const& args); // (int argc, char *const argv[])
    // ~Main() { this->close(); }

    posix::stream_descriptor& get_output(Input<tcp::socket,Soc>& sk_in) { return dec_out_; }
    posix::stream_descriptor& get_output(Input<posix::stream_descriptor,Dec>& dec_in) { return outs_[idx_]; }
    posix::stream_descriptor& get_output(Input<posix::stream_descriptor,App>& in) { return enc_outs_[idx_]; }
    tcp::socket& get_output(Input<posix::stream_descriptor,Enc>& enc_in) { return socket_; }

    template <typename S,typename T>
    boost::system::error_code input_error(boost::system::error_code ec, Input<S,T>&) const
    {
        LOG << ec << ec.message() << T();
        return ec;
    }
    boost::system::error_code input_error(boost::system::error_code ec, Input<posix::stream_descriptor,App>&);
    template <typename In>
    boost::system::error_code output_error(boost::system::error_code ec, In&) const
    {
        LOG << ec << ec.message();
        return ec;
    }

    void handle_accept(boost::system::error_code ec);
    void handle_connect(boost::system::error_code ec);
    void run();
};

static std::string make_arg0(std::string pf, std::string cf)
{
    {
        auto x = pf.find_last_of('/');
        if (x != std::string::npos) {
            pf.erase(0, x+1);
        }
    }{
        auto x = cf.find_last_of('/');
        if (x != std::string::npos) {
            cf.erase(0, x+1); // cf = std::string(it.base(), cf.end());
        }
    }
    return pf + '.' + cf;
}

Main::Main(Args const& args) // (int argc, char *const argv[])
    : acceptor_(*this)
    , socket_(*this)
    , dec_out_(*this), dec_in_(*this)
    , signals_(*this, SIGHUP, SIGUSR1, SIGCHLD)
{
    boost::asio::io_service& io_s = *this;

    ins_.reserve(args.apps_.size());
    outs_.reserve(args.apps_.size());
    enc_ins_.reserve(args.apps_.size());
    enc_outs_.reserve(args.apps_.size());

    {
        int v[2] = {0,0};
        if (exec_redir(v, args.proto_, make_arg0(args.argv[0],args.proto_)) < 0) {
            BOOST_THROW_EXCEPTION(error::fatal() << error::info(args.proto_));
        }
        dec_in_.assign(v[0]);
        dec_out_.assign(v[1]);
    }

    BOOST_FOREACH(auto &fp, args.apps_)
    {
        {
            int v[2] = {0,0};
            if (exec_redir(v, fp, make_arg0(args.argv[0],fp)) < 0) {
                BOOST_THROW_EXCEPTION(error::fatal() << error::info(fp));
            }
            ins_.emplace_back(io_s, v[0]);
            outs_.emplace_back(io_s, v[1]);
        }{
            int v[2] = {0,0};
            if (exec_redir(v, args.proto_, make_arg0(args.argv[0],args.proto_), "-e") < 0) {
                BOOST_THROW_EXCEPTION(error::fatal() << error::info(args.proto_));
            }
            enc_ins_.emplace_back(io_s, v[0]);
            enc_outs_.emplace_back(io_s, v[1]);
        }
    }

    tcp::resolver resolv_(*this);
    auto it = resolv_.resolve(tcp::resolver::query(args.host_, args.port_));
    tcp::endpoint endp = *it;

    if (args.listen_) {
        acceptor_.open(endp.protocol());
        acceptor_.set_option(tcp::acceptor::reuse_address(true));
        acceptor_.bind(endp);
        acceptor_.listen();
        acceptor_.async_accept(socket_, bind(&Main::handle_accept, this, placeholders::error));
    } else {
        socket_.async_connect(endp, boost::bind(&Main::handle_connect, this, placeholders::error));
    }
}

template <typename Input>
struct Mcontext
{
    Main* main_;
    Input* in_;
};

template <typename Input>
struct handle_write : Mcontext<Input>
{
    handle_write(Main& m, Input& in) : Mcontext<Input>{&m, &in}
    {}
    void operator()(boost::system::error_code ec, size_t bytes);
};

template <typename Input>
struct handle_read : Mcontext<Input>
{
    handle_read(Main& m, Input& in) : Mcontext<Input>{&m, &in}
    {}
    void operator()(boost::system::error_code ec, size_t bytes)
    {
        Main& m = *this->main_; Input& in = *this->in_;
        if (ec) {
            if (m.input_error(ec,in)) {
                BOOST_THROW_EXCEPTION(error::fatal() << error::info(ec));
            }
            return;
        }
        // bufsiz_ += bytes; in_.buf.resize(bufsiz_);
        auto& out = m.get_output(in);
        boost::asio::async_write(out, boost::asio::buffer(in.buf, bytes), handle_write<Input>{m,in});
    }
};

template <typename Input>
void start(Main& m, Input& in)
{
    handle_read<Input> b{m, in};
    in.async_read_some(boost::asio::buffer(in.buf), handle_read<Input>(m,in));
}

template <typename Input>
void handle_write<Input>::operator()(boost::system::error_code ec, size_t bytes)
{
    Main& m = *this->main_; Input& in = *this->in_;
    if (ec) {
        if (m.output_error(ec,in)) {
            BOOST_THROW_EXCEPTION(error::fatal() << error::info(ec));
        }
        return;
    }
    start(m, in); // in.async_read_some(boost::asio::buffer(in.buf), handle_read<Input>(m,in));
}

void Main::run()
{
    signals_.async_wait([this](boost::system::error_code ec, int sig) {
            if (ec) {
                LOG << ec << ec.message();
                return;
            }
            LOG << sig;
            if (sig==SIGUSR1) BOOST_THROW_EXCEPTION(error::signal() << error::info(sig));
        });

    boost::asio::io_service::run();
}

void Main::handle_accept(boost::system::error_code ec)
{
}

void Main::handle_connect(boost::system::error_code ec)
{
    if (ec) {
        LOG << ec << ec.message();
        BOOST_THROW_EXCEPTION(error::fatal() << error::info(ec));
    }
    // LOG << endp_ << "connected";

    idx_ = 0;

    start(*this, socket_);
    start(*this, dec_in_);
    start(*this, ins_[idx_]);
    start(*this, enc_ins_[idx_]);
}

boost::system::error_code Main::input_error(boost::system::error_code ec, Input<posix::stream_descriptor,App>&)
{
    if (ec != boost::asio::error::eof) {
        return ec;
    }
    idx_++;
    start(*this, ins_[idx_]);
    start(*this, enc_ins_[idx_]);
    return boost::system::error_code();
}

int main(int argc, char* const argv[])
{
    try
    {
        Main m(Args(argc, argv));
        m.run();
    }
    catch (myerror& e)
    {
        std::cerr << "Exception: " << e.what() << "\n";
    }
    if (sigN_) {
        LOG << sigN_;
    }

    LOG << "bye.";
    return 0;
}

