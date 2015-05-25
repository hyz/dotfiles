#ifndef UTIL_H__
#define UTIL_H__

#include <string>
#include <stdexcept>
#include <boost/assert.hpp>
#include <boost/static_assert.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/archive/iterators/binary_from_base64.hpp>
#include <boost/archive/iterators/transform_width.hpp>
#include <iconv.h>

int sendsms(const std::string& phone, const std::string& message);

std::string path_leaf(const std::string& path);

std::string urldecode(const std::string &str_source);
std::string urldecode(const char* beg, const char* end);
std::string urlencode(const std::string &str_source);

template <typename in_type, typename out_type>
out_type& wiconv(out_type& out, const char* out_code, in_type& in, const char* in_code)
{
    BOOST_STATIC_ASSERT(sizeof(wchar_t) == 4);

    char* ibuf = reinterpret_cast<char*>(const_cast<typename in_type::value_type*>(&in[0]));
    size_t ileft = in.size() * sizeof(typename in_type::value_type);

    char* obuf = reinterpret_cast<char*>(&out[0]);
    size_t oleft = out.size() * sizeof(typename out_type::value_type);

    iconv_t cd = iconv_open(out_code, in_code);
    if (cd == (iconv_t)-1)
    {
        throw std::runtime_error("iconv_open error");
    }
    size_t res = iconv(cd, &ibuf, &ileft, &obuf, &oleft);
    iconv_close(cd);
    if (res == (size_t)-1)
    {
        throw std::runtime_error("iconv error");
    }

    BOOST_ASSERT(oleft % sizeof(typename out_type::value_type) == 0);
    out.resize(out.size() - oleft/sizeof(typename out_type::value_type));

    return out;
}

inline std::wstring utf8to32(std::string const& s)
{
    std::wstring w(s.size(), '\0');
    return wiconv(w, "utf-32", s, "utf-8");
}

inline std::string utf32to8(std::wstring const& s)
{
    std::string n(s.size() * 4, '\0');
    return wiconv(n, "utf-8", s, "utf-32");
}

unsigned int randuint(unsigned int beg=0, unsigned int end = std::numeric_limits<unsigned int>::max());

std::string readfile(boost::filesystem::path const & fp);
void copyfile(std::string const & src, std::string const & dst);

template <typename Self, typename Iter>
Self& for_self(Self& sf, Iter begin, Iter end, Iter * iend=0)
{
    for (; begin != end; ++begin)
        if (!sf(*begin))
            break;
    if (iend)
        *iend = begin;
    return sf;
}

template <int N> std::string time_format(char const * sfmt, struct tm& tm)
{
    char tmp[N];
    strftime(tmp,sizeof(tmp), sfmt, &tm);
    return std::string(tmp);
}
template <int N> std::string time_format(std::string const & sfmt, struct tm& tm)
{
    return time_format<N>(sfmt.c_str(), tm);
}

template <int N> std::string time_format(char const * sfmt, time_t tp=0)
{
    if (tp == 0)
        tp = time(0);
    struct tm tm;
    localtime_r(&tp, &tm);
    return time_format<N>(sfmt, tm);
}
template <int N> std::string time_format(std::string const & sfmt, time_t tp=0)
{
    return time_format<N>(sfmt.c_str(), tp);
}

inline std::string time_string(time_t tp=0)
{
    return time_format<64>("%F %T", tp);
}

template <typename T> inline T& zeros(T& a)
{
    memset(&a, 0, sizeof(T));
    return a;
}

template <typename Iter>
std::string base64_decode(Iter begs, Iter ends)
{
    using namespace boost::archive::iterators;

    // typedef transform_width<binary_from_base64<Iter>, 8, 6> base64_dec;
    typedef transform_width<binary_from_base64<Iter>,8,6> it_binary_t;

    unsigned int npad = std::count(begs, ends, '=');
    // std::replace(base64.begin(),base64.end(),'=','A'); // replace '=' by base64 encoding of '\0'

    std::string result;
    result.assign(it_binary_t(begs), it_binary_t(ends));
    result.erase(result.end() - npad, result.end());

    return result; //std::string(it_binary_t(beg), it_binary_t(end)); // decode
}

inline std::string base64_decode(std::string const & s)
{
    return base64_decode(s.begin(), s.begin()+s.size());
}

#endif

