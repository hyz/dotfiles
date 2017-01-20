#ifndef _LOG_H_
#define _LOG_H_

#ifdef linux
#  include <syslog.h>
#  if !defined(LOG_USER) || !defined(LOG_INFO) || !defined(LOG_PID) || !defined(LOG_CONS)
#    error "!defined(LOG_USER) || !defined(LOG_INFO) || !defined(LOG_PID) || !defined(LOG_CONS)"
#  endif
#endif
#include <cstdio>
#include <set>
#include <vector>
#include <ostream>
#include <utility>
#include <algorithm>
#include <boost/thread/mutex.hpp>
#include <boost/ref.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/foreach.hpp>
#include <boost/noncopyable.hpp>
#include <boost/container/static_vector.hpp>
#include <boost/range/iterator_range.hpp>

#include <boost/iostreams/categories.hpp>
#include <boost/iostreams/stream.hpp>
//#include <iostream>

template <typename T>
std::ostream& operator<<(std::ostream& out, std::vector<T> const& v)
{
    if (!v.empty()) {
        auto it = v.begin();
        out << *it;
        for (++it; it != v.end(); ++it)
            out << " " << *it;
    }
    return out;
}

template <typename T>
std::ostream& operator<<(std::ostream& out, std::set<T> const& v)
{
    if (!v.empty()) {
        auto it = v.begin();
        out << *it;
        for (++it; it != v.end(); ++it)
            out << " " << *it;
    }
    return out;
}

template <typename X, typename Y>
std::ostream & operator<<(std::ostream & out, std::pair<X,Y> const & p)
{
    return out << p.first <<" "<< p.second;
}

inline std::ostream& operator<<(std::ostream& out, boost::asio::const_buffer const & buf)
{
    out.write(boost::asio::buffer_cast<char const*>(buf), boost::asio::buffer_size(buf));
    return out;
}
std::ostream& operator<<(std::ostream& out, boost::asio::const_buffers_1 const & bufs);

namespace logging {

template <typename T=void>
struct Flags {
    static bool syslog;
	static FILE* fp;
};
template <typename T> bool Flags<T>::syslog = 0;
template <typename T> FILE* Flags<T>::fp = NULL;

template<typename Container>
struct logger2_sink
{
    typedef typename Container::value_type  char_type;
    typedef boost::iostreams::sink_tag      category;

    logger2_sink(Container& container) : container_(container) { }

    std::streamsize write(const char_type* s, std::streamsize n)
    {
        std::streamsize m = container_.capacity() - container_.size();
        if (m > 0) {
            container_.insert(container_.end(), s, s + std::min(m, n));
        }
        // std::cout << &container_ <<" "<< container_.capacity() <<" "<< container_.size() << "\n";
        return n;
    }
    // Container& container() { return container_; }
private:
    logger2_sink operator=(const logger2_sink&);
    Container& container_;
};

template<class Tag>
struct logger_stream : boost::noncopyable
{
    typedef boost::container::static_vector<char,Tag::capacity> buffer_type;
    typedef boost::iostreams::stream<logger2_sink<boost::container::static_vector<char,Tag::capacity> > > stream_type;
    buffer_type buffer_;
    stream_type stream_; // boost::iostreams::stream<logger2_sink<boost::container::static_vector<char,Tag::capacity> > >

    logger_stream() : stream_(buffer_) // ( static_cast<buffer_type&>(*this) )
        { sizepfx=0; }

    void commit(int line, char const *name)
    {
		this->stream_.flush(); // flush to sink bufs

        // std::cout << &bufs <<" "<< bufs.capacity() <<" "<< bufs.size() <<" "<< sizepfx <<"\n";
        if (buffer_.size() > sizepfx)
        {
            int len = buffer_.size();
            char *p = &buffer_[0];
            if (Flags<>::syslog) {
#               if defined(LOG_USER) && defined(LOG_INFO) && defined(LOG_PID) && defined(LOG_CONS)
				char const* fmt = "%.*s";
				if (line && name)
					fmt = "%.*s #%d:%s";
                ::syslog(LOG_USER|LOG_INFO, fmt, len, p, line, name);
#               endif
            }
            if (Flags<>::fp) {
				std::fwrite(p, len, 1, Flags<>::fp);
				if (line && name)
					std::fprintf(Flags<>::fp, "\t#%d:%s\n", line, name);
				else
					std::fwrite("\n", 1, 1, Flags<>::fp);
				std::fflush(Flags<>::fp);
            }
            buffer_.resize(sizepfx); // clear(); seekp(sizepfx, std::ios::beg);
        }
    }

    template <typename T>
    void prefix(T const& t)
    {
        if (sizepfx >= 1) {
            buffer_.resize(buffer_.size() - 1);
            this->stream_ << " ";
        }
        this->stream_ << t << ":";
        sizepfx = buffer_.size();
    }

    template <typename T, typename ...Args>
    void prefix(T const& t, Args const& ...params)
    {
        prefix(t);
        prefix(params...);
    }

    // boost::container::static_vector<char,Tag::capacity> bufs;
    // logger2_sink sink;
    unsigned short sizepfx, _;
    boost::mutex mutex_;

    static logger_stream<Tag> instance;
};
template <typename Tag> logger_stream<Tag> logger_stream<Tag>::instance;

template <typename T>
struct lock_helper : boost::unique_lock<boost::mutex>
{
    lock_helper(logger_stream<T>& ls)
        : boost::unique_lock<boost::mutex>(ls.mutex_)
        , ls_(&ls)
    {}

    std::ostream& stream() const { return ls_->stream_; }
    void commit(int ln, char const* nm) const {
        ls_->commit(ln, nm);
    }

    logging::logger_stream<T>* ls_;
};

template <typename T>
struct logger_helper : lock_helper<T>
{
    int line_;
    const char* fname_;

    template <typename X>
    logger_helper<T> const& operator<<(X const& x) const {
        static_cast<std::ostream&>(this->stream()) <<" "<< x;
        return *this;
    }
    ~logger_helper() {
        this->commit(line_, fname_);
    }
    logger_helper(int line, const char* nm)
        : lock_helper<T>(logger_stream<T>::instance) {
        line_ = line;
        fname_ = nm;
    }
};

//template <typename Ls>
//inline logger_helper<Ls> make_logger_helper(Ls* ls, int ln, char const* nm)
//{
//    return logger_helper<Ls>(ls,ln,nm);
//}

template <typename Int> void syslog(Int opt, Int facility)
{
#if defined(LOG_USER) && defined(LOG_INFO) && defined(LOG_PID) && defined(LOG_CONS)
    Flags<>::syslog = 1;
    ::openlog(0, opt, facility);
#endif
}
inline FILE* logfile(FILE* fp)
{
	FILE* prev = Flags<>::fp;
    Flags<>::fp = fp;
	return prev;
}

struct info
{
    BOOST_STATIC_CONSTANT(int, capacity=1024);
    BOOST_STATIC_CONSTANT(int, priority=1);
};

} // namespace logging

#define LOG if(1)logging::logger_helper<logging::info>(__LINE__,__FUNCTION__)

#endif
