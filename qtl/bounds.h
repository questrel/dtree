#include "interval.h"
//#if 1
template<typename S>
qtl::boundary<S> operator--(const qtl::basic_interval<S> &i){
  NOTRACE( std::cerr << __PRETTY_FUNCTION__ << '\n'; )
   return  i.l();
}
template<typename S>
qtl::boundary<S> operator++(const qtl::basic_interval<S> &i,int){
  NOTRACE( std::cerr << __PRETTY_FUNCTION__ << '\n'; )
  return  i.u();
}
//#else
template<typename S>
qtl::boundary<S> operator--(const S &s){
  NOTRACE( std::cerr << __PRETTY_FUNCTION__ << '\n'; )
    return  qtl::boundary<S>(s,qtl::infi);
}
template<typename S>
qtl::boundary<S> operator++(const S &s,int){
  NOTRACE( std::cerr << __PRETTY_FUNCTION__ << '\n'; )
    return  qtl::boundary<S>(s,qtl::supre);
}
//#endif

namespace x{
#if 0
  class hemi{
    qtl::boundary b;
    bool upper;
    interval operator&(const hemi &h){
      if( upper && !h.upper ){
	return qtl::interval(h.b,b);
      }
    }
  }; // end class hemi
#endif
  constexpr bool lt=false;
  constexpr bool gt=true;
  template<bool is_gt,typename S=lex::string,
    typename = std::enable_if_t<std::is_base_of_v<lex::string,S>||std::is_same_v<lex::number,S>||std::is_same_v<lex::scalar,S>>
  >
  class hemi:public qtl::boundary<S>{
    typedef qtl::boundary<S> base_t;
    using base_t::base_t;
    public:
    template<typename T>
    hemi(const T&t):base_t(t){}
    qtl::basic_interval<S> operator&&(const hemi<!is_gt,S> &h){
      if constexpr( is_gt ){
	  if( h < *(base_t*)this ){
	    throw std::range_error("lower bound greater than upper bound");
          }
	return qtl::basic_interval<S>(*(base_t*)this,h);
      }else{
         if( *(base_t*)this < h ){
	    throw std::range_error("lower bound greater than upper bound");
          }
        return qtl::basic_interval<S>(h,*(base_t*)this);
      }
    }
    qtl::basic_interval<S> operator||(const hemi<!is_gt,S> &h){
      if constexpr( is_gt ){
	if( *(base_t*)this < h ){
	  NOTRACE( std::cerr << __PRETTY_FUNCTION__ << *(base_t*)this << " < " << h << '\n'; )
	    throw std::range_error("overlapping bounds");
         }
	return qtl::basic_interval<S>(*(base_t*)this,h);
      }else{
	if( (base_t)h < *(base_t*)this ){
	   NOTRACE( std::cerr << __PRETTY_FUNCTION__ << h << " < " << *(base_t*)this << '\n'; )
	    throw std::range_error("overlapping bounds");
          }
        return qtl::basic_interval<S>(h,*(base_t*)this);
      }
    }
    operator qtl::basic_interval<S>(){
     if constexpr( is_gt ){
	 return qtl::basic_interval<S>(*this,{});
      }else{
       return qtl::basic_interval<S>({},*this);
     }
    }
    std::ostream& write( std::ostream &os )const{
      os << '(';
      if constexpr (is_gt){
	  os << this->value() << (this->ma()==qtl::infi?"<=":"<") << "x::x";
      }else{
  	  os << "x::x" << (this->ma()==qtl::infi?"<":"<=") << this->value();
      }
      os << ')';
      return os;
    }
    friend std::ostream& operator<<(std::ostream& os, hemi &s){
          return s.write(os);
    }
  }; // end class hemi
  template<typename S, typename B=qtl::boundary_type_t<S> >
    qtl::basic_interval<B> operator<(const hemi<gt,B>&h,const S &s){
    return qtl::basic_interval<B>((qtl::boundary<B>)h,qtl::boundary<B>(s,qtl::infi));
    }
  template<typename S, typename B=qtl::boundary_type_t<S> >
    qtl::basic_interval<B> operator<=(const hemi<gt,B>&h,const S &s){
    return qtl::basic_interval<B>((qtl::boundary<B>)h,qtl::boundary<B>(s,qtl::supre));
    }
  class _{
  public:
    template<class T, typename B=qtl::boundary_type_t<T> >
    qtl::basic_interval<B> operator[](const T &s)const{
      NOTRACE( std::cerr << __PRETTY_FUNCTION__ << '(' << s << ')' << '\n'; )
      return qtl::basic_interval<B>((B)s);
    }
  }; // end class x;
  static inline _ x;
  template<class S>
    decltype(x[std::declval<S>()]) operator==(const _&x, const S &s){
    return x[s];
  }
  template<class S>
    decltype(x[std::declval<S>()]) operator==(const S &s, const _&x){
    return x[s];
  }
  template<class S>
    decltype(x[std::declval<S>()]) operator!=(const _&x, const S &s){
    return !x[s];
  }
  template<class S>
    decltype(x[std::declval<S>()]) operator!=(const S &s, const _&x){
    return !x[s];
  }
  template<typename S>
  qtl::boundary<S> operator|(const _ &x,const S &s){ // x::x | s
    return qtl::boundary<S>(s,qtl::infi);
  }
  template<typename S>
  qtl::boundary<S> operator|(const S &s,const _& x){ // s | x::x
    return qtl::boundary<S>(s,qtl::supre);
  }
  template<typename S>
  hemi<lt,qtl::boundary_type_t<S>> operator<(const _&x, const S &s){ // x::x < s
    return hemi<lt,qtl::boundary_type_t<S>>(qtl::boundary<qtl::boundary_type_t<S>>(s,qtl::infi));
  }
  template<typename S>  
    hemi<gt,qtl::boundary_type_t<S>> operator<(const S &s, const _ &x){ // s < x::x
    return hemi<gt,qtl::boundary_type_t<S>>(qtl::boundary<qtl::boundary_type_t<S>>(s,qtl::supre));
  }
  template<typename S>  
  hemi<gt,qtl::boundary_type_t<S>> operator>(const _ &x,const S &s){ // x::x > s
    return hemi<gt,qtl::boundary_type_t<S>>(qtl::boundary<qtl::boundary_type_t<S>>(s,qtl::supre));
  }
#if 1
  template<typename S>
    qtl::basic_interval<qtl::boundary_type_t<S>> operator<(const qtl::boundary_type_t<S>&b,const _ &x){
    return qtl::basic_interval<qtl::boundary_type_t<S>>({},b);
  }
  template<typename S>
    qtl::basic_interval<qtl::boundary_type_t<S>> operator<(const _&x,const qtl::boundary_type_t<S>&b){
    return qtl::basic_interval<qtl::boundary_type_t<S>>(b,{});
  }
#endif
  template<typename S, typename B=qtl::boundary_type_t<S>>
  hemi<lt,B> operator<=(const _&x, const S &s){ // x::x <= s
    return hemi<lt,B>(qtl::boundary<B>(s,qtl::supre));
  }
  template<typename S, typename B=qtl::boundary_type_t<S>>
   hemi<gt,B> operator<=(const S &s, const _ &x){ // s <= x::x
    return hemi<gt,B>(qtl::boundary<B>(s,qtl::infi));
  }  
  template<typename S, typename B=qtl::boundary_type_t<S>>
  hemi<gt,B> operator>=(const _&x, const S &s){ // x::x >= s
    return hemi<gt,B>(qtl::boundary<B>(s,qtl::infi));
  }

}; // end namespace x

