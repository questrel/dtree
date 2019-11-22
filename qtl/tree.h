#pragma once
//#define _LIBCPP_BEGIN_NAMESPACE_STD namespace std{
//#define _LIBCPP_END_NAMESPACE_STD }
#include "out.h"
#include "interval.h"
//#include <strstream>
#include <cmath>
#include <functional>
#include <type_traits>
#include <typeinfo>
#include <optional>
#include "operators.h"
namespace qtl{
  enum class op{
#define X(n,a,o,f,p) n,
    OP_TABLE
#undef X
}; // end class op
  //  template< class C>
  //class tree;

template<class A,template<typename...> typename V=std::vector,class P=A>
class lazyvec{
   const V<P>& v;
   public:
   std::function<A(const lazyvec&)>f=[this](const lazyvec<A,V,P>&v){ TRACE( std::cerr << "uninitialized " << __PRETTY_FUNCTION__ << '\n'; ) return A(); };
   std::function<A(const P&)>p=[this](const P&p){
     if constexpr ( std::is_convertible_v<P,A> ){
	 return p;
       }else{
        NOTRACE( std::cerr << "uninitialized " << __PRETTY_FUNCTION__ << '\n'; ) 
        return P();
      }
   };
   class const_iterator{
     const lazyvec& _;
     public:
     typename V<P>::const_iterator p;
     const_iterator(const lazyvec &v,typename V<P>::const_iterator p):_(v),p(p){}; 
     A operator *()const{ return _.p(*p); }
     operator A()const{ return _.p(*p); }
     bool operator !=(const const_iterator &r)const{ return p!=r.p; }
     const_iterator operator ++(){ ++p; return *this; }
     const_iterator operator ++(int){ auto t=*this; ++p; return t; }
   };
   lazyvec(const V<P>&v):v(v){}
   lazyvec(const V<P>&v,std::function<A(const P&)>f):v(v),f(f){}
   //it operator[](int i)const{ return it(*this,&v[i]); }
   A operator[](int i)const{ return p(v[i]); }
   const_iterator begin()const{ return const_iterator(*this,v.begin()); }
   const_iterator end()const{ return const_iterator(*this,v.end()); }
   size_t size()const{ return v.size(); }
   bool empty()const{ return v.empty(); }
   void push_back(const P&p){ v.push_back(p); }
   operator V<A>()const{
     V<A> ret;
     for( auto x:v ){
       ret.push_back(x);
     }
     return ret;
   }
}; // end class lazyvec

template<class A,template<typename...> typename V=std::vector,class P=A>
class vec{
    public:
    vec(){ }
    const V<P> &p;
    std::function<A(const P &)>a=[this](const P&p){ NOTRACE(std::cout << "uninitialized f " << "(" << v << ")" << std::endl; ) return A(); };
    class it{
      const vec& v;
    public:
      const P *i;
      it(const vec &v,const P *i):v(v),i(i){ NOTRACE( std::cout << __PRETTY_FUNCTION__ << std::endl; ) }
      A operator *(){ return v.a(*i); }
      operator A(){ return v.a(*i); }
    }; // end class it
    const it begin(){ return it(this,p.begin()); }
    const it end(){ return it(this,p.end()); }

    std::function<it(int)>at=[this](int i){ return a(p[i]); };

    A operator[](int i)const{ return *at(i); }

    vec(const V<P>&p,std::function<A(const P&)>a):p(p),a(a){}

    operator std::vector<A>(){
      std::vector<A> ret;
      for( auto x:p ){
	ret.push_back(f(x));
      }
      return ret;
    }
  }; // end class vec<A,V,P>;

#if 0
#ifdef __clang__
 template<class A,class C,class... Args>
 vec<A,C> _vec( A(C::*f)( const vec<A,C>&,Args...) ){ vec<A,C> v; return v; }
#else
 template<class A,class C,class... Args>
 vec( A(C::*f)( const vec<A,C>&,Args...) ) ->  vec<A,C> ;
#endif
#endif

  template<class C>
  class tree:public C{
  public:
   using base_t=C;
   using base_t::base_t;
   //    template<class A,class... Args>
   //    vec<A,C> vector( A(C::*f)( const vec<A,C>&,Args...) ){ vec<A,C> v; return v; }

