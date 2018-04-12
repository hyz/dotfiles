###

    cpp -dM /dev/null
    cpp -dM /dev/null |grep SIZE
    cpp -dM /dev/null |grep END
    cpp -dM /dev/null |grep VER

###

Like all references, universal references must be initialized, and it is a universal reference’s initializer that determines whether it represents an lvalue reference or an rvalue reference:

If the expression initializing a universal reference is an lvalue, the universal reference becomes an lvalue reference.
If the expression initializing the universal reference is an rvalue, the universal reference becomes an rvalue reference.

This information is useful only if you are able to distinguish lvalues from rvalues.  A precise definition for these terms is difficult to develop (the C++11 standard generally specifies whether an expression is an lvalue or an rvalue on a case-by-case basis), but in practice, the following suffices:

If you can take the address of an expression, the expression is an lvalue.
If the type of an expression is an lvalue reference (e.g., T& or const T&, etc.), that expression is an lvalue. 
Otherwise, the expression is an rvalue.  Conceptually (and typically also in fact), rvalues correspond to temporary objects, such as those returned from functions or created through implicit type conversions. Most literal values (e.g., 10 and 5.3) are also rvalues.

### http://isocpp.org/blog/2012/11/universal-references-in-c11-scott-meyers

Remember that “&&” indicates a universal reference only where type deduction takes place.  Where there’s no type deduction, there’s no universal reference.  In such cases, “&&” in type declarations always means rvalue reference.  Hence:

### http://stackoverflow.com/questions/257288/is-it-possible-to-write-a-c-template-to-check-for-a-functions-existence?noredirect=1&lq=1

### https://en.wikipedia.org/wiki/Passive_data_structure#In_C.2B.2B
### https://en.wikipedia.org/wiki/Passive_data_structure
### https://en.wikipedia.org/wiki/Plain_Old_C%2B%2B_Object
### http://stackoverflow.com/questions/146452/what-are-pod-types-in-c
### http://stackoverflow.com/questions/4178175/what-are-aggregates-and-pods-and-how-why-are-they-special/7189821#7189821

POD(Plain Old C++ Object)(Passive data structure)

A Plain Old Data Structure in C++ is an aggregate class that contains only PODS as members, has no user-defined destructor, no user-defined copy assignment operator, and no nonstatic members of pointer-to-member type.

### https://herbsutter.com/elements-of-modern-c-style/

Elements of Modern C++ Style
c++11 , Uniform Initialization and Initializer Lists

### https://isocpp.org/std/status

### https://stackoverflow.com/questions/1706346/file-macro-manipulation-handling-at-compile-time

__FILE__ __LINE__ macro manipulation handling at compile time

