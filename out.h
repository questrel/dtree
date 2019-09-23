#pragma once
#include <string>
#include <vector>
#include <map>
#include <queue>
#include <unordered_map>
#include <utility> // pair
#include <set>
#include <unordered_set>
#include <iostream>
#include <sstream>
#include <type_traits>
//#include <boost/core/demangle.hpp>
//#include <boost/type_index.hpp>
#include <boost/io/ios_state.hpp>
#include <boost/tti/has_member_function.hpp>

#define BOOST_STACKTRACE_GNU_SOURCE_NOT_REQUIRED
#include <boost/stacktrace.hpp>

//#include <boost/regex.hpp>
#include <regex>
//#ifndef __clang__
#include <optional>
//#define OPTIONAL optional
//#else
//#include <experimental/optional>
//#define OPTIONAL experimental::optional
//#endif
//#include  <experimental/type_traits>
#if defined(__GNUC__)
#define DEPRECATE(foo, msg) foo __attribute__((deprecated(msg)))
#define PP_CAT(x,y) PP_CAT1(x,y)
#define PP_CAT1(x,y) x##y

namespace detail{
  struct true_type {};
  struct false_type {};
  template <int test> struct converter : public true_type {};
  template <> struct converter<0> : public false_type {};
}; // end namespace detail

#define STATIC_WARNING(cond, msg) \
  struct PP_CAT(static_warning,__LINE__) { \
    DEPRECATE(void _(::detail::false_type const& ),msg) {}; \
    void _(::detail::true_type const& ) {}; \
    PP_CAT(static_warning,__LINE__)() {_(::detail::converter<(cond)>());} \
  };
#endif

#include "fileno.h"
#ifndef TRACE
#ifndef FUZZING 
#define TRACE(x) x
#else
#define TRACE(x)
#endif
#endif
#define NOTRACE(x) 
namespace qtl{
  template<typename S> struct rstr{};
  template<>
  struct rstr<std::string>:public std::string{
    using  base_t=std::string;
    using base_t::base_t;
    rstr(const base_t& s):base_t(s){}
  };
  template<>
  struct rstr<char const *>{
        using  base_t=char const *;
	base_t this_t;
        constexpr rstr(base_t s):this_t(s){};
	operator char const *()const{ return this_t; }
  };
 //  rstr(char *) -> rstr<char *>;
  rstr(std::string) -> rstr<std::string>;
  namespace literals{
    constexpr rstr<char const*>operator"" _r(char const*s,std::size_t){ return rstr<char const*>(s); }
  }
  using namespace literals;
  namespace ios_base{
  class fmtflags{
  public:
    //    enum class type{none,generic,id,strip,demangle,remove_const_ref}verbosity=type::demangle;
    //    enum class type{none=0,generic=1,id=2,demangle=3,namefield=3,keep_defaults=1<<2,keep_const_ref=1<<3,nullopt=1<<4,number_suffix=1<<5,string_len=1<<6,all=(1<<7)-1}verbosity=type::demangle;
    using type=int;
    static inline constexpr type none=0;
    static inline constexpr type generic=1;
    static inline constexpr type id=2;
    static inline constexpr type demangle=3;
    static inline constexpr type namefield=3;
    static inline constexpr type show_address=1<<2;
    static inline constexpr type show_cache=1<<3;
    static inline constexpr type keep_defaults=1<<4;
    static inline constexpr type keep_const_ref=1<<5;
    static inline constexpr type nullopt=1<<6;
    static inline constexpr type number_suffix=1<<7;
    static inline constexpr type string_len=1<<8;
    static inline constexpr type alltypeinfo=(1<<9)-1;
    static inline constexpr type intervalnotation=(1<<9);
    static inline constexpr type setbuilder=(0<<9);
    static inline constexpr type boundary=(1<<9);
    type verbosity = /*demangle|show_address|show_cache*/none;
    rstr<char const *> fs=", "_r; 
    rstr<char const *> rs="\n"_r; 
    int width=132;
    int height=66;
    int indent=0;
    fmtflags(){
      NOTRACE( std::cout << __PRETTY_FUNCTION__ <<  (void*) this << '\n'; )
    }
    fmtflags( std::ostream &os ){
      std::tie(width,height)=size_w_h(os);
      NOTRACE( std::cout << __PRETTY_FUNCTION__ <<  (void*) this << '\n'; )
    }
    ~fmtflags(){
      NOTRACE( std::cout << __PRETTY_FUNCTION__ <<  (void*) this << '\n'; )
    }
  };
  class fmtstate{
    int indent;
    int row;
    int col;
    std::map<std::string,std::string> seen;
  };
  static std::vector<std::pair<fmtflags,fmtstate>> fmtstack;
  const auto xfmt = std::ios_base::xalloc();  
  const auto xfmtstack = std::ios_base::xalloc();  
  }
  class setverbose { 
  public:
    ios_base::fmtflags::type v=ios_base::fmtflags::demangle;
    setverbose(ios_base::fmtflags::type v):v(v){}
  };
#define _PRETTY_FUNCTION_STRIP_  qtl::strip_default_std(std::string(__PRETTY_FUNCTION__))
static inline std::string strip_default_std(const std::string &S){
  NOTRACE( std::cout << __PRETTY_FUNCTION__ << "(" << S << ")" << '\n'<<std::flush; )
    static const auto re={
      std::basic_regex<char>(std::string("(:\\w+<(.+?)(, .+?)?), std:[\\w:]*:(less|hash|equal_to|allocator)<\\2\\s*>\\s*")),
      //      std::basic_regex<char>(std::string("(:\\w+<(.+?)), std:[\\w:]*:allocator<\\2\\s*>\\s*")),
      std::basic_regex<char>(std::string("(:map<(.+?), (.+?)), std:[\\w:]*:allocator<std:[\\w:]*:pair<\\2(?:\\s*const)?, \\3\\s*>\\s*>\\s*")),
    };
    std::string s=S;
    for( auto re:re ){
      for( std::string r; (r=std::regex_replace(s,re,"$1")).size()<s.size(); s=r ){
	     NOTRACE( std::cout << r << '\n'<<std::flush; )
      }
      NOTRACE( std::cout << s << '\n'<<std::flush; )
    }
    return s;
  }
 static inline std::string strip_const_ref(const std::string &S){
  NOTRACE( std::cout << __PRETTY_FUNCTION__ << "(" << S << ")" << '\n'<<std::flush; )
    static const auto re=std::basic_regex<char>(std::string("\\s+const&$"));
    return std::regex_replace(S,re,"");
}
static inline std::string strip_template_args(const std::string &S){
  NOTRACE( std::cout << __PRETTY_FUNCTION__ << "(" << S << ")" << '\n'<<std::flush; )
    static const auto re={
     std::basic_regex<char>(std::string("(?:^std:[\\w:]*:)?(\\w+<)(.*)(?=>)")),
    };
    std::string s=S;
    for( auto re:re ){
      for( std::string r; (r=std::regex_replace(s,re,"$1")).size()<s.size(); s=r ){
	     NOTRACE( std::cout << r << '\n'<<std::flush; )
      }
      NOTRACE( std::cout << s << '\n'<<std::flush; )
    }
    return s;
  }
#if 1
  // https://en.cppreference.com/w/cpp/named_req/LiteralType
class conststr
{
    const char* p;
    std::size_t sz;
    constexpr conststr(const char *p,std::size_t sz) : p(p), sz(sz) {}
public:
    template<std::size_t N>
    constexpr conststr(const char(&a)[N]) : p(a), sz(N - 1) {}
    constexpr char operator[](std::size_t n) const
    {
      return n < sz ? p[n] : throw std::out_of_range("");
    }
    constexpr conststr behead(int n=1)const{ return conststr(p+n,size()-n); }
    constexpr conststr betail(int n=1)const{ return conststr(p,size()-n); }
    constexpr std::size_t size() const { return sz; }
    constexpr operator std::string_view()const{ return std::string_view(p,sz); }
    constexpr const char* data()const{ return p; }
  };
#endif
constexpr conststr first(conststr s, char c='['){
  return (s.size()==0||s[0]==c) ? s : first(s.behead(),c);
}

constexpr conststr last(conststr s, char c=']'){
  return (s.size()==0||s[s.size()-1]==c) ? s : last(s.betail(),c);
}