    std::vector<tree>branches;
    template<class A, class... Args>
      A recurse( A(C::*f)(const std::vector<A>&,/*const*/ Args...)const ,/*const*/ Args... args)const{
            NOTRACE( std::cerr << __PRETTY_FUNCTION__ << '\n'; )
	    std::vector<A> r;
	    for( auto b:branches ){ 
#if 1
	      /*NO*/TRACE( std::cerr << ""; )
#else
	      NOTRACE( std::cerr << __LINE__ << '\n'; )
#endif
	      r.push_back( (b).template recurse<A,Args...>(f,args...) );  
            }
#if 1
            /*NO*/TRACE( std::cerr << ""; )
#else
            NOTRACE( std::cerr << __LINE__ << '\n'; )
#endif
	      NOTRACE( std::cerr << *this << "->*f" << r << ",args...)" << '\n'; )
	    return (this->*f)(r,args...);
    }
    template<class A,class... Args>
      A recurse( A(C::*f)(const lazyvec<A>&,Args...) , Args... args )const{
      lazyvec<A,std::vector,tree> r(
          this->branches,
          [f,args...](const tree &e){
	    NOTRACE( std::cerr << __LINE__ << '\n'; )
	    return e.template recurse<A,Args...>(f,args...);
          }
       );
       NOTRACE( std::cerr << __LINE__ << '\n'; )
       return (this->*f)(r,args...);
     }

#if 0
     template<class A,class... Args>
      A recurse( A(C::*f)(Args...), Args... args )const{
       NOTRACE( std::cerr << __LINE__ << '\n'; )
	std::vector<A> b;
       for( auto x:ret.branches ){
	 b.push_back(x.template recurse<A,Args...>(f,args...);
       }
       return {(this->*f)(b,args...),b}
     }
#endif

#if 1
 tree(const tree&t):branches(t.branches),C((C)t){
      NOTRACE( std::cerr << __PRETTY_FUNCTION__ << "(" << t << ")" << '\n'; )
    }
#endif
    //using C::C;
    tree(){};
    tree(const C &c,const std::vector<tree> &b={}):C(c),branches(b){
	NOTRACE( std::cerr << __PRETTY_FUNCTION__ << "(" << c << ", ";
	       std::cerr << "{" ;
    	      for( auto b:branches ){
		std::cerr << b << "\n";
              }
	       std::cerr << "}" ;
	       std::cerr << ")" << '\n'; )
      }

friend std::ostream& operator<<(std::ostream& os, const tree &o){
   qtl::ios_base::fmtflags f;
   os >> f;  
   std::string t=qtl::type_name<decltype(o)>(f.verbosity);
   os << (C)o;
   if( o.branches.size() ){
     os << "branches{\n";
     for( auto b:o.branches ){
       os << "  " << b << "\n";
    }
    os << "}\n";
   }
   return os;
 }

 }; // end class tree


     template<class V=qtl::interval, class V1=/*std::vector<V>*/intvec<lex::scalar>>
class operation{
  public:
  using operand_t=V;
  enum op o=(enum op)-1;
    std::optional<operand_t> mutable cachevalue;
    std::optional<std::string> identifier;
    using symbol_table=std::map<std::string,operand_t>;
    std::optional<symbol_table> sym;
    operator operand_t()const{ 
      if( cachevalue ){ return *cachevalue; }
      WARN( std::cerr << "warning: control may reach end of non-void function\n"; );
      return operand_t();
    }
  struct ps:public std::string{
    int precedence;
    public:
     ps(const std::string &s,int p):std::string(s),precedence(p){}
     ps(const std::string &s,const operation &o):std::string(s),precedence(optable[o.op].precedence){}
  };
    struct methods{
      operand_t(*eval)(/**/const operation &t,/**/const lazyvec<operand_t>&,const symbol_table &,const /**/V1/**/ /*std::vector<V>*/&);
      ps(*stringify)(const operation &t,const std::vector<ps> &v);
      std::string name;
      int arity;
      int precedence;
      methods(
	      operand_t(*eval)(/**/const operation &t,/**/const lazyvec<operand_t>&,const symbol_table &,const /**/V1/**/ /*std::vector<V>*/&),
              ps(*stringify)(const operation &t,const std::vector<ps> &v),
              const std::string &name,
              int arity,
              int precedence
	      ):eval(eval),stringify(stringify),name(name),arity(arity),precedence(precedence){}
    };
  inline int precedence()const{
    return optable.at(o).precedence;
   }
    operand_t static inline pow(const operand_t a, const operand_t b){
      if constexpr ( std::is_arithmetic_v<operand_t> ){
         return std::pow(a,b);
      }else{
	std::cerr << __PRETTY_FUNCTION__ << " not implemented\n";
	return a;
      }
    }
  operand_t static inline fact(/*const operation &t,*/ const operand_t &a){
    if constexpr ( std::is_arithmetic_v<operand_t> ){
         return std::tgamma(a+1);
      }else{
	std::cerr << __PRETTY_FUNCTION__ << " not implemented\n";
	return a;
      }
  }
  operand_t static inline fact(/*const operation &t,*/ std::vector<operand_t> &v){
    return fact(v[0]);
  }
    //    V operator^(const V&a, const V&b){
    //      return std::pow(a,b);
    //    }
    //    V eval(const std::vector<tree<operation>*>&v)const{

  template<typename... S_>
  operand_t eval(const std::vector<operand_t> &v,const S_&... s_)const{
	if( !cachevalue ){
	  if( o == op::greater ){
           NOTRACE( std::cout << __PRETTY_FUNCTION__ << '\n'; )
           NOTRACE( std::cout << "optable.at(" << (int)o << ").eval)(" << *this << "," << v << ",s_...)" << 'n'; )
	  }
	  if( auto t=(*optable.at(o).eval)(*this,v,s_...); t.is_null() ){
	     return t;
          }else{
	    NOTRACE( std::cout << "=" << t << '\n'; )
	    //cachevalue=t;
	    NOTRACE( std::cout << __LINE__ << '\n'; )
	    return t;
          }
        }else{
          NOTRACE( std::cout << __PRETTY_FUNCTION__ << '\n'; )
	  NOTRACE( std::cerr << "cachevalue=" << *cachevalue << '\n'; )
          return *cachevalue;
        }
  }

