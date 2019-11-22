#pragma once
#include "out.h"
#include "number.h"
#include "bool.h"
#include "container.h"
#include <random>
#include <cmath>
#include <functional>
#include <cstddef>
//#ifndef __clang__
#include <optional>
//#else
//#include <experimental/optional>
//#endif
#ifdef __clang__
//#define BND \U00e28baf
#else
#define BND bnd
#endif

namespace qtl{
using std::nullptr_t;
#if 0
  template<typename T>
  T operator*(const T &a,const T &b){ 
      return static_cast<T>(static_cast<int>(a)*static_cast<int>(b));
  }
#else
// error: chosen constructor is explicit in copy-initialization
//       return{a.value()*b.value(),a.ma()*b.ma()};
#endif
 template<typename S>
 class boundary_type{
 public:
   typedef typename std::conditional< 
                  std::is_same_v<S,lex::scalar>,
                  lex::scalar,
                  typename std::conditional<
                    std::is_arithmetic_v<S>||std::is_same_v<S,lex::number>,
                    lex::number,
                    lex::string
                  >::type
	   >::type type;
 };
template< class T >
using boundary_type_t = typename boundary_type<T>::type;

 template<typename T,typename = std::enable_if_t<std::is_scalar<T>::value>>
   bool operator<(T a,T b){ NOTRACE( std::cerr << __PRETTY_FUNCTION__ << "(" << a << ", " << b << ")\n"; ) return static_cast<int>(a)<static_cast<int>(b); }
  enum class lim{inf=-1,sup=1};
  constexpr lim  infi=lim::inf;
  constexpr lim supre=lim::sup;
#if 0
  lim operator-(const lim&l){return static_cast<lim>(-static_cast<int>(l));}
#endif
  std::ostream& operator<<(std::ostream& os, lim s){
    return os << (s==infi?'-':'+');
  }
  class sign{
    public:
    typedef  enum {minus=-1,zero=0,plus=1} base_t;
    base_t _this;
    sign(base_t b=zero):_this(b){}
    sign(const lex::number &s):_this((s.empty()||(unsigned char)s._this._this[0]<(unsigned char)'\x80')?minus:(unsigned char)s._this._this[0]>(unsigned char)'\x80'?plus:zero){ 
      /**/NOTRACE( std::cout << __PRETTY_FUNCTION__ << "(" << s << ")" << std::endl; )      
    }
    sign(lim l):_this(l==infi?minus:plus){};
    template<typename T,typename = std::enable_if_t<std::is_scalar<T>::value>>
    sign(T n=0):_this(n<0?minus:0<n?plus:zero){}
    //    template<typename T>
    //    inline operatorT()const{ return static_cast<T>(_this); }
    friend base_t operator||(base_t a,base_t b){ return a!=zero?a:b; }
    friend base_t operator+(base_t a,base_t b){ return a==b?a:zero; }
    inline sign operator||(const sign& b)const{ return _this || b._this; }
    inline sign operator||(const lim& b)const{ 
       NOTRACE( std::cout << __PRETTY_FUNCTION__ << "(" << *this << ", " << b << ")" << std::endl; )      
       return _this || ((sign)b)._this;
    }
    inline sign operator+(const sign& b)const{ return _this + b._this; }
    bool operator==(const sign& b)const{ return _this==b._this; }
    //sign operator*(const sign &b)const{ 
    //  return static_cast<sign>(static_cast<int>(_this)*static_cast<int>(b));
    //}
    sign operator-()const{
         return static_cast<sign>(-static_cast<int>(_this));
    }
    operator lim()const{
      assert(_this != zero);
      return _this<0?infi:supre;
    }
    operator int()const{ return static_cast<int>(_this); }
    std::ostream& write( std::ostream& os )const{
      return os << (_this<0?"--":_this>0?"++":"0");
    }
    friend std::ostream& operator<<(std::ostream& os, const sign &s){
      return s.write(os);
    }
    bool operator!=(const sign &b)const{ return static_cast<int>(_this)!=static_cast<int>(b._this); }
    bool operator<(const sign &b)const{ return static_cast<int>(_this)<static_cast<int>(b._this); }
    bool operator<=(const sign &b)const{ return static_cast<int>(_this)<=static_cast<int>(b._this); }
    using lex_t=lex::number;
    static const int depth=lex_t::depth;
    sign( const std::string_view &v ){
      *this=static_cast<lex::number>(v);
    }
#if 0
    operator lex_t()const{
      lex_t ret(lex::number(*this));
      return ret;
    }
#endif
  }; // end class sign
  sign operator+(lim a,lim b){
    //    return a==b?(a==infi?sign::minus:sign::plus):sign::zero;
       return static_cast<sign>(static_cast<int>(a)+static_cast<int>(b)-1);
  }
  template<typename S/*=lex::number*/, typename L/**/=sign/**/, lim i/**/=infi/**/, typename = std::enable_if_t<std::is_same_v<lex::string,S>||std::is_same_v<lex::number,S>||std::is_same_v<lex::scalar,S>>>
  class basic_boundary;
}; // end namespace qtl
namespace lex{
  template<typename S=lex::string/*number*/, typename L=qtl::sign, qtl::lim i=qtl::infi, typename = std::enable_if_t<std::is_same_v<lex::string,S>||std::is_same_v<lex::number,S>||std::is_same_v<lex::scalar,S>>>
  class basic_boundary:public lex::string{
  public:
  using base_t=lex::string;
  using base_t::base_t;
  static const int depth=-1;
  static const int Shallow=1;
  basic_boundary(const qtl::basic_boundary<S,L,i> &q):string(q.value(),eof((int)q.ma())){
    NOTRACE( std::cout << __PRETTY_FUNCTION__ << "(" << q << ")" << std::endl; )
    NOTRACE( std::cout << '=' << *this << std::endl; )
  }
 template<class C, typename=std::enable_if_t<std::is_convertible_v<C,S> > >
  basic_boundary(const qtl::basic_boundary<C> &q):string(q.value(),eof((int)q.ma())){
  }
#if 0
  operator qtl::basic_boundary<S,L,i>()const{
    NOTRACE( std::cout << __PRETTY_FUNCTION__ << std::endl; )
    std::string s=(*this)._this;
    s.back()='\x7e';
    qtl::basic_boundary ret{(S)s,(*this)._this.back()-0x7e};
    NOTRACE( std::cout << '=' << ret << std::endl; )
    return ret;
  }
#endif
  };
}; // end namespace lex;
namespace qtl{
  //  template<typename S=lex::number, typename L=sign, lim i=infi, typename = std::enable_if_t<std::is_base_of_v<lex::string,S>||std::is_same_v<lex::number,S>>>
  template<typename S=lex::number, typename L/*=sign*/, lim i/*=infi*/, typename/* = std::enable_if_t<std::is_same_v<lex::string,S>||std::is_same_v<lex::number,S>>*/>
  class basic_boundary:public std::pair<S,L>{
  public:
    typedef std::pair<S,L> base_t;
    using base_t::base_t;
    const S& value()const{ return std::get<0>(*this); }
    const L& ma()const{ return std::get<1>(*this); } /* ma = kanji &#38291; = gap/space  also, {infi|supre}() or ()rgin  */
    S& value(){ return std::get<0>(*this); }
    L& ma(){ return std::get<1>(*this); } /* ma = kanji &#38291; = gap/space  also, {infi|supre}() or ()rgin  */
    basic_boundary(const basic_boundary<S,sign> &b):base_t(b.value(),b.ma()||i){
      NOTRACE( std::cout << __PRETTY_FUNCTION__ << "(" << b << ")" << std::endl; )      
      NOTRACE( std::cout <<  b.ma() << " || " <<  i << std::endl; )      
    }
    basic_boundary():base_t(lex::string(),infi){};
    bool is_inf()const{
      NOTRACE( std::cerr << __PRETTY_FUNCTION__ << "(" << *this << ")\n"; )
#if 0
	if constexpr ( std::is_same_v<S,lex::number> ){
            NOTRACE( std::cerr << "is_inf=" <<  value().is_inf() << "\n"; )	    
	    if( value().is_inf() ){ return true; }
	}
#endif
      NOTRACE( std::cerr << value().empty() << "&&" << (ma()==(L)infi) << "\n"; )
      return value().empty()&&ma()==(L)infi;
    }
    basic_boundary operator-()const{
      return basic_boundary(-value(),-ma());
    }
    basic_boundary operator[](nullptr_t n)const{ 
      return basic_boundary(value(),-ma());
    }
    operator sign()const{
      return (sign)value()||(sign)ma();
    }
    std::ostream& write( std::ostream& os )const{
      qtl::ios_base::fmtflags f;
      os >> f;
      std::string t=qtl::type_name<decltype(*this)>(f.verbosity);
      switch( f.verbosity & qtl::ios_base::fmtflags::intervalnotation ){
      case qtl::ios_base::fmtflags::setbuilder:{
      };[[fallthrough]];
      case qtl::ios_base::fmtflags::boundary:{
	if( ma() < (sign)0 ){ os << "--"; }
        if( (f.verbosity & qtl::ios_base::fmtflags::alltypeinfo)!=qtl::ios_base::fmtflags::none ){
          os  << "x::x[" << value() << "]";
        }else{
          os << value();
        }
	if( (sign)0 < ma() ){ os << "++"; }
      };break;
      default:{
	// [[unreachable]]
      };break;
      }
      return os;
    }
    friend std::ostream& operator<<(std::ostream& os, const basic_boundary &s){
      return s.write(os);
    }
  bool is_pole()const{
    return value().empty() && ma()<(sign)0;
  }
    bool operator<(const basic_boundary & b)const{
      NOTRACE( std::cerr << __PRETTY_FUNCTION__ << "\n"; )
      return is_pole() || b.is_pole() || value()<b.value() || (value()==b.value()&&ma()<b.ma());
    }
    bool operator<=(const basic_boundary & b)const{
      NOTRACE( std::cerr << __PRETTY_FUNCTION__ << "\n"; )
      return is_pole() || b.is_pole() || value()<b.value() || (value()==b.value()&&ma()<=b.ma());
    }
    bool operator<(const S& s)const{
      NOTRACE( std::cerr << __PRETTY_FUNCTION__ << "(" << *this << ", " << s << ")\n"; )
      NOTRACE( {
	  auto u=is_pole() ;
          auto v_s=(value()<s);
          auto m_0=(ma()<(sign)0);
	  auto __=(value()==s);
          std::cerr << u << "||" << v_s << "|| (" << m_0  <<  "&&" << __ << ")\n"; 
	})
      	if constexpr ( std::is_same_v<S,lex::number> ){
	    if( value().D || s.D ){
	      return is_pole()|| (double)value()<(double)s || (ma()<(sign)0 && (double)value()==(double)s);
            }
       }
       return is_pole() || value()<s || (ma()<(sign)0 && value()==s);
    }
    friend bool operator<(const S& s,const basic_boundary&t){
      NOTRACE( std::cerr << __PRETTY_FUNCTION__ << "(" << s << ", " << t << ")\n"; )
      NOTRACE( std::cerr << t.is_pole() << "||" << (s<t.value()) << "|| (" << ((sign)0<t.ma()) <<  "&&" << (s==t.value()) << ")\n"; )
      	if constexpr ( std::is_same_v<S,lex::number> ){
	    if( t.value().D || s.D ){
              return t.is_pole() || (double)s<(double)t.value() || ((sign)0<t.ma() && (double)s==(double)t.value());
            }
       }
       return t.is_pole() || s<t.value() || ((sign)0<t.ma() && s==t.value());
    }
     friend bool operator<=(const S& s,const basic_boundary&t){
      NOTRACE( std::cerr << __PRETTY_FUNCTION__ << "(" << s << ", " << t << ")\n"; )
      NOTRACE( std::cerr << t.is_pole() << "||" << (s<t.value()) << "|| (" << ((sign)0<t.ma()) <<  "&&" << (s==t.value()) << ")\n"; )
      	if constexpr ( std::is_same_v<S,lex::number> ){
	    if( t.value().D || s.D ){
              return t.is_pole() || (double)s<(double)t.value() || ((sign)0<=t.ma() && (double)s==(double)t.value());
            }
       }
       return t.is_pole() || s<t.value() || ((sign)0<=t.ma() && s==t.value());
    }
    basic_boundary operator+(const basic_boundary &r)const{
      NOTRACE( std::cout << __PRETTY_FUNCTION__ << "(" << *this << ", " << r << ")" << "<" << i << ">" << std::endl; )      
#if 0
       return {(*this).value()+r.value(),((*this).ma()+r.ma())};   //error: chosen constructor is explicit in copy-initialization
#else
//      return basic_boundary((*this).value()+r.value(),((*this).ma()+r.ma())/*||i*/);
      return basic_boundary(value()+r.value(),(ma()+r.ma()));
#endif
    }
    basic_boundary operator*(const basic_boundary &b)const{
       #define a (*this)
      /**/NOTRACE( std::cout << "\n" << __LINE__ << ":\n" 
      << __PRETTY_FUNCTION__ << "(" << *this << ", " << b << ")" << "<" << i << ">" << std::endl; )
      /**/NOTRACE( std::cout << a.value() << "*" << b.value() << "=" << a.value()*b.value() << std::endl; )
       return basic_boundary(a.value()*b.value(),(sign)b.value()*a.ma()+(sign)a.value()*b.ma());
       #undef a
    }
    basic_boundary operator/(const basic_boundary &b)const{
       #define a (*this)
       NOTRACE( std::cout << __PRETTY_FUNCTION__ << "(" << *this << ", " << b << ")" << std::endl; )      
       NOTRACE( std::cout << a.value() << "/" << b.value() << "=" << a.value()/b.value() << std::endl; )      
      if( b.value()==0 ){ 
	 NOTRACE( std::cerr<< *this << "/" << b << "\n"; )      
	 NOTRACE( std::cerr << "= boundary(" << a.value()/b.value() << "," << (sign)b.value()*a.ma()<< "+" << (sign)a.value()*b.ma() << ")\n"; )
	 return basic_boundary();
        }
        return basic_boundary(a.value()/b.value(),(sign)b.value()*a.ma()+(sign)a.value()*b.ma()+a.ma()*b.ma());
       #undef a
    }
    operator lex::string(){
      lex::string ret;
      ret += value()._this;
      ret += lex::eof(ma());
      return ret;
    }
  //  using lex_t=lex::basic_boundary<S,L,i>;
  using lex_t=lex::basic_boundary<lex::string,sign,infi>;
  static const int depth=-1;
  static const int Shallow=1;
  basic_boundary(const lex_t &l){
    NOTRACE( std::cerr << __PRETTY_FUNCTION__ << "(" << l << ")" << std::endl; )
    NOTRACE( std::cerr << (void*)l._this.data()<<","<<l._this.size()<<"\n"; )
      assert(l._this.size()>=2);
     std::string ss(l._this.data(),l._this.size()-2);
    lex::string ls;
    ls._opt=ss;
    ls._this=std::string_view(ls._opt->data(),ls._opt->size());
    NOTRACE( std::cout << "string_view("<<(void*)ls._this.data()<<","<<ls._this.size()<<")\n"; )
    sign s=l._this.back()-0x7e;
    //    TRACE( std::cout << qtl::visible(*ls._opt) << "\n"; )
    //    ls._opt->back()='\x7e';
    //    TRACE( std::cout << qtl::visible(*ls._opt) << "\n"; );
    *this={(S)ls,s};
    NOTRACE( std::cout << *this << std::endl; )
  }
  explicit operator lex_t()const{
    NOTRACE( std::cout << __PRETTY_FUNCTION__ << "(" << *this << ")" << std::endl; )
    lex_t ret{value(),lex::eof((int)ma())};
    NOTRACE( std::cout << "ret:" << ret << std::endl; )
    return ret;
  }
  }; // end class basic_boundary
  template<typename S=lex::number, typename L=sign, lim i=infi>
   basic_boundary<S,L,i> operator/(double a,const basic_boundary<S,L,i> &b){
     /**/NOTRACE( std::cout << __PRETTY_FUNCTION__ << "(" << a << ", " << b << ")" << std::endl; )
     /**/NOTRACE( std::cout << "{" << a/b.value() << ", " << -b.ma()*(sign)a << "}" << std::endl; )
     return basic_boundary<S,L,i>(a/b.value(),-b.ma()*(sign)a);
   }