 template<class C>
 constexpr auto pretty_function(const C& c){ return __PRETTY_FUNCTION__; }

 template<class C>
 constexpr auto pretty_class(){
   return last(first(__PRETTY_FUNCTION__,'[').behead(5),']').betail(1);
 }
 
template<class C> 
rstr<std::string> type_name(ios_base::fmtflags::type verbosity=ios_base::fmtflags::demangle,std::string generic=""){
    using type=ios_base::fmtflags;
    if( verbosity&type::namefield ){
#if 0
      std::string s=std::string(typeid(C).name());
      if( (verbosity&type::namefield)==type::id ){
	return s;
      }
      #if 1
        s=boost::core::demangle(s.c_str());
      #else
        s=boost::typeindex::type_id_with_cvr<C>().pretty_name();
      #endif
#else
	std::string_view sv=pretty_class<C>();
	std::string s(sv.begin(),sv.size());
#endif
      if( (verbosity&type::namefield)==type::generic ){
	return strip_template_args(s);
      }
      if( (verbosity&type::keep_const_ref)==0 ){
	//	s=strip_const_ref(s);
      };
      if( (verbosity&type::keep_defaults)==0 ){
         s=strip_default_std(s);
      };
      return s;
    }
    return rstr<std::string>();
 }

 static rstr<std::string> visible(const char &c,std::ios_base::fmtflags ff=(std::ios_base::fmtflags)0){
    std::string ret;
    ret += '\'';
    int x=0;
    auto d=[&](char c){
      static char d[]={"0123456789abcdef"};
      if( x ){
	x = ~x & 0xff;
	if( x&'\300' || (ff&std::cout.basefield)==std::ios_base::hex ){
	  ret += 'x';
          ret += d[x>>4];
	  ret += d[x&0xf];
	}else{
	  if( x>077 ){
	    ret += d[x>>6];
	  };
	  if( x>=07 ){
	    ret += d[(x>>3)&7];
	  }
	  ret += d[x&7];
	}
      }else{
      }
    };
    if( c<' ' || c > '~' || c == '\'' || c=='\\' || (ff&std::ios_base::basefield)==std::ios_base::hex ){
	ret += '\\';
	if( c<' ' || c > '~' || (ff&std::ios_base::basefield)==std::ios_base::hex ){
	  x=~(c&0xff);
        }else{
	  ret += c;
        }
    }else{
	  ret += c;
    }
    d('\'');
    ret += '\'';
    return ret;
 }
 static rstr<std::string> visible(const std::string &s,std::ios_base::fmtflags ff=(std::ios_base::fmtflags)0){
    std::string ret;
    ret += '"';
    int x=0;
    auto d=[&](char c){
      static char d[]={"0123456789abcdef"};
      if( x ){
	x = ~x & 0xff;
	if( x&'\300' || (ff&std::cout.basefield)==std::ios_base::hex ){
	  ret += 'x';
          ret += d[x>>4];
	  ret += d[x&0xf];
	}else{
	  bool b= '0' <= c && c <= '9';
	  if( b || x>077 ){
	    ret += d[x>>6];
	  };
	  if( b || x>=07 ){
	    ret += d[(x>>3)&7];
	  }
	  ret += d[x&7];
      }
      }
      x=0;
    };
    for( auto c:s ){
      d(c);
      if( c<' ' || c > '~' || c == '"' || c=='\\' || (ff&std::ios_base::basefield)==std::ios_base::hex ){
	ret += '\\';
	if( c<' ' || c > '~' || (ff&std::ios_base::basefield)==std::ios_base::hex ){
	  x=~(c&0xff);
	  continue;
	}
      }
     ret += c;
    }
   d('"');
   ret += '"';
   return ret;
  }
 static rstr<std::string> visible(const std::string_view &s,std::ios_base::fmtflags ff=(std::ios_base::fmtflags)0){
   NOTRACE( std::cout << __PRETTY_FUNCTION__ << std::endl; );
   NOTRACE( std::cout << (void*)s.data() << " + " << s.size() << std::endl; )
  std::string st(s.begin(),s.end());
   return visible(st);
 }
 static rstr<std::string> visible(char const *s,std::ios_base::fmtflags ff=(std::ios_base::fmtflags)0){
    return s?visible(std::string(s),ff):std::string("NULL");
  }