  class ob:public operation{
    public:
    bool b=false;
  ob(const operation &o,bool b=false):operation(o),b(b){}
    operator operation()const{ return (operation)*this; }
    operator bool()const{ return this->b; }
  }; // end class ob;
#if 0
  ob bind(const std::vector<ob>&o,const std::map<std::string,operation>&b){
    if( auto t=(o==op::name&&identifier?b.at(o):b.end())!=b.end() ){
      return {t.second,true};
    }
    bool b=false;
    for( auto x:o ){
      b |= x;
    }
    return {*this,b};
  }
#endif
      //      template< class I=typename  std::vector<V>::const_iterator>
      //      V evalit( I it){
    //     template<class T=const vec<V>&>
    //       V evalit( T v ){
    operand_t evalit( const vec<operand_t> &v ){
      std::vector<operand_t>r;
      for( auto i=0;i<optable.at(o).arity;++i ){
	    r.push_back(v[i]);
      }
	       NOTRACE( std::cout << r << std::endl; );
	     return (optable.at(o).eval)(*this,r);
    }

/**/template<typename Vec=std::vector<operand_t>>
static V nary(const V &v0, V(*f)(const V&,const V&),Vec &v){
    NOTRACE(  std::cerr << __PRETTY_FUNCTION__ << '\n'; )
      V ret=v0;
      bool first=true;
      for( auto x:v ){
	  ret = first?x:f(ret,x);
	  first=false;
      }
      return ret;
}

operand_t  make_interval(const operand_t &a, const operand_t &b){
  return interval(a.l(),b.l());
}

#if 1
#define T(n,o,f) \
  template<typename V0=std::vector<operand_t>/**/, typename V2=std::vector<operand_t>/**/> \
 static inline operand_t eval_ ## n(const operation &t,const V0 &v,const symbol_table &syms={},const V2 /*std::vector<operand_t>*/ &cols={}){ \
 NOTRACE( std::cerr << __PRETTY_FUNCTION__  << std::endl; ) f \
}
#else
  //#define T(n,o,f)			       \
  // template<typename Vec=std::vector<operand_t>>			\
  // static inline operand_t eval_ ## n(const operation &t,const Vec &v,const symbol_table &syms={},const std::vector<operand_t> &cols={}){ \
  //    NOTRACE( std::cerr << __PRETTY_FUNCTION__  << std::endl; ) f	\
  // }
#endif
#if 0
#define X0(n,o)  T(n,o,					      \
  NORACE( std::cout << __PRETTY_FUNCTION__  << std::endl; ); \
		   if( !t.cachevalue ){				      \
		       NOTRACE( std::cerr << "!t.cachevalue" << '\n'; ) \
		     if( op::n==op::name ){    \
		       NOTRACE( std::cerr << "op::n==op::name" <<  t.identifier << '\n'; ) \
		       NOTRACE( std::cerr <<   *t.identifier << '\n'; ) \
                       if( auto p=syms.find(*t.identifier); p!=syms.end() ){ \
    		         NOTRACE( std::cerr << "__LINE__" << '\n'; ) \
    		         NOTRACE( std::cerr << p->second << '\n'; ) \
			 t.cachevalue = p->second;	\
                       }else{ \
                         std::cerr << *t.identifier << " undefined\n";  \
		         t.cachevalue = V(nullptr); \
		       } \
                     }else{						\
		       std::cerr << "don't know how to evaluate op::" << #n << '\n'; \
		       t.cachevalue = V(nullptr); \
		     } \
                   } \
		   NOTRACE( std::cerr << "__LINE__" << '\n'; ) \
                   return *t.cachevalue; \
 )
#endif
#define X1(n,o)  T(n,o, return operation::n((operand_t)(v[0])); )
#define X01(n,o) T(n,o, return std::n<operand_t>()((operand_t)(v[0])); )
#define X2(n,o)  T(n,o, using namespace std; return ((operand_t)(v[0]) o (operand_t)(v[1])); )
#define X02(n,o) T(n,o, NOTRACE( std::cerr << __PRETTY_FUNCTION__  << " X02\n"; ) return n()((operand_t)(v[0]),(operand_t)(v[1])); )
#define X002(n,o)  T(n,o, NOTRACE( std::cerr << __PRETTY_FUNCTION__  << " X002\n"; )  \
  /*return operation::nary((V)n ## _identity,&std::n<operand_t>::operator(),v ); */ \
     V ret=(operand_t)n ## _identity;			\
  bool first=true; \
  for( auto x:v ){ \
    ret = first?x:std::n()(ret,x);		\
    first=false; \
  } \
  return ret; \
)
#define Xlit(n,o)  T(n,o,					      \
  NOTRACE( std::cout << __PRETTY_FUNCTION__ << __LINE__ << std::endl; ); \
		   if( !t.cachevalue ){ \
                         std::cerr << *t.identifier << " undefined\n";  \
			 return V(nullptr); \
                   } \
                   return *t.cachevalue; \
)
#define Xcolumn(n,o)  T(n,o,					      \
  NOTRACE( std::cout << __PRETTY_FUNCTION__ << __LINE__ << std::endl; ); \
  unsigned long c = std::stoul(*t.identifier); \
  if( c < cols.size() ){ \
    return cols[c]; \
  }else{ \
    return  V(nullptr); \
  } \
)
#define Xname(n,o)  T(n,o,					      \
  NOTRACE( std::cout << __PRETTY_FUNCTION__ << __LINE__ << std::endl; ); \
		   if( !t.cachevalue ){ \
		       NOTRACE( std::cerr << "!t.cachevalue" << '\n'; ) \
			 NOTRACE( std::cerr << "t.identifier=" << (bool)t.identifier << '\n'; ) \
			 NOTRACE( std::cerr <<   t << '\n'; ) \
                       if( auto p=syms.find(*t.identifier); p!=syms.end() ){ \
    		         NOTRACE( std::cerr << "find" << '\n'; ) \
			 NOTRACE( std::cerr << "p->second=" << p->second << '\n'; ) \
			 t.cachevalue = p->second;	\
    		         NOTRACE( std::cerr << "*t.cachevalue=" << *t.cachevalue << std::endl; ) \
                       }else{ \
                         std::cerr << *t.identifier << " undefined\n";  \
		         return V(nullptr); \
		       } \
                   } \
		      /**/NOTRACE( std::cerr << "return " << *t.cachevalue << std::endl; ) \
                   return *t.cachevalue; \
 )

