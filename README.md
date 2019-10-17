# dtree

Library is header-only
depends on boost::spirit::X3
requires --std=c++17

qtl/out.h
//templates to print std::container<printable object>
qtl::ostream&opertor<<(container<object>& o) // invokes qtl::ostream << object
// todo: smart formating of nested containers

qtl/string.h
qtl::string // like std::string_view, can contain std::vector<std::string>, maintaining strcmp ordering

qtl/container.h
template<typename T> qtl::vector<T>;
template<typename T...> qtl::tuple<T...>;
// stored in qtl::string such that strcmp ordering is equivalent to lexical std::vector or std::tuple ordering
qtl::scalar::depth = 0;
qtl::vector<T>::depth = T::depth-1;
qtl::tuple<T..>::depth = std::min<T::depth...>-1;
iterator<depth> // iterates over depth level elements

qtl/number.h
qtl::number // contains any std::is_arithmetic type, stored in qtl::string with strcmp ordering

qtl/bool.h
qtl::kleen/*e*/ // True/False/Maybe logic

qtl/bounds.h
qtl::bounds<T> 
// T is a scalar type suporting <=>, can be number or string
T value;
--value or (x::x|value) // boundary between x::x < value and value <= x::x
value++ or (value|x::x) // boundary between x::x <= value and value < x::x  
// --"" is the projective infinity

qtl/interval.h
qtl::interval // interval arithmetic with trinary logic comparisons
// intervals may contain projective infinity
// (this allows sensible division of intervals containing 0,
// and also allows taking the complement of an interval)

qtl/tree.h
// expression trees 
qtl::optree(operator,vector<operands>)
using qtl::expr=optree<interval,vector<interval>>;
qtl::expr operator+(qtl::expr left, qtl::expr right);
qtl::expr operator-(qtl::expr left, qtl::expr right);
qtl::expr operator*(qtl::expr left, qtl::expr right);
qtl::expr operator/(qtl::expr left, qtl::expr right);
qtl::expr e;
e.eval() -> interval // evaluate expression 
e.bind(std::map<string,expr>).eval() -> interval // evaluate with variables bound to values
e.stringify() -> std::string  // human readable expression

qtl/operators.h
// table of operators and precedences used by tree.h

qtl/expr.h
// parse string into an expression tree
qtl::expr result;
auto p=boost:spirit::x3::phrase_parse( string.begin(),string.end(), qtl::expr_rule, boost::spirit::x3::ascii::space_type, result );
// should invert qtl::expr.stringify()


qtl/store.h
// use interval arithmetic and trinary logic to query a database
lval operator[](qtl::expr) // query on expression
lval operator[](std::vector<scalar>) // query on prefix
friend operator=(lval,std::vector<scalar>) // store value
friend operator=(lval,std::nullptr_t) // delete value
for( auto x::lvalue ){ qtl::cout << x; } // print values satisfying query

qtl/sql.h
// toy sql parser turning simple sql queries into qtl::store[] queries

qtl/randstream.h
// turn stream of random values from one arbitrary distribution
// into stream of random values from another arbitrary distriburion
// uses Arithmetic Coding for optimal entropy buffering
// used internally to turn random afl-fuzz input into nicely distributed tests

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
make ???.fuzz.out  # afl-fuzz testing http://lcamtuf.coredump.cx/afl/
AFL_NO_ARITH=1 AFL_EXIT_WHEN_DONE=1 /usr/local/bin/afl/afl-fuzz  -i- -x fuzz/???.dict -o fuzz/???.out -- ./???.fuzz.out
# may run for several weeks
```

