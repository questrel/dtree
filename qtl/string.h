#pragma once
#include "out.h"
#include <string>
#include <assert.h> 
#include <boost/core/demangle.hpp>
#include <iostream>
#include <utility>
#include <cstddef>
#include <boost/spirit/home/x3.hpp>
#include <boost/spirit/home/x3/binary/binary.hpp>
#include <boost/spirit/home/x3/directive/repeat.hpp>
//#include <cstdlib> // div
//#include  <experimental/ranges/iterator>
//#include <charconv>
//#include <boost/lambda/lambda.hpp>
//#define USE_RANGES

#ifdef USE_EXPERIMENTAL_RANGES
#include <std::experimental::ranges>
//fatal error: 'std::experimental::ranges' file not found
#endif
#define USE_BOOST_RANGE
#ifdef USE_BOOST_RANGE
#include <boost/range/iterator_range.hpp>
#endif
#if 0
//https://stackoverflow.com/questions/13292237/c-concat-two-const-char-string-literals
#include <array>

template<unsigned... Is> struct seq{};
template<unsigned N, unsigned... Is>
  struct gen_seq : gen_seq<N-1, N-1, Is...>{};
template<unsigned... Is>
struct gen_seq<0, Is...> : seq<Is...>{};

template<unsigned N1, unsigned... I1, unsigned N2, unsigned... I2>
  constexpr std::array<char const, N1+N2-1> concat(char const (&a1)[N1], char const (&a2)[N2], seq<I1...>, seq<I2...>){
  return {{ a1[I1]..., a2[I2]... }};
}

template<unsigned N1, unsigned N2>
  constexpr std::array<char const, N1+N2-1> concat(char const (&a1)[N1], char const (&a2)[N2]){
  return concat(a1, a2, gen_seq<N1-1>{}, gen_seq<N2>{});
}
#endif

#if 1
//https://stackoverflow.com/questions/45287195/combine-two-or-more-arrays-of-different-size-to-one-array-at-compiletime
template <typename T, std::size_t N1, std::size_t N2>
  constexpr std::array<T, N1 + N2> concat(std::array<T, N1> lhs, std::array<T, N2> rhs)
{
  std::array<T, N1 + N2> result{};
  std::size_t index = 0;

  for (auto& el : lhs) {
    result[index] = std::move(el);
    ++index;
  }
  for (auto& el : rhs) {
    result[index] = std::move(el);
    ++index;
  }

  return result;
}
#endif

namespace lex{
/** 

{...}
{{  {}}}
{   {}}
{{  {},{}}}
{   {},{}}
{{{ ""}}}
{{  ""}}
{   ""}
{{  "",""}}
{   "",""}
{{{ "\0"}}}
{{  "\0"}}
{   "\0"}

""
"\0"
"\0\0"
"\0\0\0"
"\0\0\0\0"
"\0\0\0\1"
"\0\0\1"
"\0\1"
"\1"
"\1\0"
"\xff\xff"
null


**/

/**

00 00 -> eof0
...
00 01 01 01 7f -> eof_0101017f
00 01 01 02 -> eof_010102
...
00 01 01 7f -> eof_01017f
00 01 02 -> eof_0102
00 01 03 -> eof_0103
...
00 01 7e -> eof_007e
00 01 7f -> eof_017f
00 02 -> eof_02
...
00 79 -> {{{{eof_79}}}}
00 7a -> {{{eof_7a}}}
00 7b -> {{eof_7b}}
00 7c -> {eof_7c}
00 7d -> eof-
00 7e -> eof
00 7f -> eof+

00 80 00 -> 00 eof
00 81 00 -> 00 00 eof
00 82 00 -> 00 00 00 eof
00 83 00 -> 00^4 eof
...
00 bf 00 -> 00^64 eof

00 c0 ?  -> 00^64 ?
...
00 fc ?  -> 00^4 ?
00 fd ?  -> 00 00 00 ?
00 fe ?  -> 00 00 ?
00 ff ?  -> 00 ?

ff ff ff xx -> ff ff ff xx
ff ff ff fc xx -> ff ff ff fc xx
ff ff ff fd xx -> ff ff ff fd fd xx
ff ff ff fe xx -> ff ff ff fd fe xx
ff ff ff ff xx -> ff ff ff fd ff xx
inf ->  ff ff ff fe
null -> ff ff ff ff

**/
using namespace std::string_literals;  
using namespace std::literals;
template<auto f,int r=125 >
  class constref{
  static inline std::vector<std::vector<std::vector<decltype(f(0))>>>cache
#ifndef ERROR_INJECT
  ={} // clang crashes without this
#endif
;
#undef E0
  public:
    const decltype(f(0)) * get(int x){
    assert(x>=0);
    int x0;
    int x1;
    int x2;
    auto q=std::div(x,r);
    std::tie(x1,x0)={q.quot,q.rem};
    q=std::div(x1,r);
    std::tie(x2,x1)={q.quot,q.rem};
    NOTRACE( std::cout <<  x2 << ", " << x1 << ", " << x0 << std::endl; )
    NOTRACE( std::cout <<  cache.size() << ":" <<(void*) cache.data() << std::endl; )
    if( cache.size()<=x2 ){
      cache.reserve(std::max(r,x2+1));
      while( cache.size()<=x2 ){
	cache.push_back( {} );
	cache.back().reserve(r);
      }
    }
    NOTRACE( std::cout <<  cache.size() << ":" <<(void*) cache.data() << std::endl; )
    NOTRACE( std::cout <<  cache[x2].size() << ":" <<(void*) cache[x2].data() << std::endl; )
    if( cache[x2].size()<=x1 ){
      cache[x2].reserve(std::max(r,x1+1));
      while( cache[x2].size() <= x1 ){
	cache[x2].push_back( {} );
	cache[x2].back().reserve(r);
      }
    }
    NOTRACE( std::cout <<  cache[x2].size() << ":" <<(void*) cache[x2].data() << std::endl; )
    NOTRACE( std::cout <<  cache[x2][x1].size() << ":" <<(void*) cache[x2][x1].data() << std::endl; )
    if( cache[x2][x1].size()<=x0 ){
      cache[x2][x1].reserve(r);
      int i=((x2*r)+x1)*r+cache[x2][x1].size();
      while( cache[x2][x1].size() <= x0 ){
	cache[x2][x1].push_back( f(i++) );
      }
    }
    NOTRACE( std::cout <<  cache[x2][x1].size() << ":" <<(void*) cache[x2][x1].data() << std::endl; )
    return &cache[x2][x1][x0];
  }
}; // end class constref<>