inline static const std::map<std::string,std::function<operand_t(const std::vector<operand_t>&)>>functab={
       {"ABS",[](const std::vector<operand_t> &v){
          if( v.size()==0 ){
	    return operand_t();
          }
	  return operand_t::abs(v[0]);
	 }},
       {"NUMERIC_TO_STRING",[](const std::vector<operand_t> &v){
          std::stringstream s;
	  int i=0;
	  for( auto x:v ){
	    if( i++ ){ s << ", "; }
	    if( x.is_point() ){
              s <<(double)x.l().value();
            }else{
	      if( !x.l().is_inf()||x.u().is_inf() ){
		s <<  (double)x.l().value() << ((lim)x.l().ma()==infi?" <=":" <");
              }
  	      s << " x::x ";
	      if( !x.u().is_inf()||x.l().is_inf() ){
                s <<  ((lim)x.u().ma()==infi?"< ":"<= ") << (double)x.u().value();
	      }
	    }
	  }
          return operand_t(std::string(s.str()));
       }}
};

#define Xfunc(n,o) T(n,o,					      \
		   if( !t.cachevalue ){ \
                       if( auto p=functab.find(*t.identifier); p!=functab.end() ){ \
			 t.cachevalue = (p->second)(v);	\
                       }else{ \
                         std::cerr << "unknown function " << *t.identifier << "()\n";  \
		         return V(nullptr); \
		       } \
                   } \
                   return *t.cachevalue; \
 )
 
#define Xpreop(n,o)  T(n,o, return o(operand_t)(v[0]); )
#define Xin2(n,o)  T(n,o, return ((operand_t)(v[0]) o (operand_t)(v[1])); )
#define Xright2(n,o)  Xin2(n,o)
#define Xleft_(n,o)  T(n,o, NOTRACE( std::cerr << __PRETTY_FUNCTION__  <<'\n'; )  \
  if( v.size()==0 ){ \
    return (operand_t)n ## _identity;			\
  } \
  V ret=v[0]; \
  bool first=true; \
  for( auto x:v ){ \
    if( first ){ \
      first=false; \
    }else{ \
      ret = (operand_t)std::n<V>()(ret,x);		\
    } \
  } \
  return ret; \
)

#define Xinterval(n,o) T(n,o, return interval(((operand_t)(v[0])).l(), ((operand_t)(v[1])).l() ); )

//#define X(n,a,o,f,p) X ## a(n,o)
#define X(n,a,o,f,p) X ## f(n,o)
#if 0
template<typename Vec=std::vector<operand_t>> static inline operand_t eval_multiplies(const operation &t,const Vec &v){
  V ret=(operand_t)multiplies_identity; 
    bool first=true;
    for( auto x:v ){
      ret = first?x:std::multiplies()(ret,x);
      first=false; 
    } 
    return ret;
}
#else
    OP_TABLE
#endif
#undef T
#undef X
#undef X0
#undef X01
#undef X1
#undef X02
#undef X002
#undef X2

 static inline std::string str_lit(const operation &t,const std::string& s,const std::vector<std::string>&v={},int precedence=0){
#if 0
  std::strstream ss;
  ss << *t.cachevalue;
  std::string d(ss.str(),ss.pcount());
  ss.freeze(false);
  //~ss;
  ss.operator~();
#else
 std::stringstream ss;
  ss << *t.cachevalue;
  std::string d(ss.str());
  //  ss.freeze(false);
  //~ss;
//  ss.operator~();
  ss.imbue(std::locale());
//  ~ss();
#endif
  return d;
}
static inline std::string str_column(const operation &t,const std::string& s,const std::vector<std::string>&v={},int precedence=0){
  //  return s + "(" + *t.identifier + ")" ;
  return *t.identifier + "_" + s ;
}
static inline std::string str_name(const operation &t,const std::string& s,const std::vector<std::string>&v={},int precedence=0){
  NOTRACE( std::cerr << __PRETTY_FUNCTION__ << '\n'; )
      return  *t.identifier;
}

