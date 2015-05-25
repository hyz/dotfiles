#include <iostream>
#include <string>
#include <boost/asio.hpp> 
#include <boost/shared_ptr.hpp>
#include <boost/smart_ptr.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <GraphicsMagick/Magick++.h>
#include <stdlib.h>

using namespace std;
using namespace boost;

typedef boost::function<void (const std::string&)> send_fn_type;
static boost::function<send_fn_type (Socket_ptr)> assoc_g_;

struct Writer_impl : boost::noncopyable, boost::enable_shared_from_this<Writer_impl>
{
    Writer_impl(Socket_ptr soc, boost::function<void (Socket_ptr)> fin);

    ~Writer_impl();

    void send(const std::string& s);

    private:
    Socket_ptr soc_;
    std::list<std::string> ls_;

    unsigned int cnt_;
    unsigned int size_;
    boost::function<void (Socket_ptr)> fin_;

    void writeb(const boost::system::error_code& err);
};

struct Writer
{
    typedef Writer_impl Worker;
    typedef boost::shared_ptr<Writer_impl> Worker_ptr;

    Writer(/*boost::asio::io_service& io_service*/) {}
    ~Writer();

    boost::function<void (const std::string&)> associate(Socket_ptr soc);

    void finish(Socket_ptr soc);

    private:
    typedef std::map<boost::asio::ip::tcp::socket::native_type, boost::weak_ptr<Worker> > Map;
    typedef Map::iterator iterator;

    Map workers_;
};

Writer_impl::~Writer_impl()
{
    LOG_I << __FILE__ << __LINE__;
    fin_(soc_);
    LOG_I << soc_ <<" "<< this <<" "<< size_ <<" "<< cnt_ <<" "<< ls_.empty() <<" "<< soc_.use_count();
    soc_->close();
    LOG_I << __FILE__ << __LINE__;
}

    Writer_impl::Writer_impl(Socket_ptr soc, boost::function<void (Socket_ptr)> fin)
    : soc_(soc)
      , fin_(fin)
{
    cnt_ = 0;
    size_ = 0;
    LOG_I << soc_ <<" "<< this <<" "<< soc_.use_count();
}

void Writer_impl::send(const std::string& s)
{
    bool empty = ls_.empty();

    if (s.empty())
    {
        if (!empty)
            ls_.erase(++ls_.begin(), ls_.end());
        return;
    }

    ls_.push_back(s);
    if (empty)
    {
        asio::async_write(*soc_, asio::buffer(ls_.front())
                , bind(&Writer_impl::writeb, shared_from_this(), asio::placeholders::error)
                );
    }

    LOG_I << soc_ <<" "<< size_ <<" "<< cnt_ <<" "<< ls_.empty() <<" "<< soc_.use_count();
}

void Writer_impl::writeb(const boost::system::error_code& err)
{
    if (err)
    {
        return ;//_error(obj, err, 9);
    }

    ++cnt_;
    size_ += ls_.front().size();
    ls_.pop_front();

    if (!ls_.empty())
    {
        asio::async_write(*soc_, asio::buffer(ls_.front())
                , bind(&Writer_impl::writeb, shared_from_this(), asio::placeholders::error)
                );
    }

    LOG_I << soc_ <<" "<< size_ <<" "<< cnt_ <<" "<< ls_.empty() <<" "<< soc_.use_count();
}

Writer::~Writer()
{
    LOG_I << __FILE__ << __LINE__;
    //for (iterator i = workers_.begin(); i != workers_.end(); ++i)
    //i->second->soc_->cancel();
}

void Writer::finish(Socket_ptr soc)
{
    workers_.erase(soc->native_handle());
    LOG_I << soc;
}

boost::function<void (const std::string&)>
Writer::associate(Socket_ptr soc)
{
    Worker_ptr ker;

    boost::weak_ptr<Worker> wptr(ker);
    std::pair<iterator, bool> ret = workers_.insert(std::make_pair(soc->native_handle(), wptr));
    if (ret.second)
    {
        ker.reset(new Worker(soc, bind(&Writer::finish, this, _1)));
    }
    else
    {
        ker = ret.first->second.lock();
        BOOST_ASSERT(ker);
    }

    LOG_I << soc <<" "<< soc.use_count();
    return bind(&Worker::send, ker, _1);
}

class download : public boost::enable_shared_from_this<download>
{
    public:
        download(boost::function<send_fn_type (Socket_ptr)> a,
                std::string path="/home/lindu/moon/trunk/download")
        {
            assoc_g_ = a;
            parent_ = path;
        }