  class _indent{
  public:
    int i;
  };
  inline _indent setindent(int i=0){ return {i}; }
  
  class manip{
    public:
    int indent;
    int x=0;
    int y=0;
    char const* sep0/*=", "*/;
    char const* sep1/*="\n"*/;
    manip(int indent=0,char const* sep0=", ", char const* sep1="\n"):sep0(sep0),sep1(sep1),indent(indent){};
  };

  inline static std::vector<manip>fmt={manip{}};
  class _push:public manip{
  public:
  _push(int i=0):manip(i){};
  _push(manip f):manip(f){};
  };
  _push push(int i=0){ return manip(i); }

  class _pop{
  };
  _pop pop(){ return {}; }
  //  template<typename T, typename=std::enable_if_t<std::is_same_v<T,std::ostream>>>class ostream:public T{
  class ostringstream:public std::ostringstream{
    public:
    typedef std::ostringstream base_t;
    ostringstream(){
      NOTRACE( std::cout << __PRETTY_FUNCTION__ << (void*) this << '\n'; )
    }
    // using base_t:base_t;
    ~ostringstream(){
      NOTRACE( std::cout << __PRETTY_FUNCTION__ << (void*) this << '\n'; )
      if( auto p=this->pword(qtl::ios_base::xfmt);p ){
	    NOTRACE( std::cout << "delete(" << (void*)p << ")\n"; )
	    delete (qtl::ios_base::fmtflags*) p;
	    this->pword(qtl::ios_base::xfmt) = 0;
      };
    }
  };
  
}//end namesspace qtl

//static out s;
qtl::ostringstream& operator<<(qtl::ostringstream& os,const std::string&s){
  using namespace qtl;
  static_cast<qtl::ostringstream::base_t&>(os) << qtl::visible(s) << "s"_r;
    return os;
}
#if 1
qtl::ostringstream& operator<<(qtl::ostringstream& os,const std::string_view&s){
  using namespace qtl;
  //  qtl::ios_base::fmtflags f;
  //  os >> f;
  //  if( f.verbosity > qtl::ios_base::fmtflags::id ){
  os << "std::string_view{"_r;
      //  }
      //  if( f.verbosity&qtl::ios_base::fmtflags::show_address ){
  os << (void*) s.data() << ": "_r;;
    //  }
  static_cast<qtl::ostringstream::base_t&>(os) << qtl::visible(s) << "s"_r;
  //  if( f.verbosity > qtl::ios_base::fmtflags::id ){
  os << "}"_r;;
    //  }
    return os;
}
#endif
qtl::ostringstream& operator<<(qtl::ostringstream& os,char c){
  static_cast<qtl::ostringstream::base_t&>(os) << qtl::visible(c);
  return os;
}
qtl::ostringstream& operator<<(qtl::ostringstream& os,unsigned char c){
  static_cast<qtl::ostringstream::base_t&>(os) << (int)(c);
  return os;
}
qtl::ostringstream& operator<<(qtl::ostringstream& os,char const *s){
  static_cast<qtl::ostringstream::base_t&>(os) << qtl::visible(s);
  return os;
}
#if 1
#if 0
template<typename T> // clang++: error: call to function 'operator<<' that is neither visible in the template definition nor found by argument-dependent lookup
qtl::ostringstream& operator<<(qtl::ostringstream& os,T i){ 
  static_cast<qtl::ostringstream::base_t&>(os) << i;
  return os;
}
#else
qtl::ostringstream& operator<<(qtl::ostringstream& os,int i){
  static_cast<qtl::ostringstream::base_t&>(os) << i;
  return os;
}
qtl::ostringstream& operator<<(qtl::ostringstream& os,long i){
  static_cast<qtl::ostringstream::base_t&>(os) << i;
  return os;
}
qtl::ostringstream& operator<<(qtl::ostringstream& os,long long i){
  static_cast<qtl::ostringstream::base_t&>(os) << i;
  return os;
}
#endif
#else
//  error: use of overloaded operator '<<' is ambiguous (with operand types 'qtl::ostringstream' and 'int')
#endif