static inline std::string str_pre(const operation &t,const std::string& s,const std::vector<std::string>&v={},int precedence=0){
  return s + " " +  par(t.precedence(),v[0]);
}
static inline std::string str_post(const operation &t,const std::string& s,const std::vector<std::string>&v={},int precedence=0){
    return v[0] + s;
}
static inline std::string str_left(const operation &t,const std::string& s,const std::vector<std::string>&v={},int precedence=0){
  NOTRACE( std::cout << __PRETTY_FUNCTION__ << "(" << s << ", " << v << ")" << std::endl; );
  return std::string(par(t.precedence(),v[0]) + s + v[1]);
}
static inline std::string str_right(const operation &t,const std::string& s,const std::vector<std::string>&v={},int precedence=0){
  return v[0]+  s +  "(" + v[1] + ")";
}
  static inline std::string str_non(const operation &t,const std::string& s,const std::vector<std::string>&v={},int precedence=0){
    return v[0] +  s +  v[1];
}
static inline std::string par(bool p,const std::string &s){
      using namespace std::string_literals;
      if( !p ){ return s; }
      return "("s + s + ")"s;
}
static inline std::string par(int precedence,const ps &s,bool a=false){
  return par(precedence-(a?1:0) < s.precedence, (std::string)s);
}
#if 0
template<template<typename...> typename Vec=std::vector>
static inline ps stri_lit(const operation &o, const std::string &s,const Vec<ps> &v){
#else
template<typename Vp=vec<ps>>
static inline ps stri_lit(const operation &o, const std::string &s,const Vp &v){
#endif
#if 0
}
#endif
  std::string t;
{
#if 0
  std::strstream ss;
  ss << *o.cachevalue;
  t=std::string(ss.str(),ss.pcount());
ss.freeze(false);
//  ~ss;
#else
  std::stringstream ss;
  ss << *o.cachevalue;
  t=std::string(ss.str());
  //ss.imbue(std::locale());
  //ss.copyfmt(ss);
#endif
}
  return ps(t,o.precedence());
}
#if 0
template<template<typename...> typename Vec=std::vector>
static inline ps stri_name(const operation &o, const std::string &s,const Vec<ps>&v){
#else
template<typename Vp=vec<ps>>
static inline ps stri_name(const operation &o, const std::string &s,const Vp &v){
#endif
#if 0
}
#endif
  NOTRACE( std::cerr << __PRETTY_FUNCTION__ << '\n'; )
    return  ps(*o.identifier,o.precedence());
}
template<typename Vp=vec<ps>>
static inline ps stri_column(const operation &o, const std::string &s,const Vp &v){
  //  return ps( s+"."+ *o.identifier,o.precedence());
  return ps( *o.identifier + "_" + s,o.precedence());
}
template<typename Vp=std::vector<ps>>
static inline ps stri_func(const operation &o,const std::string& s,const Vp &v){
  std::string a;
  std::string c;
  for( auto x:v ){
    a += c;
    c=",";
    a += x;
  }
  return ps( *o.identifier + "(" + a + ")" , o.precedence());
}

template<typename Vp=std::vector<ps>>
static inline ps stri_preop(const operation &o,const std::string& s,const Vp &v){
  return ps(s + " " +  par(o.precedence(),v[0]),o.precedence());
}
template<typename Vp=std::vector<ps>>
static inline ps stri_interval(const operation &o,const std::string& s,const Vp &v){
  return ps(v[0] + par(o.precedence(),v[1]), o.precedence());
}
template<typename Vp=std::vector<ps>>
static inline ps stri_post(const operation &o,const std::string& s,const Vp &v){
  return ps(v[0] + s,o.precedence());
}
template<typename Vp=std::vector<ps>>
static inline ps stri_left_(const operation &o,const std::string& s,const Vp&v){
  std::string ret;
  std::string s0;
  for( auto x:v){
    ret += s0+par(o.precedence(),x,true);
    s0=s;
  }
  return ps(ret,o.precedence());
}
template<typename Vp=std::vector<ps>>
static inline ps stri_right2(const operation &o,const std::string&s,const Vp &v){
  return ps(par(o.precedence(),v[0],true) +  s +  par(o.precedence(),v[1]),o.precedence());
}
template<typename Vp=std::vector<ps>>
static inline ps stri_in2(const operation &o,const std::string& s,const Vp &v){
  return ps(par(o.precedence(),v[0]) +  s + par(o.precedence(),v[1]),o.precedence());
}

#if 0
#define X(n,a,o,f,p) {\
      op::n, \
      /*methods*/{ \
        /*eval*/ eval_ ## n, \
	  /*stringify*/ [](const operation &t,const std::vector<ps>&v){	\
	  return operation::stri_ ## f(t,std::string(#o),v);	\
        },\
	/*name*/ #o, \
        /*arity*/a, \
        /*precedence*/(int)op::p \
     } \
},
inline static const std::vector<methods> tmp={
  //OP_TABLE
};
#undef X
#endif

 inline static const  std::map<enum op,struct methods>optable{
#define X(n,a,o,f,p) {\
      op::n, \
      /*methods*/{ \
        /*eval*/ eval_ ## n, \
	  /*stringify*/ [](const operation &t,const std::vector<ps>&v){	\
	  NOTRACE( std::cerr << __PRETTY_FUNCTION__ << '\n'; )		\
	  NOTRACE( std::cerr << "stri_" << #f << "(" << #o << ")" <<  '\n'; ) \
	  return operation::stri_ ## f(t,std::string(#o),v);	\
        },\
	/*name*/ #o, \
        /*arity*/a, \
        /*precedence*/(int)op::p \
     } \
},
#if 0
    std::pair<enum op,struct methods>(
       op::lit, 
       methods(
         eval_lit,
	   [](const operation &t,const vec<ps>&v){ 
	   /*   	     vec<ps>p(
               v,
               [](const std::string &s,const operation &o){
		 return ps(s,o.precedence());
               }
            );
	   */
	    return stri_lit(t,std::string(""),v);
         },
	   "", /*name*/
	     0,  /*arity*/
	     (int)op::lit /* precedence*/
	       )
				      ),
#endif
#if 0
      {op::multiplies,
	  {
	    eval_multiplies,
            [](const operation &t,const std::vector<ps>&v){ return operation::stri_left(t,std::string("*"),v); },
	    "*",
	      002,
	      (int)op::lit
	  }
      },
#else
      OP_TABLE
#endif
#undef X
     };
    //    template<class T=const vec<std::string,operation>&>
    //    std::string stringify( T v){
  //  template<template<typename...> typename Vec=std::vector>
  ps stringify(const std::vector<ps>&v)const{
    NOTRACE( std::cerr << __PRETTY_FUNCTION__ << '\n'; )	\
      if( (int)o<0 ){  ps(" invalid expr ",0); }
    return (*optable.at(o).stringify)(*this,v);
  }
