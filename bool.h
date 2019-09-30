#pragma once
#include "out.h"
namespace qtl{
  //  class kleen{ // "e" droppd by analogy with "bool"
  //    public:
  enum class kleen{F=0,U=1,T=2,N=3};
  constexpr kleen F=kleen::F;
  constexpr kleen U=kleen::U;
  constexpr kleen T=kleen::T;
  constexpr kleen N=kleen::N;
    inline static const kleen a[3][3]={
      {F,F,F},
      {F,U,U},
      {F,U,T},
    };
    inline static const kleen o[3][3]={
      {F,U,T},
      {U,U,T},
      {T,T,T},
    };
    inline static const kleen n[]={T,U,F,N};
    inline static const char c[]={'F','U','T','N'};
#if 0
  template<typename E>
  constexpr auto i(E e) -> typename std::underlying_type<E>::type 
  {
    return static_cast<typename std::underlying_type<E>::type>(e);
  }
  const kleen& operator[]( const kleen a[],const kleen &i){
    return a[static_cast<int>(i)];
  }
  const kleen[]& operator[]( const kleen a[][],const kleen &i){
    return a[static_cast<int>(i)];
  }
#endif
#define i(x)  static_cast<int>(x)
    const kleen operator &(const kleen &l, const kleen &r){
      NOTRACE( std::cout << __PRETTY_FUNCTION__ << std::endl; )
	return a[i(l)][i(r)];
    }
    const kleen operator &(const kleen &l, bool r){
      NOTRACE( std::cout << __PRETTY_FUNCTION__ << std::endl; )
	return a[i(l)][i(r?T:F)];
    }
    const kleen operator |(const kleen &l, const kleen &r){
      NOTRACE( std::cout << __PRETTY_FUNCTION__ << std::endl; )
	return o[i(l)][i(r)];
    }
    const kleen& operator !(const kleen &t){
      NOTRACE( std::cout << __PRETTY_FUNCTION__ << std::endl; )
	return n[i(t)];
    }
   std::ostream& operator<<(std::ostream& os, const kleen  &s){
     return os << c[i(s)];
    }
#undef i
    template<bool is_const_iterator = true>
    class _iterator  /*: public std::iterator<std::forward_iterator_tag,PointerType>*/{
      typedef typename std::conditional<is_const_iterator, char, char>::type base_iterator;
      typedef typename std::conditional<is_const_iterator, const kleen, kleen>::type base_value;
      public: 
      base_iterator _this;
      base_value operator*(){ return static_cast<base_value>(_this); }
      _iterator &operator++(){
	++_this;
	return *this;
      }
      _iterator(const kleen&b):_this(static_cast<base_iterator>(b)){}
      bool operator !=(_iterator &i){
        return _this!=i._this;
      }
    }; //end class kleen::_iterator
   typedef _iterator<true> const_iterator;
   const_iterator  begin(const kleen&b){
     return const_iterator(F);
   }
   const_iterator  end(const kleen&b){
     return const_iterator(N);
   }
}
#if __INCLUDE_LEVEL__ + !defined __INCLUDE_LEVEL__ < 1+defined __TEST
#pragma message XSTR(__INCLUDE_LEVEL__)
#pragma message XSTR(__FILE__)
int main(){
  qtl::kleen t;
  std::cout << t << !t << !qtl::kleen::U << std::endl;
  for( auto l:qtl::kleen() ){
    std::cout << "!" << l << "=" << !l << " ";
    for( auto r:qtl::kleen() ){
      std::cout << l << "|" << r << "=" << (l|r) << " "<< l << "&" << r << "=" << (l&r) << "  ";
    }
    std::cout << std::endl;
  }
}
#endif
