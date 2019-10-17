# dtree

Library is header-only
depends on boost::spirit::X3

qtl/out.h
templates to print std::container<printable object>
qtl::ostream&opertor<<(container<object>& o) // invokes qtl::ostream << object
// todo: smart formating of nested containers

qtl/string.h
qtl::string // like std::string and can contain std::vector<std::string> maintaining strcmp ordering

qtl/container.h
template<typename T> qtl::vector<T>;
template<typename T...> qtl::tuple<T...>;
// stored in qtl::string maintaining strcmp ordering

qtl/number.h
qtl::number // contains any std::is_arithmetic type, maintaining strcmp ordering

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

qtl/tree.h
// expression trees 
qtl::optree(operator,vector<operands>)
using qtl::expr=optree<interval,vector<interval>>;
qtl::expr e;
e.eval()->qtl::interval // evaluate expression
e.bind(std::map<string,expr>).eval() // evaluate with variables bound to values
e.stringify()->std::string  // printable expression


qtl/expr.h
// parse string into an expression tree

qtl/store.h
// use interval arithmetic and trinary logic to query a database
lval operator[](qtl::expr) // query on expression
lval operator[](std::vector<scalar>) // query on prefix
lval operator=(qtl::interval) // store value
lval operator=(std::nullptr_t) // delete value
for( auto x::lvalue ){ qtl::cout << x; } // print values

qtl/sql.h
// toy sql parser turning sql queries into qtl::store[] queries

qtl/randstream.h
// turn stream of random values from one arbitrary distribution
// into stream of random values from another arbitrary distriburion
// uses Arithmetic Coding for optimal entropy buffering

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
AFL_NO_ARITH=1 AFL_EXIT_WHEN_DONE=1 /usr/local/bin/afl/afl-fuzz  -i- -x fuzz/xxx.dict -o fuzz/xxx.out -- ./xxx.fuzz.out
# may run for several weeks
```