  static auto  make_skipeof_regex(int x){
    static auto _rv=[](int r,int c){
       static auto _x=[](int c){
           constexpr char x[]="0123456789abcdef";
	   return "\\x"s+x[c>>4]+x[c&0xf];
       };
       return "\\x00"s+(r==0?""s:"\\x01{"s+std::to_string(r)+"}"s)+"(?:("s+_x(c)+")|\\x01+[^\\x01]|[\\x00-"s+_x(c)+"])"s;
    };
     int _r=std::max(0,x)/125;
     int _x='\x7e'-(x-125*_r);
     NOTRACE( std::cerr<<__PRETTY_FUNCTION__<<"("<<x<<")=rv(" << _r << "," << _x << ")="<<_rv(_r,_x)<<"\n"; )
     return std::regex(_rv(_r,_x),std::regex::optimize);
  };
  static constref<make_skipeof_regex> skipeof_regex;
 
static std::string make_eof(int x){
  return "\0"s+std::string(x==-1?0:x/125,'\01')+(char)('\x7e'-(x==-1?-1:x%125));
}
static inline const constref<make_eof> eofstring;
//static inline const std::regex e1=make_skipeof_regex(-1);

class end{
};// end class end

class eof{
  static inline constref<make_eof> eofstring;
  static inline const std::string e1=make_eof(-1);
  const std::string *s;
  const std::string_view sv;
 public:
#if defined( ERROR_INJECT ) || defined( __clang__ )
  constexpr // error unser g++ but not in clang++
#else
#endif
 eof(int d):s(d==1?&e1:eofstring.get(-d)),sv(s->c_str(),s->size()){
    /**/NOTRACE(std::cerr << __PRETTY_FUNCTION__ << "(" << d << "):s("<<s<<"),sv=("<<sv<<")"<<std::endl; )
  }
#if defined( ERROR_INJECT ) // || defined( __clang__ )
  const // error unser g++ but not in clang++ (before 7.0.0_1)
#else
    //constexpr
#endif
  operator std::string()const{ return *s; }
  //  operator std::string_view(){ return sv; }
};// end class eof;

class string/*:public std::string*/ {
public:
  typedef std::string_view base_t;
  typedef std::string std_t;
  static inline const int depth=0;
  template<int d>
  static inline constexpr auto _eof(){
       assert( d<=1 );
    if constexpr ( d>0x01-0x7e ){
       return std::array<char,1>{'\x7e'+d}; 
    }else{
      return concat(std::array<char,1>{'\x01'},_eof<d+(0x7e - 0x01)>());
    }
  }
  //  template<int d>
  //    static inline const std::string eof="\0"s+std::string(_eof<d>().data(),_eof<d>().size());

  //  using base_t::base_t;

  mutable std::optional<std_t> _opt;
  mutable base_t _this;
  static int plength(std::string &s){
    int ret=s.size();
    if( ret==0 ){ return 0; }
#if 0
    std::string::iterator it=s.begin();
    if( ret>=4 && s[0]=='\xff' && s[1]=='\xff' && s[2]=='\xff' && s[3] > '\xfc' ){
      ++ret;
      ++it;
    }
    while( it != s.end() ){
      if( *it == '\0' ){
	int c=0;
	while( ++it != s.end() && *it == '\0' ){
	  ++c;
	}
	ret += 2+2*(c/64);
      }else{
	++it;
      }
    }
#else
  namespace x3 = boost::spirit::x3;
  using x3::_attr;
  using x3::char_;
  using x3::byte_;
  using x3::repeat;
  using x3::raw;  
  auto rf = [&]( auto& ctx ){ ++ret; };
  auto rz = [&]( auto &ctx ){ ret += 1-_attr(ctx).size(); };
  auto rs = [&]( auto &ctx ){ };
  assert( parse(s.begin(), s.end(), 
	 -( raw[ repeat(3)[ byte_(0xff)] >> &(byte_(0xfd)|byte_(0xfe)|byte_(0xff)) ][rf] )
	>> *( raw[repeat(1,64)[byte_('\0')]][rz] | raw[+(!byte_('\0')>>byte_)][rs] ) 
  ));
#endif
     return ret;
  };

