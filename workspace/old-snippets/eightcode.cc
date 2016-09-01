#include <array>
#include <iostream>
#include "astar.hpp"

struct _Directs { enum Direct { north = -3, west = -1, east = 1, south = 3 }; };

struct Code8 : _Directs
{
 bool operator==(const Code8& x) const { return code == x.code; }
 int f() const { return g() + h(); }
 int g() const { return depth; }
 int h() const { return left; }
 int g(int x) { return depth = x; }
 int h(int x) { return left = x; }

 Code8(const std::array<int, 9>& vec, int vg, int vh) : code(0), depth(vg), left(vh)
 {
  for (unsigned int i = 0; i < 9u; ++i)
   code |= (vec[i] == 8) ? i << 27 : vec[i] << (3*i);
 }

 int at(int i) const { return i == int(code >> 27) ? 8 : (code >> 3*i) & 0x07u; }

 int index(int val) const
 {
  int x = code >> 27;
  if (val != 8)
  {
   int i8 = x;
   for (x = 0; x < 9; ++x)
    if (val == int((code >> (3*x)) & 0x07) && x != i8)
     break;
  }
  return x;
 }

 void swap(int i1, int i2)
 {
  unsigned int c1 = at(i1), c2 = at(i2);
  if (c1 == 8)
   code = (code & ~(0x07u << 27)) | (i2 << 27);
  if (c2 == 8)
   code = (code & ~(0x07u << 27)) | (i1 << 27);
  code &= ~((0x07u << (3*i1)) | (0x07u << (3*i2)));
  code |= ((c1 << (3*i2)) | (c2 << (3*i1)));
 }

 static int distance(int a, int b) { return abs(a/3 - b/3) + abs(a%3 - b%3); }

 int distance(const Code8& a) const
 {
  int x = 0;
  for (int c = 0; c < 9; ++c)
   x += distance(index(c), a.index(c));
  return x;
 }

 unsigned int code;
 int depth, left;
};

std::ostream& operator<<(std::ostream& out, const Code8& a)
{
 for (int i = 0; i < 8; ++i)
  out << a.at(i) << ((i+1)%3 ? " " : "\n");
 return out << a.at(8);
}

struct EightCode : _Directs
{
 typedef Code8 element_type;

 EightCode(const char* beg, const char* end)
  : finish_(reads(end), 0, 0), start_(reads(beg), 0, 0)
 {
  start_.h(finish_.distance(start_));
 }

 template <typename Op>
 void foreach_adjacents(const Code8& a, Op op) const
 {
  for (int n = 0, i0 = a.index(0), e = north; n < 4; ++n, e += 2)
  {
   int ic = i0 + e;
   if ((e==1||e==-1) ? ic >= i0-i0%3 && ic < i0-i0%3+3 : ic >= 0 && ic < 9)
   {
    Code8 x = a;
    x.swap(i0, ic);
    x.g(x.g() + 1);
    x.h(finish_.distance(x));
    //x.h(x.h() + (Code8::distance(i0, iic) < Code8::distance(ic, iic) ? -1 : 1));
    op(x);
   }
  }
 }

 Code8 start() const { return start_; }
 Code8 finish() const { return finish_; }

 static std::array<int,9> reads(const char* s)
 {
  std::array<int,9> vec;
  for (int i = 0; s && i < 9; ++s)
   if (*s >= '0' && *s < '9')
    vec[i++] = *s - '0';
  return vec;
 }

 template <typename R> void result(const R& r, const R& rr) const
 {
#if 1
  for (typename R::const_iterator i = r.begin(), ie = r.end(); i != ie; ++i)
   std::cout << *i << "\n\n";
#else
  for (int n = 0; n < 3; ++n)
  {
   for (typename R::const_iterator i = r.begin(), ie = r.end(); i != ie; ++i)
   {
    for (int x = 0; x < 3; ++x)
     std::cout << i->at(3*n+x) << " ";
    std::cout << "| ";
   }
   std::cout << std::endl;
  }
#endif
 }

 Code8 finish_, start_;
};

int main()
{
 const char* beg = 
  "4 3 2"
  "1 5 0"
  "6 7 8";
 const char* end = 
  "0 1 2"
  "3 4 5"
  "6 7 8";
 EightCode ec(beg, end);
 Astar<EightCode> astar;
 astar.search(ec, ec.start(), ec.finish());
}