  template<typename S=lex::scalar/*number*/>
  using boundary =  basic_boundary<S,sign,infi>;
  //  using  namespace qtl::literals;
  template<typename S=lex::scalar/*number*/,  
          typename = std::enable_if_t<
             std::is_base_of_v<lex::string,S>||std::is_same_v<lex::number,S>||std::is_same_v<lex::scalar,S>
           >
   >
  class basic_interval;
}; // end class qtl
namespace lex{
  template<typename S=lex::string,  typename = std::enable_if_t<std::is_base_of_v<lex::string,S>||std::is_same_v<lex::number,S>> >
#define BASE_T lex::vector<typename qtl::boundary<lex::string>::lex_t/*,1*/>
  class interval:public BASE_T{
  using base_t=BASE_T;
  #undef BASE_T
  public:
  using base_t::base_t;
  static const int depth=base_t::depth;
  interval(const qtl::basic_interval<S> &i):base_t(static_cast<typename qtl::boundary<S>::lex_t>(i.l()),static_cast<typename qtl::boundary<S>::lex_t>(i.u())){
      NOTRACE( std::cerr << __PRETTY_FUNCTION__ << "(" << i << ")" << '\n'; )
      NOTRACE( std::cerr << "=" << *this << '\n'; )
    }
  }; // end class interval
}; // end class lex
namespace qtl{
   template<typename S/*=lex::number*/ ,  typename /*= std::enable_if_t<std::is_base_of_v<lex::string,S>||std::is_same_v<lex::number,S>>*/ >
    //  class basic_interval:public std::pair<boundary<S>,boundary<S>>{
   class basic_interval:public std::array<boundary<S>,2>{
   public:
   typedef boundary<S> b_t;
   //typedef std::pair<b_t,b_t> base_t;
   typedef std::array<b_t,2> base_t;
   base_t base_v()const{
     return *(base_t*)this;
   }
    using base_t::base_t;
    std::
  //    #if (defined __clang__)
  //    experimental::
  //    #endif
    optional<bool> mutable o;
    template<lim l>
    using b_ =  basic_boundary<S,sign,l>;
    const b_t& l()const{ return std::get<0>(*this); } 
    const b_t& u()const{ return std::get<1>(*this); } 
    b_t& l(){ return std::get<0>(*this); } 
    b_t& u(){ return std::get<1>(*this); } 
    bool bipole()const{
      if( !o ){ (this)->o=!( is_connected() ) ; }
       return 
#ifndef __clang__
       o.value();
#else
      *o;
#endif
    }
  bool is_inf()const{ return l().value().is_inf() && u().value().is_inf(); }
  bool is_point()const{ return l().value()==u().value() && !l().value().is_null() && l().ma() < u().ma(); }
  template<typename T>
  bool is_point(const T &v)const{ return l().value()==v && u().value()==v && l().ma() < u().ma(); }
  bool is_null()const{ return l().value().is_null() || u().value().is_null(); }
  bool is_connected()const{ return l()<=u() ; }
  bool is_empty()const{ return l()==u() && !l().is_pole(); }
  bool is_universal()const{ return l()==u() && l().is_pole(); }
   basic_interval():base_t(){}
   basic_interval(std::nullptr_t n):basic_interval(b_t(lex::string(),infi),b_t(S(n),infi)){}

