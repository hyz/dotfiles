#include <list>
#include <algorithm>

template <typename Matrix>
class Astar
{
 typedef typename Matrix::element_type Element;

 struct Node;
 typedef typename std::list<Node>::iterator Iterator;

 struct Node : Element
 {
  Iterator prev;
  Node(Iterator i, const Element& n) : Element(n), prev(i) {}
 };
 struct SelNode
 {
  const Element* node;
  SelNode(const Element& n) : node(&n) {}
  bool operator()(const Node& n) const { return *node == n; }
 };
 struct Alt
 {
  bool operator()(const Node& a, const Node& b) const { return a.f() < b.f(); }
 };

 struct result
 {
  template <typename R>
  void operator()(const Matrix& m, const R& r, const R& rr) const { m.result(r, rr); }
 };

 std::list<Node> open, closed;

 bool is_closed(const Element& node) const
 {
  return std::find_if(closed.begin(), closed.end(), SelNode(node)) != closed.end();
 }

 void extend_anode(Iterator prev, const Element& node);

public:
 template <typename Op>
 void search(const Matrix& mat, const Element& beg, const Element& end, Op op);
 void search(const Matrix& mat, const Element& beg, const Element& end)
 {
  return search(mat, beg, end, result());
 }
};

#include <tuple>
#include <functional>

template <typename Matrix>
template <typename Op>
void Astar<Matrix>::search(const Matrix& mat, const Element& beg, const Element& , Op op)
{
 open.clear();
 closed.clear();
 closed.push_back(Node(closed.end(), beg));
 for (Iterator i = closed.begin(); i->h(); i = closed.begin())
 {
     using namespace std::placeholders;
  mat.foreach_adjacents(*i, std::bind(&Astar<Matrix>::extend_anode, this, i, _1));
  if (open.empty())
  {
   closed.pop_back();
   op(mat, open, closed);
   return;
  }
  closed.splice(closed.begin(), open,
    std::min_element(open.begin(), open.end(), Alt()));
 }
 Iterator rbeg = closed.begin();
 for (Iterator i = rbeg->prev; i != closed.end(); i = i->prev)
  closed.splice(closed.begin(), closed, i);
 open.splice(open.end(), closed, ++rbeg, closed.end());
 op(mat, closed, open);
}

template <typename Matrix>
void Astar<Matrix>::extend_anode(Iterator prev, const Element& e)
{
 if (!this->is_closed(e))
 {
  Iterator i = std::find_if(open.begin(), open.end(), SelNode(e));
  if (i == open.end())
   open.push_back(Node(prev, e));
  else if (e.g() < i->g())
   i->prev = prev, i->g(e.g());
 }
}