        // connection::message_handler download_source0(connection::message const& m, connection& c)
        // {
        //     download_source(&c, m);
        //     return boost::bind(&download::download_source, this, &c, _1);
        // }
        void download_source(connection::message const& m, connection* const conn )
        {
            if(m.error()) {
                cout<<"error:"<<m.error()<<std::endl;
                return;
            }

            LOG_I << conn->socket()->remote_endpoint();
            if (!starts_with(m.path(), "file/")) return;
            // std::string::const_iterator i = std::find(m.path().begin()+5, m.path().end(), '/');
            string type = get(m.params(),"type","");

            string content = get_file(type, string(m.path().begin()+5,m.path().end()));
            if(!content.empty()){
            send_fn_type fn = assoc_g_(conn->socket());
            if (!fn.empty()) fn(content);
            }

        }
    private:
        string get_file(const string& type, filesystem::path fnp);
        // filesystem::path thumbnail(filesystem::path fnp, const string& source);
        // filesystem::path thumbnail(filesystem::path fnp);
        filesystem::path thumbnail(const filesystem::path& src, const filesystem::path& dst);
        filesystem::path thumbnail(const filesystem::path& fnp);
        string get_audio(const filesystem::path& fnp);
        filesystem::path get_img(filesystem::path& fnp);
        void handle_write(const boost::system::error_code& ec){}

    private:
        std::string parent_;
};

filesystem::path download::get_img(filesystem::path& fnp)
{
    if(filesystem::exists(fnp)) return fnp;

    const string& name = basename(fnp);
    if(!ends_with(name, "_thumb")){
        return std::string();
        // THROW_EX(EN_File_NotFound);
    }
    filesystem::path pwd = fnp.parent_path();
    filesystem::path source = pwd / (string(name.begin(),name.end()-6) + fnp.extension().string());
    if(!filesystem::exists(source)){
        // THROW_EX(EN_File_NotFound);
        return std::string();
    }
    fnp = thumbnail(source);
    cout<<"thumb path:"<<fnp.string()<<std::endl;
    return fnp;
}

string download::get_file(const string& type, filesystem::path fnp)
{
    filesystem::path p = parent_ / fnp;
    LOG_I<<"get_file, type:"<<type<<" ,path:"<<p.string();
    if ("img"==type ){
        p = get_img(p);
    }
    else if("ogg"==type){
        p = get_audio(p);
        cout<<"audio path:"<<p.string()<<std::endl;
    }

    if(!(p.string().empty()) && filesystem::exists(p)){
        ostringstream outs;
        std::ifstream ins(p.string().c_str());
        outs << ins.rdbuf();
        return outs.str();
    }
    else{
        return std::string();
        // THROW_EX(EN_File_NotFound);
    }
}

filesystem::path download::thumbnail(const filesystem::path& src, const filesystem::path& dst)
{
    ifstream source(src.string().c_str());
    ostringstream outs;
    outs << source.rdbuf();
    MagickLib::InitializeMagick(NULL);
    Magick::Blob block(outs.str().c_str(),outs.str().length());
    Magick::Image obj(block);

    obj.sample(Magick::Geometry("230x230"));
    obj.write(dst.string().c_str());
    cout<<" writed "<<dst.string()<<std::endl;
    return dst;
}

filesystem::path download::thumbnail(const filesystem::path& fnp)
{
    filesystem::path pwd = fnp.parent_path();
    string exten = fnp.extension().string();
    string base_name = basename(fnp) +"_thumb";
    filesystem::path thum_file = (pwd / base_name).string() + exten;
    cout<<"thumbnail:"<<thum_file.string()<<endl;

    return thumbnail(fnp, thum_file);
}

// string download::thumbnail(const filesystem::path fnp, const string& source)
// {
//     if(source.empty()) return string();
// 
//     filesystem::path pwd = fnp.parent_path();
//     string exten = fnp.extension().string();
//     string base_name = basename(fnp) +"_thumb";
//     string thum_file = (pwd / base_name).string() + exten;
//     cout<<"thumbnail:"<<thum_file<<" source length:"<<source.length()<<endl;
// 
//     MagickLib::InitializeMagick(NULL);
//     Magick::Blob block(source.c_str(), source.length());
// 
//     Magick::Image obj(block);
// 
//     obj.sample(Magick::Geometry("230x230"));
//     obj.write(thum_file);
//     cout<<" writed"<<std::endl;
//     return thum_file;
// }

// string download::thumbnail(filesystem::path fnp)
// {
//     ifstream source(fnp.string().c_str());
//     string content(istreambuf_iterator<char>(source), (istreambuf_iterator<char>()));
//     source.close();
// 
//     return thumbnail(fnp, content);
// }

string download::get_audio(const filesystem::path& fnp)
{
    filesystem::path pwd = fnp.parent_path();
    string base_name = basename(fnp);
    string new_audio = (pwd / base_name).string() + ".ogg";
    if(!filesystem::exists(fnp)){
        return string();
    }
    else{
        if(filesystem::exists(new_audio)){
            return new_audio;
        }
        cout<<"new_audio:"<<new_audio<<" source audio:"<<fnp.string()<<endl;
        string str = "ffmpeg -i "+fnp.string()+" "+new_audio;
        cout<<str<<std::endl;
        ::system(str.c_str());
        return new_audio;
    }
}