    template<typename T0,typename T1,typename S0,typename S1,lim I0,lim I1>
   basic_interval(const basic_boundary<T0,S0,I0> &l, const basic_boundary<T1,S1,I1> &u):base_t({b_t(l.value(),l.ma()),b_t(u.value(),u.ma())}){}

  basic_interval(const b_t &l, const b_t &u):base_t({l,u}){}

  //    basic_interval(const S &l,const lim &L,const S &u,const lim &U):base_t({l,L},{u,U}}){}
   basic_interval(const S &l,const lim &L,const S &u,const lim &U):base_t({b_t(l,L),b_t(u,U)}){}
  //    basic_interval(const S &l):base_t({l,infi},{l,supre}){}
  basic_interval(const S &l):base_t({b_t(l,infi),b_t(l,supre)}){}
  template<typename T>
  basic_interval(basic_interval<T> &t):base_t({b_t((S)t.l().value(),t.l().ma()),b_t(t.u().value(),t.u().ma())}){}
  basic_interval(const kleen &k){
      NOTRACE( std::cerr << __PRETTY_FUNCTION__ << "(" << k << ")" << '\n'; )
      if( k==kleen::U ){
	*this=nullptr;
      }
      if( k==kleen::F ){
        u().value()=(S)0;
        l().value()=(S)0;
      }
      if( k!=kleen::T ){
      }
      NOTRACE( std::cerr << "=" << *this << "\n"; )
  }
  operator kleen()const{
    NOTRACE( std::cerr << __PRETTY_FUNCTION__ << "(" << *this << ")" << "\n"; )
    NOTRACE( std::cerr << l() << ", " << u() << "\n"; )
    if( l() != u() ){ return kleen::U; }
    if( l().is_pole() ){ return kleen::T; }
    return kleen::F;
  }
  operator bool()const{
    NOTRACE( std::cerr << __PRETTY_FUNCTION__ << "(" << *this << ")" << "\n"; )
    NOTRACE( std::cerr <<  (kleen)*this << "=" <<  ((kleen)*this != kleen::F) << "\n"; )
    return (kleen)*this != kleen::F;
;
  }
  operator lex::string()const{
    NOTRACE( std::cerr << __PRETTY_FUNCTION__ << "(" << *this << ")" << "\n"; )    
    lex::string ret;
    for( auto x:*this ){
      NOTRACE( std::cerr << "x:"  << x << "\n"; )    
      lex::string s=x;
      NOTRACE( std::cerr << "s:"  << qtl::visible(s._this) << "\n"; )    
      ret += s;
    }
    NOTRACE( std::cerr << "ret:"  << qtl::visible(ret._this) << "\n"; )    
    return ret;
  }
  template<class C, typename=std::enable_if_t<std::is_convertible_v<C,S> > >
    basic_interval(const basic_interval<C> &c):basic_interval((b_t)c.l(),(b_t)c.u()){
    NOTRACE( std::cerr << __PRETTY_FUNCTION__ << "(" << c << ")" << "\n"; )
  }
  //  template<class C, typename=std::enable_if_t<std::is_base_of_v<C,S> || std::is_base_of_v<S,C>> >
  //template<class C, typename=std::enable_if_t<std::is_convertible_v<C,S> && !std::is_same_v<C,lex::interval> > >
  template<class C, typename=std::enable_if_t<std::is_convertible_v<C,S> > >
  basic_interval(const C &c):basic_interval((S)c){}
  basic_interval(const S &l,const S &r):basic_interval(b_t(l,infi),b_t(r,infi)){}
    //    template<typename T0,typename T1>
    //       basic_interval(const T0 &l,const T1 &u):basic_interval({(S)l,L},{(S)u,U}){}
    //    basic_interval(const lex::number &l,const lex::number &u):basic_interval({(S)l,L},{(S)u,U}){}

#if 0
/* 
         <   =   >   []  ()      && 
[ ] ( )  T   F   F    x   x      0
[ ] ) (  U   U   U    x          [ ]
[ ) ] (  U   U   U    x          [ )
) [ ] (  U   F   U    x          0
[ ( ] )  U   U   U    x   x      ( ]
[ ( ) ]  U   U   U    x   x      ( )
[ ) ( ]  U   U   U    x          [ ] | ) ( 
) [ ( ]  U   U   U    x          ( ]
( [ ] )  U   U   U    x   x      [ ]
( [ ) ]  U   U   U    x   x      [ )
( ) [ ]  F   F   T    x   x      0
) ( [ ]  U   U   U    x          [ ]
] [ ( )  U   U   U        x      ( )
] [ ) (  U   U   U               [ ) 
^     ^
] ) [ (  U   U   U               ] (
^     ^    
) ] [ (  U   U   U               ) (
] ( [ )  U   U   U        x      [ )
] ( ) [  U   F   U        x      0
] ) ( [  U   U   U               ] [
) ] ( [  U   U   U               ) [
( ] [ )  U   U   U        x      ( ) | ] [
( ] ) [  U   U   U        x      ( ]
( ) ] [  U   U   U        x      ( )
) ( ] [  U   U   U               ( ]

<
[ ] ( )  T
( ) [ ]  F

>
[ ] ( )  F
( ) [ ]  T

==
[ ] ( )  F
) [ ] (  F
( ) [ ]  F
] ( ) [  F
] ( ) [  F
 
  */
#endif
    
#if 0
  class partial_ordering{
/*
<  <= == >=  > !=   < = >
T  T  F  F  F  T    T F F   0 <=> 1
F  T  T  T  F  F    F T F   0 <=> 0
F  F  F  T  T  T    F F T   0 <=> -1
U  T  U  U  F  U    U U F   0 <=> (0<=x::x<=1)
U  U  F  U  U  T    U F U   0 <=> (x::x!=0)
F  U  U  T  U  U    F U U   0 <=> (-1<=x::x<=0)
U  U  U  U  U  U    U U U   0 <=> (-1<=x::x<=1)

*/
}; // end class partial_ordering
#endif
    kleen operator<(const basic_interval &r)const{
      NOTRACE( std::cerr << __PRETTY_FUNCTION__ << "(" << *this << ", " << r << ')' << '\n'; ) // "f"_s, (  x::x <= "f"_s ))  // ("78"_s, ( "12"_s < x::x  ))
      if( !is_connected() || !r.is_connected() ){ return kleen::U; }
      NOTRACE( std::cerr << __LINE__ << '\n'; )
      if( is_point() && r.is_point() ){ return l().value() < r.l().value() ? kleen::T : kleen::F; }
      NOTRACE( std::cerr << __LINE__ << '\n'; )
      if( !(r.l()<u()) ){ return kleen::T; }
      NOTRACE( std::cerr << __LINE__ << '\n'; )
	NOTRACE(  std::cerr << "l():" << l() << ".value():" << l().value() <<  ", r.u():" << r.u() << ",value()" <<  r.u().value() << ", " << (l()<r.u()) << (l().value()<r.u().value()) <<  '\n'; )
    if( /*!is_null() && !r.is_null() &&*/ !l().is_inf() && !r.u().is_inf() && !(l().value()<r.u().value()) ){ return kleen::F; } // ! ([1] < [0,1])   // [1] < (0,...) // [a] < [...,...]
      NOTRACE( std::cerr << __LINE__ << '\n'; )
      return kleen::U;
    } 
    kleen operator>(const basic_interval &r)const{
     #if 1
      NOTRACE( std::cerr << __PRETTY_FUNCTION__ << "(" << *this << ", " << r << ')' << '\n'; )
      NOTRACE( std::cerr << "=" << (r < *this) << '\n'; )
      return r<*this;
     #else
      if( !is_connected() || !r.is_connected() ){ return kleen::U; }
      if( is_point() && r.is_point() ){ return l() > r.l() ? kleen::T : kleen::F; }
      if( !(l()<r.u()) ){ return kleen::T; }
      if( !(r.l()<u()) ){ return kleen::F; }
      return kleen::U;
     #endif
    } 
    kleen operator<=(const basic_interval &r)const{
      return !(*this>r);
    }
    kleen operator>=(const basic_interval &r)const{
      return !(*this<r);
    }
    kleen operator==(const basic_interval &r)const{
      if( is_point() && r.is_point() && u().value() == r.u().value() ){ return kleen::T; }
      if( (*this<r)==kleen::T ){ return kleen::F; }
      if( (r<*this)==kleen::T ){ return kleen::F; }
      return kleen::U;
    }

