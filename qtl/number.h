#pragma once
#define __STDC_WANT_DEC_FP__
#include <stdlib.h>
#include <string>
#include <assert.h> 
#include <boost/core/demangle.hpp>
#include <iostream>
#include <utility>
#ifndef __APPLE__ // my include paths have gotten messed up. Hopefully the Docker installation will fix this 
#include <charconv>
#endif
#include "string.h"
#include "out.h"
#ifndef __clang__
//#include <decimal/decimal>
#include <optional>
#else
//#include <experimental/optional>
#include <optional>
#endif
#include <boost/multiprecision/cpp_dec_float.hpp>
#include <algorithm>
#include <cstring>
#include <cmath>
#include <array>
#include <iomanip>
#define TABLE_CMP \
  X(<,less) \
  X(<=,less_equal) \
  X(==,equal_to) \
  X(>=,greater_equal) \
  X(>,greater) \
  X(!=,not_equal_to) \
// end define TABLE_CMP

#if 1
template <typename S>
/*friend*/ std::ostream& operator<<(std::ostream& os, const
    //		typename std::enable_if_t<
    //		                   std::is_member_function_pointer<decltype(&S::write)>::value
    //				  ,S>::type 
  //    		typename std::enable_if_t<
  //				    !std::is_convertible<S,char *>::value
  //    				  ,S>::type 
				  //  std::enable_if_t<std::is_class<S>::value, S>
				        std::enable_if_t<!std::is_pointer<S>::value, S>
				  //S
  & s
  ){
    return s.write(os);
  }
#endif
namespace lex{
class number;
};
namespace std{
   template<> class numeric_limits<lex::number> {
  public:
    static constexpr int min_exponent10 = -6176;
    static constexpr int max_exponent10 = 6144;
  }; // end class std::numeric_limits<lex::number>
 }; // end namespace std;