  std_t& std_(){ if( !_opt ){ _opt=""s; } return *_opt; }
  std_t std_()const{
    if( !_opt ){
      if( _this.size()==0 ){
          _opt=""s;
      }else{
	_opt=std::string(_this.data(),_this.size());
      }
    }
    return *_opt; 
  }
  decltype(_this)& view_()const{ if( _opt ){ _this=base_t(_opt->data(),_opt->size()); } return _this; }
#define inherit(f) void f{ std_().f; view_(); }
  inherit(clear())
#undef inherit
#define inherit(f) auto f{ return std_().f;  }
  inherit(size())
#undef inherit
    auto operator+=(const string &s){ std_()+=_string(s); return view_(); }
#define inherit(f) template<typename T> auto f(T r){ std_().f(r); return view_(); }
    inherit(operator+=)
#undef inherit
 string():_opt(),_this(){}
 string(std::nullptr_t nullp):_opt("\xff\xff\xff\xff"s),_this(*_opt){}
  bool is_null()const{ return _opt ? *_opt == "\xff\xff\xff\xff"s:_this=="\xff\xff\xff\xff"sv; }
 string(const std::string_view &s):_this(s){}
 string(const string &s):/*_this(s._this),*/_opt(s._opt){
   /**/NOTRACE( std::cerr << __PRETTY_FUNCTION__ << "(" << s._opt << "," << s._this << ")\n"; )  
   if( _opt ){
        _this=base_t(_opt->data(),_opt->size());
   }else{
     _this=s._this;
   }
 }
 string(const std::string &s){
#if 1
   NOTRACE( std::cerr << __PRETTY_FUNCTION__ << "(" <<  qtl::visible(s) << ")\n"; )
     /**/NOTRACE( std::cout << "s=" << qtl::visible(s) << " s.size()=" << s.size() <<std::endl; );
    if( !_opt ){
      _opt=""s;
    }
   _opt->reserve(s.size()+2);

  namespace x3 = boost::spirit::x3;
  using x3::_attr;
  using x3::char_;
  using x3::byte_;
  using x3::repeat;
  using x3::raw;  
  auto rf = [&](auto& ctx){ _opt = "\xff\xff\xff\xfd"; };
  auto rz = [&]( auto &ctx){ 
       int c=_attr(ctx).size();
       *_opt += '\0';
       if(  _attr(ctx).end() == s.end() ){
	  *_opt += (char)(0x7f+c);
        }else{
   	   *_opt += (char)(0x100-c);
	}
       NOTRACE( std::cout << "*opt=" << qtl::visible(*_opt) <<'\n' );
 };
  auto rs =  [&]( auto &ctx){ _opt->append(_attr(ctx).begin(),_attr(ctx).end());  };
  assert( parse(s.begin(), s.end(), 
		-( raw[ repeat(3)[byte_(0xff)] >> &(byte_(0xfd)|byte_(0xfe)|byte_(0xff)) ][rf] )
		>> *( raw[repeat(1,64)[byte_('\0')]][rz] | raw[+(!byte_('\0')>>byte_)][rs] ) 
		 ));
#endif
  /**/NOTRACE( std::cout << "*opt=" << (void*)_opt->data() << ":" << _opt->capacity() << "/" << _opt->size() << "=" << qtl::visible(*_opt) <<std::endl; );
  /**/NOTRACE( std::cout <<(void*)this << '\n'; )
       _this=std::string_view(_opt->data(),_opt->size());
       assert(_this.data()==_opt->data());
  };
  string& operator=(const string& s){
    /**/NOTRACE( std::cerr << __PRETTY_FUNCTION__ << "(" << s._opt << "," << s._this << ")\n"; )
    /**/NOTRACE( std::cerr << (void*)this << "="<< (void*)&s << "\n"; )
    _opt=s._opt;
    if( _opt ){
      _this=std::string_view(_opt->data(),_opt->size());
    }else{
       _this=s._this;
    }
    /**/NOTRACE( std::cerr << _opt << "," << _this << "\n"; )
    return *this;
  }
  std::string static inline _string(const string &s){ 
   NOTRACE( std::cerr << __PRETTY_FUNCTION__ << /* "(" << s << ")" << */ '\n'; )
    if( !s._opt ){
      s._opt= std_t(s._this.begin(),s._this.end());
    }
    NOTRACE( std::cerr << "*s._opt=" << qtl::visible(*s._opt) << '\n'; )
   return *s._opt;
  }
  std::string static inline _string(const std::string &s){ return s; }
  std::string static inline _string(const lex::eof &s){
    /**/NOTRACE( std::cerr << __PRETTY_FUNCTION__ << "(" << qtl::visible((std::string)s) << ")" << std::endl; )
   return (std::string)s;
  }
#if 0
  template<typename S, typename=std::enable_if_t<std::is_base_of_v<lex::string,S>>>
  std::string static inline _string(const S &s){ 
    /**/NOTRACE( std::cerr << __PRETTY_FUNCTION__ << "(" << (string)s << ")" << std::endl; )
    /**/NOTRACE( std::cerr << "=" << qtl::visible(_string((string)s))<< std::endl; ) 
   return _string((string)s);
 }
#else
  template<typename S, typename=std::enable_if_t<std::is_base_of_v<lex::string,S> || std::is_base_of_v<lex::string,decltype(std::declval<S>()._this)>>>
  std::string static inline _string(const S &s){ 
    /**/NOTRACE( std::cerr << __PRETTY_FUNCTION__ << "(" << (string)s << ")" << std::endl; )
    /**/NOTRACE( std::cerr << "=" << qtl::visible(_string((string)s._this))<< std::endl; ) 
    if constexpr( std::is_base_of_v<lex::string,S> ){
      return _string((string)s);
    }else{
       return _string((string)s._this);
    }
 }
#endif
  template<typename S0, typename S1, typename ...S_>
    string(const S0& s0,const S1 &s1,const S_&... s_):_opt( string::_string(s0)+(string::_string(s1)+...+string::_string(s_)) ),_this(_opt->data(),_opt->size()){ 
    /**/NOTRACE(std::cout << __PRETTY_FUNCTION__ << "(" << qtl::visible(s0) <<"," <<  qtl::visible(s1) <<  "...)" << std::endl;)   
      /**/NOTRACE(std::cout <<  qtl::visible(_string(s0)+(_string(s1)+...+_string(s_))) << std::endl; );
    /**/NOTRACE(std::cout << qtl::visible(*_opt) << std::endl; );
      assert( _opt->data()==_this.data() );
  }
#if 0
  string(const string &s, const std::string &ss):_this(s+ss){
    /**/NOTRACE(std::cout << __PRETTY_FUNCTION__ << "(" << qtl::visible(s._this) << "+" << qtl::visible(ss) << ")" << std::endl;)
  }
#endif

  //  template<int d>
  //  string(const std::string &s):string(s,eof<d>()){
  //  }

  template<typename S>
  string(const S &s,end):string(s,eof(S::depth)){
    NOTRACE(std::cout << __PRETTY_FUNCTION__ << "(" << s << "+" << qtl::visible(eof(S::depth)) << ")" << std::endl;)
    NOTRACE(std::cout << __PRETTY_FUNCTION__ << (s+eof(S::depth))  << std::endl;)
  }
  bool empty()const{
    if( _opt ){ return _opt->empty(); }
    return _this.empty();
  }
  string operator-(){
    NOTRACE( std::cerr << __PRETTY_FUNCTION__ << '(' << qtl::visible(*_opt) << ')' << '\n'; )
      if( empty()||is_null() ){ return *this; }
      assert( _opt->data()==_this.data() );
    for( auto &c:*_opt ){
#if 1
      assert(c!='\0');
      c=0x100-c;
      //assert(c!='\0');
#else
#endif
    }
    NOTRACE( std::cerr << '=' << qtl::visible(*_opt) << '\n'; )
     assert( _opt->data()==_this.data() );
    return *this;
  }
  string operator-()const{
    if( empty()||is_null() ){ return *this; }
    string ret=*this;

    return -ret;
  }
  int compare(const string &r)const{
    return view_().compare(r.view_());
  }
  bool operator<(const string &r)const{
    return compare(r)<0;
  }
  bool operator<=(const string &r)const{
    return compare(r)<=0;
  }
  bool operator==(const string &r)const{
    return compare(r)==0;
  }
  bool operator!=(const string &r)const{
    return compare(r)!=0;
  }