#if 0
  ps stringify(const vec<ps>&v){
    return (*optable.at(o).stringify)(*this,v);
  }
#endif
  operation(){}
  operation(enum op o):o(o){
	    NOTRACE( std::cerr << __PRETTY_FUNCTION__ << "(" << (int)o << ")"; )
  }
#if 1
      operation(enum op o,const V& v):o(o),cachevalue(v){
	    NOTRACE( std::cerr << __PRETTY_FUNCTION__ << "(" << (int)o << ", " << v << ")\n"; )
	    if constexpr ( std::is_base_of_v<std::string,decltype(v)> ){
		NOTRACE( std::cerr << "v:id\n" );
		identifier=v;
	    }
	    if( o==op::name ){
		NOTRACE( std::cerr << "o:id\n" );
		if constexpr( std::is_convertible_v<V,std::string> ){
		  identifier=v;
		}else{
		  std::cerr << "don't know how to make " << qtl::type_name<V>() << " into an identifier\n";
		}
	    }
       }
       operation(enum op o,const std::string& s):o(o){
	    NOTRACE( std::cerr << __PRETTY_FUNCTION__ << "(";
		   std::cerr << (int)o << ", " << s << ")\n"; )
	    switch( o ){
	    case op::lit:{ 
	      if constexpr( std::is_assignable_v<decltype(cachevalue),decltype(s)> ){
                  cachevalue=s;
	      }else{
		std::cerr << "don't know how to make " << s << " into a literal\n";
	      }
            };break;
	    case op::name:{ identifier=s; };break;
	    default:{  identifier=s; };break;
            }
       }
#else       
       operation(enum op o,const std::string& s):o(o){
	    NOTRACE( std::cerr << __PRETTY_FUNCTION__ << "(";
		   std::cerr << (int)o << ", " << s << ")\n"; )
	    switch( o ){
	    case op::lit:{ cachevalue=s; };break;
	    case op::name:{ identifier=s; };break;
	    default:{  std::cerr << (int)o << ": don't know what to do with " << s << "\n"; };break;
            }
       }
#endif
friend std::ostream& operator<<(std::ostream& os, const operation &op){
   qtl::ios_base::fmtflags f;
   os >> f;  
   std::string t=qtl::type_name<decltype(op)>(f.verbosity);
   //   t="";
     os << t << "{";
   os<< "op=" ;
   os << (int)op.o;
   if( optable.count(op.o) ){
     os << ":" << optable.at(op.o).name;
   }
   os << ", ";
   if( op.cachevalue ){
       os << "V=";
       os  /* << qtl::setverbose(qtl::ios_base::fmtflags::show_cache) */ <<  *op.cachevalue;
       if( op.cachevalue->is_point() ){
         os << "=" << op.cachevalue->l().value().raw();
       }
       os <<  ", ";
    }
   if( op.identifier ){ os << "id=" << *op.identifier << ", "; }
   os << "}";
   return os;
 }

 }; // end class operation

