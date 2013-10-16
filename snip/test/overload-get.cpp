#include <string>
#include <vector>
#include <boost/lexical_cast.hpp>

typedef std::vector<std::string> keyvalues;

const std::string& get(keyvalues &l, const std::string& name)
{
    keyvalues::iterator it = std::find(l.begin(),l.end(),name);
    if (it == l.end())
        exit(44);
    return *it;
}

template <typename T>
const T get(keyvalues &l, const std::string& name)
{
    return boost::lexical_cast<T>(get(l,name));
}

int main()
{
    keyvalues l;
    l.push_back("foo");
    l.push_back("123");

    std::cout << get(l,"123") << "\n";
    std::cout << get<int>(l,"123") << "\n";

    return 0;
}