namespace lex{
#if 0
template<typename T>
T operator +(const T& l,const T& r){
  return (T) ((double)l + (double)r);
}
#endif
#if 0
#define x(o) \
  template<typename T> \
  double operator o(const T& l,double r){ \
    return ((double)l o r); \
  } \
  template<typename T> \
  double operator o(double l,const T& r){ \
    return (l o (double)r); \
  } \
// end define
x(+)
#if !(defined __clang__ && defined __apple_build_version__ && __apple_build_version__ <= 9000038)
x(-)
#else
#pragma message "error: use of overloaded operator '-' is ambiguous (with operand types 'std::__1::__wrap_iter<const lex::number::p *>' and 'std::__1::__wrap_iter<const lex::number::p *>')"
#endif
x(*)
x(/)
#undef x
#endif

//class sign;

#define N(s) (s.data()?qtl::visible(s,std::ios_base::hex):"NULL")

class number{
public:
  //std::ostream& write(std::ostream&);
  class sem{
  public:
    std::ostream& write( std::ostream &os )const{
    qtl::ios_base::fmtflags f;
    os >> f;
    std::string t=qtl::type_name<decltype(*this)>(f.verbosity);
    if( !t.empty() ){
        os << t << "{ s=" << s << ", e=" << e << ", m=" <<  m <<" }";
    }else{
	    os << (s<0?"-":os.flags()&std::ios_base::showpos?"+":"");
	    if( m.empty() ){ 
              os << "0";
            }else if( !(os.flags()&std::ios_base::showpoint) && e < std::numeric_limits<int>::digits10 && m.size()-1 <= e && e <= m.size()+2 ){
		os << m;
		os << &"000"[m.size()+2-e];
	    }else if( -3 <= e && e <= (signed)m.size()-1 && e < std::numeric_limits<float>::digits10 ){
	      if( e<0 ){ os << "0." << &"00"[3+e]; }
	      int d=e+1;
	      for( auto c:m ){
	        os << c;
		if( --d == 0 ){ os << "."; }
              }
            }else{
	      int d=-2;
	      for( auto c:m ){
		if( ++d == 0 ){ os << "."; }
		os << c;
              }
	      if( e ){ os << 'e' << e; }
            }
      }
      return os;
    }
    friend std::ostream& operator<<(std::ostream& os, const sem &s){
        return s.write(os);
    }
    int s=1; // sign
    int e=0; // exponent
    std::string m; // mantissa
    sem normalize(){
      #if 0
      //      std::cout << __PRETTY_FUNCTION__ << "(" << *this << ")" << std::endl;
      #else
      NOTRACE( std::cerr << __PRETTY_FUNCTION__ << "(s=" << s << ", e=" << e << ", m=" << m << ".size()=" << m.size() << ")" << std::endl; )
     #endif
      auto i=m.data();
      auto j=i+m.length();
      NOTRACE( std::cerr << "i=" << (void*)i << " j=" << (void*)j << std::endl; )
      for( ; i!=j && *i=='0'; ++i,--e ){ }
      if( i==j ){ e=0; s=0; m.clear(); return *this; }
      for( ; j!=i && *--j=='0'; ){ /* *j='\0'; */ }
      NOTRACE(  std::cout << "i=" << (void*)i << " j=" << (void*)j << std::endl; )
      std::memmove(reinterpret_cast<void*>(const_cast<char*>(m.c_str())),reinterpret_cast<void*>(const_cast<char*>(i)),++j-i);
      NOTRACE( std::cerr << __PRETTY_FUNCTION__ << "s=" << s << ", e=" << e << ", m=" << m << ".size()=" << m.size() << std::endl; )
      NOTRACE( std::cerr << "i=" << (void*)i << " j=" << (void*)j << std::endl; )
      m.resize(j-i);
      NOTRACE( std::cerr << __PRETTY_FUNCTION__ << "s=" << s << ", e=" << e << ", m=" << m << ".size()=" << m.size() << std::endl; )
	if( e>std::numeric_limits<number>::max_exponent10 ){ return std::numeric_limits<number>::max_exponent10+1; }
      return *this;
    }
    sem(int sign,int exp,const std::string &mantisa):s(sign),e(exp),m(mantisa){ 
      normalize();
      //      std::cout << __PRETTY_FUNCTION__ << "= s=" << s << ", e=" << e << ", m=\"" << m <<"\""<< std::endl;
    }
#define NUM_0 \
          "0(?:" \
                "[xX](?:" \
                         "(?:"\
                             "(" \
                                            "\\.(?:[[:xdigit:]](?:\'[[:xdigit:]])*)+" \
                             "|" \
                                              "(?:[[:xdigit:]](?:\'[[:xdigit:]])*)+(?:\\.(?:[[:xdigit:]](?:\'[[:xdigit:]])*)*)?" \
	                     ")" \
                             "[pP][-+]?\\d" \
			 ")" \
                    "|" \
                         "[[:xdigit:]]" \
                    ")" \
           "|" \
                  "[oO][0-7]" \
           "|" \
                  "[bB][01]" \
           ")" \
// end define NUM_0
  sem( const std::string d="" ){
    NOTRACE( std::cerr << __PRETTY_FUNCTION__ << "(" << d << ")" << std::endl; )
    static const std::regex num_0("^[-+]?" NUM_0,std::regex::optimize);
    std::smatch sm;
    if( std::regex_search(d,sm,num_0) ){
      try{
        if( sm[1].length() ){
	  NOTRACE(  std::cerr << "sem(std::stod(" << d << "))\n"; )
	  NOTRACE(  std::cerr << "sem(" << std::stod(d) << ")\n"; )
	  *this=sem(std::stod(d));
        }else{
	  NOTRACE(  std::cerr << "sem(std::stol(" << d << "))\n"; )
          NOTRACE(  std::cerr << "sem(" << std::stoll(d,0,0) << ")\n"; )
	  *this=sem(std::stoll(d,0,0));
        }
      }catch( const std::exception& ex ){
	std::cerr << "caught " << ex.what() << ": " << qtl::visible(d) << "\n";
           s=d[0]=='-'?-1:1;
	   e=std::numeric_limits<number>::max_exponent10+1;
           m="1"s;
      }
      return;
    }
    m.reserve(d.length());
    e=-1;
    s=1;
    bool point=false;
    const char *p;
    for( p=d.c_str();p!=d.c_str()+d.length();++p ){
        switch( *p ){
        case'0':case'1':case'2':case'3':case'4':case'5':case'6':case'7':case'8':case'9':{
	  m.push_back(*p);
  	  if( !point ){ ++e; }
        };break;
        case '.':{
	  point=true;
        };break;
        case '-':{
	  s = -s;
        };break;
	case '\'': case '+': case ' ':{ };break;
        case 'e':case 'E':{
	  char *end;
	  e += strtol(++p,&end,10);
        };[[fallthrough]];
        default:{ p=d.c_str()+d.length()-1; }
        }
      }
      NOTRACE( std::cerr << m << ".size()=" << m.size() <<  std::endl; )
      normalize();
      NOTRACE( if( !d.empty() ){ std::cout << __PRETTY_FUNCTION__ << "(" << d << ")=" << s << "," << e << ", " << m <<  std::endl; } )
    //      ˜return *this;
    }
    sem(const char *s):sem(std::string(s)){
     NOTRACE( std::cerr << __PRETTY_FUNCTION__ << "(" << s << ")" << std::endl; )
    }
    //    template< typename FloatingType >
    //    sem(const typename std::enable_if<!std::is_floating_point<FloatingType>::value>::type &f){
    typedef double FloatingType;
    sem(const FloatingType &f){
      /**/NOTRACE( std::cerr << __PRETTY_FUNCTION__ << "(" << f << ")" << std::endl; )
	if( !std::isfinite(f) ){
	  s=f<0?-1:1; e=std::numeric_limits<number>::max_exponent10+1; m="1"s;
	  return;
        }
      constexpr auto mantdigits = std::numeric_limits<FloatingType>::max_digits10;
      constexpr auto exp = std::numeric_limits<FloatingType>::max_exponent10;
      constexpr auto expdigits=exp<10?1:exp<100?2:exp<1000?3:exp<10000?4:10;
      char c[mantdigits+expdigits+6];
      #if 1
      snprintf(c,sizeof(c),"%.*e",mantdigits-1-1,f);
      #else
           auto r=to_chars(c,&c[sizeof(c)],f,std::chars_format::scientific,mantdigits-2);
      #endif
	   NOTRACE( std::cerr << __PRETTY_FUNCTION__ << "(" << f << ")" << c << std::endl; )
      *this=sem(c);
    }
    sem(const string &_this){
      if( _this.empty() ){ return; }
    NOTRACE( std::cerr << __PRETTY_FUNCTION__ << '(' << _this << ')' << '\n'; )
    if( (unsigned char)_this.view_()[0] < (unsigned char)'\x80' ){
      /**/NOTRACE( std::cerr << __PRETTY_FUNCTION__ << "-(-" << _this << "=" << -_this << ")" << std::endl; )
      *this= -sem(-_this);
      return;
    }else if(  (unsigned char)_this._this[0] == (unsigned char)'\x80' ){
      return; // 0
    }
    NOTRACE( std::cerr << "_this._this:" << _this._this << '\n'; )
    NOTRACE( std::cerr << "_this.std_():" << _this.std_()/*.c_str()*/ << '\n'; )
    NOTRACE( std::cerr << "p=" << p(_this._this) << '\n'; )
      auto i=std::upper_bound(schema.begin(),schema.end(),p(_this._this));
    if( i==schema.end() ){
      std::cerr << "overflow" << '\n';
      *this=sem(1,std::numeric_limits<int>::max(),"1");
    }
    --i;
    NOTRACE( std::cerr << i-schema.begin() << ":" << schema.end()-i << std::endl; )
    /**/NOTRACE( std::cerr << "i={" << *i << "}" << std::endl; )
    long l=p255(_this.std_(),i->b.b);
    /**/NOTRACE( std::cerr << l << " - "<< i->b.l << " = "; )
    l -= i->b.l;
    long r=l%i->b.d;
    l=l/i->b.d;
    /**/NOTRACE( std::cerr << l << ":" << r << std::endl; )
      if( i->s.data() && i->b.d <= 2 ){ // int
	NOTRACE(  std::cerr << "i + i->l=" <<  l+i->i <<  std::endl; )
      *this<<=l+i->i;
	NOTRACE( std::cerr << "m.size()-1=" << m.size()-1 <<  std::endl; )
      e= m.size()-1;
    }else{
      e=l+i->e;
      s=1;
       auto [fp,fr]=dd.at(i->b.d);
       /**/NOTRACE( std::cerr << *this << "<<=" <<  (r+i->b.d/9)/fr << std::endl; )
	if( i->b.d>=9 ){  *this <<= (r+i->b.d/9)/fr; }
       /**/NOTRACE(  std::cout << "=" << *this << std::endl; )
    }
    NOTRACE( std::cerr << "_this:" <<  _this << std::endl; )
    NOTRACE( std::cerr << "_this.std_():" <<  qtl::visible(_this.std_()) << std::endl; )
    NOTRACE( std::cerr << "i->b.b:" <<  i->b.b << std::endl; )
    NOTRACE( std::cerr << std::string(_this.std_(),i->b.b) << std::endl; )
    /**/NOTRACE( std::cerr << "<<=" <<  qtl::visible(std::string(_this.std_(),i->b.b),std::ios_base::hex) << std::endl; )
      for( auto c:std::string(_this.std_(),i->b.b) ){
	  int dd=((unsigned char)c/2)-0x40+50;
	  m += '0'+dd/10;
	  if( (c&1)||dd%10 ){ 
	    m += '0'+dd%10;
          }
        }
    normalize();
  }
  operator string()const{
      /**/NOTRACE( std::cerr << __PRETTY_FUNCTION__ << "(" << *this << ")" << std::endl; )
	if( m.empty() ){ return string(std::string("\x80")); }
    auto i=std::upper_bound(schema.begin(),schema.end(),p(*this));
    if( i==schema.begin() ){
      std::cerr << "underflow " << *this << '\n';
      return string(std::string("\x80"));
    }
    if( i==schema.end() ){
       std::cerr << "overflow " << s << '\n';
       if( s<0 ){
         return string(std::string("\x01\x01"));
       }
       return string(std::string("\xff\xff"));
    }
    --i;
    string ret;
    /**/NOTRACE( std::cerr << "i={" << *i << "}" << std::endl; )
      if( i->s.data() && i->b.d <= 2 ){ //integer
      auto l=ip();
      /**/NOTRACE( std::cerr << "l={" << std::get<0>(l) << ", " << std::get<1>(l) << "}" << std::endl; )
      int fp=std::get<1>(l);
      int l2= !(i->b.d&1) && fp < m.size() ;
      long l255=(std::get<0>(l)-i->i)*i->b.d+i->b.l+l2;
      /**/NOTRACE( std::cerr << "p255(" << " (" << (long)std::get<0>(l)<<"-"<<i->i<<")*"<<i->b.d<<"+"<<i->b.l<<-10 << "=" << ((long)std::get<0>(l)-i->i)*i->b.d+i->b.l << "+" << l2 << ", " << i->b.b << ")" << std::endl; )
    if( l2 ){
      switch( i->b.d ){
      case 9:{
	l2 =  (m[fp]-'0');
      };break;
      case 18:{
	l2 = (m[fp]-'0')*2+(fp>m.size());
	++fp;
      };break;
      case 90:{
	l2= (m[fp]-'0')*10+(fp+1<m.size()?m[fp+1]-'0':0);
	fp+=2;
      };break;
      case 180:{
	l2= ((m[fp]-'0')*10+(fp+1<m.size()?m[fp+1]-'0':0))*2+(fp+1>m.size());
	fp+=2;
      };[[fallthrough]];
      case 1:case 2: break;
      }
    }
      ret.std_() += p255(l255,i->b.b);
      for( auto d:dp(*this,fp) ){
        ret.std_() += d;
      }
    }else{
      auto [fp,fr]=dd.at(i->b.d);
      auto l=ip(fp);
      //      bool d2=(i->b.d&2)==0;
      bool d1=fr>1&&(fp<m.size());
      int d0=i->b.d/9;
      long _e=(e-i->e)*i->b.d+(std::get<0>(l)-i->b.d/(9*fr))*fr+d1+i->b.l;
      /**/NOTRACE( std::cerr << "p255(" << " (" << e << "-" << i->e<<")*"<<i->b.d<<"+("<< std::get<0>(l) <<"-" << i->b.d/(9*fr) << ")*"<<(fr)<<"+" <<d1 << "+" <<i->b.l << "=" << e << ")" << std::endl;  )
      ret.std_() += p255(_e,i->b.b);
      for( auto d:dp(*this,fp) ){
        ret.std_() += d;
      }
    }
    //if( _this.std_().empty() ){
    //      NOTRACE( std::cerr << __PRETTY_FUNCTION__ << "(" << s << ")\n"; )
    //	assert(!_this.std_().empty());
    //    }
    ret.view_();
    if( s<0 ){
      return -ret;
    }
    return ret;
  }

