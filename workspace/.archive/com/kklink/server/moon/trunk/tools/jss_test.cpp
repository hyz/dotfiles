#include <stdio.h>
#include <ostream>
#include <string>
#include <vector>
#include <list>
#include <stack>
#include <stdexcept>
#include <boost/range.hpp>
#include <boost/variant.hpp>
// #include <boost/ref.hpp>
#include <boost/noncopyable.hpp>

#include <iostream>
#include "../jss.h"

using namespace std;
using namespace boost;

////////////////////////////////
//
//

string test_printer()
{
    json::object jso;
    jso ("k-float", 1.0123f) ("k-int", 1) ("k-string", "hello") ("k-bool", false) ("k-null", json::none);
    {
        json::array jsa;

        jsa(123)(true)("\r\narr\"{}[]ay");
        jsa(0.1f);

        cout << jsa << endl;

        jso ("k-array", jsa);
    }
    cout << jso << endl;
    {
        json::object jso2 = jso;
        jso2("k-obj", jso);
        cout << jso2 << endl;

        jso ("k-obj2", jso2);
    }
    cout << jso << endl;

    ostringstream oss;
    oss << jso;
    return oss.str();
}

json::variant test_parser(string js)
{
    json::variant result;
    json::decode(result, js);

    return result;
}

void test_x()
{
    json::object obj;

    obj.put("cr", "\r\n");
    string enc = json::encode(obj);

    json::object o2 = json::decode(enc);
    string cr = o2.get<string>("cr");

    cout << enc << endl;
    cout << cr << endl;
}

int main()
{
    test_x();

    // int a[] = { 1,2,3 }; for_each(begin(a), end(a), jsa);
    string js = test_printer();
    json::variant jv = test_parser(js);

    cout << endl
        << json::encode(jv) << endl;

    json::object& obj = get<json::object>(jv);
    cout << obj << endl;

//typedef boost::variant<int,unsigned int,long long int,double,bool,null_type,std::string, array, object> variant;
    cout << endl
        << sizeof(list<int>) << " "
        << sizeof(vector<int>) << " "
        << sizeof(boost::variant<vector<int>,int>) << " "
        << sizeof(boost::variant<list<int>,int>) << " "
        << sizeof(json::variant) << " "
        << sizeof(json::array) << " "
        << sizeof(json::object) << " "
        << endl;

    //cout << sizeof(json::variant) << " " << sizeof(std::list<variant<json::object,json::array> >) << endl;
    //cout << sizeof(json::object) << " " << sizeof(json::array) << endl;
    //cout << sizeof(json::variant) << " " << sizeof(std::list<variant<json::object,json::array> >) << endl;

    return 0;
}

