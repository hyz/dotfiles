#include <boost/foreach.hpp>
#include "log.h"
#include "json.h"

#include <boost/range.hpp>
#include <boost/optional/optional.hpp>
#include "util.h"

typedef std::vector<std::string>::const_iterator vector_const_iterator;

struct anys_finder : json::access_dummy_helper<boost::optional<vector_const_iterator> >
{
    std::vector<std::string> const *subs_;
    anys_finder(std::vector<std::string> const& subs) : subs_(&subs) {}

    using json::access_dummy_helper<result_type>::access;

    template <typename T, typename K>
    result_type access(T const& p, K const& k, std::string const& txt) const
    {
        auto it = find_anys(txt, *subs_);
        if (it != subs_->end()) {
            return result_type(it);
        }
        return result_type();
    }
};

vector_const_iterator search_any_of(json::object const& jv, std::vector<std::string> const& subs)
{
    if (subs.empty()) {
        return subs.end();
    }
    anys_finder sf(subs);
    if (auto res = json::jsv_walk(jv, sf)) {
        return res.value();
    }
    return subs.end();
}

vector_const_iterator search_any_of(std::string const& txt, std::vector<std::string> const& subs)
{
    if (subs.empty()) {
        return subs.end();
    }
    return find_anys(txt, subs);
}