#if 1
template<typename S>
qtl::ostringstream& operator<<(qtl::ostringstream& os,const qtl::rstr<S>s){
  static_cast<qtl::ostringstream::base_t&>(os) << static_cast<S>(s);
  return os;
}
#endif
#if 1
template<typename S>
std::ostream& operator<<(std::ostream& os,const qtl::rstr<S>s){
  os << static_cast<S>(s);
  return os;
}
#endif
std::ostream& operator<<(std::ostream& os,const qtl::setverbose &v){
  auto p=os.pword(qtl::ios_base::xfmt); 
 NOTRACE( std::cout << __PRETTY_FUNCTION__ << " p=" << p << '\n'<<std::flush; )
  if( !p ){
    p = os.pword(qtl::ios_base::xfmt) = new qtl::ios_base::fmtflags(os);
    NOTRACE( std::cout << __PRETTY_FUNCTION__ << (void*) &os << ".pword=" << p << '\n'<<std::flush; )
      os.register_callback([](std::ios::event ev, std::ios_base& stream, int index){
	  NOTRACE( std::cout << __PRETTY_FUNCTION__ << '\n'; )
	    switch (ev){
	    case std::ios_base::erase_event:{
              if( auto p= stream.pword(qtl::ios_base::xfmt);p ){
 	       NOTRACE( std::cout << "delete(" << (void*)p << ")\n"; )
	       delete (qtl::ios_base::fmtflags*) p;
	       stream.pword(qtl::ios_base::xfmt) = 0;
             };break;
	    default:{ /* 'imbue_event' and 'copyfmt_event' */ };break;
            };break;
	    }
	},0);
  }
 ((qtl::ios_base::fmtflags*)p)->verbosity=v.v;
   return os;
  }
  std::ostream& operator>>(std::ostream& os,qtl::ios_base::fmtflags &f){
      auto p=os.pword(qtl::ios_base::xfmt); 
     if( !p ){
#if 0
       p = os.pword(qtl::ios_base::xfmt) = new qtl::ios_base::fmtflags(os);
       TRACE( std::cout << __PRETTY_FUNCTION__ << (void*) &os << ".pword=" << p << '\n'<<std::flush; )
      os.register_callback([](std::ios::event ev, std::ios_base& stream, int index){
	  TRACE( std::cout << __PRETTY_FUNCTION__ << '\n'; )
	    switch (ev){
	    case std::ios_base::erase_event:{
              if( auto p= stream.pword(qtl::ios_base::xfmt);p ){
 	       TRACE( std::cout << "delete(" << (void*)p << ")\n"; )
	       delete (qtl::ios_base::fmtflags*) p;
	       stream.pword(qtl::ios_base::xfmt) = 0;
             };break;
	    default:{ /* 'imbue_event' and 'copyfmt_event' */ };break;
            };break;
	    }
	},0);
#else
       f=qtl::ios_base::fmtflags(os);
       return os;
#endif
     };
     f=*((qtl::ios_base::fmtflags*)p);
     return os;
 }
  std::ostream& operator<<(std::ostream& os,qtl::ios_base::fmtflags &f){
      auto p=os.pword(qtl::ios_base::xfmt); 
     if( !p ){
       p = os.pword(qtl::ios_base::xfmt) = new qtl::ios_base::fmtflags(os);
       NOTRACE( std::cout << __PRETTY_FUNCTION__ << (void*) &os << ".pword=" << p << '\n'<<std::flush; )
     };
     *((qtl::ios_base::fmtflags*)p)=f;
     return os;
 }
namespace qtl{
template <typename T>
class type{
  using base_t=T;
 }; // end cass qtl::type
}; // end namespace qtl
template <typename T>
std::ostream& operator<<(std::ostream& os,const qtl::type<T> &t){
  qtl::ios_base::fmtflags f;
  os >> f;
  os << qtl::type_name<T>(f.verbosity);
  return os;
}
//
#if 1 // os << std::pair
#ifdef __clang__
//template<class T1, class T2>
//    std::pair(T1, T2) -> std::pair<T1, T2>;
#endif
template <class T0, class T1>
std::ostream& operator<<(std::ostream& os, const std::pair<T0,T1>& obj){
  // os << boost::core::demangle(typeid(obj).name()) ;
  using namespace qtl;
  qtl::ios_base::fmtflags f;
  os >> f;
  //  if( f.verbosity>qtl::ios_base::fmtflags::generic ){
  //    os << "std::pair";
  //  }else{
    os << boost::core::demangle(typeid(obj).name());
    //  }
  os << "{"_r << obj.first <<  /*s.sep0*/ ", "_r << obj.second << "}"_r /* << s.sep1 "\n" */ ;
  return os;
}
#endif // os << std::pair