    kleen operator!=(const basic_interval &r)const{
      return !(*this==r);
    }

   basic_interval operator-()const{
     NOTRACE(  std::cout << __PRETTY_FUNCTION__ << "(" << *this << ")" <<  std::endl; )
     NOTRACE(  std::cout << "(" << -u() << ", " << -l() <<  std::endl; )
     NOTRACE(  std::cout << "return " << basic_interval(-u(),-l()) <<std::endl; )
      return basic_interval(-u(),-l());
    }
    std::ostream& write( std::ostream& os=std::cout )const{
      qtl::ios_base::fmtflags f;
      os >> f;  //
      if( is_null() ){
	os << "NULL";
	if( f.verbosity==qtl::ios_base::fmtflags::none ){
	  return os;
        }
      }
      os // << boost::core::demangle(typeid(*this).name()) 
      << (f.verbosity==qtl::ios_base::fmtflags::none&&is_point()?"":"( ");
      switch( f.verbosity & qtl::ios_base::fmtflags::intervalnotation ){
      case qtl::ios_base::fmtflags::setbuilder:{
	if( l()==u() ){
	  if( !l().value().empty() ){
	    os << "FALSE";
	    if( l().value() != 0 ){
	      NOTRACE( os << "(" << l().value() << ")";)
	    }
          }else{
	    os << "TRUE";
          }
	}else if( is_point() ){ 
	    os << (f.verbosity==qtl::ios_base::fmtflags::none?"":"x::x == ") << l().value();
        }else if( bipole() ){
          os << "x::x " << ((lim)u().ma()==infi?"< ":"<= ") << u().value() << " || " <<  l().value() << ((lim)l().ma()==infi?" <=":" <") << " x::x";
	}else{
	  //	  if( l().value()==u().value() && (lim)l().ma() == infi && (lim)u().ma()==supre ){
	    if( !l().is_inf()||u().is_inf() ){
              os <<  l().value() << ((lim)l().ma()==infi?" <=":" <");
            }
  	    os << " x::x ";
	    if( !u().is_inf()||l().is_inf() ){
              os <<  ((lim)u().ma()==infi?"< ":"<= ") << u().value();
	    }
        }
      };break;
      case qtl::ios_base::fmtflags::boundary:{
	if( is_point() ){
	  os << "x::x[" << l().value() << "]";
        }else{
	  os << l() << ", " << u();
        }
      };break;
      default:{
	// [[unreachable]]
      os << 
            ((lim)l().ma()==infi?")[":"](") << l().value() << 
	    (!bipole()?"...":" | ") <<
            u().value() << ((lim)u().ma()==infi?")[":"](");
      };
      }

#if 0
      os << (f.verbosity==qtl::ios_base::fmtflags::none&&is_point()?"":" )");
#else
      auto t=(f.verbosity==qtl::ios_base::fmtflags::none&&is_point()?"":" )");
      os << t;
#endif
      return os;
    }
    bool contains(const S&s)const{
      NOTRACE( std::cerr << __PRETTY_FUNCTION__ << "(" << *this << ", " << s << ")\n"; )
      //if( l()==u() ){ return l().value().empty(); }
      if constexpr ( std::is_same_v<S,lex::number> ){
	  if( l().value().D || u().value().D || s.D ){
	    bool lu=l()<u();
	    if( !lu && l().value()==u().value() ){ return true; }
	    basic_boundary<S> ll(l().value(),infi);
	    basic_boundary<S> uu(u().value(),!u().is_pole()?supre:infi);
           NOTRACE( {
	       NOTRACE( std::cerr << ll << "<" << s << "<" << uu << "\n"; )
	     auto ls=ll<s;
	     auto su=s<uu;
             std::cerr << lu << "^" << ls<< "^" << su << "\n";
	   } )
	    return (l()<u())^(ll<s)^(s<uu);
	  }
      NOTRACE( {
	  auto lu=l()<u();
	  auto ls=l()<s;
	  auto su=s<u();
          std::cerr << lu << "^" << ls<< "^" << su << "\n";
	} )

      }
      return (l()<u())^(l()<s)^(s<u());
    }
    friend std::ostream& operator<<(std::ostream& os, const basic_interval &s){
         return s.write(os);
    }
   basic_interval operator+(const basic_interval &b)const{
    NOTRACE( std::cout << __PRETTY_FUNCTION__ << "(" << *this << ", " << r << ")" << std::endl; )      
    if( !bipole() && !b.bipole() ){
	basic_interval ret={b_<supre>(l()+b.l()),
                          b_<infi> (u()+b.u())};
	if( ret.l().value()==ret.u().value() ){
	return basic_interval(ret.l().value(),infi,ret.u().value(),supre);
      }
      return ret;
    }else if( bipole() && b.bipole() ){
      return basic_interval();
    }
    auto ll=l()+b.l();
    auto uu=u()+b.u();
    if( uu<ll ){
     return {b_<supre>(ll), b_<infi>(uu)};
    }else{
      return basic_interval();
    }
   }
   basic_interval operator-(const basic_interval &r)const{
     NOTRACE(  std::cout << __PRETTY_FUNCTION__ << "(" << *this << ", " << r << ")" <<  std::endl; )
     NOTRACE(  std::cout << "+ " << -r <<   std::endl; )
     NOTRACE(  std::cout << "return " << (*this + -r) <<  std::endl; )
     return *this + -r;
   }
   basic_interval operator!()const{
     NOTRACE(  std::cout << __PRETTY_FUNCTION__ << "(" << *this << ", " << r << ")" <<  std::endl; )
     NOTRACE(  std::cout << "+ " << -r <<   std::endl; )
     NOTRACE(  std::cout << "return " << (*this + -r) <<  std::endl; )
     #if 0
     return (basic_interval)!(kleen)*this;
     #endif 
     return basic_interval( u(),l() );
   }
   basic_interval operator~()const{
     if( u() != l() ){
        return basic_interval(u(),l());
     }
     if( is_universal() ){
       return kleen::F;
     }else{
       return kleen::T;
     }
   }
  static basic_boundary<S> min(const basic_boundary<S> &A,const basic_boundary<S> &B){
    if( A.is_pole()/*value().empty()*/ ){ return B; }
    if( B.is_pole()/*value().empty()*/ ){ return A; }
    return std::min(A,B);
  }
  static basic_boundary<S> max(const basic_boundary<S> &A,const basic_boundary<S> &B){
    if( A.is_pole()/*.value().empty()*/ ){ return B; }
    if( B.is_pole()/*.value().empty()*/ ){ return A; }
    return std::max(A,B);
  }
  static basic_boundary<S> abs(const basic_boundary<S> &A){
    return A<0?-A:A;
  }
#define a (*this) 
#if 0
#define x(o,p)						    \
   basic_interval operator o(const basic_interval &B)const{ \
     p; \
     if( a.bipole() || b.bipole() ){			     \
       if( (sign)a.u() < (sign)a.l() && (sign)b.u() < (sign)b.l() ){ \
       NOTRACE( \
          auto ul = a.u() o b.l(); \
	  auto lu = a.l() o b.u(); \
          auto ll = a.l() o b.l(); \
          auto uu = a.u() o b.u(); \
	  std::cout << "ll=" << ll << " uu= " << uu << std::endl; \
	  std::cout << "ul=" << ul << " lu= " << lu << std::endl; \
	  std::cout << "min=" << std::min(a.u() o b.u(),a.l() o b.l()) << std::endl; \
	  std::cout << "max=" << std::max(a.l() o b.u(),a.u() o b.l()) << std::endl; \
       ) \
          return{ std::min(a.u() o b.u(),a.l() o b.l()),std::max(a.l() o b.u(),a.u() o b.l()) };  \
       }else{ \
          return basic_interval(); \
       } \
     } \
     int x=(sign)a.l()+(sign)a.u(); \
     int y=(sign)b.l()+(sign)b.u(); \
     if( y==0 && x==0 ){ \
       NOTRACE( \
          auto ul = a.u() o b.l(); \
	  auto lu = a.l() o b.u(); \
          auto ll = a.l() o b.l(); \
          auto uu = a.u() o b.u(); \
	  std::cout << "ll=" << ll << " uu= " << lu << std::endl; \
	  std::cout << "ul=" << ul << " lu= " << lu << std::endl; \
	  std::cout << "min=" << std::min(a.u() o b.l(),a.l() o b.u()) << std::endl; \
	  std::cout << "max=" << std::max(a.l() o b.l(),a.u() o b.u()) << std::endl; \
       ) \
          return{ std::min(a.u() o b.l(),a.l() o b.u()),std::max(a.l() o b.l(),a.u() o b.u()) }; \
     } \
     NOTRACE( \
      NOTRACE( std::cout << __PRETTY_FUNCTION__ << "(" << *this << ", " << b << ")" << std::endl; ) \
     auto la=(2 o y-x<0?a.u():a.l()); \
     auto lb=(2 o x-y>0?b.u():b.l()); \
     auto l=la o lb; \
     NOTRACE( std::cout << la << " o " << lb << "=" << l << std::endl; ) \
     auto ua=(2 o y+x>0?a.u():a.l()); \
     auto ub=(2 o x+y>0?b.u():b.l()); \
     auto u=ua o ub; \
     NOTRACE( std::cout << ua << " o " << ub << "=" << u << std::endl; ) \
     ) \
     return { \
       (2 o y-x<0?a.u():a.l()) o (2 o x-y<0?b.u():b.l()), \
       (2 o y+x>0?a.u():a.l()) o (2 o x+y>0?b.u():b.l()) \
     }; \
   } \
// end define x
#endif

bool contains0()const{
  return bipole()?( (sign)l()<=sign(0) || sign(0)<=(sign)u() )
                 :( (sign)l()<=sign(0) && sign(0)<=(sign)u() );
}
#define X(o,d0,d1)					    \
   basic_interval operator o(const basic_interval &B)const{ \
     NOTRACE( std::cerr << __PRETTY_FUNCTION__ << "(" << *this << ", " << B << ")" << '\n'; ) \
     d0; \
     if( a.bipole() || B.bipole() d1 ){				     \
       NOTRACE( std::cerr << ((sign)a.u()) << "<" << (sign)a.l() << " && " << (sign)b.u() << "<" <<  (sign)b.l() << '\n'; ) \
	 if( ((sign)a.u() < (sign)a.l()) \
          && ((sign)b.u() < (sign)b.l()) \
         ){ \
	 NOTRACE( std::cerr << (sign)a.u() << "<" <<  (sign)a.l() << "&&" <<  (sign)b.u() << "<" <<  (sign)b.l()<< "\n"; ) \
         NOTRACE( std::cerr << "min(" << (a.u() o b.u()) << "," << (a.l() o b.l()) << "), max(" << (a.l() o b.u()) << "," << (a.u() o b.l()) << ")\n";  ) \
         return{ min(a.u() o b.u(),a.l() o b.l()),max(a.l() o b.u(),a.u() o b.l()) };  \
	 }								\
       if( 1 || !a.contains0() && !b.contains0() ){			\
          auto ul = a.u() o b.l(); \
	  auto lu = a.l() o b.u(); \
          auto ll = a.l() o b.l(); \
          auto uu = a.u() o b.u(); \
	  NOTRACE( std::cerr << "ll=" << ll << " uu= " << lu << std::endl; ) \
	  NOTRACE( std::cerr << "ul=" << ul << " lu= " << lu << std::endl; ) \
	    if( uu <= ll && sign(0) < (sign)uu ){  return basic_interval(); } \
       }\
       if( a.bipole() && b.contains0() \
        || b.bipole() && a.contains0() \
       ){ \
  	  NOTRACE(  std::cerr << "return " << basic_interval() << "\n"; )\
          return basic_interval(); \
         } \
     }									\
     if( 0 o 1 == 0 && a.is_point(0) ){ return basic_interval(0); } \
     int x=(sign)a.l()+(sign)a.u(); \
     int y=(sign)b.l()+(sign)b.u(); \
     if( y==0 && x==0 ){ \
       NOTRACE( \
          auto ul = a.u() o b.l(); \
	  auto lu = a.l() o b.u(); \
          auto ll = a.l() o b.l(); \
          auto uu = a.u() o b.u(); \
	  std::cout << "ll=" << ll << " uu= " << lu << std::endl; \
	  std::cout << "ul=" << ul << " lu= " << lu << std::endl; \
	  std::cout << "min=" << std::min(a.u() o b.l(),a.l() o b.u()) << std::endl; \
	  std::cout << "max=" << std::max(a.l() o b.l(),a.u() o b.u()) << std::endl; \
       ) \
          return{ std::min(a.u() o b.l(),a.l() o b.u()),std::max(a.l() o b.l(),a.u() o b.u()) }; \
     } \
     NOTRACE( std::cout << __LINE__ << ": " << a << #o << b << std::endl; )	\
     NOTRACE( {std::cout << (2*y-x<0?a.u():a.l()) << #o << (2*x-y<0?b.u():b.l()) << std::endl; \
	   auto t=(2*y-x<0?a.u():a.l()) o (2*x-y<0?b.u():b.l()); \
	    std::cout << "b_<supre>(" << t << ")=" << b_<supre>(t) << std::endl; \
       })   \
     NOTRACE({ std::cout <<  (2*y+x>0?a.u():a.l()) << #o << (2*x+y>0?b.u():b.l())<< std::endl; \
	    auto t=(2*y+x>0?a.u():a.l()) o  (2*x+y>0?b.u():b.l()); \
	      std::cout << "b_<infi>("<<t<<")=" << b_<infi>(t)  << std::endl; \
     })	      \
     return { \
       b_<supre>((2*y-x<0?a.u():a.l()) o (2*x-y<0?b.u():b.l())),	\
        b_<infi>((2*y+x>0?a.u():a.l()) o (2*x+y>0?b.u():b.l()))		\
     }; \
   } \
// end define x

#define a (*this)
#define b B
   X(*,,)
#undef b

#if 1

  X(/,auto b=basic_interval(B.u().value(),-B.u().ma(),B.l().value(),-B.l().ma());/**/NOTRACE( std::cout << b << "=!" << B << '\n');,^((sign)b.u() < (sign)b.l()))
#endif
#undef a
#undef X

#if 0
   basic_interval operator/(const basic_interval &b)const{
    NOTRACE( std::cout << __PRETTY_FUNCTION__ << "(" << *this << ", " << b << ")" << std::endl; )
    NOTRACE( std::cout << "*{" << basic_interval(1/b.u(),1/b.l()) << "}" << std::endl; )
      return *this*basic_interval(1/b.u(),1/b.l());
   }
#endif
  basic_interval operator^(const basic_interval &b)const{
    if( is_point() && b.is_point() ){
      return (lex::number)std::pow((lex::number)this->l().value(),(lex::number)b.l().value());
    }
    std::cerr << __PRETTY_FUNCTION__ << " not implemented\n";
    return *this;
  }

  basic_interval operator&&(const basic_interval &b)const{  
    static auto cp=[](const  basic_interval &a, const  basic_interval &b)->basic_interval{ 
      assert( a.is_connected() && !b.is_connected() );
	if( a.u() <= b.u() || b.l() <= a.l() || ( a.l() < b.u() && b.l() < a.u() ) ){
           NOTRACE( std::cerr << __LINE__ << '\n'; )
	  return a;
        }
        if( a.l() < b.u() ){
           NOTRACE( std::cerr << __LINE__ << '\n'; )
	  return {a.l(),b.u()};
        }
        if( b.l() < a.u() ){
         NOTRACE( std::cerr << __LINE__ << '\n'; )
	  return {b.l(),a.u()};
        }
      //	return  {{0,infi},{0,infi}};
      return  {b_t((S)0,infi),b_t((S)0,infi)};
    };

    NOTRACE( std::cerr << __PRETTY_FUNCTION__ << '\n'; )
    if( is_connected() ){ // 
      if( b.is_connected() ){
        NOTRACE( std::cerr << __LINE__ << '\n'; )
        auto lb=max(l(),b.l());
        auto ub=min(u(),b.u());
        NOTRACE( std::cerr << lb << "=max(" << l()<< "," << b.l() << ")\n"; );
        NOTRACE( std::cerr << ub << "=min(" << u()<< "," << b.u() << ")\n"; );
        if( lb<ub ){ 
          NOTRACE( std::cerr << __LINE__ << '\n'; )
          return {lb,ub};
        }else{
	  NOTRACE( std::cerr << __LINE__ << '\n'; )
	    return {b_t(0,infi),b_t(0,infi)}; 
        }
      }else{ 
	return cp(*this,b);
      }
    }else{
      if( b.is_connected() ){
	return cp(b,*this);
      }else{
	if( l() <= b.u() || b.l() <= u() ){
          NOTRACE( std::cerr << __LINE__ << '\n'; )
	  return basic_interval();
        }
        auto lb=max(l(),b.l());
        auto ub=min(u(),b.u());
        return {lb,ub};
      }
    }
  }

  basic_interval operator||(const basic_interval &b)const{
    static auto cp=[](const  basic_interval &a, const  basic_interval &b)->basic_interval{ 
      NOTRACE( std::cerr << __LINE__ << '\n'; )
      assert( a.is_connected() && !b.is_connected() );
        NOTRACE( std::cerr << __LINE__ << '\n' );
	if( a.l() < b.u() && a.u() < b.l() ){
	  return {b.l(),std::max(a.u(),b.u())};
        }
	if( b.l() < a.u() && b.u() < a.l() ){
	    return {std::min(a.l(),b.l()),b.u()};
        }
	return basic_interval();
    };
    NOTRACE( std::cerr << __PRETTY_FUNCTION__ << '\n'; )
    if( is_connected() ){ // 
      if( b.is_connected() ){
	static auto hemi=[](const  basic_interval &a, const  basic_interval &b)->basic_interval{
	  NOTRACE( std::cerr << __LINE__ << '\n' );
	  assert( a.l().is_inf() && b.u().is_inf() );
	  if( b.l() <= a.u() ){
	    NOTRACE( std::cerr << __LINE__ << '\n' );
	    return basic_interval();
          }else{
	    return {b.l(),a.u()};
          }
        };
	if( l().is_inf() && b.u().is_inf() ){
	  return hemi(*this,b);
        }
	if( u().is_inf() && b.l().is_inf() ){
	  return hemi(b,*this);
        }
        auto lb=min(l(),b.l());
        auto ub=max(u(),b.u());
        return {lb,ub};
      }else{ 
        NOTRACE( std::cerr << __LINE__ << '\n'; )
	return cp(*this,b);
      }
    }else{
      if( b.is_connected() ){
        NOTRACE( std::cerr << __LINE__ << '\n'; )
        return cp(b,*this);
      }else{
        auto lb=min(l(),b.l());
        auto ub=max(u(),b.u());
	if( ub < lb ){
           return {lb,ub};
        }
        NOTRACE( std::cerr << __LINE__ << '\n' );
	return basic_interval();
      }
    }
  }
   S rand()const{
     static std::default_random_engine gen;
     static bool init;
     if( !init ){
        std::random_device rd;
        gen = std::default_random_engine(rd());
	init=true;
     }
     return rand(gen);
    }
     S rand(std::default_random_engine &gen)const{     
       NOTRACE( std::cout << __PRETTY_FUNCTION__ << '\n'; )
       NOTRACE( std::cout << *this << '\n'; )
       if( is_point() ){
	 return l().value();
       }
     auto rs = [&gen](const lex::number &l,const lex::number &u){
        if( !l.empty() && !u.empty() && l._this._this[0]<'\x80' && u._this._this[0]>'\x80' ){
          if( std::uniform_int_distribution<>(0,3)(gen)==0 ){
   	    return lex::number(0.0);
          }
        }
        std::string s;
	while( 1 ){
	s.clear();
  	auto il=l._this.begin();
	auto iu=u._this.begin();

#if 0
	for( ;il!=l._this.end() && iu!=u._this.end() && *il==*iu;++il,++iu ){
	      s += *il;
       }
#endif
       for( ;il!=l._this.end() && iu!=u._this.end() ;++il,++iu ){
         char c=std::uniform_int_distribution<>(*il,*iu)(gen);
	 if( (*il&&*iu&1) ){ c |= 1; }
	 s += c;
	 if(  c!=*il || c!=*iu ){ break; }
       }
       if( il!=l._this.end() && iu!=u._this.end() ){
          break;
       }
       if( l._this._this == u._this._this ){
           break;
       }
       NOTRACE( std::cout << __PRETTY_FUNCTION__ << "(" << l << ", " << u << ")" << __LINE__ << std::endl; )
       }
       return lex::number(lex::string(s));
   };
     auto r = [](const b_t &l,const b_t &r){
        return 0.0;
     };
     auto d0 = [&gen](double l,double u){
       NOTRACE( std::cerr << __PRETTY_FUNCTION__ << " d0(" << l << ", " << u << ")\n"; )
        double x=std::uniform_real_distribution<>(0,1)(gen);
	x*=x*(3-2*x);
        NOTRACE( std::cerr << "x=" << x << "\n"; )
	return l*x+u*(1-x);
     };
     auto d1 = [d0,&gen](double l,double u){
       NOTRACE( std::cerr << __PRETTY_FUNCTION__ << " d1(" << l << ", " << u << ")\n"; )
        if( 0<=l || u<=0 ){
           return d0(l,u);
        }else if( std::uniform_int_distribution<>(0,3)(gen) == 0 ){
          return 0.0;
        }else if( std::uniform_real_distribution<>(l-(u-l)/4,u+(u-l)/4)(gen) < 0 ){
	   return d0(l,0);
	}else{
	   return d0(0,u);
        }
     };
     std::function<double(double,double)> d2 = [d0,d1,&d2,&gen](double l,double u){
       NOTRACE( std::cerr << __PRETTY_FUNCTION__ << " d2(" << l << ", " << u << ")\n"; )
       if( l<u ){
         NOTRACE( std::cerr << "l<u\n"; )
	 return d1(l,u); // l < x::x < u
       }else if( u<0 && 0<l ){  // x::x < u < 0 < l < x::x 
         NOTRACE( std::cerr << "u<0 && 0<l\n" );
          return 1.0/d0(1.0/u,1.0/l); 
       }else if( 0 <= u ){  // x::x <  0 <=  x::x < u <= l < x::x 
	 NOTRACE( std::cerr << "u>=0\n"; )
         switch(  std::uniform_int_distribution<>(0,u==0?1:2)(gen) ){
	 case 0:{ NOTRACE( std::cerr << " x::x < 0\n"; ) return -exp(std::uniform_real_distribution<>(-1,1)(gen)/sqrt(std::uniform_real_distribution<>(0,1)(gen)));  }; break; //   x::x < 0
	 case 1:{ NOTRACE( std::cerr << "l < x::x\n"; )
                 if(l==0){
		   return exp(std::uniform_real_distribution<>(-1,1)(gen)/sqrt(std::uniform_real_distribution<>(0,1)(gen)));
		 }else{
                    return 1/d0(0,1/l);  
		 }       
         }; break; // l < x::x 
	 case 2:{ NOTRACE( std::cerr << "0 <= x::x < u\n"; ) return d0(0,u);  }; break; //  0 <=  x::x < u  
         default:{ assert(!__LINE__); };break;
         }
       }else if( l<=0 ){ // x::x < u < l < x::x <= 0 < x::x
          return -d2(-u,-l);
       }else{  
	 NOTRACE( std::cerr << __PRETTY_FUNCTION__ << "(" << l << ", " << u << ")\n"; )
	 assert(!__LINE__);
       }
     };
     int i=std::uniform_int_distribution<>(0,5)(gen);
    switch( i ){
    case 0:{ if( l().ma()<(sign)0 ){ NOTRACE( std::cerr << "return l()=" << l().value() << '\n'; ) return l().value(); } };break;
    case 5:{ if( (sign)0<u().ma() ){ NOTRACE( std::cerr << "return u()=" << u().value() << '\n'; ) return u().value(); } };break;
     };
    return (S)(lex::number)d2((S)l().value(),(S)u().value());
   }
  
    static basic_interval abs(const basic_interval &a){
      if( a.l() < (S)0 && (S)0 < a.u() ){
	//        return {{0,infi},{max(-a.l(),a.u())}};
	return {b_t((S)0,infi),b_t(max(-a.l(),a.u()))};
      }
      if( a.u()<(S)0 && (S)0 < a.l() ){
	NOTRACE( std::cerr << min(-a.u(),a.l()) << '\n'; )
	NOTRACE( std::cerr << basic_interval({min(-a.u(),a.l())},{lex::string,infi}) << '\n'; )
	//	return {{min(-a.u(),a.l())},{}};
	return {b_t(min(-a.u(),a.l())),b_t()};
      }
      if( (S)0 <= a.l() && (S)0 <= a.u() ){
	return a;
      }
      /*if( a.l()<=0  &&  a.u()<=0 )*/{
	return -a;
      }
      assert( false );
    }
    static basic_interval min(const basic_interval &a){
      return {a.l(),a.l()};
    }
    static basic_interval max(const basic_interval &a){
      return {a.r(),a.r()};
    }
  //  using lex_t=lex::vector<typename boundary<S>::lex_t,-1>;
using lex_t=lex::interval<lex::string>;
//using lex_t=lex::vector<boundary<lex::string>::lex_t>;
//using lex_t=lex::interval<S>
//using lex_t=lex::interval;

#if 1
  basic_interval(const lex_t& l){
    NOTRACE( std::cerr << __PRETTY_FUNCTION__ << "(" << l << ")" << '\n'; )
    std::vector<typename boundary<lex::string>::lex_t> sl=l;
    assert( sl.size()==2 );
    auto b0=(boundary<S>)sl[0];
    auto b1=(boundary<S>)sl[1];
    *this={b0,b1};
    NOTRACE( std::cerr << __FUNCTION__ << "=" << *this << '\n'; )
  }
#endif
#if 0
  operator lex_t()const{
    auto L=static_cast<typename boundary<S>::lex_t>(l());
    auto U=static_cast<typename boundary<S>::lex_t>(u());
    lex_t ret{L,U};
    return ret;
  }
#endif
 }; // end class basic_interval

template<typename S>
#define BASE_T  std::pair<basic_interval<S>,S>
  class intfix:public BASE_T{
    using base_t=BASE_T;       
    #undef BASE_T
    using base_t::base_t;
    public:
    kleen operator==(const S&s){
      if( std::get<0>(*this).is_point() ){ return std::get<0>(*this)==s; }
      return std::get<1>(*this)==s?kleen::T:kleen::F; 
    }
  }; // end class intfix;

template<typename T=lex::scalar>
  class intvec{ // acts like a const vector of intervals 
  using iT=basic_interval<T>;
    //    std::optional<const std::vector<interval>&> I;
  const std::vector<basic_interval<T>>* I;
    //    std::optional<const std::vector<T>&> S;
  const std::vector<T>* S;
  std::function<size_t()> size_;
  std::function<iT(int i)> at_;
  std::function<std::ostream&(std::ostream& o)> put_;
  size_t size_I(){ return I->size(); }
  size_t size_S(){ return S->size(); }
  iT at_I(int x){ return (*I)[x]; }
  iT at_S(int x){ return (*S)[x]; }
  std::ostream& put_I(std::ostream& s){ s<<(*I); return s; }
  std::ostream& put_S(std::ostream& s){ s<<(*S); return s; }
  public:
    size_t size()const{ return size_(); }
    iT operator[](int i)const{ return at_(i); }
  intvec():size_( [](){ return 0; } ){}
 intvec(const std::vector<iT>&i):I(&i),
  size_( [this](){return I->size();} ),
  at_( [this](int x){return (*I)[x];} ),
  put_([this](std::ostream& s)->std::ostream&{  s<<(*I); return s;} )
  {
            NOTRACE( qtl::cerr << __PRETTY_FUNCTION__ << '(' << i << ')' << '\n'; )
  }
 intvec(const std::vector< T>&s):S(&s),
 size_( [this](){return S->size();} ),
  at_( [this](int x){return (*S)[x];} ),
  put_( [this]( std::ostream& s)->std::ostream&{ s<<(*S); return s;} )
 {}
  intvec(const std::vector<iT>&i,const std::vector<T>&s):I(&i),S(&s),
      size_( [this](){return std::max(I->size(),S->size());} ),
      at_([this](int x){ 
        NOTRACE( std::cerr << __PRETTY_FUNCTION__ << '(' << x << ')' << '\n'; )
        if( x<S->size() ){
	  if( x<I->size() && (*I)[x].is_point() ){
	    NOTRACE( std::cerr << __LINE__ << "(*I)[x]=" << (*I)[x] << '\n'; )
	    return (*I)[x];
	  }
	  NOTRACE( std::cerr << __LINE__ << "(*S)[x]=" << (*S)[x] << '\n'; )
          return iT((*S)[x]);
	  //	}else if( x<I->size() ){
	  //  return (*I)[x];
	}else{
	  NOTRACE( std::cerr << __LINE__ << "iT()=" << iT() << '\n'; )
	 return iT();
        }
      }),
  put_([this](std::ostream &o)->std::ostream&{ 
	  size_t s=size();
	  for( size_t x=0;x<s; ++x ){
	    if( x > 0 ){ o << ", "; }
            o << "{"; 
	    if( x<I->size() ){ o<< (*I)[x]; }
	    o << ", ";
	    if( x<S->size() ){ o<< (*S)[x]; }
	    o << "}";
        }
        return o;
    })
    {}
    friend std::ostream& operator<<(std::ostream& os, const intvec &s){
      return s.put_(os);
    }
    class iterator{
    public:
      const intvec &v;
      int i;
      iterator operator++(){ ++i; return *this; }
      iT operator*()const{ return v[i]; }
      bool operator!=(const iterator &r)const{ return i!=r.i; }
      iterator(const intvec &v,int i=0):v(v),i(i<0?v.size()+i:i){}
      iterator(const intvec &v,nullptr_t):v(v),i(v.size()){}
    }; // end class intvec::iterator  
    iterator begin()const{ return iterator(*this); }
    iterator end()const{ return iterator(*this,nullptr); }
}; // end class intvec

}; // end namespace qtl
namespace lex{
}; // end namespace lex
namespace qtl{
#if 1
  template<typename T,typename = std::enable_if_t<std::is_scalar<T>::value>>
  T operator-(T r){
      return static_cast<T>(-static_cast<int>(r));
  }
#endif
    //  template<typename S=lex::number>
    //  class interval:public basic_interval<S,infi,infi>{
#if 0
  class interval:public basic_interval<lex::number>{
  public:
    typedef basic_interval<lex::number> base_t;
    using base_t::base_t;
    template<typename T>
    interval(const T& l,const T&u):base_t((lex::number)l,infi,(lex::number)u,infi){}
  };
#else
  using interval=basic_interval<lex::scalar>;
#endif
#if 0
  template<typename S=lex::number>
  using closed_interval=basic_interval<S,infi,supre>;
  template<typename S=lex::number>
  using open_interval=basic_interval<S,supre,infi>;
#endif
  template<typename S=lex::number>
  class closed_interval:public basic_interval<S>{
  public:
    typedef basic_interval<S> base_t;
    using base_t::base_t;
    template<typename T>
    closed_interval(const T& l,const T&u):base_t((S)l,infi,(S)u,supre){}
  };
  template<typename S=lex::number>
    class open_interval:public basic_interval<S>{
  public:
  typedef basic_interval<S> base_t;
    using base_t::base_t;
    template<typename T>
    open_interval(const T& l,const T&u):base_t((S)l,supre,(S)u,infi){}
  };

#if 0
  template<typename S=lex::number>
  interval<S> operator+(const interval<S> &l,const interval<S> &r){
    return {b_<S,supre>(l.l())+b_<S,supre>(r.l()), b_<S,infi>(l.u())+b_<S,infi>(r.u())};
  }
  template<typename S>
  interval<S> operator-(const interval<S> &l,const interval<S> &r){
    return l + -r;
  }
#endif
}
#if __INCLUDE_LEVEL__ + !defined __INCLUDE_LEVEL__ < 1+defined TEST_H
#include "bounds.h"
//#pragma message XSTR(__INCLUDE_LEVEL__)
//#pragma message XSTR(__FILE__)
//#pragma message XSTR(__BASE_FILE__)
unsigned long seed=1;
qtl::interval rnd(){
     static std::default_random_engine gen;
     static bool init;
     if( !init ){
#if 1
#ifdef seed
        std::random_device rd;
        gen = std::default_random_engine(rd());
#else
        gen = std::default_random_engine(seed);
#endif
#else
        gen = std::default_random_engine(1);
#endif
	init=true;
     }
    auto v=[](){
      return (lex::number)(std::uniform_real_distribution<>(-1,1)(gen)/std::uniform_real_distribution<>(-1,1)(gen));
    };
    auto m=[](){
       return (qtl::lim)std::uniform_int_distribution<>(0,1)(gen);
    };
   return qtl::interval(v(),m(),v(),m());
}
#define NUM_RE \
          "0(?:" \
                "[xX](?:" \
                         "(?:"\
                             "(?:" \
                                            "\\.(?:[[:xdigit:]](?:\'[[:xdigit:]])*)+" \
                             "|" \
                                              "(?:[[:xdigit:]](?:\'[[:xdigit:]])*)+(?:\\.(?:[[:xdigit:]](?:\'[[:xdigit:]])*)*)?" \
	                     ")" \
                             "([pP])[-+]?(?:\\d(?:\'\\d)*)*" \
			 ")" \
                    "|" \
                         "(?:[[:xdigit:]](?:\'[[:xdigit:]])*)+" \
                    ")" \
           "|" \
                  "(?:[0-7](?:\'[0-7])*)+" \
           "|" \
                  "[bB](?:[01](?:\'[01])*)+" \
           ")" \
     "|" \
           "(?:" \
	        "("   /* decimal float literal */ \
                  "(?:\\d(?:\'\\d)*)+(?:\\.(?:\\d(?:\'\\d)*)*)?" \
	       "|" \
                  "\\.(?:\\d(?:\'\\d)*)+" \
               ")" \
               "(?:[eE][-+]?(?:\\d(?:\'\\d)*)+)?" \
           ")" \
// end NUM_RE
static const std::regex num_re(
    "(?:([\\[\\]\\(\\)]|--(?:\\s*(?:x::x\\s*)\\[)?)\\s*)?([-+]?(?:" NUM_RE "))"
     "(?:(?:_s|[fFlL]|[uU]?[lL]{0,2}|[lL]{1,2}[uU])?\\s*((?:\\]\\s*)?\\+\\+|[\\[\\]\\(\\)]))?"  ,std::regex::optimize);
std::default_random_engine gen(1);
int main(int argc, char *argv[]){
if( argc==1 || argv [1][0] == '<' ){
  std::ifstream in;
  std::streambuf *cinbuf;
  if( argc>1 && argv[1][0] == '<' ){
    in = std::ifstream(argv[1][1]?argv[1]+1:argv[2]);
    cinbuf = std::cin.rdbuf(); 
    std::cin.rdbuf(in.rdbuf()); //redirect std::cin
  }
  int tellg=0;
  #if 0
  {
  std::string i;
  std::getline(std::cin, i);
  try{
   seed=std::stoull(i,NULL,10);
  }catch( const std::exception& ex ){
    TRACE( std::cerr <<  "caught " << ex.what() << ": " << i << "\n"; )
  }
  }
  #endif
  std::deque<qtl::boundary<lex::number>> q={--x::x[0],x::x[0]++,--x::x[1],x::x[1]++};
    std::cout <<  qtl::setverbose(qtl::ios_base::fmtflags::none) << std::setprecision(std::numeric_limits<double>::digits10);
  while( std::cin.good() && tellg<1000 ){
    int n=0;
    std::string l;
    std::getline(std::cin, l);
    tellg += l.size()+1;
    /**/NOTRACE( std::cerr << "rdstate=" << std::cin.rdstate() << " good=" << std::cin.good() << " eof=" << std::cin.eof() << " bad=" <<  std::cin.bad() << " fail=" << std::cin.fail() <<  "  tellg()=" <<  std::cin.tellg() << " gcount=" <<  std::cin.gcount() << " size=" << l.size() << '\n'; )
    if( !std::cin.good() || tellg>2000 ){ break; }
    std::smatch m;
    while( std::regex_search(l,m,num_re) ){
      NOTRACE( std::cerr << '\n' << m.prefix() << ": " <<  m.str() << '\n'; )
	NOTRACE( 
	for( auto i=0;  i < m.size(); ++i ){
	  NOTRACE( std::cerr << m[i].matched << ": " << m[i] << '\n'; )
	}
	)
      qtl::sign ma;
      if(  m[1].matched &&  m[5].matched && !((m[1].str()[0]!='-') ^ (m[5].str().back()!='+')) && q.size()<=6 ){
	NOTRACE( std::cout << m[1] << " ...  "  << m[5] << "\n"; )
	  q.push_back(qtl::basic_boundary<lex::number>(lex::number((double)lex::number::sem(m[2].str())),-1));
	ma=1;
      }else if( m[1].matched ){
	switch( m[1].str()[0] ){ 
	case '-':case '[': case ')':{ ma=-1; };break; 
	case '(':case ']':{ ma=1; };break;
        default:{ NOTRACE( std::cerr << m[1].str() << '\n' ) }
	}
      }else if( m[5].matched ){
	    switch( m[5].str()[0] ){
	    case '+':case ']': case '(':{ ma=1; };break;
	    case '[': case ')':{ ma=-1; };break;
	    default:{ NOTRACE( std::cerr << m[5].str() << '\n'; ) }
	    }
      }else{
        ma=(m.prefix().str().size()&1)?1:-1;
      }
      NOTRACE( std::cerr << q.size() << ": " << m[2] << '\n' );
      if( q.size()<8 ){
	q.push_back(qtl::basic_boundary<lex::number>((double)lex::number::sem(m[2].str()),ma));
      }else{
	++n;
	++seed;
      }
      l = m.suffix().str();
    }
      NOTRACE( std::cerr << q << '\n'; )
      if( q.size() > 4 ){ q.pop_front(); }
      if( q.size() > 4 ){ q.pop_front(); }
      if( q.size() > 4 ){ q.pop_front(); }
      if( q.size() > 4 ){ q.pop_front(); }
      auto a=(q[0],q[1]);
      auto b=(q[2],q[3]);
      if( std::isinf((double)a.l().value()) || std::isinf((double)a.u().value()) || std::isinf((double)b.l().value()) || std::isinf((double)b.u().value()) ){
	std::cerr << "overflow\n";
        continue;
      }
      gen=std::default_random_engine(seed);
#define x(o) {	 \
   decltype(a) c=a o b;					\
   std::cout << std::defaultfloat << a << " " << #o << " " << b << " = " << c << '\n';  \
   for( auto i=0; i<((a.is_point()?2:5)*(b.is_point()?2:5))/4 || i<n; ++i ){ \
     auto bb=b.rand(gen);				\
     if( 2 o 2 == 1 && bb==0 ){ \
      if( b.is_point() ){ \
	/*assert( c.is_inf() );	*/		\
	break; \
      }\
      continue; \
     } \
     auto aa=a.rand(gen);					\
     auto cc=aa o bb; \
     std::cout << (double)aa << " " << #o << " " << (double)bb << " = " << (double)cc << "   " ; \
     /* std::cout << std::flush; */					\
     if( #o [0] != '&' ){ \
       assert( c.contains((lex::number)cc) );	\
     } \
     std::cout << "\n"; \
   } \
   std::cout << "\n"; \
}// end define x
x(+)
x(-)
x(*)
x(/)
  //x(<)
  //x(>)
  //x(==)
  //x(<=)
  //x(>=)
  //x(!=)
  //x(&&)
  //x(||)
#undef x
  }
  std::cout << "test passed\n";
  exit(0);
 }
 if( std::strcmp(argv[1],"dict")==0 ){
   std::cout << "\"++\"\n";
   std::cout << "\"--\"\n";
   #ifdef FUZZING
   exit(0);
   #endif
   std::cout <<  qtl::setverbose(qtl::ios_base::fmtflags::none|qtl::ios_base::fmtflags::setbuilder) << std::setprecision(std::numeric_limits<double>::digits10);
   std::cout << (--x::x[-1],x::x[2]++) << "\n";
   std::cout << (0 < x::x < 1) << "\n";
   std::cout << x::x[1] << "\n";
   std::cout <<  qtl::setverbose(qtl::ios_base::fmtflags::none|qtl::ios_base::fmtflags::boundary);
   std::cout << (--x::x[-1],x::x[2]++) << "\n";
   std::cout << (0 < x::x < 1) << "\n";
   std::cout << x::x[1] << "\n";
  }
#if 0
 std::cout << "(--x::x[-1],x::x[2]++) (x::x[-1]++,--x::x[2])\n";
 std::cout << "(--x::x[3],--x::x[-4]) (x::x[5]++,x::x[-6]++)\n";
 std::cout << "(x::x[7]++,--x::x[8]) (x::x[9]++,--x::x[0.1])\n";
 std::cout << "(--x::x[-7],x::x[-8]++) (--x::x[-9],x::x[-0.1]++)\n";
 std::cout << "[0] [99]\n";
#endif
 using  namespace lex::literals;
std::cout <<  qtl::setverbose(qtl::ios_base::fmtflags::none|qtl::ios_base::fmtflags::boundary);
 std::cout << (--x::x[-1_s],x::x[2_s]++) << " + " <<  (x::x[-1_s]++,--x::x[2_s]) << "\n";
#if 1
 std::cout << (--x::x[3_s],--x::x[-4_s]) << " - " << (x::x[5_s]++,x::x[-6_s]++) << "\n";
 std::cout << (x::x[7_s]++,--x::x[8_s]) << " * " << (x::x[9_s]++,--x::x[0.5_s]) << "\n";
 std::cout << (--x::x[-7_s],x::x[-8_s]++) << "/" <<  (--x::x[-9_s],x::x[-0.5_s]++) << "\n";
 std::cout << x::x[0_s] << " " << x::x[99_s] << "\n";
#endif
 exit(0);
#if 1
  //  std::cout << std::dec << std::plus<qtl::interval>()(qtl::interval(1,2),qtl::interval(3,5)) << std::endl;
  std::cout << qtl::setverbose(qtl::ios_base::fmtflags::none);
  std::cout << std::dec << qtl::interval(1_s,2_s) + qtl::interval(3_s,5_s) << '\n';
  std::cout << std::dec << (1 <= x::x < 2) + (3 <= x::x < 5) << '\b';
#endif
#if 0
  std::cout << std::endl;
/*
./qtl/container.h:430:2: error: multiple overloads of 'tuple' instantiate to the same signature 'void ()'
 tuple(const T&... t):base_t( (... + string(t,eof(T::depth)) ) ),cache({t...}){
 ^
./qtl/interval.h:1385:16: note: in instantiation of template class 'lex::tuple<>' requested here
  std::cout << std::endl;
               ^
./qtl/container.h:410:2: note: previous declaration is here
 tuple():base_t(){} //
 ^
1 error generated.
*/
#else
  std::cout << '\n';
#endif
  //  std::cout << qtl::setverbose(qtl::ios_base::fmtflags::generic);
  for( int n=0;n<100;++n ){
   auto a=rnd();
   auto b=rnd();
#define x(o) {	 \
   auto c=a o b; \
   std::cout << std::defaultfloat << a << #o "\n" << b << " =\n" << c << '\n';  \
   for( int m=0;m<5;++m ){ \
     auto aa=a.rand(); \
     auto bb=b.rand(); \
     while( 2 o 2 == 1 && bb==0 ){ bb=b.rand(); } \
     auto cc=aa o bb; \
     std::cout << (double)aa << #o << (double)bb << "=" << (double)cc << "  "; \
     /* std::cout << std::flush; */					\
     assert( c.contains((lex::number)cc) );				\
   } \
   std::cout << '\n';				\
}// end define x
   //x(+)
   //x(-)
x(*)
x(/)
#undef x
  std::cout << '\n';
  }
}
#endif
