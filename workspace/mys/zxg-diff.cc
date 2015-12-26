#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/phoenix_stl.hpp>
#include <boost/locale/encoding_utf.hpp>
#include <boost/function_output_iterator.hpp>
#include <set>
#include <fstream>

namespace east
{
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

	std::set<int> get_ZXG(char const* fn/* = FN_ZXG*/)
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

std::set<int> read_ints(char const* fn)
{
    std::set<int> ints;
    std::string line;
    std::ifstream fp(fn, std::ios::binary);
    while (getline(fp, line)) {
        ints.insert(atoi(line.c_str()));
    }
    return std::move(ints);
}

struct print { void operator()(int x) const { printf("%06d\n", x); } };

int main(int argc, char* const argv[])
{
    if (argc != 3)
        return 1;
    std::set<int> dfzxg = east::get_ZXG(argv[1]);
    std::set<int> ss = read_ints(argv[2]);
    std::set_difference(dfzxg.begin(), dfzxg.end(), ss.begin(), ss.end()
            , boost::make_function_output_iterator(print()));
    return 0;
}

// zxg-diff "StockwayStock.ini" "99/lis"