#if 0
//http://jguegant.github.io/blogs/tech/sfinae-introduction.html#sfinae-introduction
template <class T> struct has_mapped_type{
	     // We test if the type has mapped_typee using decltype and declval.
	     //	     template <typename C> static constexpr decltype((bool)(typename C::mapped_type)std::get<1>(*std::declval<C>().begin())) test(int /* unused */)
	     template <typename C> static constexpr decltype((bool)(std::declval<typename C::mapped_type>())) test(int /* unused */)
	     {
	       // We can return values, thanks to constexpr instead of playing with sizeof.
               NOTRACE( std::cerr <<  __PRETTY_FUNCTION__ << '\n'; )
	       return true;
	     }
	     template <typename C> static constexpr bool test(...)
	     {
               NOTRACE( std::cerr <<  __PRETTY_FUNCTION__ << '\n'; )
	       return false;
	     }
	     // int is used to give the precedence!
	     static constexpr bool value = test<T>(int());
}; // end struct has_mapped_type
#endif
#if 1
//https://stackoverflow.com/questions/35293470/checking-if-a-type-is-a-map
#include <type_traits>

namespace detail {
  // Needed for some older versions of GCC
  template<typename...>
    struct voider { using type = void; };

  // std::void_t will be part of C++17, but until then define it ourselves:
  template<typename... T>
    using void_t = typename voider<T...>::type;

  template<typename T, typename U = void>
    struct is_mappish_impl : std::false_type { };

  template<typename T>
    struct is_mappish_impl<T, void_t<typename T::key_type,
    typename T::mapped_type,
    decltype(std::declval<T&>()[std::declval<const typename T::key_type&>()])>>
    : std::true_type { };
}

template<typename T>
struct is_mappish : detail::is_mappish_impl<T>::type { };
#endif

//namespace qtl{//

template <class T,
  typename = std::enable_if_t<!std::is_same_v<char,T> && !std::is_convertible_v<T, std::string>  && ! std::is_pointer_v<T> && !std::is_same_v<std::string_view,T> > // ok
    ,typename =typename T::iterator
#if 0
  ,class O, typename=std::enable_if_t<std::is_base_of_v<std::ostream,O>|std::is_same_v<O&,std::ostream&>>
>
static  std::ostream& operator<<(O& os, const T& obj)
#else
>
static  std::ostream& operator<<(std::ostream& os, const T& obj)
#endif
{
  NOTRACE( std::cout << __PRETTY_FUNCTION__ <<  has_mapped_type<T>::value << '\n'<<std::flush; )
  TRACE( std::cout << __PRETTY_FUNCTION__ << " is_mappish: " <<  is_mappish<T>{} << '\n';;) 
  qtl::ios_base::fmtflags f;
  using namespace qtl::literals;
  os >> f;
  os << qtl::type_name<decltype(obj)>(f.verbosity);
  os << "{"_r;
    auto qs=qtl::ostringstream();
  qs<<f;
  //  auto sep=f.fs;
  if( f.verbosity>=qtl::ios_base::fmtflags::id ){
      qs << qtl::setverbose(qtl::ios_base::fmtflags::generic);
  }
  //  if constexpr ( has_mapped_type<T>::value ){
   if constexpr ( is_mappish<T>{} ){
      qs << f.rs;
      qtl::rstr<std::string> indent= std::string((f.indent+1)*2,' ');
      for( auto [k,v]:obj ){
	if constexpr ( std::is_pointer_v<decltype(v)> ){
	    qs << indent << "{"_r << k << ", /*"_r << v << "*/"_r;
	    if( v ){ qs << *v; }
            qs << "},"_r << f.rs;
	}else{
	    qs << indent << "{"_r << k << ", "_r << v << "},"_r << f.rs;
	}
      }
    }else{
    //      sep=f.fs;
      bool next=false;
      for( auto i:obj ){
        if( next ){ qs << f.fs; }
	qs << i;
        next=true;
      }
  }
  os << qtl::rstr(qs.str());
  os << "}"_r;
  //  os << f;
  return os;
}

template <class Q>
static  std::ostream& operator<<(std::ostream& os, const std::queue<Q>& obj)
{
  NOTRACE( std::cout << __PRETTY_FUNCTION__ <<  has_mapped_type<T>::value << '\n'<<std::flush; )
  qtl::ios_base::fmtflags f;
  using namespace qtl::literals;
  os >> f;
  os << qtl::type_name<decltype(obj)>(f.verbosity);
  os << "{"_r;
    auto qs=qtl::ostringstream();
  qs<<f;
  //  auto sep=f.fs;
  if( f.verbosity>=qtl::ios_base::fmtflags::id ){
      qs << qtl::setverbose(qtl::ios_base::fmtflags::generic);
  }
    //      sep=f.fs;
      bool next=false;
      auto q=obj;
      while( !q.empty() ){
        if( next ){ qs << f.fs; }
	qs << q.front(); q.pop();
        next=true;
      }

  os << qtl::rstr(qs.str());
  os << "}"_r;
  //  os << f;
  return os;
}
#if 0
//}; // end namespace qtl
  namespace qtl{//
template <class T,
     typename = std::enable_if_t<!std::is_same_v<char,T> && !std::is_convertible_v<T, std::string>  && ! std::is_pointer_v<T> > // ok
    ,typename =typename T::iterator
#if 1
  ,class O, typename=std::enable_if_t<std::is_base_of_v<std::ostream,O>|std::is_same_v<O&,std::ostream&>>
>
static  std::ostream& operator<<(O& os, const T& obj)
#else
static  std::ostream& operator<<(std::ostream& os, const T& obj)
#endif
{
  NOTRACE( std::cout << __PRETTY_FUNCTION__ <<  has_mapped_type<T>::value << '\n'<<std::flush; )
  qtl::ios_base::fmtflags f;
  using namespace qtl::literals;
  os >> f;
  os << qtl::type_name<decltype(obj)>(f.verbosity);
  os << "{"_r;
    auto qs=qtl::ostringstream();
  qs<<f;
  //  auto sep=f.fs;
  if( f.verbosity>=qtl::ios_base::fmtflags::id ){
      qs << qtl::setverbose(qtl::ios_base::fmtflags::generic);
  }
  if constexpr ( /*has_mapped_type<T>::value*/ is_mappish<T>{} ){
      qs << f.rs;
      qtl::rstr<std::string> indent= std::string((f.indent+1)*2,' ');
      for( auto [k,v]:obj ){
	qs << indent << "{"_r << k << ", "_r << v << "},"_r << f.rs;
      }
    }else{
    //      sep=f.fs;
      bool next=false;
      for( auto i:obj ){
        if( next ){ qs << f.fs; }
	qs << i;
        next=true;
      }
  }
  os << qtl::rstr(qs.str());
  os << "}"_r;
  //  os << f;
  return os;
}
}; // end namespace qtl
#endif