  template< typename it>
      class Bi_iter:public std::iterator<
      std::bidirectional_iterator_tag,
    std::remove_const_t<std::remove_reference_t< decltype(*std::declval<it>()) >>, //      decltype(*std::declval<std::remove_const_t<it>>()),
      decltype(std::declval<it>()-std::declval<it>()),
    std::remove_const_t<std::remove_reference_t<it>>*,     //std::remove_const_t<it>,
    std::remove_const_t<std::remove_reference_t<it>>& //      decltype(&*std::declval<std::remove_const_t<it>>())
     >{
  public:
        using iterator_category = std::bidirectional_iterator_tag;
        using value_type = std::remove_const_t<std::remove_reference_t< decltype(*std::declval<it>()) >>;
        using reference = std::remove_const_t<std::remove_reference_t< decltype(*std::declval<it>()) >>*;
	reference _this;
        Bi_iter(it &i):_this(const_cast<decltype(_this)>(i)){}
        Bi_iter():_this(){}
#define inherit(o) auto operator o(){ return o _this; }
	inherit(++)
	inherit(--)
        inherit(*)
#undef inherit
#define inherit(o) auto operator o(int){ return _this o; }
	inherit(++)
	inherit(--)
#undef inherit
	 
  operator decltype(_this)()const{ 
    NOTRACE( std::cerr << __PRETTY_FUNCTION__ << std::endl; )
   return _this;
 }
    operator reference(){ return _this; }
#define inherit(o) template<typename t>auto operator o(const t& r)const{  \
   return _this o (decltype(_this)) r ;\
  } \
/*end define*/
	  inherit(==)
	    inherit(!=)
	    inherit(-)
#undef inherit

  }; // end class Bi_iter<>

      //  template< typename it>
      //      Bi_iter(it &i) -> Bi_iter<it>;
  template<bool is_const_iterator = true>
    class _iterator  /*: public std::iterator<std::forward_iterator_tag,PointerType>*/{
  public:
    typedef typename std::conditional<is_const_iterator, base_t::const_iterator, base_t::iterator>::type base_iterator;
    typedef typename std::conditional<is_const_iterator, const char, char>::type base_value;
  //    base_iterator  p;
#ifdef USE_RANGES
  boost::iterator_range<base_iterator,base_iterator>r;
  static auto p=&r.first;
  static auto e=&r.second;
#else
#if 0
    std::string_view::iterator p;
    base_iterator  e;
#endif
#if 0
    base_iterator p;
    base_iterator  e;
#endif
#if 0
    std::string_view::iterator xxp;
    base_iterator  xxe;
  std::string_view::iterator &p=xxp;
  base_iterator &e=xxe;
#endif
#if 1
  std::pair<base_iterator,base_iterator>r;
  inline const base_iterator& P()const{ return r.first; }
  inline const base_iterator& E()const{ return r.second; }
  inline base_iterator& P(){ return r.first; }
  inline base_iterator& E(){ return r.second; }

  //  std::string_view::iterator &p=r.first;
  //  base_iterator &e=r.second;
#endif
#endif

