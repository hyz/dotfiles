#include "stdafx.h"

#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/phoenix_stl.hpp>
#include <boost/locale/encoding_utf.hpp>
#include <set>
#include <fstream>
#include "log.h"
#include "tdxdef.h"

namespace dfcf
{
	static char const* const FN_ZXG = "D:\\Program_Files\\eastmoney\\swc8\\config\\User\\m8124094360778600\\StockwayStock.ini";

	namespace qi = boost::spirit::qi;
	namespace ascii = boost::spirit::ascii;
	namespace phoenix = boost::phoenix;

	template <typename Iterator, typename Set>
	bool parse_numbers(Iterator begin, Iterator end, Set& v)
	{
		using qi::int_;
		using qi::char_;
		using qi::lit;
		using qi::_1;
		using phoenix::insert;
		bool r = qi::phrase_parse(begin, end
			, ('=' >> ((char_ >> '.' >> int_[insert(phoenix::ref(v), _1)]) % ',') >> *lit(','))
			, ascii::space, v);
		return (begin == end && r);
	}

	std::set<int> get_ZXG(char const* fn = FN_ZXG)
	{
		static const char kZXG[] = { '\xea', '\x81', '\x09', '\x90', '\xa1', '\x80' }; //UTF-16 "自选股"

		std::string line;
		std::ifstream fp(fn, std::ios::binary);
		while (getline(fp, line)) {
			const char *it, *end;

			end = &line[line.size()];
			it = std::search(line.c_str(), end, &kZXG[0], &kZXG[sizeof(kZXG)]);
			if (it == end) {
				//std::cerr << line.size() <<"\t[Not-Found]\n";
				continue;
			}
			it += sizeof(kZXG);
			if ((end - it) & 1) {
				//std::cerr << line.size() <<"\t"<< "\t[Length-Error]\n";
				continue;
			}

			std::string utf8 = boost::locale::conv::utf_to_utf<char>
				(reinterpret_cast<const char16_t*>(it), reinterpret_cast<const char16_t*>(end));
			//std::cerr << utf8 <<"\t[]\n";
			it = &utf8[0];
			end = &utf8[utf8.size()];

			if (*it != '=') // TODO
				continue;
			//it = std::find(it, end, '=');

			std::set<int> v;
			parse_numbers(it, end, v);
			return std::move(v);
		}
		return std::set<int>();
	}

}

BOOL dfcf_read_ZXG(char const* Code, short nSetCode
		, int Value[4]
		, short DataType, NTime time1, NTime time2, BYTE nTQ, unsigned long unused)  //选取区段
{
    LOG << Code << "Type:" << DataType << (int)nTQ << time1 << time2 << boost::make_iterator_range(&Value[0], &Value[4]);
    static std::set<int> zxg;
    if (zxg.empty()) {
        zxg = dfcf::get_ZXG();
        zxg.insert(0);
    }
    if (zxg.find(atoi(Code)) != zxg.end()) {
        return TRUE;
    }
    return FALSE;
}


