/// http://stackoverflow.com/questions/10474571/how-to-match-unicode-characters-with-boostspirit?rq=1
//
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/support_istream_iterator.hpp>
#include <boost/foreach.hpp>
#include <boost/regex/pending/unicode_iterator.hpp>
namespace qi = boost::spirit::qi;

int main() {
  std::string str = "На берегу пустынных волн";
  boost::u8_to_u32_iterator<std::string::const_iterator>
      begin(str.begin()), end(str.end());
  typedef boost::uint32_t uchar; // a unicode code point
  std::vector<uchar> letters;
  bool result = qi::phrase_parse(
      begin, end,             // input
      +qi::standard_wide::char_,  // match every character
      qi::space,              // skip whitespace
      letters);               // result
  BOOST_FOREACH(uchar letter, letters) {
    std::cout << letter << " ";
  }
  std::cout << std::endl;
}

///#include <boost/spirit/include/qi.hpp>
///#include <boost/spirit/include/support_istream_iterator.hpp>
///#include <boost/foreach.hpp>
///namespace qi = boost::spirit::qi;
///
///int main() {
///  std::cin.unsetf(std::ios::skipws);
///  boost::spirit::istream_iterator begin(std::cin);
///  boost::spirit::istream_iterator end;
///
///  std::vector<char> letters;
///  bool result = qi::phrase_parse(
///      begin, end,  // input     
///      +qi::char_,  // match every character
///      qi::space,   // skip whitespace 
///      letters);    // result    
///
///  BOOST_FOREACH(char letter, letters) {
///    std::cout << letter << " ";
///  }
///  std::cout << std::endl;
///}

