// pseudo c++ synopsis for objects in namespace lex
#include <type_traits>
#include <string>
#include <vector>
#include <tuple>

template<typename S, typename=std::enable_if_t<std::is_same_v<S,std::string> > >
using is_lexable_v<S> = true;

template<typename S, typename=std::enable_if_t<std::is_arithmetic_v<S> > >
using is_lexable_v<S> = true;

template<typename S, typename=std::enable_if_t<std::is_lexable_v<S> >
using is_lexable_v<std::vector<S>> = true;

template<typename S0, typename... Sn, typename=std::enable_if_t< is_lexable_v<S> && is_lexable_v<Sn...> >
using is_lexable_v<std::tuple<S0, Sn...>> = true;

if constexpr ( std::is_same_v<lex::T,lex::number> && std::is_arithmetic_v<S>
       || is_lexable_v<std::T> && std::is_same_v(std::T,S>
){
  S s0;
  S s1;
  lex::T x0 = s0;
  lex::T x1 = s1;

  assert( std::is_same_v<decltype(x0)::std_t, decltype(s0)> );

  assert( s0 == (decltype(s0))x0 );

  assert( std::clamp(x0 <=> x1,-1,1) == std::clamp(s0 <=> s1,-1,1) );

  assert(std::clamp(std::strcmp(x0.raw(),std::x1.raw()),-1,1) == std::clamp(x0 <=> x1,-1,1) );
}