  int c=0; 
  base_value& operator*(){  return *P(); }
    void operator ()(){
      /**/NOTRACE(std::cout << __PRETTY_FUNCTION__ << " " << __LINE__ << std::endl;)
#if 0
      if( p!=e && *p=='\0' ){
#elsif 0
      using boost::spirit::x3::byte_;
      using boost::spirit::x3::char_;
      using boost::spirit::char_encoding::ascii;
      if( parse(p,e, &(byte_(0)>>byte_),c) ){
      //      if( parse(p,e, &(char_(0)>>char_(0x80,0xff)),c) ){
#elsif 0
      }}
#else
      if( P()!=E() && *P()=='\0' && P()+1!=E() && (unsigned char)*(P()+1)>=0x80 ){
#endif
	c=0x100-(unsigned char)*(P()+1);
         if( c > 64 ){
	   c = 129-c;
        }
	 if( c > 64 || c<=0 ){
	   std::cerr << "c=" << c << std::endl;
         }
	 assert( c<=64 && c > 0 );
      }
    }
     _iterator &operator++(){
       /**/NOTRACE(std::cout << __PRETTY_FUNCTION__ << " " << __LINE__ << std::endl;)
      if( c>0 ){
	/**/NOTRACE(std::cout << "c=" << c << std::endl;)
	if( --c != 0 ){	 
   	  return *this;
        }
        /**/NOTRACE(std::cout << "*p=" << (int)*p << std::endl;)
        assert( P()!=E() );
        ++P();
	if( P()==E() ){
	   /**/NOTRACE( std::cout << "p==e" <<  std::endl; )
	   return *this;
         }
      }
       assert( P()!=E() );
       ++P();
      (*this)();
      return *this;
    };
    decltype(r.first) operator*=(int d){
      NOTRACE( std::cout << __PRETTY_FUNCTION__ << "(" << d << ")" << std::endl ; )
      int _r=d<0?-d/125:0;
      char _e=('\x7e'+d+125*_r);
#if 1
      namespace x3 = boost::spirit::x3;
      using x3::repeat;
      using x3::byte_;
      using x3::char_;
      using x3::no_skip;
      using x3::omit;
      using x3::raw;
      using x3::inf;
      using x3::_attr;
      //      using boost::spirit::char_encoding::ascii;
      //      if( parse(p,e, byte_(0) >> no_skip[ repeat(_r)[ byte_(1) ] >> char_('\0',_e) ] ) ){
      auto ee=byte_(0) >> repeat(_r)[ byte_(1) ] >> (char_('\0',_e) | (+byte_(1) >> -byte_) );
      NOTRACE( std::cout << _r << ": " << (int)_e << '=' << _e  << '\n'; )
	auto ret=P();
      auto rs =  [&]( auto &ctx){ 
	NOTRACE( std::cout << qtl::type_name<decltype(ctx)>() << "\n"; )
	NOTRACE(  std::cout << qtl::visible(std::string(_attr(ctx).begin(),_attr(ctx).end())) << '\n'; )
	NOTRACE( std::cout << qtl::type_name<decltype(_attr(ctx).begin())>() << "\n"; )
	ret=_attr(ctx).begin();
      };
      if( parse(P(),E(), *(omit[byte_ - ee])  >> raw[-ee][rs]) ){	
	NOTRACE( std::cout <<  qtl::visible(std::string(P(),E())) << std::endl; )
	 //	 TRACE( std::cout << qtl::type_name<decltype(ctx)>() << "\n"; )
	 //TRACE( std::cout <<  _attr(ctx).size() << std::endl; )
      }else{
	NOTRACE( std::cout << "not found\n"; )
	  NOTRACE(
      	        TRACE( std::cout <<  "_r=" << _r << " _e=" <<  '\x7e'+d+125*_r << std::endl; )
		parse(P(),E(), byte_(0) >> repeat(_r)[ byte_(1) ] );
      	        TRACE( std::cout <<  qtl::visible(std::string(p,e)) << std::endl; )
		parse(P(),E(), byte_(0) );
      	        TRACE( std::cout <<  qtl::visible(std::string(p,e)) << std::endl; )
		parse(P(),E(), repeat(_r)[ byte_(1) ] >> char_('\0',(char)('\x7e'+d+125*_r)) );
      	        TRACE( std::cout <<  qtl::visible(std::string(p,e)) << std::endl; )
		parse(P(),E(),  char_('\0',(char)('\x7e'+d+125*_r)));
      	        TRACE( std::cout <<  qtl::visible(std::string(p,e)) << std::endl; )
	   )
	  P()=E();
      }
#else
      char _x[3];
#if 0
      std::to_chars(&_x[0],&_x[3],'\x7e'+d+125*_r,16);
#else
      static char x[]="0123456789abcdef";
      _x[0]=x[('\x7e'+d+125*_r)>>4];
      _x[1]=x[('\x7e'+d+125*_r)&0xf];
      _x[2]='\0';
#endif
      std::string rs;
      /*static const*/ auto re=std::basic_regex<char>((rs="\\x00\\x01{"s+std::to_string(_r)+",}[\\x00-\\x"s+std::string(_x)+"]"s));
     NOTRACE( std::cout <<            qtl::visible(rs) << std::endl; )
      std::smatch cm;
      //TRACE( std::cout << re << std::endl; )
     if( std::regex_search(P(),E(),cm,re) ){
	NOTRACE( std::cout << cm.size() <<  " at " <<  qtl::visible(cm.prefix()) << std::endl; )
	  auto c0=cm[0];
	NOTRACE( std::cout << ":" << cm[0].second-cm[0].first<< std::endl; std::cout << "+" << qtl::visible(cm.suffix()) << std::endl; )
        P += cm.prefix().length()+cm[0].second-cm[0].first;
      }else{
	NOTRACE( std::cout << "nomatch" << std::endl; )
      }
#endif
      return ret;
    }
  

#if 0
      template<class Function, std::size_t... Indices>
      static constexpr auto make_vector_helper(Function f, std::index_sequence<Indices...>)
      -> std::vector<decltype(f(std::size_t{}))>
      {
        return {{ f(Indices)... }};
      }
      template<std::size_t N, class Function>
      static constexpr auto make_vector(Function f)
      {
        return make_vector_helper(f, std::make_index_sequence<N>{});
      }
#endif
      static inline const std::regex e1=make_skipeof_regex(-1);
      static constexpr const std::regex& skipeof(int d){
       NOTRACE( std::cerr << __PRETTY_FUNCTION__ << "(" << d << ")" << std::endl; )
        if( d==1 ){ return e1; }
	assert( d<=0 );
	return *skipeof_regex.get(-d);
      }
  //      decltype(p) operator+=(const std::regex& re){
        decltype(r.first) operator+=(const std::regex& re){
	NOTRACE( std::cout << __PRETTY_FUNCTION__ << '\n'; );
	c=0;
	std::string_view::iterator ret=P();
      //      std::smatch cm;
	//	std::match_results<Bi_iter<decltype(P())>> cm;
      //      TRACE( std::cout <<   qtl::type_name< Bi_iter<decltype(p)>::value_type  >() << std::endl; )
      //      if( std::regex_search(P(),E(),cm,re) ){
	std::string_view sv(P(),E()-P());
	#if 0
	std::match_results<Bi_iter<decltype(P())>> cm;
#else
	//	std::match_results<Bi_iter<decltype(sv.begin())>> cm;
	std::match_results<const char*> cm; 
#endif
      NOTRACE(
	      auto pp=Bi_iter<decltype(P())>(P());
	      std::cout <<  qtl::type_name<decltype<(pp)>() << std::endl;
        std::cout << "iterator_category " <<  qtl::type_name< typename std::iterator_traits<decltype(pp)>::iterator_category >() << std::endl;
        std::cout <<  "value_type " << qtl::type_name< typename std::iterator_traits<decltype(pp)>::value_type >() << std::endl;
        std::cout <<  "reference " << qtl::type_name< typename std::iterator_traits<decltype(pp)>::reference >() << std::endl;
        std::cout <<  "_this " << qtl::type_name< decltype(pp._this) >() << std::endl;
      )
	//	if( std::regex_search(Bi_iter(P()),Bi_iter(E()),cm,re) ){
	if( sv.size() && std::regex_search((const char *)(sv.begin()),(const char*)(sv.end()),cm,re) ){
			NOTRACE( std::cout << cm.size() <<  " at " <<  qtl::visible(cm.prefix()) << std::endl; )
			  //	 auto c0=cm[0];
			NOTRACE( std::cout << ":" << cm[0].second-cm[0].first<< std::endl; std::cout << "+" << qtl::visible(cm.suffix()) << std::endl; )
			  P() += cm.prefix().length()+(cm[0].second-cm[0].first);
		  ret += cm.prefix().length();
		  if( !cm[1].matched ){
		    E()=P();
                  }
      }else{
	NOTRACE( std::cout << "nomatch" << std::endl; )
	ret=P()=E();
      }
      	  NOTRACE( std::cout <<  qtl::type_name<decltype(cm.prefix().length())>() << std::endl; )
      	  NOTRACE( std::cout <<  qtl::type_name<decltype(cm[0].second)>() << std::endl; )
      	  NOTRACE( std::cout <<  qtl::type_name<decltype(cm[0].first)>() << std::endl; )
      	  NOTRACE( std::cout <<  qtl::type_name<decltype(cm[0].second-cm[0].first)>() << std::endl; )
	(*this)();
      return ret;
    }
    bool operator !=(const _iterator &i)const{
      return P()!=i.P() || c!=i.c;
    }
  }; // end class qtl::string::_iterator
  typedef  _iterator<false> iterator;
  typedef _iterator<true> const_iterator;
  template<typename _iterator = const_iterator>
    _iterator begin()const{
    const_iterator ret;
    NOTRACE( std::cout << __PRETTY_FUNCTION__ << " " << __LINE__ << std::endl ; )
    NOTRACE( std::cout << "_this=" << (void*)_this.data() << "+" << _this.size() << ":" << qtl::visible(_this) << std::endl; )
    NOTRACE( std::cout << "ret.c=" << ret.c << std::endl; )
    ret.P()=view_().begin();
    ret.E()=_this.end();
    NOTRACE( std::cout << "ret=" << ret.c<< ":" << qtl::visible(std::string(ret.P(),ret.E())) << std::endl ; )
    if( _this.size() > 1 ){
#if 0
     if( *(ret.p)=='\xff' && *(ret.p+1)=='\xff' && *(ret.p+2)=='\xff' && *(ret.p+3)>'\xfc' ){
	ret.p += 2;
#else
      using boost::spirit::x3::repeat;
      using boost::spirit::x3::byte_;
      using boost::spirit::char_encoding::ascii;
      NOTRACE( std::cout << "ret=" << ret.c<< ":" << qtl::visible(std::string(ret.p,ret.e)) << std::endl ; )
	if( parse(ret.P(),ret.E(), repeat(2)[ byte_(0xff)] >> &(byte_(0xff) >> (byte_(0xfd)|byte_(0xfe)|byte_(0xff))) ) ){
	NOTRACE(  std::cout << __PRETTY_FUNCTION__ << std::endl ; )
	  NOTRACE( std::cout << qtl::visible(std::string(_this.begin(),_this.end())) << std::endl; )
	  NOTRACE( std::cout << qtl::visible(std::string(ret.p,ret.e)) << std::endl; )
#endif
        ret.c=3;
	//ret.p=_this.begin()+2;
      }else{
	NOTRACE( std::cout << "ret=" << ret.c<< ":" << qtl::visible(std::string(ret.p,ret.e)) << std::endl ; )
        ret();
	NOTRACE( std::cout << "ret=" << ret.c<< ":" << qtl::visible(std::string(ret.p,ret.e)) << std::endl ; )
#if 0
  }
#else
  }
#endif
    }
    return ret;
  };
  template<typename _iterator = const_iterator>
    _iterator begin(const std::regex& re)const{
    const_iterator ret;
    NOTRACE( std::cout << __PRETTY_FUNCTION__ << std::endl ; )
    ret.P()=view_().begin();
    ret.E()=_this.end();
    ret.c=0;
    return ret;
  };
  template<typename _iterator = const_iterator>
  _iterator end()const{
    const_iterator ret;
    ret.P()=ret.E()=_this.end();
    ret.c=0;
    return ret;
  }
  operator std::string() const {
    /**/NOTRACE( std::cerr << __PRETTY_FUNCTION__ << " " << __LINE__ << std::endl;)
    std::string ret;
    for( const auto& i:/*std::as_const*/(*this) ){
      /**/NOTRACE( std::cout << visible(ret) << " += " << (int)i << std::endl;)
	ret += (char)i;
    }
    return ret;
  }
  using lex_t=string;
}; // end class lex::string 
class raw:public string{
  using base_t=string;
  using base_t::base_t;
 public:
  raw( const std::string &s="" ){
    _opt=s;
    _this=std::string_view(_opt->data(),_opt->size());
  }
  operator std::string(){
    return *_opt;
  }
}; // end class lex::raw