    int compare(const sem&r)const{
      if( (e<std::numeric_limits<number>::min_exponent10?0:s) == 0 && (r.e<std::numeric_limits<number>::min_exponent10?0:r.s) == 0 ){ return 0; }
      if( s!=r.s ){ return s-r.s; }
      if( e>std::numeric_limits<number>::max_exponent10 && r.e>std::numeric_limits<number>::max_exponent10 ){ return 0; }
      if( e!=r.e ){ return e-r.e; }
      return m.compare(r.m);
    }
#define X(o,n) bool operator o (const sem&r)const{ return compare(r) o 0; }
TABLE_CMP
#undef X
    bool is_inf()const{
     NOTRACE( std::cerr << __PRETTY_FUNCTION__ << '\n'; )
     NOTRACE( std::cerr << *this << '\n'; )
     return e>std::numeric_limits<number>::max_exponent10;
    }
    sem abs(){
      s=1;
      return *this;
    }
    sem abs()const{
      if( s==1 ){ return *this; }
      sem ret=*this;
      ret.s=1;
      return ret;
    }
    sem operator-()const{
      NOTRACE( std::cerr << __PRETTY_FUNCTION__ << "(" << *this << ")" << std::endl; )
      sem ret=*this;
      ret.s=-ret.s;
      NOTRACE( std::cerr << __PRETTY_FUNCTION__ << "=" << s << std::endl;  )
      return ret;
    }
    std::tuple<int,int> ip(int d=-1)const{
#if 1
      //      std::cout << __PRETTY_FUNCTION__ << "(" <<  *this << ", " << d << ")" << std::endl;
#else
      //      this->write( std::cout << __PRETTY_FUNCTION__ << "(" ) << ")" << std::endl;
#endif
      char c[12];
      if( d<0 ){
        d=e+1;
      }
        assert(d<sizeof(c));
      auto l=std::min(
		      {d,(int)m.size(),(int)sizeof(c)-1}
		     // {(int)e+1,(int)(n-m),(int)sizeof(c)-1}
		      //		      		     std::initializer_list<int>({e+1,(int)(n-m),sizeof(c)-1})
		      // std::initializer_list<int>{e+1,(int)m.size(),sizeof(c)-1}
		     // ,[](const int& i0, const int& i1) { return i0<i1; }
                      );
      std::strncpy(c,m.data(),l);
      while( l<d ){ c[l++]='0'; }
      c[l]='\0';
      //      std::cout << "={" << c << ',' << l<< "}" << std::endl;
      return {atoi(c),l};
    }
    operator std::string()const{
      NOTRACE( std::cerr << __PRETTY_FUNCTION__ << '\n'; ) 
      std::string d;
      d.reserve(m.size()+1+1+1+std::numeric_limits<decltype(e)>::digits10);
      if( s<0 ){ d += '-'; }
      bool dot=false;
      for( auto c:m ){
	d += c;
	if( !dot ){ d += '.'; dot=true; }
      }
#if 0
      return std::atof(d.c_str())*std::pow(10,e);
#else
      if( e!=0 ){
	d += 'e';
	char ee[std::numeric_limits<decltype(e)>::digits10+3];
	#if 0
	 std::to_chars(ee,ee+9,e,10);
	 #else
	 snprintf(ee,sizeof(ee),"%d",e);
        #endif
	d += ee;
      }
      NOTRACE( std::cerr << "d=" << d << '\n'; ) 
      NOTRACE( std::cerr << "d.c_str()=" << d.c_str() << '\n'; ) 
      NOTRACE( std::cerr << "std::atof(d.c_str())=" << std::atof(d.c_str()) << '\n'; ) 
      return d;
#endif
    }
    operator double()const{
      NOTRACE( std::cerr << __PRETTY_FUNCTION__ << '\n'; ) 
	return std::atof(operator std::string().c_str());
    }
#if 1 // iterator
    template<bool is_const_iterator = true>
    class _iterator  /*: public std::iterator<std::forward_iterator_tag,PointerType>*/{
      #if 0
        typedef typename std::conditional<is_const_iterator, std::string::const_iterator, std::string::iterator>::type base_iterator;
        typedef typename std::conditional<is_const_iterator, const char, char>::type base_value;
      #else // equivalent
        using base_iterator = typename std::conditional<is_const_iterator, std::string::const_iterator, std::string::iterator>::type;
        using base_value = typename std::conditional<is_const_iterator, const char, char>::type;
      #endif
      public:
      base_iterator p;
      base_iterator e;
      int x;
      base_value operator*(){ return p!=e?*p:'0'; }
      _iterator &operator++(){
	if( p!=e ){ ++p; }
	++x;
	return *this;
      }
      _iterator &operator+(int i){
	for( ;i>0;--i ){ ++*this; }
	return *this;
      }
      bool operator !=(const _iterator &i)const{
	return p!=i.p /*|| x != i.x*/;
      }
    };  //end class sem::_iterator
    //    typedef  _iterator<false> iterator;
    typedef _iterator<true> const_iterator;
    const_iterator  begin()const{
         const_iterator ret;
	  ret.p=m.begin();
	  ret.e=m.end();
	  ret.x=0;
	  return ret;
    }
    const_iterator  end()const{
         const_iterator ret;
	  ret.p=m.end();
	  ret.e=m.end();
	  ret.x=m.size();
	  return ret;
    }
#endif // iterator
    sem& operator <<=(unsigned int i){
      if( i >= 10 ){
	*this <<= i/10;
      }
      //      std::cout << __PRETTY_FUNCTION__ << "(" << i <<").size()= " <<  m.size() << std::endl;
      m += '0'+i%10;
      s |= i!=0;
      //      std::cout <<  m.size() << std::endl;
      return *this;
    };
  }; // end class sem
    class dp{
    public:
      typedef sem base_t;
      base_t _this;
      int offset;
      dp(base_t s, int o=0):_this(s),offset(o){}
    template<bool is_const_iterator = true>
    class _iterator  /*: public std::iterator<std::forward_iterator_tag,PointerType>*/{
      typedef base_t::_iterator<is_const_iterator> base_iterator;
      typedef typename std::conditional<is_const_iterator, const unsigned char, unsigned char>::type base_value;
      public:
      base_iterator p;
      base_value operator*(){
	base_iterator pp=p;
	char x=(*p-'0')*10+*(++pp)-'0';
        ++pp;
	return x*2+(pp.p!=pp.e)+0x80-100;
      }
      _iterator &operator++(){
	++p; ++p;
	return *this;
      }
      _iterator &operator+(int i){
	for( ;i>0;++i ){ ++*this; }
	return *this;
      }
      bool operator !=(const _iterator &i)const{
	return p!=i.p;
      }
    }; // end class dp::_iterator
    typedef _iterator<true> const_iterator;
    const_iterator begin()const{
      const_iterator ret;
      ret.p = _this.begin()+offset;
      return ret;
    }
    const_iterator end()const{
      const_iterator ret;
      ret.p = _this.end();
      return ret;
    }
    }; // end class dp