#if 1 // https://youtu.be/ULX_VTkMvf8?t=30m31s
  template<typename T, auto P>
class AddPre{
  const T& ref; // ref to argument passed in constructor
  AddPre(const T& r): ref(r) { }
  friend std::ostream & operator<< (std::ostream& os, AddPre<T,P> s){
    return os << P << s.ref; // output prefix and passed arguments
  }
};
/**
template <typename First, typename... Args>
void printAllArgs (First, firstarg, Args... args) {
  std::cout << firstarg;
  (std::cout << ... << AddPre<Args,", ">(args)) << '\n';
}
**/
#endif // https://youtu.be/ULX_VTkMvf8?t=30m31s 

#if 1  // based on  https://github.com/swansontec/map-macro/blob/master/map.h
#if 0
/ *
 * Copyright (C) 2012 William Swanson
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
 * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNection
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the names of the authors or
 * their institutions shall not be used in advertising or otherwise to
 * promote the sale, use or other dealings in this Software without
 * prior written authorization from the authors.
 */
#endif
#define EVAL0(...) __VA_ARGS__
#define EVAL1(...) EVAL0(EVAL0(EVAL0(__VA_ARGS__)))
#define EVAL2(...) EVAL1(EVAL1(EVAL1(__VA_ARGS__)))
#define EVAL3(...) EVAL2(EVAL2(EVAL2(__VA_ARGS__)))
#define EVAL4(...) EVAL3(EVAL3(EVAL3(__VA_ARGS__)))
#define EVAL(...)  EVAL4(EVAL4(EVAL4(__VA_ARGS__)))

#define APPLY_END(...)
#define APPLY_OUT
#define APPLY_COMMA ,

#define APPLY_GET_END2() 0, APPLY_END
#define APPLY_GET_END1(...) APPLY_GET_END2
#define APPLY_GET_END(...) APPLY_GET_END1

#define APPLY_NEXT0(test, next, ...) next APPLY_OUT
#define APPLY_NEXT1(test, next) APPLY_NEXT0(test, APPLY_COMMA next, 0)
#define APPLY_NEXT(test,  next) APPLY_NEXT1(APPLY_GET_END test, next)
#define APPLY0(f,a, x, peek, ...) f(a,x) APPLY_NEXT(peek, APPLY1)(f,a, peek, __VA_ARGS__)
#define APPLY1(f,a, x, peek, ...) f(a,x) APPLY_NEXT(peek, APPLY0)(f,a, peek, __VA_ARGS__)

/**
 * Applies the function macro `f` to each of the remaining parameters and
 * inserts commas between the results.
 */
#define APPLY(f,a, ...) EVAL(APPLY1(f,a, __VA_ARGS__, ()()(), ()()(), ()()(), 0))
#endif // based on  https://github.com/swansontec/map-macro/blob/master/map.h

#define MEMBER_NAME2(t,n) member_name/*<t,decltype(t)::n>*/(&decltype(t)::n,#n)
#define MEMBER_NAME1(t,n) decltype(t)::n

#define NAMED_MEMBERS(t,...) named_members(t,APPLY(MEMBER_NAME2,t, __VA_ARGS__))

template<typename T,typename M>
class member_name{
    public:
    M (T::*member);
    std::string name;
  member_name( M (T::*m), std::string n):member(m),name(n){
        NOTRACE( std::cout << __PRETTY_FUNCTION__ << '\n'<<std::flush; )
	NOTRACE(  std::cout << (void*)this << ": " <<  (void*)&name << " = " << name << '\n'; )
     }
  member_name(const std::pair<M (T::*), std::string>&p):member(std::get<0>(p)),name(std::get<1>(p)){}
  member_name(const std::pair<M (T::*), char const*>&p):member(std::get<0>(p)),name(std::get<1>(p)){}

}; // end class member_name

#if 1 // os << member_name
template <typename T,typename M> 
  std::ostream& operator<<(std::ostream& os, const member_name<T,M>& obj){
  using namespace qtl::literals;
#if 0
  os << (void*)&obj << std::flush << "{"<< std::flush << (void*)&obj.name << std::flush << ":" << std::flush
     <<   (void*)obj.name.data() << std::flush 
     <<  "(" << obj.name.size() << ")" <<  std::flush 
     <<  obj.name << "}" << std::flush ;
#else
  os << qtl::type_name<decltype(obj)>() << "{"_r << "member=" << obj.member<< "; name="_r << obj.name << "; }"_r;
#endif
  return os;
}
#endif // os << member_name

//template<typename T,typename M>
//  member_name(M (T::*member), std::string name) -> member_name<T,M>;