template<typename V=qtl::interval,typename V1=/*std::vector<V>*/intvec<lex::scalar>>
#define BASE_T tree<operation<V,V1>>
class optree:public BASE_T{
 using base_t=BASE_T;
#undef BASE_T
 using OP=operation<V,V1>;
 public:
 // using value_type=optree;
 using base_t::base_t;
 // optree(const base_t &o):base_t(o){}
 // optree(const OP &o):base_t(o){}
 optree(const OP &o,const std::vector<optree> &vo={}):base_t(o){
   std::vector<base_t> b;
   for( auto i:vo ){
     b.push_back(i);
   }
   base_t::branches=b;
 }
 optree(enum op o,const std::vector<optree> &vo={}):base_t(OP(o)){
   std::vector<base_t> b;
   for( auto i:vo ){
     b.push_back(i);
   }
   base_t::branches=b;
 }
// optree(enum op o,const V &v):base_t(OP(o,v)){}
#if 0
 optree(enum op o,const std::string &id,const std::vector<optree> &vo={}):base_t(OP(o,id)){
   std::vector<base_t> b;
   for( auto i:vo ){
     b.push_back(i);
   }
   base_t::branches=b;
 }
#endif
 optree(enum op o,const V &v,const std::vector<optree> &vo/*={}*/):base_t(OP(o,v)){
   std::vector<base_t> b;
   for( auto i:vo ){
     b.push_back(i);
   }
   base_t::branches=b;
 }
 std::string stringify()const{
   return base_t::template recurse<struct OP::ps>(&OP::stringify);
  }
#if 0
 // V eval(const typename OP::symbol_table &syms={},const std::vector<V> &cols={})const{
 //   return base_t::template recurse<V,const typename OP::symbol_table &,const std::vector<V> &>(&OP::eval,syms,cols);
 //  }
#else
 // template<typename Vec=std::vector<V>,V1=std::vector<V>>
 // V eval(const typename OP::symbol_table &syms={},const V1 &cols={})const{
 //   return base_t::template recurse<V,const typename OP::symbol_table &,const Vec &>(&OP::eval,syms,cols);
 // }
 // template<typename V1=std::vector<V>>
 V eval(const typename OP::symbol_table &syms={},const V1 &cols={})const{
   return base_t::template recurse<V,const typename OP::symbol_table &,const V1 &>(&OP::eval,syms,cols);
 }
#endif
 class ob:public optree{
    bool b=false;
 public:
    ob(const optree &o,bool b=false):optree(o),b(b){}
    operator optree()const{ return (optree)*this; }
    operator bool()const{ return this->b; }
 }; // end class ob;
 ob bind(const std::map<std::string,optree>&s){
   NOTRACE( std::cerr << __PRETTY_FUNCTION__ << '\n'; )
   if( auto p=(base_t::o==qtl::op::name &&base_t::identifier?s.find(*base_t::identifier):s.end()); p!= s.end() ){
     return {p->second,true};
   }
   NOTRACE( std::cerr << *this << '\n'; )
   std::vector<base_t>v;
   bool b=false;
   for( auto const &x:base_t::branches ){
     NOTRACE( std::cerr << "x=" << x << '\n'; )
       NOTRACE( std::cerr << "(base_t)x=" << (base_t)x << '\n'; )
       NOTRACE( std::cerr << "base_t(x)" << base_t(x) << '\n'; )
       auto y=(optree((base_t)x,x.branches)).bind(s);
     NOTRACE( std::cerr << "y=" << y << '\n'; )
     b = (b||y);
     v.push_back(y);
   }
   if( !b ){ return {*this,b}; }
   //   return {{(base_t)*this,v},b};
      return {{*(base_t*)this,v},b};
 }
 //  operator base_t(){ return (base_t)*this; }
#define BASE_T std::map<int,std::set<typename V::b_t>>
 class hints:public BASE_T{
   using base_t=BASE_T;
   #undef BASE_T
   using base_t::base_t;
 }; // end class optree::hints
 hints hints(){
   class hints ret;
   auto set=[&ret,this](sign s){
    using b_t=typename V::b_t;
     if( base_t::branches[0].o==qtl::op::column && base_t::branches[1].cachevalue ){
       auto i=std::stoul(*base_t::branches[0].identifier);
       if( s==sign(0) ){
	 ret[i].insert(base_t::branches[1].cachevalue->l());
	 ret[i].insert(base_t::branches[1].cachevalue->u());
       }else{
         ret[i].insert({base_t::branches[1].cachevalue->l().value(),s});
       }
     }else if(  base_t::branches[1].o==qtl::op::column && base_t::branches[0].cachevalue ){
       auto i=std::stoul(*base_t::branches[1].identifier);
       if( s==sign(0) ){
	 ret[i].insert(base_t::branches[0].cachevalue->l());
	 ret[i].insert(base_t::branches[0].cachevalue->u());
        }else{
         ret[i].insert({base_t::branches[0].cachevalue->l().value(),-s}); 
       }
     }
   };
   switch( base_t::o ){
   case qtl::op::equal_to: case qtl::op::not_equal_to: { set(sign(0)); }; break;
   case qtl::op::less: case qtl::op::greater_equal:{ set(sign(-1)); }; break;     
   case qtl::op::greater: case qtl::op::less_equal:{ set(sign(1)); }; break;     
   case qtl::op::logical_not: case qtl::op::logical_and: case qtl::op::logical_or: {
      for( auto const &x:base_t::branches ){
	for( auto const &h:(optree((base_t)x,x.branches)).hints() ){
	  ret[h.first].insert(h.second.begin(),h.second.end());
        }
     }
   };break;
   default: {  };break;
   }
   return ret;
 }
};//end class optree

 using expr=optree<qtl::interval,intvec<lex::scalar>>;

template<typename V=qtl::interval,typename V1=std::vector<V>>
#define BASE_T std::optional<optree<V,V1>>
 class optexpr:public BASE_T{
   using base_t=BASE_T;
   #undef BASE_T
    public:
   using base_t::base_t;
   //    optexpr(const qtl::expr &e):base_t(std::optional<std::reference_wrapper<qtl::expr>>(std::ref(e))){}
   //   optexpr(const qtl::expr &e):base_t(e){
   //     (*(base_t*)this)=std::ref(e);
   //   }
   //   template<typename V/*=std::vector<qtl::interval>*/>
   kleen admits(const V1  &v)const{
     NOTRACE( std::cout << __PRETTY_FUNCTION__ << '(';
	    for( auto i:v ){ std::cout << i << ", "; }
	    std::cout << ')' << '\n'; )
       if( !(*this).has_value() ){ return kleen::T; }
     NOTRACE(
	     if( (*this).value().cachevalue.has_value() ){
	     std::cerr << (*this).value();
	   }
     )
       NOTRACE( std::cerr << (*this).value().stringify() << '=' << (*this).value().eval({},v) << '\n'; )
       return (*this).value().eval({},v);
   }  
 }; // end class optexpr
#if 1
#define X(o,p) \
 qtl::expr operator o(const qtl::expr &l,const qtl::expr &r){	\
   return qtl::expr(qtl::op::p,{l,r}); \
 } \
 // end define
   X(*,multiplies)
   X(/,divides)
   X(+,plus)
   X(-,minus)
   X(<,less)
   X(<=,less_equal)
   X(!=,not_equal_to)
   X(==,equal_to)
   X(>=,greater_equal)
   X(>,greater)
   X(&&,logical_and)
   X(||,logical_or)