  class bdls{
  public:
    int b;
    int d; // enum{1,2,9,18,90,180,900,1800,9000,18000}
    unsigned long l;
    //    const char *s;
    std::string_view s;
    bdls(int b=0,int d=2,long l=0, const std::string_view s={}):b(b),d(d),l(l),s(s){}
    //bdls(int b=0,int d=2,long l=0, const char *s):b(b),d(d),l(l),s(s){}
    std::ostream& write( std::ostream &os )const{
      os << boost::core::demangle(typeid(*this).name()) << "{ b=" << b << ", d=" << d << ", l=" << l << ", s=" << qtl::visible(s) << "}";
      return os;
    }    
  friend std::ostream& operator<<(std::ostream& os, const bdls &s){
     return s.write(os);
  }

  }; // end class bdls
#ifdef  __DEC128_MANT_DIG__
#if 0
    class decnum{
      std::decimal::decimal128 d;
      void out(){
	//	d=strtod128("1234.567e89");
       	std::cout << decimal128_to_double(d);
	printf(" %Da\n",d);
	//	std::cout << d;
      }
    };
#endif
#endif
  class p{
  public:
    int e; 
    int i;
    //    const char *s;
    std::string_view s;
    bdls b;
    p(int e=0,const char *is=0,int b=0,int d=2,long l=0,const char*s=0):
      e(e),
      i(is?std::atoi(is):0),
      s(is?std::string_view(is):std::string_view()),
      b(b,d,s?p255(std::string(s,b)):l,s)
    { 
      /**/NOTRACE(  std::cout << __PRETTY_FUNCTION__ << "(" << e << ", " << N(is) << ", " << b << ", " << d << ", " << l << "(" << this->b.l << ")" << ", " << qtl::visible(s) << "=" << (s?p255(std::string(s,b)):0) <<  ")"  << std::endl;  )
    }
    p(const sem &s):e(s.e),s(s.m/*.c_str()*/){
      
    }
    p(const char *s){
      b.s=std::string_view(s);
    }
    p(const std::string_view &s){
      b.s=s;
    }
    int strcmp(const char*l, const char *r)const{
      if( l&&r ){ return std::strcmp(l,r); }
      if( l ){ return -1; }
      if( r ){ return 1; }
      return 0;
    }
    int strcmp(const std::string_view&l, const std::string_view&r)const{
      if( l.data()&&r.data() ){
	auto ret=std::strncmp(l.data(),r.data(),std::min(l.size(),r.size()));
	return ret==0?l.size()-r.size():ret;
      }
      if( l.data() ){ return -1; }
      if( r.data() ){ return 1; }
      return 0;
    }
    int cmp(const p&p)const{
      return( (b.s.data()&&p.b.s.data())?
	      strcmp(b.s,p.b.s)
	      :
	      e!=p.e?e-p.e:strcmp(s,p.s)
	      );
      /*
      return( e!=p.e?
                    e-p.e
	      :
                  b.s&&p.b.s? 
      	            strcmp(b.s,p.b.s)
                  :
                    strcmp(s,p.s)
	      );
      */
    }
    bool operator<(const p&p)const{
      //std::cout << "cmp(" << *this << "\n  < " << p << ")\n=" << cmp(p) << std::endl;
      return cmp(p)<0;
    }
    std::ostream& write( std::ostream &os )const{
      #if 1
      //      os << boost::core::demangle(typeid(*this).name()) << "{ e=" << e << ", i=" << i << ", s=" << N(s) << ", b=" << b << "}";
      os << boost::core::demangle(typeid(*this).name()) << "{ e=" << e << ", i=" << i << ", s=" << N(s) << ", b=" << b << "}";
     #else
      b.write( os << boost::core::demangle(typeid(*this).name()) << "{ e=" << e << ", i=" << i << ", s=" << N(s) << ", b=" ) << "}";
     #endif
      return os;
    }
    friend std::ostream& operator<<(std::ostream& os, const p &s){
      // os << boost::core::demangle(typeid(*this).name()) << "{ e=" << e << ", i=" << i << ", s=" << N(s) << ", a="<< a << ", b=" << b << "}";
      return s.write(os);
    }    
    #undef N
  }; // end class p
  inline static const std::vector<p> schema={
    p(-6176,0,5,18,          0,"\x81\x02\x01\x01\01"),
    p(-2501,0,5,180,  16868250/255,"\x81\x02\x02\x05\x6a"),
    p(-1925, 0, 4, 18, 1199711250, "\x81\x02\x03\x9d"),
    p(-937, 0, 4, 180, 2487206250, "\x81\x02\x49\x5b"),
    p(-827,0, 3, 18, 2487206250, "\x81\x02\x97"),
    p(-62, 0, 3, 180, 388999057500, "\x81\x38\x97"),
    p(-9, 0, 2, 18, 388999057500, "\x81\x5e"),
    p(0,"1", 1,2,         69005050200000/255,"\x82"),
    p(1,"51", 2,2,        69005050200000/255,"\xe6\x01"),
    p(3,"1069", 3,2,      69005050200000/255,"\xed\xfc\x01"),
    //    p(5,"553116", 4,2,   0,"\xfe\xf6\xc8\x01\x01\x01"),
    p(5,"553116",3,180,                   0,"\xfe\xf6\xc8"),
    p(10, 0, 3,          18, 80909515730250,"\xfe\xfa\x50"),
    p(27, 0, 3,          2,  80914589631000,"\xfe\xfb\x83"),
    p(43, 0, 3,          1,  80915120235000,"\xfe\xfb\xa3"),
    p(134, 0,4,         180, 80916629140125,"\xfe\xfb\xfe\x01"),
    p(777, 0, 4, 18,         80924155133625,"\xfe\xfd\xc5\xe2"),
    p(1333, 0, 4, 1,         80924805903825,"\xfe\xfd\xed\x22"),
    p(6145, 0, 2, 1,                      0, "\xfe\xfe"),
  };
  typedef string base_t;
  base_t _this;
  static const int depth=base_t::depth;
  std::optional<sem> mutable S;
  std::optional<double> mutable D;
  using cpp_dec_float_34 = boost::multiprecision::number<boost::multiprecision::cpp_dec_float<34> >;
  std::optional<cpp_dec_float_34> mutable DF/*=cpp_dec_float_34("3.14159265358979323846264338327950288419716939937510")*/;
  inline static std::string p255(unsigned long l=0){
    std::string ret;
    if( l>255 ){ ret=p255(l/255); }
    ret += (char)(l%255+1);
    return ret;
  }
  inline static std::string p255(unsigned long l,size_t s){
    if( !s ){ return p255(l); }
    std::string ret;
    if( s>1 ){ ret=p255(l/255,s-1); }
    ret += (char)(l%255+1);
    return ret;
  }
  inline static unsigned long p255(const std::string s){
   NOTRACE( std::cerr << __PRETTY_FUNCTION__ << std::endl; )
    unsigned long ret=0;
    for( auto c:s ){ ret=ret*255+(unsigned char)c-1; }
    return ret;
  }
  inline static unsigned long p255(const std::string s,size_t l){
    unsigned long ret=0;
    for( int i=0;i<l;++i ){
      ret = ret*255+(i<s.size()?(unsigned char)s[i]-1:0);
    }
    return ret;
  }
  int sgn()const{
    //return (empty()||(unsigned char)_this._this[0]<(unsigned char)'\x80')?-1:(unsigned char)_this._this[0]>(unsigned char)'\x80'?1:0;
    return empty()?-1:std::clamp((int)(unsigned char)_this._this[0]-0x80,-1,1);
  }
  number operator-(){
    NOTRACE( std::cerr << __PRETTY_FUNCTION__ << "(" << *this << ")" << std::endl; )
    NOTRACE( std::cerr << "_this=" << _this << std::endl; )
#if 0
    for( auto &c:_this.std_() ){
      c=0x100-c;
    }
#else
    _this=-_this;
#endif
    NOTRACE( std::cerr << "-_this=" << _this << std::endl; )
    #ifndef ERROR_INJECT
    if( D ){
      NOTRACE( std::cerr << "D=" << D.value() << std::endl; )
      D = -*D;
      NOTRACE( std::cerr << "-D=" << D.value() << std::endl; )
    }
    if( DF ){
      NOTRACE( std::cerr << "D=" << D.value() << std::endl; )
      DF = -*DF;
      NOTRACE( std::cerr << "-D=" << D.value() << std::endl; )
    }
    if( S ){
      NOTRACE( std::cerr << "S=" << *S << '\n'; )
      S = -*S;
      NOTRACE( std::cerr << "-S=" << *S << '\n'; )
    }
    #endif
    return *this;
  }
 number operator-()const{
    NOTRACE( std::cerr << __PRETTY_FUNCTION__ << "(" << *this << ")" << std::endl; )
    number ret=*this;
    return -ret;
  }
  int cmp(const number &r)const{
    return _this.compare(r._this);
  }
#define X(o,name)						     \
bool operator o (const number &r)const{ return cmp(r) o 0; } \
 template<typename N,typename = std::enable_if_t<std::is_scalar<N>::value>> bool operator o (const N &r)const{ return *this o (number)r; } \
 template<typename N,typename = std::enable_if_t<std::is_scalar<N>::value>> friend bool operator o (const N &l,const number &r){ return (number)l o r; } \
// end define X
TABLE_CMP
#undef X