std::ostream& operator<<(std::ostream& os, const string &s){
  qtl::ios_base::fmtflags f;
  os >> f;  
  std::string t=qtl::type_name<string>(f.verbosity);
  if( !t.empty() ){ os << t << "{"; }
  NOTRACE( std::cerr << "has_value:" << s._opt.has_value() << "\n"; )
  s.view_();
  if( f.verbosity&qtl::ios_base::fmtflags::show_address ){
    os << "/*" << (void*) s._this.data() << "*/";
  }
  os << qtl::visible((std::string)s) ;
  if( t.empty() ){ os << "_s"; }
  if( f.verbosity&qtl::ios_base::fmtflags::show_cache ){
    using  namespace qtl::literals;
    if( s._opt ){
      os << ", std::optional{"_r << (void*)s._opt->data() << ": " << qtl::visible(*(s._opt)) << "}"_r;
    }else{
      os << ",std::optional<"_r << qtl::type_name<decltype(*s._opt)>(f.verbosity) << ">{}"_r;
    }
  }
  if( !t.empty() ){
    os << "}";
  }
  return os;
}
qtl::ostringstream& operator<<(qtl::ostringstream& os, const string &s){
  static_cast<qtl::ostringstream::base_t&>(os) << s ;
  return os;
}
  template<bool isconst>
  std::ostream& operator<<(std::ostream& os, const string::_iterator<isconst> &i){
    os << qtl::type<decltype(i)>() << "{" << (void*)&*i.P() << ":" << qtl::visible(std::string(i.P(),i.E())) << "}";
  return os;
}
string operator+(const string &a,const string &b){
  /**/NOTRACE( std::cout << __PRETTY_FUNCTION__ << "(" << a << ", " << b << ")" << std::endl; )
    return (raw)(string::_string(a)+string::_string(b));
}
namespace literals{
  string operator""_s(const char *c,std::size_t s){
    return string(std::string(c,s));
  }
}; // end namespace lex::literals
}; // end namespace lex

