# dtree

Library is header-only\
requires --std=c++17\
depends on boost::spirit::X3 and boost::multiprecision::cpp_dec_float (if you want decfloat support)\
(dependence on boost::core::demangle being deprecated in favor of `__PRETTY_FUNCTION__`\
This may reduce portability of our output formats, though it has been tested under clang and g++)\
todo: get g++ compilations working again (which started failing for other unknown reasons)

#### qtl/out.h
```c++
//templates to print std::container<printable elements>
qtl::ostream& operator<<(const container<object>& o); // invokes qtl::ostream << object
qtl::ostream& operator<<(const tuple<object...>& o); //  invokes qtl::ostream << object...
qtl::ostream& operator<<(const object&); // invokes std::stream << object or object.write(qtl::ostream&)
// todo: smart formating of nested containers
```

#### qtl/string.h
```c++
class qtl::string; // like std::string_view, can contain any std::string, maintaining memcmp ordering
// can also contain out of band separator tokens
// the separator tokens compare less than any std::string
// there is also a qtl::string value that compares greater than a qtl::string value containing any std::string
```

#### qtl/container.h
```c++
template<typename T> class qtl::vector<T>;
template<typename T...> class qtl::tuple<T...>;
// stored in qtl::string with separators such that memcmp ordering is equivalent to std::lexicographical_compare
qtl::scalar::depth = 0;
qtl::vector<T>::depth = T::depth-1; // note going down in depth gets more negative
qtl::tuple<T...>::depth = std::min<T::depth...>-1;
iterator<depth> // iterates over depth level elements
#if 0
// Erin, why is there an orange bar under here?
Oh, I see, it's a scroll bar.  I thought it was like a <hr> separator.
Would best practice be to keep lines short enough to not scroll?
#endif
```

#### qtl/number.h
```c++
class qtl::number; // contains values from std::is_arithmetic type or decfloat, stored in qtl::string with memcmp ordering
// curently supports E−6176 to E+6144 range of IEEE decimal128, but is extensible with additional table entries
// todo: explicitly define unlimited extension schema
// supports IEEE ±infinity, (although we have our own projective infinity (vide infra) that is not signeed,
// so IEEE ±infinity are treated more like overflow values than a proper infinity)
// does not support -0 (0 is unique, although we can have underflow values distinct from a proper 0)
// todo: figure out library paths to get <charconv> working on my development system 
```

