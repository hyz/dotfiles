#include <string>
#include <vector>
#include <map>
#include <boost/lexical_cast.hpp>

typedef std::map<std::string,std::string> keyvalues;

const std::string& get(keyvalues &l, const std::string& name)
{
    keyvalues::iterator it = l.find(name);
    if (it == l.end())
        exit(44);
    return it->second;
}

template <typename T>
const T get(keyvalues &l, const std::string& name)
{
    return boost::lexical_cast<T>(get(l,name));
}

const char* get(keyvalues &l, const std::string& name, const char* defa)
{
    std::cout << "const char*\n";
    keyvalues::iterator it = l.find(name);
    if (it == l.end())
        return defa;
    return it->second.c_str();
}

std::string get(keyvalues &l, const std::string& name, const std::string& defa)
{
    std::cout << "const string&\n";
    keyvalues::iterator it = l.find(name);
    if (it == l.end())
        return defa;
    return it->second;
}

template <typename T>
const T get(keyvalues &l, const std::string& name, const T& defa)
{
    const char *p = get(l, name, static_cast<const char*>(0));
    if (p == 0)
        return defa;
    return boost::lexical_cast<T>(p);
}


int main()
{
    keyvalues l;
    l.insert(std::make_pair("foo","foo"));
    l.insert(std::make_pair("bar","123"));

    std::cout << get(l,"foo") << "\n";
    std::cout << get(l,"foobar", "bar") << "\n";
    std::cout << get(l,"foobar", std::string("bar")) << "\n";
    std::cout << get<int>(l,"bar") << "\n";
    std::cout << get(l,"bar", 1) << "\n";
    std::cout << get(l,"barbar", 1) << "\n";

    return 0;
}