  inline static const std::map<int,std::tuple<int,int> >dd={
    {1,{0,1}},     {2,{0,2}},
    {9,{1,1}},    {18,{1,2}},
    {90,{2,1}},    {180,{2,2}},
    {900,{3,1}},    {1800,{3,2}},
    {9000,{4,1}},    {18000,{4,2}},
  };
  number(){};
  number(std::nullptr_t nullp):_this(nullp)/*,D(NAN)*/{}
  number(const string &s):_this(s){
      NOTRACE( std::cerr << __PRETTY_FUNCTION__ << "(" << s << ")\n"; ) 
      NOTRACE( std::cerr << "_this._opt=" << std::boolalpha << (bool)_this._opt << '\n'; ) 
      NOTRACE( if( _this._opt ){ std::cerr << "*_this._opt=" << *_this._opt << '\n'; } ) 
      NOTRACE( std::cerr << "_this._this=" << _this._this << '\n'; ) 
  }
 number(const std::string_view &v):_this(v){}
 number(const std::string &s):number(sem(s)){}
 number(const char *s):number(sem(s)){}
 number(const sem&s):S(s),_this((string)s){}
 template<class T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
   number(const T&t):number(sem(t)){
       NOTRACE( std::cerr <<  __PRETTY_FUNCTION__ << '(' << t << ")\n"; );
       NOTRACE( std::cerr <<  "=" << *this << '\n' );
     }
  operator sem()const{
    if( !S ){
      S=sem(_this);
    }
    return *S;
  }
  bool empty()const { return _this.empty(); }
  bool is_inf()const { 
     using namespace lex::literals;
     NOTRACE( std::cerr << __PRETTY_FUNCTION__ << '\n'; )
     NOTRACE( std::cerr << *this << '\n'; )
       NOTRACE( std::cerr << _this.empty() << "||" << (_this <= "\1\1"_s) << "||" << ("\xff\xff"_s <= _this) << "\n"; )
       return _this.empty() || _this <= "\1\1"_s || "\xff\xff"_s <= _this;
 }
  bool is_null()const{ return _this.is_null(); }
  string raw()const{
    return _this;
  }
  operator string()const{
    return _this;
  }
  operator double()const{
   NOTRACE( std::cerr << __PRETTY_FUNCTION__ << '\n'; )
    NOTRACE( std::cerr << "_this._opt=" << std::boolalpha << (bool)_this._opt << '\n'; )
    NOTRACE( if( _this._opt ){ std::cerr << "*_this._opt=" << (void*) _this._opt->data() << "=" <<  qtl::visible(*_this._opt) << '\n'; } )
    NOTRACE( std::cerr << "_this._this.data()=" << (void*) _this._this.data()  << '\n'; )
    NOTRACE( std::cerr << "_this._this=" << qtl::visible(_this._this) << '\n'; )
     if( !D ){
       if( DF ){
	 D=static_cast<decltype(D)>(*DF);
       }else{
       // return (double)(sem)*this; 
       //  #pragma message "clang error: ambiguous conversion for C-style cast from 'const lex::number' to 'lex::number::sem'"
	NOTRACE( std::cerr << __LINE__ << '\n'; )
	   if( is_null() ){
	     return std::numeric_limits<double>::quiet_NaN();
           }
	 D=(double)((*this).operator sem());    
	 NOTRACE( std::cerr << __LINE__ << '\n'; )
       }
     }
     return *D;
  }