#undef X
#endif
   //   double operator~(const operation<double>&x){
   //     return !x;
   //  }
   //  double operator|(const operation<double>&l,operation<double>&r){
   //    return (long)l | (long)r;
   //  }
 namespace literals{
     expr operator""_name(const char *c){
       return expr(qtl::op::name,std::string(c));
     }
     expr operator""_column(unsigned long long int i){
        std::stringstream s;
	s << i;
        return expr(qtl::op::column,s.str());
     }
   }
}
#if __INCLUDE_LEVEL__ + !defined __INCLUDE_LEVEL__ < 1+defined TEST_H
using qtl::expr=optree<qtl::interval,intvec<lex::string>>;
qtl::expr e5(qtl::operation(qtl::op::lit,qtl::interval(5)));
qtl::expr e2(qtl::operation(qtl::op::lit,qtl::interval(2)));
qtl::expr e2p5(qtl::op::plus,{e5,e2});
qtl::expr e2t5(qtl::op::multiplies,{e5,e2});
qtl::expr e(qtl::op::minus,{e2t5,e2p5});
qtl::expr _e(qtl::op::negate,{{e}});
int main(){
#if 0
    std::cout << e.recurse<&qtl::operation<long>::eval>() << std::endl;
#elseif 0
    //std::cout << e.recurse<&decltype(e)::eval>(0) << std::endl;
    //    std::cout << e.recurse<&qtl::operation<long>::eval>() << std::endl;
    std::cout << e.eval() << std::endl; 
    std::cout << e.recurse<&qtl::expr<long>::eval>() << std::endl;
    std::cout << e.recurse<&decltype(e)::eval>() << std::endl;
    //    std::cout << e.recurse<&(e.eval)>() << std::endl;
    std::cout << (long)e << std::endl;
    std::cout << e.re()  << std::endl;
#else
std::cout << e2.stringify() << std::endl;
exit(0);
std::cout << e.eval() << std::endl;
std::cout << e.stringify() << std::endl;
#endif

    //    std::cout << operator+(1,2) << std::endl;
          std::cout << std::operator+(std::string("a"),std::string("b")) << std::endl;

	  //          std::cout << std::operator+(1.2,2.3) << std::endl;
	  //	  std::cout << (1).std::operator+(2) << std::endl;
	  //          std::cout << std::string("a").operator+(std::string("a"),std::string("b")) << std::endl;

	  //	  std::cout << e.inorder(&decltype(e)::evalit ) << std::endl;
	  //	  std::cout << e.traverse(&decltype(e)::evalit ) << std::endl;

	  int x=1;
#ifdef __clang__
#define π M_PI // g++ error: macro names must be identifiers
#define π M_PI // g++ error: macro names must be identifiers
//#define  …  U342_200_246
#define º degree // g++ error: macro names must be identifiers  
	  //#define ⋯  uU+22EF
	    //	    int x…=0;  error: non-ASCII characters are not allowed outside of literals and identifiers

	  //	  int ∞=0; /*∞*/ // non-ASCII characters are not allowed outside of literals and identifiers
	    //	  int \U00e2889e=0; /*∞*/ // expected unqualified-id

	    //#define § section
	    //#define \U00e2889e xx  error: macro name must be an identifier
	    //	  int \U00e28baf/*⋯*/=0;
	    //	    int \u03C0 = 3;
#define ω omegaU+03C9 cf 89
#endif
#define \u03c9 ω omegaU+03C9 cf 89 // clang warning: 'ω' macro redefined 
#ifdef __clang__
#define ¨ U+00A8 DIAERESIS
#define ª U+00AA FEMININE ORDINAL INDICATOR
#define ¯ U+00AF MACRON
#define ˈ U+02C8 MODIFIER LETTER VERTICAL LINE
#define ᐞ U+141E CANADIAN SYLLABICS GLOTTAL STOP
#define ⁔ U+2054 INVERTED UNDERTIE
#define ₍ U+208D SUBSCRIPT LEFT PARENTHESIS
#define ₎ U+208E SUBSCRIPT RIGHT PARENTHESIS
#endif
#define \uFE5F ﹟                    ⟺
//#define ⟺	 U+27FA
//#define \u27FA LONG LEFT RIGHT DOUBLE ARROW
   //#define  ﹌   U+FE4C
#ifdef __clang__
  //	  /*
#define ð U+100B7 LINEAR B IDEOGRAM B176 TREE
	  //#define ð ð 
#define ð U+1F814 LEFTWARDS ARROW WITH EQUILATERAL ARROWHEAD  ð
//#define ð ðð
#endif
//#define \u2202 boundary
#define \u02da interor

	    //#define \u221E infinity
	    //#define \u29DE ⧞ INFINITY NEGATED WITH VERTICAL BAR
	    //#define \u29DD ⧝ TIE OVER INFINITY 
	    //#define \u267E ♾ PERMANENT PAPER SIGN
	    ///#define \u29DC ⧜ INCOMPLETE INFINITY
	    //#define \u00B1 ± PLUS-MINUS SIGN
	    //
	    //#define \u2213  minus-plus sign (∓) 
	    //#define ⋕ incomparable 
#define \u03B4 δ
#define \u1D5F ᵟ
	    //#define \u27e0  ⟠
#define \U0001d789 ð
#define \U0001d6db  ð
	    //#define \u2044 ⁄ Fraction slash
#define \u9650 é
//#define \u2300 null // error: macro name must be an identifier
//#define \u25CB ○ // error: macro name must be an identifier
//#define \u2193 ↓ // error: macro name must be an identifier
//#define \u22A5 ⊥ // error: macro name must be an identifier
//#define \u2349 ⍉ // error: macro name must be an identifier
//#define \u22A3 ⊣ // error: macro name must be an identifier
//#define \u22A2 ⊢ // error: macro name must be an identifier
}
#endif