template<typename T, typename... M>
#ifdef ERROR_INJECT
  class named_members:public std::pair<const T&,const std::tuple<member_name<T,M>...>& > {
  typedef                    std::pair<const T&,const std::tuple<member_name<T,M>...>& > base_t;
#else
  class named_members:public std::pair<T,std::tuple<member_name<T,M>...> > {
  typedef                    std::pair<T,std::tuple<member_name<T,M>...> > base_t;
#endif
#if 0
  }
#endif
  public:   
  named_members(const T&t,  const std::tuple<member_name<T,M>...>&n):base_t(t,n){
  NOTRACE( std::cout << __PRETTY_FUNCTION__ << '\n'<<std::flush; )
#if 0
  TRACE(  std::cout << __LINE__<< '\n'; )
  TRACE(  std::cout << qtl::type_name<decltype(n)>() << '\n'; )
  TRACE(  std::cout << n << '\n'; )
  TRACE( std::cout << (void*) this << '\n' ;)
  TRACE( std::cout << (void*) &std::get<1>(*this) << '\n' ;)
  TRACE( std::cout << std::get<1>(*this) << '\n' ;)
  TRACE(  std::cout << __LINE__<< '\n'; )
#endif
  }
  named_members(const T& t, const member_name<T,M>& ...n):base_t(t,std::tuple<member_name<T,M>...>(n...)){
    NOTRACE( std::cout << __PRETTY_FUNCTION__ << '\n' <<std::flush; )
      }
  named_members(const T&t, const std::pair<M T::*, char const *>& ...n):std::pair<T, std::tuple<member_name<T,M>...>>(t,std::tuple<member_name<T,M>...>(member_name(n)...)){
    NOTRACE( std::cout << __PRETTY_FUNCTION__ << '\n' <<std::flush; )
          NOTRACE(  std::cout << __LINE__ << '\n' ; )
      }
}; // end class named_members
//  template<typename T, typename... M>
//    named_members(const T&t, const std::tuple<member_name<T,M>...>&n) -> named_members<T,M...>;
//  template<typename T, typename... M>
//    named_members(const T&t, const member_name<T,M...>&n...) -> named_members<T,M...>;

template<class TupType, size_t... I>
void inline print(std::ostream& os,const TupType& _tup, std::index_sequence<I...>)
{
  NOTRACE( std::cout << __PRETTY_FUNCTION__ << std::endl; )
  auto t=std::get<0>(_tup);
  NOTRACE( std::cout << t <<  std::endl; )
  (..., (os << (I == 0? "" : ", ") << std::get<I>(_tup)));
}

template<class T, class M, size_t... I>
void inline print(std::ostream& os, const T& obj, const M& _tup, std::index_sequence<I...>){
  using namespace qtl::literals;
  qtl::ios_base::fmtflags f;
  os >> f;
  (..., (os << qtl::type_name< decltype(obj.*std::get<I>(_tup).member)>(f.verbosity) << " "_r << std::get<I>(_tup).name << "= "_r << obj.*std::get<I>(_tup).member << ";\n"_r  ) );
}

template<typename T, typename... M>
std::ostream& operator<<(std::ostream& os, const named_members<T,M...> &obj){
  using  namespace qtl::literals;
  NOTRACE( std::cout << __PRETTY_FUNCTION__ << '\n'<<std::flush; )
  NOTRACE(  std::cout << qtl::type_name<decltype(obj)>() << '\n'; )
  qtl::ios_base::fmtflags f;
  os >> f;
  os << qtl::type_name<decltype(std::get<0>(obj))>(f.verbosity);
  NOTRACE( os << '\n' << qtl::type_name<decltype(std::get<1>(obj))>() << '\n'; )
  os << "{\n"_r;
  NOTRACE( std::cout << __LINE__ << '\n'<<std::flush; )
  print<T>(os,obj.first,obj.second,std::make_index_sequence<sizeof...(M)>());
  os << '}';
  return os;
}

template <typename... T> // https://ideone.com/Rihfre
std::ostream& operator<<(std::ostream& os, const std::tuple<T...>& obj){
  using  namespace qtl::literals;
  os /* << boost::core::demangle(typeid(obj).name()) << */ << "std::tuple{"_r;   
  print(os,obj,std::make_index_sequence<sizeof...(T)>());
  os << "}"_r;
  return os;
}

template <typename T> 
static inline std::ostream& operator<<(std::ostream& os, const std::optional<T>& obj){
  using  namespace qtl::literals;
  if( obj ){
    os << "std::optional{"_r <<  *obj << "}"_r;
  }else{
    qtl::ios_base::fmtflags f;
    os >> f;
    os << "std::optional<"_r << qtl::type_name<decltype(*obj)>(f.verbosity) << ">{}"_r;
  }
  return os;
}

template <class T,
#if 1
	  typename = std::enable_if_t<!std::is_same<char,T>::value> >
#else
	  typename = std::enable_if_t<std::is_compound<T>::value> >
#endif
std::ostream& operator<<(std::ostream& os, const T* obj){
  using  namespace qtl::literals;
  NOTRACE( std::cout << __PRETTY_FUNCTION__ << '\n'<<std::flush; )
  NOTRACE(  std::cout << qtl::type_name<decltype(obj)>() << '\n'; )
  NOTRACE(  std::cout <<  qtl::type_name<decltype(*obj)>() << '\n'; )
  os << "/*"_r << ((void*)obj) << "*/"_r;
  if constexpr ( !std::is_function_v<T>){  
  if( obj ){
     os <<  "&("_r << *obj<< ")"_r;
  }
  }
  return os;
}
  namespace qtl{
  template <typename... T>
  rstr<std::string> stream_str(const T&... t){
   auto qs=ostringstream();
    (qs << ... << t);
    return rstr<std::string>(qs.str());
  }
  }; // end namespace qtl