  /*  operator string()const{
    return _this;
  }
  */
  std::ostream& write(std::ostream& os)const{
    if( _this.empty() ){
      os << "\"\"_s";
      return os;
    }
    qtl::ios_base::fmtflags f;
    os >> f;
    std::string t=qtl::type_name<decltype(*this)>(f.verbosity);
    if( !t.empty() ){
      os << t << "{";
      if( /**/1||/**/ f.verbosity & qtl::ios_base::fmtflags::show_cache ){
	if( S ){ 
          os << "S=" << *S << ", ";
        }
	if( D ){
	  os << "D=" << *D << ", ";
        }
	if( DF ){
         os<< "DF=" << *DF << ", ";
	}
        os << "_this=";
	if( f.verbosity & qtl::ios_base::fmtflags::show_address ){
  	   if(  _this._opt ){
	     os<< "{ string.data()=" << (void*)_this._opt->data() << ", string_view.data()=";
           }
	   os << (void*) _this._this.data() << ", ";
	}
        os << qtl::visible(_this._this,os.flags()) << ", ";
        if(  _this._opt ){ os << "}"; }
      }
    }
    if( is_null() ){ 
      os << "NULL";
    }else{
      if( S||D||DF ){
	os << (*this).operator sem() << "_s";
      }else{
	os << _this;
      }
    }
    if( !t.empty() ){
      os << "}";
    }
    return os;
  }
  friend std::ostream& operator<<(std::ostream& os, const number &s){
     return s.write(os);
  }
  using lex_t=lex::number;
 }; // end class lex::number
 int common(const number &a,const number &b){
   if( a.DF || b.DF ){ return 0; }
   return 1;
 };
#define OP(o)						\
    number operator o(const number &a,const number &b){ \
      return number( (double)a o (double)b ); \
    } \
//end define OP
 OP(+)
 OP(-)
 OP(*)
 OP(/)
#undef OP
   std::string static _string( const lex::number& n ){
     return _string(n._this);
   }
#define BASE_T lex::number
 class scalar: public BASE_T{ using base_t=BASE_T;
#undef BASE_T
 public:
   using base_t::base_t;
   //   std::optional<lex::number> num;
   template<typename T,typename = std::enable_if_t<std::is_scalar<T>::value>>
   scalar(const T &t):base_t(t){}
   scalar(const lex::number &s):base_t(s){}
   scalar(const char *s):base_t(std::string(s)){}
   scalar(const std::string &s):base_t(lex::raw(lex::string(s))){}
   scalar(const lex::string &s):base_t(lex::raw(s)){}
   size_t size(){ return _this.size(); }
#define f(o) \
  friend bool operator o(const scalar &s, const base_t &n){	\
     return static_cast<base_t>(s) o n;				\
 } \
 friend bool operator o(const base_t &s, const scalar &n){		\
     return static_cast<base_t>(s) o n;				\
 } \
 template<typename T,typename = std::enable_if_t<std::is_scalar<T>::value>> \
 friend bool operator o(const lex::scalar &l, T s){ \
   return l o lex::number(s); \
 } \
 template<typename T,typename = std::enable_if_t<std::is_scalar<T>::value>> \
 friend bool operator o(T s,const lex::scalar &l){ \
   return lex::number(s) o l; \
 } \
 friend bool operator o(const lex::scalar &l,const lex::scalar &r){	\
   return l._this o r._this;								\
 } \
 // end define f
   f(==)
   f(!=)
   f(<)
   f(<=)
     //   f(>)
     //   f(>=)
#undef f
   operator std::string()const{ return (std::string)(lex::string)*this; }
   friend std::ostream& operator<<(std::ostream& os,const scalar &s){
     os<<(lex::string)s;
     return os;
   }
   friend qtl::ostringstream& operator<<(qtl::ostringstream& os,const scalar &s){
     os<<(lex::string)s;
     return os;
   }
 }; // end class lex::scalar

 namespace literals{
  number operator""_s(const char *c){
    NOTRACE( std::cerr << __PRETTY_FUNCTION__ << "(" << c << ")\n"; )
    return number(c);
  }
  number operator""_s(unsigned long long int i){
    NOTRACE( std::cerr << __PRETTY_FUNCTION__ << "(" << i << ")\n"; )
    return number(i);
  }
 }
} // end namespace lex