#if __INCLUDE_LEVEL__ + !defined __INCLUDE_LEVEL__ < 1+defined TEST_H
#include <vector>
  void test(){
#if 0
using namespace std::string_literals;  
    std::string s="xyz\0\1def\0\1\1ghi\0\1\1\1jkl"s;
      int _r=2;
      char _e='j';
      	        NOTRACE( std::cout <<  "_r=" << _r << " _e=" <<  _e << std::endl; )

    {
    	TRACE( std::cout << "boost::spirit::parse\n"; )

    auto p=s.begin();
    auto e=s.end();
      using boost::spirit::x3::repeat;
      using boost::spirit::x3::byte_;
      using boost::spirit::x3::char_;
      using boost::spirit::x3::no_skip;
      using boost::spirit::x3::inf;
      using boost::spirit::x3::raw;
      using boost::spirit::x3::omit;
      //      using boost::spirit::char_encoding::ascii;
      auto r1=repeat(_r,inf)[ byte_(1)] >> char_('\0',_e);
      auto z=byte_(0) >> r1;
      std::cout << qtl::type_name<decltype(r1)>() << "\n";
      parse(p,e, r1 ); NOTRACE( std::cout <<  qtl::visible(std::string(p,e)) << std::endl; )
      auto r001= *~char_(0) >>  char_(0) >> !r1 ;
      std::cout << qtl::type_name<decltype(r001)>() << "\n";
      parse(p,e, r001 ); NOTRACE( std::cout <<  qtl::visible(std::string(p,e)) << std::endl; )
      auto r0=repeat[ r001 ];
      std::cout << qtl::type_name<decltype(r0)>() << "\n";
      auto k0=*omit[r001];
      std::cout << qtl::type_name<decltype(k0)>() << "\n";
      parse(p,e, raw[ k0 ] ); NOTRACE( std::cout <<  qtl::visible(std::string(p,e)) << std::endl; )
      auto pp= omit[*(byte_ - z)] >> raw[ z ];
      std::cout << qtl::type_name<decltype(pp)>() << "\n";
      p=s.begin();
      e=s.end();
      if(  parse(p,e, pp ) ){
	TRACE( std::cout <<  qtl::visible(std::string(p,e)) << std::endl; )
	  }else{
	p=s.end();
	e=s.end();
    	TRACE( std::cout << "not found\n"; )
         NOTRACE(
		parse(p,e, *~char_(0) >> byte_(0) >> repeat(_r,inf)[ byte_(1) ] );
      	        TRACE( std::cout <<  qtl::visible(std::string(p,e)) << std::endl; )
		parse(p,e, ~char_ >> byte_(0) );
      	        TRACE( std::cout <<  qtl::visible(std::string(p,e)) << std::endl; )
		parse(p,e, *char_ >> repeat(_r,inf)[ byte_(1) ] >> char_('\0',_e) );
      	        TRACE( std::cout <<  qtl::visible(std::string(p,e)) << std::endl; )
		parse(p,e,  *char_ >> char_('\0',(char)(_e)));
      	        TRACE( std::cout <<  qtl::visible(std::string(p,e)) << std::endl; )
		parse(p,e,  *char_ >> repeat(_r,inf)[ byte_(1) ] );
      	        TRACE( std::cout <<  qtl::visible(std::string(p,e)) << std::endl; )
		parse(p,e,  *~char_(0) >> byte_(0) );
      	        TRACE( std::cout <<  qtl::visible(std::string(p,e)) << std::endl; )
		parse(p,e,  *char_ );
      	        TRACE( std::cout <<  qtl::visible(std::string(p,e)) << std::endl; )
	   )
  }
    }
    if(0){
    	TRACE( std::cout << "std::regex_search\n"; )
    auto p=s.cbegin();
    auto e=s.cend();
    std::string rs;
    //     TRACE( std::cout <<            qtl::visible("\\x00\\x01{"s+std::to_string(_r)+"}[\\x00-\\x6a]"s) << std::endl; )
       static const auto re=std::basic_regex<char>((rs="\\x00\\x01{"s+std::to_string(_r)+"}[\\x01-\\x6a]"s));
      std::smatch cm;
      NOTRACE( std::cout << qtl::visible(rs) << std::endl; )
      if( std::regex_search(p,e,cm,re) ){
	NOTRACE( std::cout << cm.size() <<  " at " <<  qtl::visible(cm.prefix()) << std::endl; )
	  auto c0=cm[0];
	NOTRACE( std::cout << ":" << cm[0].second-cm[0].first<< std::endl; std::cout << "+" << qtl::visible(cm.suffix()) << std::endl; )
        p += cm.prefix().length()+cm[0].second-cm[0].first;
      }else{
	NOTRACE( std::cout << "nomatch" << std::endl; )
	  }
    }
#endif
  }
  static inline int signum(int x){ return std::clamp(x,-1,1); }