//static inline qtl::boundary<lex::scalar> ε=0_s++; // epsilon

template<typename S>
qtl::basic_interval<S> operator,(const qtl::boundary<S> &l,const qtl::boundary<S> &r){
	  return qtl::basic_interval<S>(l,r);
}
#if __INCLUDE_LEVEL__ + !defined __INCLUDE_LEVEL__ < 1+defined TEST_H
int main(int argc, char *argv[]){
if( argc==1 || argv[1][0] == '<' ){
  std::ifstream in;
  std::streambuf *cinbuf;
  if( argc>1 && argv[1][0] == '<' ){
    in = std::ifstream(argv[1][1]?argv[1]+1:argv[2]);
    cinbuf = std::cin.rdbuf(); 
    std::cin.rdbuf(in.rdbuf()); //redirect std::cin
  }
  int n=0;
  lex::number l(0.0);
  lex::number r(1.0);
  std::cout << qtl::setverbose(qtl::ios_base::fmtflags::none);
  while( std::cin.good() && std::cin.tellg()<1000 ){
    std::string i;
    std::getline(std::cin, i);
    if( std::cin.tellg()*++n>2000 ){ break; }
    r=lex::number::sem(i);
    std::cout << (l <= x::x < r) << '\n';
    std::cout << x::x[l] << '\n';
    std::cout << (x::x == r) << '\n';
    try{
      std::cout << ( l <= x::x && x::x < r ) << '\n';
    }catch( const std::range_error& e ){
     std::cerr << e.what() << '\n';
     std::cout << ( l <= x::x  || x::x < r ) << '\n';
    }
    try{
      std::cout << (x::x < l || r <= x::x) << '\n';
    }catch( const std::range_error& e ){
     std::cerr << e.what() << '\n';
     std::cout << (x::x < l && r <= x::x)<< '\n';
    }
    l=r;
  }
  exit(0);
 }
 if( std::strcmp(argv[1],"dict")==0 ){
     std::cout << "\"9\"\n";
#ifdef FUZZING
  exit(0);
#endif
#include "string.h"
 using  namespace lex::literals;

 std::cout << qtl::setverbose(qtl::ios_base::fmtflags::none);
 using qtl::ε;
   std::cout << -ε << '\n';
   std::cout << ε << '\n';
   std::cout << 0-ε << '\n';
   std::cout << 0+ε << '\n';
   std::cout << --1.0_s << '\n';
   std::cout << 1.0_s-ε << '\n';
   std::cout << 1.0_s++ << '\n';
   std::cout << 1.0_s+ε << '\n';
   std::cout << --"1.0"_s << '\n';
   std::cout << "1.0"_s-ε << '\n';
   std::cout << "1.0"_s++ << '\n';
   std::cout << "1.0"_s+ε << '\n';
   }else{
 std::cout << 1.0 << '\n';
 std::cout << 2.0 << '\n';
    exit(0);
   }
 using namespace lex::literals;
 std::cout << qtl::setverbose(qtl::ios_base::fmtflags::none);
 std::cout << (--x::x[0.0], x::x[1.0]++) << '\n';
 std::cout << (x::x["0.0"_s]++, --x::x["1.0"_s]) << '\n';
 auto l= 0.5;
 auto r= 1.5;
 std::cout << l << '\n';
 std::cout << r << '\n';
 std::cout << (l <= x::x < r) << '\n';
 auto lt =  l <= x::x;
 std::cout << (lt < r) << '\n';
 std::cout << ( l <= x::x  && x::x < r) << '\n';;
 std::cout << x::x[l] << '\n';
 std::cout << x::x[r] << '\n';
 std::cout << ( x::x == l) << '\n';
 std::cout << ( r == x::x ) << '\n';
 std::cout << (r < x::x <= l) << '\n';
 auto gt = x::x < r;
 // std::cout << (l <= x::x) << '\n';
 try{
  std::cout << (x::x <= l || r < x::x);
 }catch( const std::range_error& e ){
     std::cerr << e.what() << '\n';
     std::cout << (x::x <= l && r < x::x);
 }
 try{
   std::cout << ( r <= x::x  && x::x < l) << '\n';;
 }catch( const std::range_error& e ){
   std::cerr << e.what() << '\n';
   std::cout << ( r <= x::x  || x::x < l) << '\n';;
 }
}
#endif