#if __INCLUDE_LEVEL__ + !defined __INCLUDE_LEVEL__ < 1+defined TEST_H
 void dd(lex::number::sem m0){
    /**/NOTRACE( std::cerr << __PRETTY_FUNCTION__ << std::endl; )
    /**/NOTRACE( std::cerr << '(' << m0 << ')' << std::endl; )
    /**/NOTRACE( std::cerr << "m0.s=" << m0.s << std::endl; )
    /**/NOTRACE( std::cerr << "m0.e=" << m0.e << std::endl; )
    /**/NOTRACE( std::cerr << "m0.m=" << qtl::visible(m0.m) << std::endl; )
    /**/NOTRACE( std::cerr << "m0.m.data()=" << (void*)m0.m.data() << std::endl; )
  lex::number s = lex::number(m0);
  std::cout << "number(" << m0 << ")=" << s << std::endl;
  lex::number::sem m=s;
  std::cout << "sem(" << s << ")=" << m << std::endl;
  //  std::cout << std::endl;
  assert( m0==m );
}
template<typename T>
void d(T d){
  NOTRACE( std::cerr << __PRETTY_FUNCTION__ << '(' << d << ')' << std::endl; )
  lex::number::sem m=d;
  dd(m);
  dd(-m);
  std::cout << std::endl;
}
void o(double d){
 char c[sizeof(double)+2];
 std::memcpy((void*)c,(void*)&d,sizeof(d));
 c[sizeof(double)]='\0';
 std::cout << c << '\n';
}
int main(int argc, char *argv[]){
if( argc==1 || argv[1][0] == '<' ){
  std::ifstream in;
  std::streambuf *cinbuf;
  if( argc>1 && argv[1][0] == '<' ){
    in = std::ifstream(argv[1][1]?argv[1]+1:argv[2]);
    cinbuf = std::cin.rdbuf(); 
    std::cin.rdbuf(in.rdbuf()); //redirect std::cin
  }
  int tellg=0;
  while( std::cin.good() && tellg<1000 ){
    std::string i;
    std::getline(std::cin, i);
    tellg += i.size()+1;
    NOTRACE( std::cerr << "rdstate=" << std::cin.rdstate() << " good=" << std::cin.good() << " eof=" << std::cin.eof() << " bad=" <<  std::cin.bad() << " fail=" << std::cin.fail() <<  "  tellg()=" <<  std::cin.tellg() << " gcount=" <<  std::cin.gcount() << " size=" << i.size() << '\n'; )
    if( !std::cin.good() || tellg>2000 ){ break; }
    std::smatch m;
    static const std::regex num_re("^[-+]?(" NUM_0 "|(\\d(\'\\d)*)*\\.?\\d*([eE][-+]?(\\d(\'\\d)*)+)?)",std::regex::optimize);
    if( std::regex_search(i,num_re) ){
      d(i);
    }else{
       double D;
       D=0;
       std::memcpy((void*)&D,(void*)i.data(),std::min(sizeof(D),i.size()));
       if( std::isnan(D) ){ continue; }
       d(D);
    }
    std::cout <<  qtl::setverbose(qtl::ios_base::fmtflags::none);
  }
  std::cout << "test passed\n";
  exit(0);
 }
 if( std::strcmp(argv[1],"dict")==0 ){
   std::cout << "\"9\"\n";
#ifdef FUZZING
   exit(0);
#endif
 }
NOTRACE( std::cerr << NUM_0 << "\n"; )
  NOTRACE(
 using namespace lex::literals;
    std::cout << -0b1'00'11'00'11_s << "\n";
    std::cout << 1_s << "\n";
    std::cout << 1'1'1_s << "\n";
    std::cout << 0b10_s << "\n";
	)
#if 0
  lex::number::sem s(1234.5678);
  std::cerr << s << "=" <<std::setprecision(9)<< (double)s << std::endl;
  std::cerr << lex::number::sem(1.23456789123456789123456789e56) << std::endl;
  std::cerr << lex::number::sem(-1.23456789123456789123456789e-56) << std::endl;
  std::cout << lex::number::sem("-1234.5679e-1234") << std::endl;
  std::cout << lex::number::sem("-0000.0000e-1200") << std::endl;
  std::cout << lex::number::sem(-1,23,"456000") << std::endl;
  std::cout << std::endl;
 d(0);
#endif
#if 1
 using namespace lex::literals;
 std::cout <<  qtl::setverbose(qtl::ios_base::fmtflags::none);
 std::cout << -1 << '\n';
 std::cout << 0 << '\n';
 // std::cout << "1e-6176\n";
 // std::cout << -1_s; // warning: missing terminating ' character [-Winvalid-pp-token]
 std::cout << -1'2e-2'4_s << '\n'; 
 std::cout << -0x1'2_s << '\n';;
// std::cout << -01'8_s; // error: invalid digit '8' in octal constant
 // std::cout << -01'8._s << '\n';
 // std::cout << -0b1'1_s << '\n';
 // std::cout << -0x_s; //invalid suffix 'x_s' on integer constant
 // std::cout << -0xe_s << '\n';
 // std::cout << -0x._s; // error: hexadecimal floating literal requires a significand
 // std::cout << -0x.e0_s; // error: hexadecimal floating literal requires an exponent
 // std::cout << -0x'0.e0_s; // invalid suffix 'x'0.e0_s' on integer constant
 // std::cout << -0x.0p0_s << '\n';
 // std::cout << -1p9_s; //  error: invalid suffix 'p9_s' on integer constant
 // std::cout << 0xp9_s; //  error: invalid suffix 'p9_s' on integer constant
 // std::cout << -0x1.p9_s; 
 // std::cout << -0x.p9_s; // error: hexadecimal floating literal requires a significand
 // std::cout << -1e_s; // error: exponent has no digits
   // std::cout << 1'_s; //  error: digit separator cannot appear at end of digit sequence
   //   std::cout <<  _s; // error: use of undeclared identifier '_s'
   // std::cout <<  -1''2_s; // warning: empty character constant [-Winvalid-pp-token]
 // std::cout <<  -'1_s; // warning: missing terminating ' character [-Winvalid-pp-token]
 std::cout << 1.005e-6176_s<<"\n";
 std::cout << 1.2e-6176_s<<"\n";
 std::cout << 1.23e-6176_s<<"\n";
// std::cout << 2e-6176_s<<"\n"; // WARNING: No new instrumentation output, test case may be useless.
 std::cout << 8.0e-2502_s<<"\n"; // WARNING: No new instrumentation output, test case may be useless. 
  std::cout << 8.9e-2502_s<<"\n";
//  std::cout << 9.9e-2502_s<<"\n"; // WARNING: No new instrumentation output, test case may be useless.
//  std::cout << 9.95e-2502_s<<"\n";// WARNING: No new instrumentation output, test case may be useless.
  std::cout << 1e-2501_s<<"\n";
  //  std::cout << 1.01e-2501_s<<"\n";// WARNING: No new instrumentation output, test case may be useless.
  //  std::cout << 1.23e-2501_s<<"\n";// WARNING: No new instrumentation output, test case may be useless.
  //  std::cout << 1.234e-2501_s<<"\n";// WARNING: No new instrumentation output, test case may be useless.
  //  std::cout << 2e-2501_s<<"\n";// WARNING: No new instrumentation output, test case may be useless.
  //  std::cout << 9.995e-1926_s<<"\n";// WARNING: No new instrumentation output, test case may be useless.
  std::cout << 1e-1925_s<<"\n";
  std::cout << 9.995e-938_s<<"\n";
  std::cout << 1.005e-937_s<<"\n";
  //  std::cout << 9.95e-828_s<<"\n";// WARNING: No new instrumentation output, test case may be useless.
  std::cout << 1.005e-827_s<<"\n";
  //  std::cout << 9.995e-63_s<<"\n";// WARNING: No new instrumentation output, test case may be useless.
  std::cout << 1.005e-62_s<<"\n"; // WARNING: No new instrumentation output, test case may be useless. 
  //  std::cout << 9.995e-10_s<<"\n";// WARNING: No new instrumentation output, test case may be useless.
#endif
  std::cout << 12345 << '\n';
  std::cout << 123456 << '\n';
  //  std::cout << 553115 << '\n';// WARNING: No new instrumentation output, test case may be useless.
  std::cout << 553115.995 << '\n';
  //  std::cout << 553116 << '\n';// WARNING: No new instrumentation output, test case may be useless.
  //  std::cout << 553116.5 << '\n';
  std::cout << 1234567 << '\n';
std::cout << 9.995e8 << '\n';
#if 1
//  std::cout << 1.0e9 << '\n';// WARNING: No new instrumentation output, test case may be useless.
//  std::cout << 1.005e9 << '\n';// WARNING: No new instrumentation output, test case may be useless.
//  std::cout << 1.05e9 << '\n';// WARNING: No new instrumentation output, test case may be useless.
//  std::cout << 1.5e9 << '\n';// WARNING: No new instrumentation output, test case may be useless.
//  std::cout << 2e9 << '\n';// WARNING: No new instrumentation output, test case may be useless.
//  std::cout << 9.5e9 << '\n';// WARNING: No new instrumentation output, test case may be useless.
//  std::cout << 9.95e9 << '\n';// WARNING: No new instrumentation output, test case may be useless.
//  std::cout << 9.995e9 << '\n';// WARNING: No new instrumentation output, test case may be useless.
//  std::cout << 1e10 << '\n';// WARNING: No new instrumentation output, test case may be useless.
//  std::cout << 1.005e10 << '\n;'// WARNING: No new instrumentation output, test case may be useless.
//  std::cout << 1.05e10 << '\n';// WARNING: No new instrumentation output, test case may be useless.
//  std::cout << 1.5e10 << '\n';// WARNING: No new instrumentation output, test case may be useless.
//  std::cout << 2e10 << '\n';// WARNING: No new instrumentation output, test case may be useless.
  //  std::cout << 9.95e26 << '\n';// WARNING: No new instrumentation output, test case may be useless.
  std::cout << 1.0e27 << '\n';
  //  std::cout << 2.0e27 << '\n';// WARNING: No new instrumentation output, test case may be useless.
  std::cout << 9.95e42 << '\n'; // WARNING: No new instrumentation output, test case may be useless. 
  //  std::cout << 1.0e43 << '\n';// WARNING: No new instrumentation output, test case may be useless.
  std::cout << 9e133_s<<"\n";
  //  std::cout << 9.95e133_s<<"\n";// WARNING: No new instrumentation output, test case may be useless.
  //  std::cout << 1e134_s<<"\n";// WARNING: No new instrumentation output, test case may be useless.
  //  std::cout << 2e134_s<<"\n";// WARNING: No new instrumentation output, test case may be useless.
  //  std::cout << 9.9e134_s<<"\n";// WARNING: No new instrumentation output, test case may be useless.
  //  std::cout << 9.95e134_s<<"\n";// WARNING: No new instrumentation output, test case may be useless.
  //  std::cout << 9.95e176_s<<"\n";// WARNING: No new instrumentation output, test case may be useless.
  //  std::cout << 1e177_s<<"\n";// WARNING: No new instrumentation output, test case may be useless.
  //  std::cout << 9.95e1332_s<<"\n";// WARNING: No new instrumentation output, test case may be useless.
  std::cout << 1e1333_s<<"\n";
  //  std::cout << 9e6144_s<<"\n";// WARNING: No new instrumentation output, test case may be useless.
  //  std::cout << 9.995e6144_s<<"\n";// WARNING: No new instrumentation output, test case may be useless.
  std::cout << "1e6145\n";
  std::cout << "9e-6177\n";
  std::cout << "0x1d12eddeadc0ffeeaffec7edoffbea7b19badA5C11200\n";
#endif
  exit(0);
#if !(defined __clang__ && defined __apple_build_version__ && __apple_build_version__ <= 9000038)
  //#pragma message "g++ " __VERSION__
  std::cout << lex::number(12) + lex::number(3.4) << std::endl;
  std::cout << (double)lex::number(12) << "+" << (double)lex::number(3.4) << std::endl;
  std::cout << (double)(lex::number(12) + lex::number(3.4)) << std::endl;

  std::cout << lex::number(12) * lex::number(3.4) << std::endl;
  std::cout << (double)(lex::number(12) * lex::number(3.4)) << std::endl;

  std::cout << lex::number(12) / lex::number(3.4) << std::endl;
  std::cout << (double)(lex::number(12) / lex::number(3.4)) << std::endl;

  std::cout << lex::number(12) - lex::number(3.4) << std::endl;
  std::cout << (double)(lex::number(12) - lex::number(3.4)) << std::endl;
#else
#pragma message "clang error: invalid cast from type ‘lex::number’ to type ‘double’"
#pragma message "clang error: no matching conversion for C-style cast from 'lex::number' to 'double'"
#pragma message "clang error: invalid operands to binary expression ('lex::number' and 'lex::number')"
#endif
}
#endif