int main( int argc, char *argv[] ){
if( argc==1 || argv[1][0] == '<' ){
  std::vector<std::string> i0;
  std::vector<std::array<int,2>> t;
  std::map<int,int>count;
  std::cout << argv[0] << '\n';
  while( std::cin.good() ){
     std::string i;
    std::getline(std::cin, i);
    NOTRACE( std::cout << qtl::visible(i) << std::endl; )
    lex::string q=i;
    NOTRACE( std::cout << "q=" << qtl::visible(q._this) << std::endl; )
    std::string s=q;
    NOTRACE( std::cout << "s=" << qtl::visible(s) << std::endl; )
    NOTRACE( std::cout << qtl::visible(s) << " = " << qtl::visible(q._this) << std::endl; )
    if( s!=i ){
      std::cout << qtl::visible(s) << " != " << qtl::visible(i) << std::endl;
      q=i;
      s=q;
    }
    assert( s==i );
    // std::cout << std::endl;
    std::array<int,2> a={-1,0};
    if( !i0.empty() ){
      lex::string q0=i0.back();
      assert( signum(i0.back().compare(i)) == signum(q0.compare(q)) );
      int b=i0.size()-1;
      while( b>=0 && i0[b].compare(i) > 0 ){
        //a={b,t[b][1]-1};
	if( a[0] < 0 ){ a[0]=b; }
	--a[1];
	b=t[b][0];
      }
    }
    i0.push_back(i);
    t.push_back(a);
    if( !count.count(a[1]) ){
      count[a[1]]=0;
    }
    ++count[a[1]];
  }
  --count[t.back()[1]];
  NOTRACE( std::cerr << t << std::endl; )
  //  exit(1);
  {
    lex::string s;
  for( int i=0;i<i0.size(); ++i ){
    NOTRACE({ auto tmp=eof(t[i][1]); std::cout << qtl::visible(tmp) << '\n'; })
      lex::string q=i0[i];
    //    TRACE( std::cout << "q="<< q << '\n' );
    NOTRACE( std::cout << "lex::string(i0["<<i<<"])="<< lex::string(i0[i]) << '\n' );
    s+=lex::string(q,lex::eof(t[i][1]));
    NOTRACE(std::cout << s << std::endl; )
  }
  NOTRACE( std::cout << s << '\n'; )
    for( int d=0;count.count(d); --d ){
      NOTRACE( std::cout << d << ":" << count[d] << std::endl; )
      auto p=s.begin();
      auto e=s.begin();
      auto e0=(e+=lex::string::iterator::skipeof(d));
      int n=0;
          NOTRACE(
	  auto se=s.end();
	  NOTRACE( std::cout << "se=" << (void*) se.p << "," << (void*) se.e << '\n'; )
	  )
      while( p!=s.end() ){
         NOTRACE( std::cout << "p="      << (void*) p.      p << "," << p.      c << "," << (void*) p.e << '\n'; )
         NOTRACE( std::cout << "e="      << (void*) e.      p << "," << e.      c << "," << (void*) e.e << '\n'; )
         NOTRACE( std::cout << "s.end()" << (void*) s.end().p << "," << s.end().c << "," << (void*) s.end().e << '\n'; )
	   NOTRACE( std::cout << qtl::visible(std::string_view(p.P(),e0-p.P())) << ":" << qtl::visible(std::string_view(e0,e.P()-e0)); )
	   if( e.P()==e.E() ){
	     NOTRACE( std::cout << "+" << qtl::visible(std::string_view(e.P(),p.E()-e.P())); )
	     e.E()=s.end().E();
         }else{
	   ++n;
         }
         NOTRACE( std::cout << '\n'; )
            p=e;
         e0=(e+=lex::string::iterator::skipeof(d));
      }
      NOTRACE( std::cout << n << "/" << count[d] << std::endl; )
      assert(n==count[d]);
    }
  }
  exit(1);
}
  using namespace std::string_literals;
  std::vector<std::string> t={
    ""s,
    "\0"s,
    "\0\0"s,
    "\0\0\0"s,
    "\0\0\0\0"s,
    "123"s,
//    "abc"s,
    "\x66\x99\xcc\xff\x89\xab\xcd\xef"s,
    "\x66\x99\xcc\xff\x01\x23\x45\x67\x89\xab\xcd\xef"s,
    "\xcc"s,
    "\xff"s, 
    "\xff\0"s,
    //
    "\xff\0\0"s,
    "\xff\0."s,
    "\xff."s,
    "\xff\xff"s,
    "\xff\xff\0"s
    "\xff\xff\0."s,
//    "\xff\xff."s,
    "\xff\xff\xff"s,
    "\xff\xff\xff\xfc"s,
//    "\xff\xff\xff\xfc."s,
    "\xff\xff\xff\xfd"s,
//    "\xff\xff\xff\xfd."s,
    "\xff\xff\xff\xfe"s,
//    "\xff\xff\xff\xfe."s,
    "\xff\xff\xff\xff"s,
    "\xff\xff\xff\xff"s+std::string(1,'\0'),
    "\xff\xff\xff\xff"s+std::string(63,'\0'),
    "\xff\xff\xff\xff"s+std::string(64,'\0'),
    "\xff\xff\xff\xff"s+std::string(65,'\0'),
//    "\xff\xff\xff\xff"s+std::string(66,'\0'),
    "\xff\xff\xff\xff"s+std::string(66,'\0')+'.',
    "\xff\xff\xff\xff"s+std::string(65,'\0')+'.',
//    "\xff\xff\xff\xff"s+std::string(64,'\0')+'.',
//    "\xff\xff\xff\xff"s+std::string(63,'\0')+"..",
//    "\xff\xff\xff\xff"s+std::string(1,'\0')+"..",
  };
  for( auto i : (t) ){
    std::cout <<  i << "\n";
  }
exit(0);
  std::cout << lex::string("abc\0"s) << '\n';
  std::cout << lex::string("abc\0\0"s) << '\n';
  std::cout << lex::string("abc\0\0\0"s) << '\n';
  std::cout << lex::string("abc\0\0\1"s) << '\n';
  std::cout << lex::string("abc\0\0\2"s) << '\n';
  std::cout << lex::string("abc\0\1"s) << '\n';
std::cout << lex::string("abc\0\1\0"s) << '\n';
  std::cout << lex::string("abc\0\1\1"s) << '\n';
  std::cout << lex::string("abc\1"s) << '\n';
  std::cout << lex::string("abc\1\0"s) << '\n';
  std::cout << lex::string("abc\1\1"s) << '\n';
  qtl::cout << lex::string::_eof<0>() << '\n'<<std::flush;
  qtl::cout << lex::string::_eof<-2>() << '\n'<<std::flush;
  //  std::cout << lex::string::eof<-2> << '\n'<<std::flush;
    qtl::cout << lex::string::_eof<-9>() << '\n'<<std::flush;
  //  std::cout << lex::string::eof<-9> << '\n'<<std::flush;
    std::cout << qtl::visible(lex::eof(1)) << '\n'<<std::flush;
    std::cout << qtl::visible(lex::eof(0)) << '\n'<<std::flush;
    std::cout << qtl::visible(lex::eof(-1)) << '\n'<<std::flush;
    std::cout << qtl::visible(lex::eof(-2)) << '\n'<<std::flush;
    std::cout << qtl::visible(lex::eof(-100)) << '\n'<<std::flush;
    std::cout << qtl::visible(lex::eof(-124)) << '\n'<<std::flush;
    std::cout << qtl::visible(lex::eof(-125)) << '\n'<<std::flush;
    std::cout << qtl::visible(lex::eof(-126)) << '\n'<<std::flush;
    std::cout << qtl::visible(lex::eof(-200)) << '\n'<<std::flush;
    std::cout << qtl::visible(lex::eof(-249)) << '\n'<<std::flush;
    std::cout << qtl::visible(lex::eof(-250)) << '\n'<<std::flush;
    std::cout << qtl::visible(lex::eof(-251)) << '\n'<<std::flush;
    std::cout << qtl::visible(lex::eof(-300)) << '\n'<<std::flush;
    std::cout << qtl::visible(lex::eof(-400)) << '\n'<<std::flush;
    std::cout << qtl::visible(lex::eof(-499)) << '\n'<<std::flush;
    std::cout << qtl::visible(lex::eof(-500)) << '\n'<<std::flush;
    std::cout << qtl::visible(lex::eof(-501)) << '\n'<<std::flush;
    //    std::cout << qtl::visible((std::string)lex::eof(0)+lex::eof(-2)+lex::eof(-125)) << '\n'<<std::flush;
    auto e=lex::string("aa"s,lex::eof(0),"bb"s,lex::eof(-2),"ccc"s,lex::eof(-4),"dddd"s,lex::eof(-125),"eeeee"s,lex::eof(-300),"ffffff"s);
  //  std::cout << __LINE__ << '\n';
  std::cout << e <<  '\n';
  //  std::cout << __LINE__ << '\n';
  for( auto i:{1,0,-1,-2,-3,-4,-5,-124,-125,-126,-299,-300,-301,-500} ){
    auto p=e.begin();
    auto x= p+=lex::string::iterator::skipeof(i);
    // std::cout << i << ": " << (void*)&*x <<":" <<  p << "\n";  // ./qtl/string.h:827:8: error: invalid operands to binary expression ('std::ostream' (aka 'basic_ostream<char>') and 'qtl::type<decltype(i)>' (aka 'type<const _iterator<true> &>'))

  }
  //  std::cout << lex::string::eof<2> << '\n';
    //    std::cout << lex::string("abc"s,2) << '\n';
    std::cout << lex::string("a\0bc"s) << '\n';
    std::cout << lex::string(lex::string("a\0bc"s)) << '\n';
    std::cout << lex::string(lex::string("a\0bc"s),lex::eof(0)) << '\n';
  //  test();

}
#endif