template<typename T>
auto operator<<(std::ostream& os, const T& t) -> decltype(t.print(os), os) {
  t.write(os);
  return os;
}
#if 0
template <typename T, typename = std::enable_if_t<std::is_aggregate<T>::value> >
std::ostream& operator<<(std::ostream& os, const T& obj){
  os << qtl::type_name<decltype(obj)>;
  os << '{';
  auto[a,b]=obj;
  os << a << ", " << b;
  os << '}';
  return os;
}
#endif
#if 0
constexpr bool isequal(char const *one, char const *two) {
    return (*one && *two) ? (*one == *two && isequal(one + 1, two + 1))
      : (!*one && !*two);
}
constexpr int c_strcmp( char const* lhs, char const* rhs )
{
    return (('\0' == lhs[0]) && ('\0' == rhs[0])) ? 0
      :  (lhs[0] != rhs[0]) ? (lhs[0] - rhs[0])
      : c_strcmp( lhs+1, rhs+1 );
}
//#if isequal(XSTR(__FILE__),XSTR(__BASE_FILE__))
//#if 0==c_strcmp("A","B")
//#praagma message XSTR(TEST_H)
//#endif
#endif
#define XSTR(s) STR(s)
#define STR(s) #s
template <typename T> class TypeDisplay;

#if __INCLUDE_LEVEL__ + !defined __INCLUDE_LEVEL__ < 1+defined TEST_H
std::vector<int> v={1,2,3,4,5,6,};
std::vector<std::vector<int>> vv={{10,20},v,{30,40},{},{60},};
std::map<int,int>m={{1,2},{3,4}};
using  namespace std::string_literals;
std::map<std::string,const char *>sm={{"\x7f\x80"s,"abc"},{"\03\\\"\xff\0\1\2"s,"def"}};
int main(){
  struct {
    int i;
    double d;
    int const ci=4;
    int & ri=i;
  }test={1,1.23};
  std::cout << qtl::type_name<decltype(test)>() << "\n";
  std::cout << typeid(test).name() << "\n";
  std::cout << typeid(&decltype(test)::d).name() << "\n";
  std::cout << qtl::type_name<decltype(&decltype(test)::d)>() << "\n";
  std::cout << qtl::type_name<decltype(test.*(&decltype(test)::d))>() << "\n";
  std::cout << named_members( test,  std::make_tuple(member_name(&decltype(test)::i,"i"), member_name(&decltype(test)::d,"dbl") ) ) << "\n";
  //  std::cout << __LINE__ << '\n';
  //  std::cout << named_members( test,  {member_name(&decltype(test)::i,"i"), member_name(&decltype(test)::d,"dbl")} ) << "\n";
  //  std::cout << __LINE__ << '\n';
  std::cout << named_members( test,  member_name(&decltype(test)::i,"i"), member_name(&decltype(test)::d,"dbl") ) << "\n";
  std::cout << NAMED_MEMBERS( test,  i,d,ci) << "\n";
  #ifndef __clang__
  {
    std::pair t{&decltype(test)::i,"i"};
      std::cout << qtl::type_name<decltype(t)>() << "\n";
      std::cout << named_members( test,  std::pair{&decltype(test)::i,"i"}, std::pair{&decltype(test)::d,"dbl"} ) << "\n";
  }
  #endif
      //      std::cout << named_members( test,  {&decltype(test)::i,"i"}, {&decltype(test)::d,"dbl"} ) << "\n";
      //  std::cout << __LINE__ << '\n';
  //  std::cout << test;
  int a[3]={1,2,3};
  std::optional<int> o0;
  std::optional<int> o1=1;

      auto t=[&](){
 std::cout << m << '\n'<<std::flush;
 std::cout << sm << '\n'<<std::flush;
  std::cout << v << '\n'<<std::flush;
  std::cout << &v << '\n'<<std::flush;
  std::cout << a << '\n'<<std::flush;
  std::cout << std::vector<int*>{&a[1],&a[0]} << '\n'<<std::flush;
  std::cout << vv << '\n'<<std::flush;
  std::cout << std::tuple<int,std::vector<int>*,std::vector<int>,double>(2,&v,v,3.14) << '\n'<<std::flush;
  std::cout << o0 << "\n" << o1 << '\n'<<std::flush;
  std::cout << std::set<int>{2,3,5,7} << '\n'<<std::flush;
  std::cout << std::unordered_set<int>{2,3,5,7} << '\n'<<std::flush;
  };
  std::cout << qtl::setverbose(qtl::ios_base::fmtflags::alltypeinfo);
  t();
  std::cout << qtl::setverbose(qtl::ios_base::fmtflags::none);
  t();
  std::cout << qtl::setverbose(qtl::ios_base::fmtflags::id);
  t();
  std::cout << qtl::setverbose(qtl::ios_base::fmtflags::demangle);
  t();
  #ifdef ERROR_INJECT
  //   TypeDisplay<decltype(vv)> dummy;
  #endif
#if defined(__GNUC__)
  //  STATIC_WARNING(1<2, "1<2" )
    //  STATIC_WARNING(2<1, "2<1" )
    //  STATIC_WARNING(1<2, typeid(test).name() )
#endif
}
#endif // defined TEST_H