/*
(-19, -51, 1, 2, 0.100416664757331, 0, 69005050200000, "a6 01 01 01 01 01 ", "e6 01 01 01 01 01 "),
  (-51, -1069, 2, 2, 0.254262919270472, 69005050200000, 77613768472500, "e6 01 01 01 01 01 ", "ed fc 01 01 01 01 "),
  (-1069, -553116, 3, 2, 0.0977412212706538, 77613768472500, 95921165121750, "ed fc 01 01 01 01 ", "fe f6 c8 01 01 01 "),
  (5, 10, 3, 180, -0.0334078647420115, 80894592492750, 80909515730250, "fe f6 c8 01 01 01 ", "fe fa 50 01 01 01 "),
  (10, 27, 3, 18, -0.00379976862122415, 80909515730250, 80914589631000, "fe fa 50 01 01 01 ", "fe fb 83 01 01 01 "),
  (27, 43, 3, 2, -9.31986948801547e-05, 80914589631000, 80915120235000, "fe fb 83 01 01 01 ", "fe fb a3 01 01 01 "),
  (43, 134, 3, 1, -2.03386428282787e-05, 80915120235000, 80916629140125, "fe fb a3 01 01 01 ", "fe fb fe 01 01 01 "),
  (134, 777, 4, 180, -2.23928641888919e-07, 80916629140125, 80924155133625, "fe fb fe 01 01 01 ", "fe fd c5 e2 01 01 "),
  (777, 1333, 4, 18, -1.73087829755861e-10, 80924155133625, 80924805903825, "fe fd c5 e2 01 01 ", "fe fd ed 22 01 01 "),
  (1333, 6145, 4, 1, -2.71215311428825e-11, 80924805903825, 80925118804125, "fe fd ed 22 01 01 ", "fe fe 01 01 01 01 "),
*/

 /*
(-6176, -2501, 5, 18, 0.000500499068054354, 0, 16868250, 81 02 01 01 01 01 , 81 02 02 05 6a 01 )
(-2501, -1925, 5, 180, 0.00237075805806431, 16868250, 43306650, 81 02 02 05 6a 01 , 81 02 03 9d 01 01 )
(-1925, -937, 4, 18, 0.0456278171438809, 43306650, 1199711250, 81 02 03 9d 01 01 , 81 02 49 5b 01 01 )
(-937, -827, 4, 180, 0.018631943823812, 1199711250, 2487206250, 81 02 49 5b 01 01 , 81 02 97 01 01 01 )
(-827, -62, 3, 18, 1.34233075101583, 2487206250, 230812740000, 81 02 97 01 01 01 , 81 38 97 01 01 01 )
(-62, -9, 3, 180, 0.95930678633806, 230812740000, 388999057500, 81 38 97 01 01 01 , 81 5e 01 01 01 01 )
(-9, 0, 2, 18, 0.288024553936712, 388999057500, 1073975658750, 81 5e 01 01 01 01 , 82 01 01 01 01 01 )

  */
