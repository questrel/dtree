# dtree

Library is header-only\
depends on boost::spirit::X3 and boost::multiprecision::cpp_dec_float (if you want decfloat support)\
(dependence on boost::core::demangle being deprecated in favor of `__PRETTY_FUNCTION__`)\
requires --std=c++17\
todo: get g++ working again


qtl/out.h
```c++
//templates to print std::container<printable elements>
qtl::ostream& operator<<(const &object&) // invokes std::stream << object or object.write(qtl::ostream&)
qtl::ostream& operator<<(const container<object>& o) // invokes qtl::ostream << object
qtl::ostream& operator<<(const tuple<object...>& o) //  invokes qtl::ostream << object...
// todo: smart formating of nested containers
```

qtl/string.h
```c++
qtl::string // like std::string_view, can contain std::vector<std::string>, maintaining memcmp ordering
```

qtl/container.h
```c++
template<typename T> qtl::vector<T>;
template<typename T...> qtl::tuple<T...>;
// stored in qtl::string such that memcmp ordering is equivalent to std::lexicographical_compare
qtl::scalar::depth = 0;
qtl::vector<T>::depth = T::depth-1;
qtl::tuple<T..>::depth = std::min<T::depth...>-1;
iterator<depth> // iterates over depth level elements
// Erin, why is there an orange bar under here?
Oh, I see, it's a scroll bar.  I thought it was like a <hr> separator.
Would best practice be to keep lines short enough to not scroll?
```

qtl/number.h
```c++
qtl::number // contains any std::is_arithmetic type or decfloat, stored in qtl::string with memcmp ordering
// todo: figure out library path to make <charconv> work 
```

qtl/bool.h
```c++
qtl::kleen/*e*/ // True/False/Maybe logic
```

qtl/bounds.h
```c++
qtl::bounds<T> 
// T is a scalar type suporting <=>, can be number or string
T value;
--value or (x::x|value) // boundary below value. i.e. between x::x < value and value <= x::x
value++ or (value|x::x) // boundary above value. i.e. between x::x <= value and value < x::x  
// --"" is the projective infinity
// note: projective infinity violates transitivity, since
// --"" < declval<T>() and declval<T>() < --"" are both true
// this is ok, since --"" is not part of T (!std::is_same<T,decltype(--"")>) but caution in corner cases is advised 
```

qtl/interval.h
```c++
qtl::interval // interval arithmetic with trinary logic comparisons
// intervals may contain projective infinity
// (this allows sensible division by intervals containing 0,
// and also allows taking the complement of an interval)
// note: std::partial_ordering is inadequate to express <=> for intervals,
// since we could know that a < b is false, but not know whether a==b or a>b
// or know that a==b is false but not know whether a<b or a>b
// todo: implement qtl::partial_ordering for c++20
```

qtl/tree.h
```c++
// expression trees 
qtl::optree(operator,vector<operands>)
using qtl::expr=optree<interval,vector<interval>>;
#define op(o) qtl::expr operator o(const qtl::expr &left, const qtl::expr &right);
op(+) op(-) op(*) op(/) op(<) op(<=) op (==) op(!=) op(>=) op(>) op(&&) op(||)
#undef op
e.eval() -> interval // evaluate expression 
e.bind(std::map<string,expr>).eval() -> interval // evaluate with named variables bound to values
e.stringify() -> std::string  // human readable expression
e.recurse<function>(Args) // descend tree, recursively performing function on each branch
    // lazyvec args allow short-circuit evaluation for operators like &&, ||
    //  e.recurse<&decltype(e)::eval>() is equivalent to e.eval() 
    //  e.recurse<&decltype(e)::ps>(&decltype(e)::stringify) is equivalent to e.stringify()
// todo: split this into separate .h files, separating tree structure abstraction from definition of branch operators
// todo???: can store.h be defined as an instantiation of optree?  store.recurse<find>(predicate)? 
```

qtl/operators.h
```c++
// table of operators and precedences used by tree.h
```

qtl/expr.h
```c++
// parse string into an expression tree
qtl::expr result;
auto p=boost:spirit::x3::phrase_parse( string.begin(),string.end(), qtl::expr_rule, boost::spirit::x3::ascii::space_type, result ); // should invert qtl::expr.stringify()
// todo: generate parse rules from operators.h
```

qtl/store.h
```c++
// use interval arithmetic and trinary logic to query a database
lval operator[](qtl::expr) // query on expression predicate
lval operator[](std::vector<scalar>) // query on prefix
operator=(lval,std::vector<scalar>) // store value
operator=(lval,std::nullptr_t) // delete value
for( auto x::lvalue ){ qtl::cout << x; } // print values satisfying query
/* todo:
  lval operator[]( project subset of columns );
  allows unified map< variant< prefix, expression, projection >, vector<column selection> > abstraction
  and makes for( auto x::lvalue ){ qtl::cout << x; } more practical when you only want specific columns
  also makes lvalue = vector<value> and lvalue = vector<vector<value>> more useful
  possible implementation:
  extend lval operator[](std::vector<scalar>) // query on prefix
  to lval operator[](std::vector<interval>) // where the (--0,--0), the False or Empty interval, means to ignore that column
  or use std::ignore?
*/
```

qtl/sql.h
```c++
// toy sql parser turning simple sql queries into qtl::store[] queries
#if 0
Erin, I'm not sure there's much point in documenting the behavior if this module,
Which is basically as much of SQL as one cares to implement, since abundant SQL doccumentation already exists.
Rather, I think we want to explain the underlying model well enough
so that users can understand how to implement whatever SQL or other behavior they may be interested in.

The basic abstraction I want to present is like 
  std::map<selector,selection>
where selection is a vector (rows) of vectors (columns) of values,
amd selection can be an arbitrary predicate to be satisfied by the values within the selected rows,
or a restriction to columns with particular values or to a particular subset of columns.
You would be able to retrieve a selection with
  selection = map[selector];
or insert new selections with
  map[selector] = selection;
also
  selection = selecton[selector];
could be used to refine a selection.
(so the initial map in map[selection] can be thought of as a selection with a universal selector)

This may seem like a stretch of the std::map concept, and unlike the usual map because the result of
  map[selector]
can be changed by
  map[different_selector] = selection;
But I'm not seeing a guarantee in the c++ documentation that std::map values must be independent for different keys
Or, it may be conceptually better to think of the selection returned by map[selector] as a pointer to the actual selection
Especially when shared memory is implemented and different users can influence each others results.

I do see a specification that says std::map.count(key) returns 0 or 1
but not a specification that would require map[key].size() to return 0 or 1
#endif
```


qtl/randstream.h
```c++
// turn stream of random values from one arbitrary distribution
// into stream of random values from another arbitrary distriburion
// uses Arithmetic Coding for optimal entropy buffering
// used internally to turn random afl-fuzz input into nicely distributed tests
```

test.cpp
```c++
// invokes qtl/module.h to generate a test exercizer for module
//#define TEST_H "qtl/module.h"
#include TEST_H
```

## Installation Instructions

compiling
```c++
#define TEST_H "qtl/???.h"
#include TEST_H
```
will generate a test exercizer for ??? library

```bash
make  # make all ???.test.out
```

```bash
make CXX='clang++ -DDEBUG'  # with diagnostic trace prints
```

```bash
make ???.fuzz.out  # afl-fuzz testing ( see http://lcamtuf.coredump.cx/afl/ )
AFL_NO_ARITH=1 AFL_EXIT_WHEN_DONE=1 /usr/local/bin/afl/afl-fuzz  -i- -x fuzz/???.dict -o fuzz/???.out -- ./???.fuzz.out
# may run for several weeks
```