#### qtl/bool.h
<!-- language: c++ --> 
```class qtl::kleen/*e*/; // True/False/Maybe logic``` [en.wikipedia.org/wiki/Three-valued_logic#Kleene_and_Priest_logics](https://en.wikipedia.org/wiki/Three-valued_logic#Kleene_and_Priest_logics)\
```//"e" is dropped from (Stephen) Kleene, as "e" is dropped from (George) Boole```\
@emckean, is there a way to have both c++ syntax highlighting and links on the same line?

#### qtl/bounds.h
```c++
template<typename T> class qtl::bounds<T>; // T is a scalar type suporting <=>, can be number or string
constexpr T value;
--value or (x::x|value) // boundary below value. i.e. between (x::x<value) and (value<=x::x)
value++ or (value|x::x) // boundary above value. i.e. between (x::x<=value) and (value<x::x) 
// --""_s is the [projective infinity](https://en.wikipedia.org/wiki/Point_at_infinity)
// note: the projective infinity violates transitivity, since
// --""_s < declval<T>() and declval<T>() < --""_s are both true
// that's ok, since --""_s is not part of T (!std::is_same<T,decltype(--""_s)>) but caution in corner cases is advisable 
// todo: \U221E ( ∞ ) and x::x/0 could be synonyms for --""_s
// ("infinity" seems too confusable with std::numeric_limits<T>::infinity(), and "projective_infinity" seems too verbose
// the only standard notation I found for the projective infinity was ∞, and that can still be confounded 
// with other notions of "infinity") 
// (""_s is not the string representation of any scalar value, and is the infimum
// (which would be another counfounding interpretation of using "Inf" as a name)
// of all possible string values, leaving room to interpret a boundary lower than it as ∞)
```

#### qtl/interval.h
```c++
class qtl::interval; // interval arithmetic, with trinary logic comparisons
// intervals may contain the projective infinity <https://en.wikipedia.org/wiki/Division_by_zero#Projectively_extended_real_line>
// (this allows sensible division by intervals containing 0,
// and also allows taking the complement of an interval)

// note: std::partial_ordering is inadequate to express <=> for intervals,
// since we could know that a < b is false, but not know whether a==b or a>b
// or know that a==b is false but not know whether a<b or a>b
// todo: implement qtl::partial_ordering for c++20

// The interval from projective infinity to projective infinity (∞<x::x<∞)
// represents a value that could be anything.
// It is also the interval representation of the kleen::True value.

// The kleen::False value can be represented by the interval
// (0<=x::x<0), with (x::x|0), i.e. (--0_s), for both boundaries,
// which is an interval that contains no values.
// (all empty intervals are equivalent, but our canonical representation,
// returned by static_cast<interval>(kleen::False)
// uses the boundary between negative and non-negative scalars.
// ?would it be better to use (""_s<x::x<=""_s), with (""_s|x::x), i.e. (""_s++), for both boundaries
// since (--""_s) i.e. (x::x|""_s) is our representation of the projective infinity?)

// kleen::Maybe is represented by the interval (0<=x::x<=1)
// so the && and || operations on intervals operate properly on True/False/Maybe values.

// Those choices may seem unconventional if one was expecting
// x::x[0] to represent False and x::x[1] to represent True.
// (0<=x::x<=1) for Maybe would naturally fit that convention,
// and && and || might be implemented as min and max
// but then && and || as logical operators on static_cast<interval>(Kleen)
// won't work as intersection and union on intervals
// If && and || on intervals are to work as intersection and union,
// so that (a < x::x) && (x::x < b) is the same as (a < x::x <b),
// then the interval that represents Maybe must be a superset of the interval that represents False,
// and the interval that represents True must be a superset of theinterval that reperesents Maybe.

// Unfortunately, operator !/*not*/(0<=x::x<=1), doesn't bridge intervals and kleens quite as neatly,
// since !Maybe and Maybe would be different as intervals, but the same as kleens.
// So either Maybe && !Maybe is unexpectedly False instead if Maybe,
// or !(x==0) is unexpectedly different from (x!=0)
// operator ~/*complement*/(Maybe) can be used, but it would seem semantically confusing if
// (0<=x::x<=1) was the same as the complement of (0<=x::x<=1)
// But users would not normally need to deal directly with the fact that kleen values are
// represented as interval values when evaluating expressions, and the user could always use
// static_cast<interval>(static_cast<kleen>(Interval))
// to force kleen logic behavior when that's what they want.
// And users would normally be applying logical operators to comparisons of intervals,
// rather than directly to intervals.
// So I think it will be less confusing that
// !(x==0) is the same as (x!=0), and ~(x::x==0) is the same as (x::x!=0),
// but !(x::x==0) is the same as !(x::x!=0), and ~(x==0) is different from ~(x!=0)
// than would be if 
// ~(x==0) was the same as (x!=0), and !(x::x==0) was the same as (x::x!=0)
// but ~(x::x==0) was the same as ~(x::x!=0), and ~(x==0) was different from ~(x!=0)

// (x==0) is an expression (vide infra),
// where x is a variable with a value that could be an interval,
// which would evaluate to an interval representation of a kleen
// whereas (x::x==0) is an interval
   
// (∞<x::x<∞) could also be described as a completely unknown value, so it could be
// semantically confusing that Unknown is also True.
// (which is part of why I called the third logic value Maybe instead of Unknown)

// note: a union of intervals, or an intersection with an interval that contains the projective infinity
// (I haven't found or settled on a standard terminology for such intervals.
// internally, I have the test is_bipole(), but a term more intuitive to others may be
// useful when trying to document nuances like these) 
// could result in two disjoint intervals.
// I didn't want to generalize the qtl::interval concept to encompass multiple disjoint intervals,
// since that could entail combinatorial explosions as you perform operations on them.
// Also, expressions like sin(a) < 0 could represent an infinite number of disjoint intervals.
// So when the result of an expression would contain multiple disjoint intervals,
// I cannonically return a single interval covering all.
// When there is an ambiguity which way to wrap, intervals not containing the projective infinity, or smaller intervals are prefered
// Predicates satisfied by multiple disjoint intervals can stil be described by expressions containing && and ||
// without need of having a fundamental object that represents multiple disjoint intervals.
// Although sin(a) < 0 may have to return Maybe when a spans a large interval,
// we can still resolve it to True or False as a gets sufficiently refined.

// note: typical treatments of interval arithmetic
// <https://en.wikipedia.org/wiki/Interval_arithmetic>
// <https://www.boost.org/doc/libs/1_66_0/libs/numeric/interval/doc/interval.htm>
// <http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2006/n2137.pdf>
// don't distinguish intervals including end points from intervals excluding end points.
// that distinction seems unimportant for modeling rounding errors
// but is important for predicates in a query on exact values.
// Typical treatments may also punt on issues of division by intervals contaning 0
// or trimodal comparison logic.
// ("punt" may be too much of an Americanism,
// Wordnik seems to be one of the few on-line dictionaries that include my intended sense
// --but even there I didn't find a good synonym)
// (some of the synonyms looked more like synonyms for "pun"
// it may be more useful if synonyms could be divided by sense)
```

#### qtl/tree.h
```c++
// generic expression trees with branches of 
qtl::optree(Operator,vector<Operands>);

using qtl::expr=optree<interval,vector<interval>>;
#define op(O) qtl::expr operator O(const qtl::expr& left, const qtl::expr& right);
op(+) op(-) op(*) op(/) op(<) op(<=) op (==) op(!=) op(>=) op(>) op(&&) op(||) ...
#undef op
qtl::expr e;
interval i = e.eval(); // evaluate expression 
interval i = e.bind(std::map<string,expr>).eval(); // evaluate with named variables bound to values
std::string s = e.stringify();  // human readable expression
                                // todo: e.stringify({pretty print options});
auto result = e.recurse<function>(Args); // descend tree, recursively performing function on each branch
    // lazyvec args allow short-circuit evaluation for operators like &&, ||
    //  e.recurse<&decltype(e)::eval>() is equivalent to e.eval() 
    //  e.recurse<&decltype(e)::ps>(&decltype(e)::stringify) is equivalent to e.stringify()
// todo: split this into separate .h files, separating tree structure abstraction from definition of branch operators
// todo???: can store.h be defined as an instantiation of optree?  store.recurse<find>(predicate)? 
```

#### qtl/operators.h
```c++
// table of operators and precedence hierarchy used by tree.h
```

#### qtl/expr.h
```c++
// parse string into an expression tree
qtl::expr result;
auto p=boost:spirit::x3::phrase_parse( string.begin(),string.end(), qtl::expr_rule, boost::spirit::x3::ascii::space_type, result ); // should invert result.stringify()
// <http://charette.no-ip.com:81/programming/doxygen/boost/namespaceboost_1_1spirit_1_1x3.html#a7872ffa13c602499eb94ae6d611f738a>
// todo: expr operator""_expr(const char *c,std::size_t s);
// todo: expr(char*);
// todo: generate parse rules from operators.h
```

### qtl/store.h
```c++
// use interval arithmetic and trinary logic to query a database
lval operator[](qtl::expr); // query on expression predicate
lval operator[](std::vector<scalar>); // query on prefix
operator=(lval,std::vector<scalar>); // store value
operator=(lval,std::nullptr_t); // delete value
for( auto x::lvalue ){ qtl::cout << x; } // print values satisfying query
/* todo:
  lval operator[]( project subset of columns );
  allows unified map< variant< prefix, expression, projection >, vector<column selection> > abstraction
  and makes for( auto x::lvalue ){ qtl::cout << x; } more practical when you only want specific columns
  also makes lvalue = vector<value> and lvalue = vector<vector<value>> more useful
  possible implementation:
  extend lval operator[](std::vector<scalar>) // query on prefix
  to lval operator[](std::vector<interval>) // where the False or Empty interval, means to ignore that column
  or use std::ignore?
*/
```

### qtl/sql.h
```c++
// toy sql parser turning simple sql queries into qtl::store[] queries
#if 0
@Erin, I'm not sure there's much point in documenting the behavior of this module,
which is basically as much of SQL as one cares to implement, since abundant SQL doccumentation already exists.
Rather, I think we want to explain the underlying model well enough
so that users can understand how to implement whatever SQL or other behavior they may be interested in.

The basic abstraction I want to present would be
  std::map<selector,selection>
where selection is a vector (rows) of vectors (columns) of values,
and selection can be an arbitrary predicate to be satisfied by the values within the selected rows,
or a restriction to columns with particular values [or to a particular subset of columns].
You'd be able to retrieve a selection with
  selection = map[selector];
or insert new selections with
  map[selector] = selection;
also
  selection = selection[selector];
could be used to refine a selection.
(so in the initial map[selection], map can be thought of as a selection with a universal selector)

This may seem like a stretch of the std::map concept, and unconventional in that the result of
  map[selector]
can be changed by
  map[different_selector] = selection;
But I don't see a guarantee in the c++ documentation that std::map values must be independent for different keys
Or, it may be conceptually better to think of the object returned by map[selector] as an accessor to the actual selection
Especially when shared memory is implemented and different users can influence each others results.

note on the name Predicate Lattice:
I find an existing related notion of a Concept Lattice
<https://en.wikipedia.org/wiki/Formal_concept_analysis#Concept_lattice_of_a_formal_context>
Like we might think of boundaries as a Dedekind–MacNeille completion of scalars, 
a Concept Lattice seems to be a Dedekind–MacNeille completion of a partial order of attributes
<https://en.wikipedia.org/wiki/Dedekind%E2%80%93MacNeille_completion#Examples>
#endif
```

### qtl/randstream.h
```c++
// turn stream of random values from one arbitrary distribution
// into stream of random values from another arbitrary distriburion
// uses [Arithmetic Coding](https://en.wikipedia.org/wiki/Arithmetic_coding) for optimal entropy buffering
#if 0
This is essentially the application for which I had originally invented Arithmetic Coding,
a year before it appeared in a widely circulated journal article <https://dl.acm.org/citation.cfm?doid=214762.214771>
from which I leared that it had been previously invented a decade earlier.
#endif
// used internally to turn random afl-fuzz input into nicely distributed tests
```

### test.cpp
```c++
// invokes qtl/module.h to generate a test exercizer for module
//#define TEST_H "qtl/module.h"
#include TEST_H
```
[see also](doc)

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
make ???.fuzz.out  # [afl-fuzz testing](http://lcamtuf.coredump.cx/afl/)
AFL_NO_ARITH=1 AFL_EXIT_WHEN_DONE=1 /usr/local/bin/afl/afl-fuzz  -i- -x fuzz/???.dict -o fuzz/???.out -- ./???.fuzz.out
# may run for several weeks
```

