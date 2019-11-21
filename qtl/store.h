#pragma once
//#define _LIBCPP_BEGIN_NAMESPACE_STD namespace std{
//#define _LIBCPP_END_NAMESPACE_STD }
#include "bounds.h"
#include "interval.h"
#include "out.h"
//#include <strstream>
#include "container.h"
#include "tree.h"
#include <cmath>
#include <functional>
//#include <optional>
#include <type_traits>
#include <typeinfo>
#include <unordered_set>
#include <cstddef>
#include <algorithm>
#include <variant>
#include "radix_map.h"
#if 0
template<typename T>
  std::shared_ptr<T>::operator void*()const{
   return this->get();
  }
#endif
#if __linux__ || __clang_major__ < 7
template<class InputIt, class Size, class UnaryFunction>
static   InputIt for_each_n(InputIt first, Size n, UnaryFunction f)
{
  for (Size i = 0; i < n; ++first, (void) ++i) {
    f(*first);
  }
  return first;
}
#else
#endif
namespace qtl {
//using std::nullptr_t;
#if 1
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

template<typename T>
std::ostream &
operator<<(std::ostream &os, const std::vector<T> &obj){
  bool next = false;
  os << "{ ";
  for(auto x : obj){
    if(next){
      os << ", ";
    }
    os << x;
    next = true;
  }
  os << "}";
  return os;
}
template<typename T>
std::ostream &
operator<<(std::ostream &os, const std::set<T> &obj){
  bool next = false;
  os << "{";
  if( obj.size()>1 ){ os << "\n"; }
  //  auto qs = qtl::ostringstream();  
  for(auto x : obj){
    if(next){
      os << ", ";
    }
    os << x;
    next = true;
  }
  //  auto rs=qtl::rstr(qs.str());
  //  if( qs.str().size()>20 ){ os << "\n"_r; }
  //os<<rs;
  os << "}\n";
  return os;
}
template<typename T>
#define BASE_T std::vector<T>
  class sample: public BASE_T{
   using base_t=BASE_T;
 #undef BASE_T
   int n=0;
   int lg=0;
   int nl=1;
 public:
   int count=0;

   using base_t::base_t;
   void push_back( const T& v){
     if( ++count >= n ){
       base_t::push_back(v);
       if( lg ){ n += count/lg; }
     }
     if( count > nl ){
       ++lg;
       nl |= count;
     }
   }
   void clear(){
     base_t::clear();
     count=0;
     n=0;
     lg=0;
     nl=1;
   }
  inline auto& write(std::ostream &os=std::cerr)const{
    os << "{";
    os << " count="<< count;
    os << " size()="<< base_t::size();
    os << " base_t="<< *reinterpret_cast<const base_t*>(this);
    os << "}";
    return os;
  }
  friend std::ostream& operator<<(std::ostream &os, const sample &o ){
    return o.write(os);
  }
}; // end class sample

template<typename T>
std::ostream &
operator<<(std::ostream &os, const std::shared_ptr<T> &obj){
  os << "{ ";
  os << "use_count()=" << obj.use_count() << ", ";
  os << "get()=" << obj.get() ;
  if( obj.get() ){
     os << "=" << *obj.get();
  }
  os << " }";
  return os;
}
template<typename T>
std::ostream &
operator<<(std::ostream &os, const std::weak_ptr<T> &obj){
  os << "{ ";
  os << "use_count()=" << obj.use_count() << ", ";
  if( !obj.expired() ){ os << "lock()=" << obj.lock() << " "; }
  os << "}";
  return os;
}
#endif

#if 0
/*
    class lattice:public map<path,shared_ptr<vertex>{};

    class splits{ qtl::interval bounds;  vector< boundary_t>& div; }
    class splits::itetator{
     splits* base;
     bool first;
     std::set<boundary_t>::const_iterator it;
   }
   class facets:public shared_ptr<vector<splits>{}
   class facets::iterator public tuple<int /*d()*/,splits::iterator /*it()*/>{}
   class vertex:public variant< vector<row>>, vector<splits> >{};

   class path:public std::vector<interval>{}

    class kv:public std::pair<path /*path()*/ ,std::shared_ptr<vertex> /*vertex()*/{  }

 class lattice::pve:public tuple< path, shared_ptr<vertex>, requirements > {
  class lattice::pve::iterator{
    {
    pve* r();
#  if 0
    {
    facets f();
    facets::iterator it();
    }
#  else
    range<facets>
#  endif
      }
    kleen e;
  };

  class lattice::itertator{
    stack;
    range<rows>;
    sample<row>pass;
    sample<row>fail;
   ];
 }
   */
#endif
template<class R, class I = typename std::remove_const_t<typename std::remove_pointer_t<std::remove_reference_t<R>>::iterator>>
#define BASE_T std::pair<R *, I>
class range : public BASE_T {
public:
  using base_t = BASE_T;
#undef BASE_T
  using base_t::base_t;
  auto it() const {
    return std::get<1>(*this);
  }
  auto &it(){
    return std::get<1>(*this);
  }
  auto r() const {
    return std::get<0>(*this);
  }
  auto &r(){
    return std::get<0>(*this);
  }
static inline /*const*/ R empty_v={};
range(){}
  range(R *o, const I &i)
    : base_t(o, i){
    assert(o);
  }
  //   range(const R&o):base_t(o,static_cast<I>(const_cast<R>(o)->begin())){}

 range(R *o):base_t(o, o->begin()){}
  auto operator*(){
    return *it();
  }
 range(nullptr_t n):range(&empty_v){}
  auto operator++(){
    return ++it();
  }
  bool operator!=(const I &t)const{
    return it() != t;
  }
  template<typename T>
  bool operator==(T t)const{
    return !(*this != t);
  }
  bool operator!=(std::nullptr_t n) const {
    return it() != end();
  }
  bool operator==(std::nullptr_t n) const {
    return !(it() != end());
  }
  auto begin() const {
    return r()->begin();
  }
  auto end() const {
    return r()->end();
  }
 inline auto& write(std::ostream &os=std::cerr)const{
   os << "{" << r() << "," ;
   if( !r() || it()==end() ){ os << "end()"; }else{ os << *it(); }
   os << "}";
   return os;
 }
  auto operator=(std::nullptr_t n){
    r()=const_cast<R*>(&empty_v);
    it()=r()->end();
  }
#if 1 
friend std::ostream& operator<<(std::ostream &os, const range<R> &o){
      o.write(os);
      return os;
}
#endif
}; // end template class range
 
#define BASE_T std::vector<lex::scalar>
 class row : public BASE_T {
  using base_t = BASE_T;
#undef BASE_T
  template<typename S>
    static inline std::vector<lex::scalar> ss(const std::vector<S> &v){
    std::vector<lex::scalar> s;
    s.reserve(v.size());
    for( auto x:v ){
      s.push_back(x);
    }
    return s;
  }

public:
  using base_t::base_t;
  row(const std::vector<lex::scalar> &v) : base_t(v){}

  template<typename S=lex::string>
    row(const std::vector<S> &v): base_t(ss(v)){}

  auto operator+(const row &r) const {
    auto ret = *this;
    for(auto x : r){
      ret.push_back(x);
    }
    return ret;
  }
  using lex_t=lex::vector<lex::scalar>;
  operator lex_t(){
    lex_t ret;
    ret = *this;
    return ret;
  }
  operator lex::string(){
    lex::string ret;
    ret=(lex_t)*this;
    return ret;    
  }

}; // end class row

#define BASE_T std::vector<row>
class rows : public BASE_T {
  using base_t = BASE_T;
#undef BASE_T
public:
  using base_t::base_t;
#if 0
#  define BASE_T std::tuple<rows *, std::vector<roe>::iterator>
       class iterator:public BASE_T{
         using base_t=BASE_T;
#  undef BASE_T
         public:
         using  base_t::base_t;
	 auto it(){ return std::get<1>(*this); }
	 auto r(){ return std::get<0>(*this); }
	 auto operator*(){ return *it(); }
       }; // end class rows::iterator
#endif
  using lex_t=lex::vector<row::lex_t>;
  operator lex_t(){
    lex_t ret;
    ret=*this;
    return ret;
  }
  operator lex::string(){
    lex::string ret;
    ret=(lex_t)*this;
    return ret;
  }
}; // end class rows
#define NOELIDECOLUMN
//class path;
class store {
  /*
      usage:
      store data;
      auto rows=data[expr];  
      for( auto row:rows ){
        std::cout << row << '\n';
      }
      data[{"col0"}]={"col1","col2","col3"};
      data[{"col0","col1","col2","col3"]=nulptr;
     */
  using boundary_t = interval::b_t;
  #define BASE_T std::set<boundary_t>
  class parts : public BASE_T{
  using base_t = BASE_T;
  #undef BASE_T
  public:
  using base_t::base_t;
  using lex_t=lex::set<boundary_t::lex_t>;
  operator lex_t(){
    lex_t ret;
      ret = *this;
      return ret;
    }
  parts(const base_t&b):base_t(b){}
  operator lex::string(){
    lex::string ret;
    ret = (lex_t)*this;
      return ret;
    }
  
  };// end class parts

  //using interval=basic_interval<lex::string>;
public:
#define BASE_T std::tuple<interval, /*parts*/ std::set<boundary_t>>
  class splits : public BASE_T {
    using base_t = BASE_T;
#undef BASE_T
  public:
    using base_t::base_t;

    //      qtl::interval bounds;
    auto bounds() const {
      return std::get<0>(*(base_t *)this);
    }
    auto &bounds(){
      return std::get<0>(*(base_t *)this);
    }
    auto &div() const {
     /**/NOTRACE(std::cerr << __PRETTY_FUNCTION__ << '\n';)
     /**/NOTRACE(std::cerr << "this=" << this << '\n';)
      return std::get<1>(*(base_t *)this);
    }
    //      /*const*/ std::vector< boundary_t>/*&*/ div;
    auto empty() const {
      return div().empty();
    }
    splits(const interval &b, const std::set<boundary_t> &d)
      : base_t(b, d){
     /**/NOTRACE(std::cerr << __PRETTY_FUNCTION__ << '\n';)
    }

    class iterator : public std::set<boundary_t>::const_iterator {
      using base_v = std::set<boundary_t>;
      using base_t = base_v::const_iterator;
    public:
      bool first;
      //	boundary_t l(){ return first?base.bounds().l():*((base_t)*this); }
      boundary_t l()const{
        return first ? base->bounds().l() : *(*(base_t *)this);
      }
      boundary_t r(const base_t &i)const{
        return i == base->div().end() ? base->bounds().u() : *i;
      }
      //	boundary_t u(){ return r((base_t)*this+(first?0:1)); }
      boundary_t u()const{
        return r(std::next(*(base_t *)this, (first ? 0 : 1)));
      }

      public:
      const splits *base;
      interval operator*(){
        return { l(), u() };
      }
      iterator operator++(){
	NOTRACE(std::cerr << __PRETTY_FUNCTION__ << '\n';)
	NOTRACE(std::cerr << *this << '\n';)
        if(first){
          first = false;
        } else {
          base_t::operator++();
        }
	NOTRACE(std::cerr << "path()=" << path() << '\n'; )
	NOTRACE(std::cerr << __FUNCTION__ << '=' << *this << '\n';)
        return *this;
      }
      bool operator!=(const iterator &e)const{
        return first != e.first || *(base_t *)this != (base_t)e;
      }
      bool operator!=(nullptr_t n)const{
        return first != false || *(base_t *)this != base->div().end();
      }
      iterator(){
       NOTRACE(std::cerr << __PRETTY_FUNCTION__ << '\n';)
      }
      iterator(const splits *b, base_t i, bool f = true)
        : base(b)
        , base_t(i)
        , first(f){
       NOTRACE(std::cerr << __PRETTY_FUNCTION__ << '\n';)
       /**/NOTRACE(std::cerr << "this=" << this << '\n';)
       /**/NOTRACE(std::cerr << "b=" << b << '\n';)
      }
      auto operator=(nullptr_t n){
	first=false;
	*(base_t *)this=base->div().end();
	return *this;
      }
      inline auto& write(std::ostream &os=std::cerr)const{
        //TRACE(
	      os << "{" << first << ":*" << base << ":" << *base /* << "->div().begin()+" << *(base_t*)this-base->div().begin() */ ;
	      if( *(base_t*)this != base->div().end() ){
                os << ":" << l() << "," << u() ;
	      }else{
		os << "end()" ;
              }
             os << "}" ;
        //)
        return os;
      }
      
    }; // end class splits::iterator;
    using lex_t=parts::lex_t;
    splits( const lex_t &l ){
       NOTRACE( std::cerr << __PRETTY_FUNCTION__ << "(" << l << ")" << '\n'; )
	 if( l._this.data()[0]=='f' ){
	   auto debughook=1;
         }
       auto ls=static_cast<lex::set<boundary_t/*,1*/>>(l);
       NOTRACE( std::cerr << qtl::type_name<decltype(ls)>() << ls << '\n'; )
       NOTRACE( std::cerr << "static_cast<lex::set<boundary_t>>(l)" << static_cast<std::set<boundary_t>>(ls) << '\n'; )
	 parts p = static_cast<std::set<boundary_t>>(ls);
       NOTRACE( std::cerr << "(parts)" << p << '\n'; )
	 *this={(interval){},p};
       NOTRACE( std::cerr << __FUNCTION__ << "=" << *this  << '\n'; )
    }
    operator lex_t(){
      // lex::set<boundary_t::lex_t>
      lex_t ret = div();
      return ret;
    }
#if 1
    friend std::ostream& operator<<(std::ostream &os, const iterator &o){
      o.write(os);
      return os;
    }
#endif
    iterator begin() const {
     /**/NOTRACE(std::cerr << __PRETTY_FUNCTION__ << '\n';)
     /**/NOTRACE(std::cerr << "this=" << this << '\n';)
      return iterator(this, div().begin());
    }
    iterator end() const {
      return iterator(this, div().end(), false);
    }
    static inline std::ostream &write(std::ostream &os, const splits &t, int indent = 0){
      os << "{bounds" << t.bounds() << ", ";
      os << "div=" << t.div() << "}";
      return os;
    }

  }; // end class splits;
#if 1
  friend std::ostream &operator<<(std::ostream &os, const splits &o){
    o.write(os, o);
    return os;
  }
#endif
  static int choose(const std::vector<splits> &s){
    return 0;
  }
  class lattice;
  class vertex;
  class trie;
  class segment {
  public:
    int      d;
    qtl::interval interval;
    segment(int d, const qtl::interval &i)
      : d(d)
      , interval(i){}
    std::ostream &write(std::ostream &os = std::cerr) const {
      //	 os << NAMED_MEMBERS( this, d, interval );
      os << "{ d=" << d << ", interval=" << interval << "}";
      return os;
    }
#if 1
    friend std::ostream &operator<<(std::ostream &os, const segment &o){
      o.write(os);
      return os;
    }
#endif
  }; // end class segment

#define BASE_T std::vector<interval>
  class path : public BASE_T {
    using base_t = BASE_T;
#undef BASE_T
  public:
    using base_t::base_t;
    class requirements;
    //       operator base_t(){ return *(base_t*)this; }
    base_t base_v(){
      return *(base_t *)this;
    }
    template<class L>
    path(const std::vector<L> &v){
      for(auto x : v){
      base_t:
        push_back(x);
      }
    }
    base_t base_v() const {
      return *(base_t *)this;
    }
    path operator+=(const segment &s){
      while(size() <= s.d){
        push_back({});
      }
      (*this)[s.d] = (*this)[s.d] && s.interval;
      return *this;
    }
    path operator+(const segment &s) const {
      path ret = *this;
      return ret += s;
    }
    path trim(){
      while( size() && !back().is_point() ){ pop_back(); }
      return *this;
    }
#define BASE_T intvec<lex::scalar>
    class pathspec:public BASE_T{
      using base_t=BASE_T;
      #undef BASE_T
    public:
      using base_t::base_t;
      //    pathspec(const path&p):base_t(p.base_v()){}
      //      operator base_t(){ return *(base_t*)this; }
      kleen satisfies(const requirements &r) const ;
      kleen satisfies(const std::vector<lex::scalar> &p) const {
      auto i   = (*this).begin();
      auto r   = (p).begin();
      auto ret = kleen::T;
      while(i != (*this).end() && r != p.end()){
        ret = ret & ((*i) == *r);
        if(ret == kleen::F){
          break;
        }
        ++i;
        ++r;
      }
      return ret;
    }
      kleen satisfies(const optexpr<interval,intvec<lex::scalar>> &e) const {
        return e.admits(*this);
      }
    }; // end class pathspec
    using expr=optree<interval,intvec<lex::scalar>>;
    using optexpr=optexpr<interval,intvec<lex::scalar>>;

 pathspec operator,(const std::vector<lex::scalar> &v)const{
      return pathspec(*this,v);
    }
    path operator+(const std::vector<lex::scalar> &v){
      NOTRACE(std::cout << __PRETTY_FUNCTION__ << "(" << v << ")" << '\n'; )
      NOTRACE( this->write(std::cout); std::cout << '\n'; )
      path ret = *this;
      ret.reserve(ret.size()+v.size());
      auto j   = v.begin();
      for(auto &i : ret){
        if(!i.is_point()){
	  if( j==v.end() ){
            return ret.trim();
	  }
          i = *j;
	  ++j;
        }
      }
      while(j != v.end()){
        ret.push_back(*j);
        ++j;
      }
      NOTRACE( std::cout << ret << ".trim()\n"; )
      return ret.trim();
    }
#if 1
    //  namespace qtl{//
    template<class T,
             typename = std::enable_if_t<!std::is_same_v<char, T> && !std::is_convertible_v<T, std::string> && !std::is_pointer_v<T>> // ok
             ,
             typename = typename T::iterator
#  if 0
  ,class O, typename=std::enable_if_t<std::is_base_of_v<std::ostream,O>|std::is_same_v<O&,std::ostream&>>
>
  std::ostream& operator<<(O& os, const T& obj)
#  else
             >
    friend std::ostream &operator<<(std::ostream &os, const T &obj)
#  endif
             {
     NOTRACE(std::cout << __PRETTY_FUNCTION__ << has_mapped_type<T>::value << '\n'; )
     NOTRACE(std::cout << __PRETTY_FUNCTION__ << is_mappish<T>{} << '\n'; )
      qtl::ios_base::fmtflags f;
      using namespace qtl::literals;
      os >> f;
      os << qtl::type_name<decltype(obj)>(f.verbosity);
      os << "{"_r;
      auto qs = qtl::ostringstream();
      //  ./qtl/store.h:255:5: error: invalid operands to binary expression ('qtl::ostringstream' and 'qtl::ios_base::fmtflags')
      //  qs<<f;

      //  auto sep=f.fs; 
     if(f.verbosity >= qtl::ios_base::fmtflags::id){
        //    ./qtl/store.h:258:10: error: invalid operands to binary expression ('qtl::ostringstream' and 'qtl::setverbose')
        //      qs << qtl::setverbose(qtl::ios_base::fmtflags::generic);
      }
     if constexpr( /*has_mapped_type<T>::value*/ is_mappish<T>{} ){
        qs << f.rs;
        qtl::rstr<std::string> indent = std::string((f.indent + 1) * 2, ' ');
        for(auto [k, v] : obj){
          if constexpr(std::is_pointer_v<decltype(v/*.get()*/ )>){
            qs << indent << "{"_r << k << ", /*"_r << v << "*/"_r;
	    auto qss = qtl::ostringstream();
	    qss<<f.rs;
            if(v){
	      qss << *v;
	      //  qs << *v;
            }
    	    auto rss=qtl::rstr(qss.str());
    	    if( qss.str().size()>10 ){ qs << "\n"_r; }
    	    qs<<rss;
            qs << "},"_r << f.rs;
          } else {
            qs << indent << "{"_r << k << "/*:*/, "_r << v << "},\n"_r << f.rs;
          }
        }
      }else{
        //      sep=f.fs;
        bool next = false;
        for(auto i : obj){
          if(next){
            qs << f.fs;
          }
          NOTRACE( std::cerr << has_mapped_type<T>::value  << '\n'; );
          NOTRACE( std::cerr <<  qtl::type_name<T>() << '\n'; );
          NOTRACE( std::cerr <<  qtl::type_name<typename T::mapped_type>() << '\n'; );
	            qs << i;
	  //          i.write(qs);
          next = true;
        }
      }
      os << qtl::rstr(qs.str());
      os << "}"_r;
      //  os << f;
      return os;
    }
//}; // end namespace qtl
#endif
#if 0
    bool operator<(const path &p) const { // lexicographical compare for map<>
     /**/NOTRACE(
          //  /* operator<<( (std::cerr << __PRETTY_FUNCTION__ << "(" ), (base_t)*this ) << ", " << (base_t)p << ")\n"; */
          std::cerr << __PRETTY_FUNCTION__ << "(" << *(base_t *)this << ", " << *(base_t *)&p << ")\n";)
      lex::string s0 = *this;
      lex::string s1 = p;
     /**/NOTRACE(std::cerr << s0 << '<' << s1 << '=' << (s0 < s1) << '\n';)
      return s0 < s1;
    }
#else
    class lexicographical_less{
    public:
      /*constexpr*/ bool operator()(const path &lhs, const path &rhs) const 
    {
      lex::string s0 = lhs;
      lex::string s1 = rhs;
	return s0 < s1;
    }
    }; // end class path::lexicographical_less
#endif



    kleen satisfies(const std::vector<lex::scalar> &p) const {
      NOTRACE(std::cerr << __PRETTY_FUNCTION__ << '(' << p << ')' << '\n';)
      auto i   = (*this).begin();
      auto r   = (p).begin();
      auto ret = kleen::T;
      while(i != (*this).end() && r != p.end()){
	NOTRACE( std::cerr << "ret = " << ret << " & (" << *i << " == " << *r << ");\n"; )
        ret = ret & (*i == *r);
        if( ret == kleen::F){
          break;
        }
        ++i;
        ++r;
      }
      return ret;
    }
    kleen satisfies(const optexpr &e) const {
      return e.admits(*this);
    }
#define BASE_T std::tuple<row , optexpr>
    class requirements : public BASE_T {
      using base_t = BASE_T;
#undef BASE_T
      using base_t::base_t;
      using this_t = requirements;

    public:
      auto prefix() const {
        return std::get<0>(*this);
      }
      auto &prefix(){
        return std::get<0>(*this);
      }
      auto expr() const {
        return std::get<1>(*this);
      }
      auto &expr(){
        return std::get<1>(*this);
      }
      requirements(){}
    requirements(const std::vector<lex::scalar> &p, const optexpr &e = {})
        : base_t(p, e){}
      static inline std::ostream &write(std::ostream &os, const requirements &r){
        os << '[' << r.prefix() << ']';
        if(r.expr()){
          os << '[' << (*r.expr()).stringify() << ']';
        }
        return os;
      }
template<typename R>
      bool admits(const R &p)const{
	NOTRACE( std::cerr << __PRETTY_FUNCTION__ << '\n'; )
  	NOTRACE( std::cerr << *this << ": " << p  << '\n'; )
	NOTRACE( std::cerr << "prefix()= " << prefix()  << '\n'; )

#define TEMP
#ifdef  TEMP
	auto pref=prefix();
        auto i   = pref.begin();
	if( !prefix().empty() ){ NOTRACE( std::cerr << "*prefix().begin()=" << *(prefix().begin()) << '\n'; ) }
	if( !pref.empty() ){ NOTRACE( std::cerr << "*pref.begin()=" << *i << '\n'; ) }
#else
	// chris/chris-desktop/didb/fuzz/store.out/crashes/id:000001,sig:11,src:000069,op:flip8,pos:965
	//ERROR: AddressSanitizer: heap-use-after-free on address 
	//  freed by thread T0 here:
        // #8 0x100396ba0 in qtl::store::path::requirements::admits(qtl::row const&) const /Users/dmi/pl/./qtl/store.h:720:9
	// previously allocated by thread T0 here:
	// #10 0x10039699a in qtl::store::path::requirements::admits(qtl::row const&) const /Users/dmi/pl/./qtl/store.h:720:9
	auto i=prefix().begin();
#endif
        auto r   = (p).begin();
        if( !(p).empty() ){ NOTRACE( std::cerr << "*row.begin()=" << *r << '\n'; ) }
        while( i !=
#ifdef TEMP
                  pref.end() 
#else
                  prefix().end()
#endif
//#undef TEMP
           && r != p.end()
        ){
	  NOTRACE( std::cerr << *i << "?=" << *r << '\n'; )
          if( *i != *r ){ return false; }
          ++i;
          ++r;
        }
	if( expr().has_value() && expr().value().cachevalue.has_value() ){
	  NOTRACE( std::cerr <<  expr().value() << '\n'; );
        }
	NOTRACE( 
	      auto t= expr().admits(path(p)) ;
	      std::cerr << "expr().admits(path(p))=" << t <<"; !=F,=T"<< (t!=kleen::F) <<","<<  (t==kleen::T) << "\n";
       );
        return expr().admits(path(p))==kleen::T;
      }
    }; // end class requirements

    friend std::ostream &operator<<(std::ostream &os, const requirements &o){
      o.write(os, o);
      return os;
    }
    kleen satisfies(const requirements &r) const {
     /**/NOTRACE(std::cerr << __PRETTY_FUNCTION__ << '(' << r << ')' << '\n';)
	NOTRACE( std::cerr << *this << '\n';)
	auto ret = satisfies(r.prefix());
      if(ret == kleen::F){
        return ret;
      }
      NOTRACE(std::cerr << "=" << ret << "&&" << satisfies(r.expr()) << '\n';)
	return ret & satisfies(r.expr());
    }
    operator lex::string() const {
      lex::string ret;
      for(auto x : *this){
        lex::string xs = x;
        NOTRACE(std::cerr << "+=" << x << "(" << xs << ")";);
        ret += xs;
        NOTRACE(std::cerr << ret.size() << '\n';);
      }
      return ret;
    }
    using lex_t=lex::vector<interval::lex_t>;
    operator lex_t(){
      NOTRACE(std::cerr << __PRETTY_FUNCTION__ << '\n';)
      lex_t ret;
      for( auto x:*this ){
        NOTRACE(std::cerr << x << '\n';)
	ret.push_back(x); 
      }
      NOTRACE(std::cerr << "=" << ret << '\n';)
      return ret;
    }
    // function 'write' with deduced return type cannot be used before it is defined
    inline /*auto*/ /**/std::ostream/**/ & write(std::ostream &os=std::cerr)const{
      os << (base_t)*this << '\n'; 
        return os;
    }
    // function 'operator<<' with deduced return type cannot be used before it is defined
    friend /*auto*/ /**/std::ostream/**/& operator<<(std::ostream &os, const path &o){
      o.write(os);
      return os;
    }

    path(const lex_t &l){
       NOTRACE(std::cerr << __PRETTY_FUNCTION__ << "(" << l << ")" << "\n"; )
       for( auto x:l ){
         NOTRACE(std::cerr << "x:" << x << "\n"; )
	   interval i=x;
	   NOTRACE(std::cerr << "i:" << i << "\n"; )
	   base_t::push_back(i);
       }
      NOTRACE(std::cerr << "*this:" << *this << "\n"; )
    }
  }; // end class path

  using expr=path::expr;
  using optexpr=path::optexpr;
  friend row /*std::vector<lex::string>*/ operator+(const row /*std::vector<lex::string>*/ &v, const path &p){
#ifndef NOELIDECOLUMN
    NOTRACE(std::cerr << __PRETTY_FUNCTION__ << "(" << v << ", " << p << ')' << '\n'; )
    row /*std::vector<lex::string>*/ ret;
    ret.reserve(std::max(p.size(), v.size()));
    auto j = v.begin();
    for(auto i : p){
      if(i.is_point()){
	NOTRACE(std::cerr << i << ": " << ret << '\n' );
        ret.push_back(i.l().value().raw());
      } else if(j != v.end()){
        ret.push_back(*(j++));
      } else {
        ret.push_back(nullptr);
      }
    }
    for(; j != v.end(); ++j){
      ret.push_back(*j);
    }
    return ret;
#else
    return v;
#endif
  };

  friend /*std::vector<lex::string>*/ row operator%(const row /*std::vector<lex::string>*/ &v, const path &p){
#ifndef NOELIDECOLUMN
    row /*std::vector<lex::string>*/ ret;
    auto j = v.begin();
    for(auto i : p){
      if( j==v.end() ){ 
         return ret;
      }
      if(!i.is_point()){
        ret.push_back(*j);
      }
      ++j;
    }
    while(j != v.end()){
      ret.push_back(*j);
      ++j;
    }
    return ret;
#else
    return v;
#endif
  }
  friend int operator+(int i, const path &p){
#ifndef NOELIDECOLUMN
    for(auto x = p.begin(); x != p.end(); ++x){
      if( !x->is_point() && i-- == 0){
        return std::distance(p.begin(), x);
      }
    }
    return p.size() + i;
#else
    return i;
#endif
  }
  friend int operator%(int i, const path &p){  // 
#ifndef NOELIDECOLUMN
    int ret = i;
    //assert(!p[i].is_point());
#if __linux__ || __clang_major__ < 7
    #else
    std::
    #endif
    for_each_n(p.begin(), std::min(i + 1,(int)p.size()), [&ret](auto &x){ if( x.is_point() ){ --ret; } });
    return ret;
#else
    return i;
#endif
  }

  friend std::optional<splits>& operator^=(std::optional<splits>&l, const splits &r){

    if( !l ){
      l = r;
    }else{
      *l ^= r;
    }
    return l;
  }

  friend splits& operator^=(splits &l, const splits &r){
    if( l.bounds().l()!=r.bounds().l() || l.bounds().u()!= r.bounds().u() ){ NOTRACE( std::cerr << __PRETTY_FUNCTION__ << "(" << l << " != " << r << ")\n"; ) return l=splits(); }
    if( l.div() != r.div() ){ return l=splits(); }
    return l;
  }
  friend std::optional<std::vector<splits>>& operator^=(std::optional<std::vector<splits>> &l,const std::vector<splits> &r){
    NOTRACE( std::cerr << __PRETTY_FUNCTION__ << "(" << l << " ^= " << r << ")\n"; ) 
    if( l ){
      *l ^= r;
       NOTRACE( std::cerr << __FUNCTION__ << "=" << *l << "\n"; ) 
    }else{
      l = r;
    }
    return l;
  }
  friend std::vector<splits>& operator^=(std::vector<splits> &l,const std::vector<splits> &r){
    if( r.size()<l.size() ){ l.resize(r.size()); }
    for( int i=0;i<l.size();i++ ){
      l[i]^=r[i];
    }    
    return l;
  }

#define BASE_T std::vector<splits>*
 class facets /*: public BASE_T*/ {
  using base_t = BASE_T;
#undef BASE_T
   base_t base_;

  public:
      //    using base_t::base_t;
      
    auto base()const{
      return base_;
    }
    facets(){}
    facets(/*const*/ std::vector<splits> *s)
      : base_(s){
     /**/NOTRACE(std::cerr << __PRETTY_FUNCTION__ << '\n';)
     /**/NOTRACE(std::cerr << "this=" << this << '\n';)
     /**/NOTRACE(std::cerr << "s=" << s << " base()=" << base() << '\n';)
    }
    
#define BASE_T std::tuple<int, splits::iterator>
    class iterator : public BASE_T {
      using base_t = BASE_T;
#undef BASE_T
    public:
      using base_t::base_t;
      iterator(){
       /**/NOTRACE(std::cerr << __PRETTY_FUNCTION__ << '\n';)
      }
      auto& d(){
        return std::get<0>(*(base_t *)this);
      }
      auto d() const {
        return std::get<0>(*(base_t *)this);
      }
      auto& it(){
        return std::get<1>(*(base_t *)this);
      }
      auto it() const {
        return std::get<1>(*(base_t *)this);
      }
      auto operator!=(const iterator &i) const {
        return d() != i.d() || it() != i.it();
      }
      template<typename T>
      auto operator!=(T t) const {
        return it() != t;
      }
      template<typename T>
      auto operator==(T t) const {
        return !(*this != t);
      }
      segment operator*() const {
        return { d(), *it() };
      }
      auto operator++(){
     /**/NOTRACE(std::cerr << __PRETTY_FUNCTION__ << '\n';)
        return ++it();
	NOTRACE(std::cerr << __FUNCTION__ << '=' << *this << '\n';)
      }
      iterator &operator=(const iterator &other) = default;
      iterator(const iterator &)                 = default;
      auto operator=(nullptr_t n){
	it()=nullptr;
	return *this;
      }
      inline auto& write(std::ostream &os=std::cerr)const{
        os << "{ d()="<< d() << "," << "it()=" << it() << "}" << '\n'; 
        return os;
      }
    }; // end class facets::iterator
    friend std::ostream& operator<<(std::ostream &os, const iterator &o){
      o.write(os);
      return os;
    }
    iterator begin() const {
     /**/NOTRACE(std::cerr << __PRETTY_FUNCTION__ << '\n';)
     /**/NOTRACE(std::cerr << "this=" << (void *)this << '\n';)
     /**/NOTRACE(std::cerr <<  "*this=" << *this << '\n';)
	/**/NOTRACE(std::cerr <<  "this->base()=" << this->base() << '\n';)
	/**/NOTRACE(std::cerr <<  "*this->base()=" << *this->base() << '\n';)
      int d = 0;
      for(auto x : *this->base() ){
        if(!x.empty()){
          break;
        }
        ++d;
      }
     /**/NOTRACE(std::cerr << "d=" << d << '\n';)
     /**/NOTRACE(std::cerr << "&(**(base_t*)this)[d]=" << &(*this->base())[d] << '\n';)
     /**/NOTRACE(std::cerr << "(**(base_t*)this)[d]=" << (*this->base())[d] << '\n';)
     /**/NOTRACE(std::cerr << "path()=" << path() << '\n';)
     /**/NOTRACE(std::cerr << iterator(d, (*this->base())[d].begin()) << '\n';)
	return iterator(d, (*this->base())[d].begin());
    }
    auto end() const {
      return nullptr;
    }
    inline auto& write(std::ostream &os=std::cerr)const{
      os << base_;
      return os;
    }
  }; // end class facets
    friend 
#if 0
      auto // error: function 'operator<<' with deduced return type cannot be used before it is defined
#else
      std::ostream    
#endif
      & operator<<(std::ostream &os, const facets &o){
    o.write(os);
    return os;
  }


#define BASE_T std::variant<rows, std::vector<splits>>
  class vertex : public BASE_T {
    using base_t = BASE_T;
#undef BASE_T
  public:
    using base_t::base_t;
    auto log() const {
      return std::get<0>(*(base_t *)this);
    }
    auto &log(){
      return std::get<0>(*(base_t *)this);
    }
    auto divs() const {
      return std::get<1>(*(base_t *)this);
    }
    auto &divs(){
      return std::get<1>(*(base_t *)this);
    }
    operator auto(){
      std::tuple<std::vector<splits>,rows> ret;
      if( index()==0 ){
	ret={{},log()};
      }else{
	ret={divs(),{}};
      }
      return ret;
    }
    using lex_t=lex::tuple<lex::vector<splits::lex_t>,rows::lex_t>;
    vertex(const lex_t &l){
            NOTRACE( std::cerr << __PRETTY_FUNCTION__ << "(" << l << ")" << '\n'; )
	    std::tuple<lex::vector<splits::lex_t>,rows::lex_t>st=l;
	    auto [lv,lr]=st;
            NOTRACE( std::cerr << "{lv,lr}:" << lv << "," << lr << "}" << '\n'; )
	    std::vector<splits::lex_t> v = lv;
	    if( v.size()==0  ){
   	      decltype(lr)::std_t ls = lr;
	      rows r;
	      for( auto x:ls ){
  	         NOTRACE( std::cerr << '(' << qtl::type_name<decltype(x)>() << ')' << x << '\n'; )
  	         NOTRACE( std::cerr << qtl::type_name<decltype(r[0])>() << '\n'; )
	         NOTRACE( std::cerr << qtl::type_name<decltype(r)::value_type>() << '\n'; )
	         NOTRACE( std::cerr << qtl::type_name<decltype(x)::std_t>() << '\n'; )
		   //r.push_back(x);
                   decltype(x)::std_t sx=x;
  	         NOTRACE( std::cerr << '(' << qtl::type_name<decltype(sx)>() << ')' << x << '\n'; )
		   r.push_back(sx);
	      }
	      NOTRACE( std::cerr << "r:" << r << '\n'; )
		//	      log()=r;
		*this=r;
	      NOTRACE( std::cerr << "log():" << log() << '\n'; )
	    }else{
	      NOTRACE(std::cerr << "v:" << v << '\n';  )
	      std::vector<splits> ss;
	      for( auto x:v ){
		splits s=x;
	         NOTRACE(std::cerr << s << "=" << x << '\n';  )
		ss.push_back( s);
              }
	      NOTRACE(std::cerr << "ss:" << ss << '\n';  )
	      //divs()=ss;
            NOTRACE( std::cerr << this->index() << '\n'; )
	      *this=ss;
            NOTRACE( std::cerr << this->index() << '\n'; )
            }
	    NOTRACE(std::cerr << __FUNCTION__ << '=' << *this << '\n'; )
    }
    operator lex_t(){
      NOTRACE( std::cerr << __PRETTY_FUNCTION__ << '\n'; )
      std::vector<splits::lex_t> sl;
      NOTRACE( std::cerr << (*this).index() << '\n'; )
      NOTRACE( std::cerr << *this << '\n'; )
      if( (*this).index()==1 ){
	//        for( auto x:std::get<0>((std::tuple<std::vector<splits>,rows>)*this)){
	for( auto x:divs() ){
	  splits::lex_t l=x;
          NOTRACE( std::cerr << l << "=" << x << "\n"; )
          sl.push_back(l);
        }      
      }
      rows::lex_t r;
      if( (*this).index()==0 ){
	r=log();
        NOTRACE( std::cerr << r << "=" << log() << "\n"; )
      };
      return {sl,r};

    }
    operator lex::string(){
#if 0
      lex::string ret;
      auto s=(std::tuple<std::vector<splits>,rows>)*this;
      auto sv=std::get<0>(s);
      std::vector<splits::lex_t> sl;
      for( auto x:sv ){
	splits::lex_t l=x;
	sl.push_back(l);
      }
      lex::vector<splits::lex_t>lvs;
      lvs=sl;
      rows::lex_t r;
      r=std::get<1>(s);
      lex::tuple<lex::vector<splits::lex_t>,rows::lex_t> t{lvs,r};
      ret = (lex::string)t;
      return ret;
#else
      return (lex::string)(lex_t)*this;
#endif
    }
    bool is_leaf()const{
      return (*this).index() == 0;
    }
    bool empty()const{
      switch( (*this).index() ){
      case 0: { return std::get<0>(*(base_t *)this).empty(); };break;
      case 1: { return std::get<1>(*(base_t *)this).empty(); };break;
      default: { assert(!"can never get here"); return true; };break;
      }
    }
    static inline std::ostream &write(std::ostream &os, const vertex &t, int indent = 0){
      switch(t.index()){
      case 0: {
        os << std::get<0>(*(base_t *)&t);
      };break;
      case 1: {
        os << std::get<1>(*(base_t *)&t);
      };break;
      }
      return os;
    }
  }; // end class vertex;
  friend std::ostream &operator<<(std::ostream &os, const vertex &o){
    o.write(os, o);
    return os;
  }



#define GET(n,i)  \
    auto n()const{ return std::get<i>(*this); } \
    auto& n(){ return std::get<i>(*this); } \
// end define
    #define BASE_T std::tuple<path,std::shared_ptr<vertex>>
    class vertex_ptr:public BASE_T{
      using base_t=BASE_T;
      #undef BASE_T
      GET(path,0)
      GET(vp,1)
      auto v()const{ return *vp(); }
      auto& v(){ return *vp(); }
      inline auto& write(std::ostream &os=std::cout )const{
	os << "{path="<<path()<<" v="<< vp() << ":" << v() << "}"; 
	return os;
      }
  }; // end class vertex_ptr

  static inline std::map<path,std::optional<std::vector<splits>>,path::lexicographical_less> confluence;

#define BASE_T std::pair<path, std::shared_ptr<vertex>>
  class kv : public BASE_T , public std::enable_shared_from_this<kv> {
    using base_t = BASE_T;
#undef BASE_T
    using this_t = kv;

  public:
    using base_t::base_t;
    GET(path,0)
    GET(vertex,1)
    kv(const class path &p)
      : base_t(p, root[p]){
      /**/NOTRACE(std::cerr << __PRETTY_FUNCTION__ << "(" << p << ")" << '\n';)
     /**/NOTRACE(std::cerr << "this=" << this << '\n';)
     /**/NOTRACE(std::cerr << "path()=" << path() << '\n';)
     /**/NOTRACE(std::cerr << "vertex()=" << vertex() << '\n';)
    }
    //       #define BASE_T std::tuple<this_t *,facets::iterator>
    //       #define BASE_T std::tuple<std::shared_ptr<kv>,facets,facets::iterator>
    class iterator /*:public BASE_T*/
    {
      /* using base_t=BASE_T;*/
      /*#undef BASE_T*/
    public:
      //      kv* _r;
      std::shared_ptr<kv> _r;
      facets              _f;
      facets::iterator    _it;
      auto it()const{ return _it; }
      auto& it(){ return _it; }
      auto f(){
        return _f;
      }
      auto r()const{ return _r; }
      auto& r(){ return _r; }
      iterator(){
       /**/NOTRACE(std::cerr << __PRETTY_FUNCTION__ << '\n';)
      }
    iterator(std::shared_ptr<kv> k)
        : _r(k)
        , _f(&(k->vertex()->divs()))
        , _it(_f.begin()){
	
       /**/NOTRACE(std::cerr << __PRETTY_FUNCTION__ << '\n';)
       /**/NOTRACE(std::cerr << "this=" << this << '\n';)
       /**/NOTRACE(std::cerr << "k=" << k << " r()=" << r() << '\n';) ///
       /**/NOTRACE(std::cerr << "f=" << f() << '\n';)
       /**/NOTRACE(std::cerr << "it().d()=" << it().d() << '\n';)
       /**/NOTRACE(std::cerr << "it().it().base=" << it().it().base << '\n';)
       /**/NOTRACE(std::cerr << "r()=" << r() << '\n';)
       /**/NOTRACE(std::cerr << "r()->vertex()=" << r()->vertex() << '\n';)
      }
      auto operator*(){
        return *it();
      }
      auto operator++(){
	NOTRACE(std::cerr << __PRETTY_FUNCTION__ << '\n';)
        NOTRACE(std::cerr << it() << '\n';)
        ++it();
	NOTRACE(std::cerr << __FUNCTION__ << '=' << it() << '\n';)
	  //        if(it() == r()->vertex()->divs()[it().d()].end()){
	  // if(it() == it().end() ){
         /**/NOTRACE(std::cerr << "split()\n";)
	  //        }
        return *this;
      }
      auto operator=(nullptr_t n){
	NOTRACE(std::cerr << __PRETTY_FUNCTION__ << '\n';)
        it()=nullptr;
	return *this;
      }
      bool operator!=(const iterator &e)const{ return it()!=e.it(); }
      bool operator!=(nullptr_t n)const{
       /**/NOTRACE(std::cerr << __PRETTY_FUNCTION__ << '\n';)
       /**/NOTRACE(std::cerr << "r()=" << /*(void*)*/ r() << '\n';)
       /**/NOTRACE(std::cerr << "r()->vertex():" << r()->vertex() << '\n';)
        assert(!r()->vertex()->is_leaf());
	NOTRACE(std::cerr << "return:" << (it() != n) << '\n';)
        return it() != n;
      }
      bool operator==(nullptr_t n)const{
        return !(*this != n);
      }
      auto end()const{ return nullptr; }
      // iterator operator=(const iterator &i){ return *this=i; } // infinite recursion
      inline auto& write(std::ostream &os=std::cerr)const{
	os << "{" << it().d() << ":" << !it().it().first << ","<< it().it() << "," << !(it().it()!=nullptr)<<  "}";
	return os;
      }
    }; // end class kv::iterator
    friend std::ostream& operator<<(std::ostream &os, const iterator &o){
      o.write(os);
      return os;
    }
    auto begin(){
     /**/NOTRACE(std::cerr << __PRETTY_FUNCTION__ << '\n';)
	NOTRACE( std::cerr << "vertex():" << vertex() << "\n"; )
	NOTRACE( std::cerr << "*vertex():" << *vertex() << "\n"; )
	//      NOTRACE( std::cerr << "*this:" ; this->write(); std::cerr << "\n"; )
      assert(!vertex()->is_leaf());
     /**/NOTRACE(std::cerr << "this=" << (void *)this << '\n';)
     /**/NOTRACE(std::cerr << "vertex().use_count()=" << vertex().use_count() << '\n';)
     /**/NOTRACE(std::cerr << "vertex().get()=" << (void *)vertex().get() << '\n';)

	auto ret = iterator( shared_from_this() );
     /**/NOTRACE(std::cerr << "this=ret.r()=" << /*(void*)*/ ret.r() << '\n';)
     /**/NOTRACE(std::cerr << "ret.r()->vertex().use_count()=" << ret.r()->vertex().use_count() << '\n';)
     /**/NOTRACE(std::cerr << "ret.r()->vertex().get()=" << (void *)ret.r()->vertex().get() << '\n';)
      return ret;
    }
    nullptr_t end(){
     /**/NOTRACE(std::cerr << __PRETTY_FUNCTION__ << '\n';)
     /**/NOTRACE(std::cerr << "this=" << (void *)this << '\n';)
     /**/NOTRACE(std::cerr << "vertex().use_count()=" << vertex().use_count() << '\n';)
     /**/NOTRACE(std::cerr << "vertex().get()=" << (void *)vertex().get() << '\n';)
     /**/NOTRACE(std::cerr << "path()=" << path() << '\n';)
      assert(!vertex()->is_leaf());
      return nullptr; /*iterator(this,facets(vertex()->divs()).end());*/
    }
    auto this_p(){
      return this;
    }
    inline auto& write(std::ostream &os=std::cout )const{
	os << "{path="<<path()<<" *("<< vertex() << ")=" << *vertex() << "}"; 
	return os;
    }
    friend std::ostream& operator<<(std::ostream &os, const kv &o){
      o.write(os);
      return os;
    }
  }; // end class kv
  //     template<int i,typename T>
  //     inline auto getbase(T *t){ return std::get<i>(*(T::base_t*)t); }

#define BASE_T std::tuple<kv, int>
  class section : public BASE_T {
    using base_t = BASE_T;
#undef BASE_T
    using this_t = section;

  public:
    using base_t::base_t;
    auto kv(){
      return std::get<0>(*this);
    }
    auto i(){
      return std::get<1>(*this);
    }
#define BASE_T std::tuple<section *, facets::iterator>
    class iterator : public BASE_T {
      using base_t = BASE_T;
#undef BASE_T
    public:
      using base_t::base_t;
      auto v() const {
        return std::get<0>(*(base_t *)this);
      }
      auto &v(){
        return std::get<0>(*(base_t *)this);
      }
      auto it() const {
        return std::get<1>(*(base_t *)this);
      }
      auto &it(){
        return std::get<1>(*(base_t *)this);
      }
      auto operator*() const {
        return *it();
      }
      auto operator++(){
        /**/NOTRACE(std::cerr << __PRETTY_FUNCTION__ << '\n';)
        return ++it();
      }
      auto operator!=(const iterator &i) const {
        return it() != i.it();
      }
      auto operator!=(nullptr_t n) const {
        return it() != v()->kv().end();
      }
      template<typename T>
      auto operator==(T t) const {
        return !(*this != t);
      }
    }; // end class section::iterator
  }; // end class section

#define BASE_T std::map<path, std::shared_ptr<vertex>,path::lexicographical_less,questrel::shared_memory_allocator<std::map<path,std::shared_ptr<vertex>>::value_type>>
  class lattice : public BASE_T {
    using base_t = BASE_T;
#undef BASE_T
  using lex_t=lex::vector<lex::tuple<path::lex_t,vertex::lex_t>>;
  std::optional<lex_t> buf;
  public:
    using base_t::base_t;
    base_t *m;
#if 1
  lattice():lattice((std::string)__BASE_FILE__){}
  lattice(std::string file_name){
     NOTRACE(std::cerr << __PRETTY_FUNCTION__ << '(' << file_name << ")" << '\n';);
    const char *map_file_name_c_str = (file_name + ".map").c_str();
    if (access(map_file_name_c_str, F_OK) == 0) { // file already exists
      questrel::shared_memory_allocator<base_t> a(map_file_name_c_str);
	m = a.get_root();
    } else { // file does not exist
      questrel::shared_memory_allocator<base_t> a(map_file_name_c_str);
	base_t *root = reinterpret_cast<base_t*>(a.allocate(sizeof(base_t)));
	a.set_root(root);
	m = new (root) base_t(a);
    }
    NOTRACE( std::cerr << "m=" << m << "\n"; )
	std::cerr << m->size() << ": " << m->max_size() << "\n"; 
    //    std::cerr << std::distance(m->begin(),m->end()) << "\n"; 
	for( auto x:*m ){
	     std::cerr << '{' << x.first << ", " << x.second  << "}" << '\n';
	}

    std::cerr << (*m)[{}] << "\n";
    (*m)[{}]= (*m)[{}];
	std::cerr << m->size() << ": " << m->max_size() << "\n"; 
	for( auto x:*m ){
	     std::cerr << '{' << x.first << ", " << x.second  << "}" << '\n';
	}

  }
#endif

     base_t::mapped_type operator[](const path &p) const {
     /**/NOTRACE(std::cerr << __PRETTY_FUNCTION__ << '(' << p << ")const" << '\n';);
      auto v = (*m)[p];
      if(!v){
        NOTRACE(std::cerr << "!v\n";);
        v = std::make_shared<vertex>();
        NOTRACE(std::cerr << (void *)v.get() << '\n';)
        NOTRACE(std::cerr << *v << '\n';)
        //	   base_t::operator[](p)=v;
        (*m)[p] = v;
        NOTRACE(std::cerr << (void *)base_t::operator[](p).get() << '\n';)
        NOTRACE(std::cerr << *base_t::operator[](p) << '\n';)
      }
      NOTRACE(std::cerr << (void *)v.get() << '\n';)
      NOTRACE(std::cerr << *v << '\n';)
      return v;
    }
#if 1
    base_t::mapped_type &operator[](const path &p){
     /**/NOTRACE(std::cerr << __PRETTY_FUNCTION__ << '(' << p << ')' << '\n';);
      auto v = (*m)[p];
      if(!v){
        NOTRACE(std::cerr << "!v\n";);
        v = std::make_shared<vertex>();
       /**/NOTRACE(std::cerr << v << '\n';)
        //	   base_t::operator[](p)=v;
        (*m)[p] = v;
        NOTRACE(std::cerr << (void *)base_t::operator[](p).get() << '\n';)
        NOTRACE(std::cerr << *base_t::operator[](p) << '\n';)
      }
      NOTRACE(std::cerr << (void *)v.get() << '\n';)
      NOTRACE(std::cerr << *v << '\n';)
	return (*m)[p];
    }
#endif

#if 1
#  define BASE_T std::tuple<std::shared_ptr<kv>, path::requirements>
    class kve : public BASE_T {
      using base_t = BASE_T;
#  undef BASE_T
      using this_t = kve;

    public:
      using base_t::base_t;
      auto k() const {
       /**/NOTRACE(std::cerr << __PRETTY_FUNCTION__ << '\n';)
       /**/NOTRACE(std::cerr << "this=" << this << '\n';)
        return *std::get<0>(*this);
      }
      auto &k(){
       /**/NOTRACE(std::cerr << __PRETTY_FUNCTION__ << '\n';)
       /**/NOTRACE(std::cerr << "this=" << this << '\n';)
        return *std::get<0>(*this);
      }
      auto ksp()const{
	return std::get<0>(*this);
      }
      auto v() const {
        return k().vertex();
      }
      auto req() const {
        return std::get<1>(*this);
      }
      auto& req(){
        return std::get<1>(*this);
      }
      //         auto end(){ return kv().r()->end(); }
      kve(const path &p, const path::requirements &r)
        : base_t( std::make_shared<kv>(p), r){
	 
      }
      //         #define BASE_T std::tuple<const kve *,kv::iterator>
      class iterator /*:public BASE_T*/
      {
        /*     using base_t=BASE_T; */
        /* #undef BASE_T */
      public:
        //     using base_t::base_t;
        std::shared_ptr<kve> _r; 
	std::shared_ptr<kv>  _kv;
        kv::iterator         _it;
        kleen                e;
	std::vector<splits> links;
        auto it()const{ return _it; }
        auto& it(){ return _it; }
        auto r()const{ return _r; }
        auto& r(){ return _r; }
        //	   auto index(){ return it().it().index(); }
        auto path()const{
          return r()->k().path();
        }
        auto begin(){
         /**/NOTRACE(std::cerr << __PRETTY_FUNCTION__ << '\n';)
         /**/NOTRACE(std::cerr << "this==" << this << '\n';)
         /**/NOTRACE(std::cerr << "r()=" << /*(void*)*/ r() << '\n';)
         /**/NOTRACE(std::cerr << "r()->k().this_p()=" << r()->k().this_p() << '\n';)
         /**/NOTRACE(std::cerr << "&_kv=" << &_kv << '\n';)
          auto ret = _kv->begin();
         /**/NOTRACE(std::cerr << "ret.r()=" << /*(void*)*/ ret.r() << '\n';)
         /**/NOTRACE(std::cerr << "ret.k()=" << /*(void*)*/ ret.r() << '\n';)
         /**/NOTRACE(std::cerr << "ret.r()->vertex().use_count()=" << ret.r()->vertex().use_count() << '\n';)
         /**/NOTRACE(std::cerr << "ret.r()->vertex().get()=" << (void *)ret.r()->vertex().get() << '\n';)
          return ret;
        }
        auto end(){
         /**/NOTRACE(std::cerr << __PRETTY_FUNCTION__ << '\n';)
         /**/NOTRACE(std::cerr << "this=" << (void *)this << '\n';)
         /**/NOTRACE(std::cerr << "r()=" << /*(void*)*/ r() << '\n';)
         /**/NOTRACE(std::cerr << "it().r()=" << /*(void*)*/ it().r() << '\n';)
         /**/NOTRACE(std::cerr << "it().r()->vertex().get()=" << (void *)it().r()->vertex().get() << '\n';)
         /**/NOTRACE(std::cerr << "it().r()->vertex().use_count()=" << it().r()->vertex().use_count() << '\n';)
          return r()->k().end();
        }

        iterator(){
         /**/NOTRACE(std::cerr << __PRETTY_FUNCTION__ << '\n';)
	    
        }
        iterator(std::shared_ptr<kve> r)
          : _r(r)
          , _kv( r->ksp() )
          , _it(_kv->begin()){
	  /**/NOTRACE(std::cerr << __PRETTY_FUNCTION__ << "(" << r << ")" << '\n';)
         /**/NOTRACE(std::cerr << "r=" << r << '\n';)
	    /**/NOTRACE(std::cerr << "r->req()=" << r->req() << '\n';)
         /**/NOTRACE(std::cerr << "*this=" << *this << '\n';)
         /**/NOTRACE(std::cerr << "r()=" <<  this->r() << '\n';)
          //     /**/NOTRACE( std::cerr << "&r->k()=" << &r->k() << '\n'; ) // error: taking the address of a temporary object of type 'qtl::store::kv' [-Waddress-of-temporary]
         /**/NOTRACE(std::cerr << "*_kv=" << *_kv << '\n';)
         /**/NOTRACE(std::cerr << "it().r()=" << /* (void*)*/ it().r() << '\n';)
          assert(it().r());
         /**/NOTRACE(std::cerr << "*it().r()->vertex():" << *it().r()->vertex() << '\n';)
	    //	    NOTRACE( std::cerr <<  "_r" << _r << '\n'; )
	    //	    NOTRACE( std::cerr <<  "_r->req():" << _r->req() << '\n'; )
	 NOTRACE( std::cerr << "path():" << path() << "\n"; );
	  confluence[path()] ^= it().r()->vertex()->divs();
	  NOTRACE( std::cerr <<  "confluence[" << path() << "]=" << confluence[path()] << "\n"; )
{
  //TRACE( std::cerr << "confluence:" <<  confluence << '\n'; )
	    //  static std::map<std::string,std::string> mss;
	    //   TRACE( std::cerr << "std::map<std::string,std::string>:" <<  mss << '\n'; )
	    //  static std::map<int,int> mii;
	    //   TRACE( std::cerr <<  mii << '\n'; )
	    //     static std::map<std::vector<int>,std::vector<int>> mvivi;
	    //   TRACE( std::cerr <<  mvivi << '\n'; )
	    //   static  std::map<store::path,std::vector<splits>> mpvs;
	    //   TRACE( std::cerr << "std::map<path,std::vector<splits>>:" <<  mpvs << '\n'; )

     //     static  std::map<splits,store::path> msp;
     //   TRACE( std::cerr << "std::map<path,std::vector<splits>>:" <<  msp << '\n'; )

     //   static  std::map<store::path,splits> mps;
     //   TRACE( std::cerr << "std::map<path,splits>:" <<  mps << '\n'; )

     //     static  std::map<std::vector<splits>,splits> mvss;
     //   TRACE( std::cerr << "mvss:" <<  mvss << '\n'; )
 }
	  TRACE( std::cerr << "confluence:" <<  confluence << '\n'; )
	    satisfy();
	  // links= it().
        }
        //	   iterator(const kve &r):base_t(&r,r.k().begin()){
        //	    /**/NOTRACE( std::cerr << __PRETTY_FUNCTION__ << '\n'; )
        //           }
        iterator(const class path &p, const path::requirements &r)
          : iterator(std::make_shared<kve>(p, r)){
         /**/NOTRACE(std::cerr << __PRETTY_FUNCTION__ << "(" << p << "," << r << ")" << '\n';)
         /**/NOTRACE(std::cerr << "this=" << this << '\n';)
         /**/NOTRACE(std::cerr << "r()=" << this->r() << '\n';)
	 /**/NOTRACE(std::cerr << "r()->req*(=" << this->r()->req() << '\n';)
        }
#  if 0
	   auto operator=(const iterator&i){
	    /**/NOTRACE( std::cerr << __PRETTY_FUNCTION__ << '\n'; )
                *(base_t*)this=i;
                std::get<0>(*(base_t*)this)=std::get<0>(i);
                std::get<1>(*(base_t*)this)=std::get<1>(i);
	   }
#  endif
         void satisfy(){
         NOTRACE( std::cerr << __PRETTY_FUNCTION__ << '\n'; )
	  NOTRACE( std::cerr <<  "r()" << r() << '\n'; )
	 NOTRACE( std::cerr << "r()->req():" << r()->req() << '\n'; )
	   NOTRACE( std::cerr << "path():" << path() << '\n'; )
	   while( it()!=it().end() && (path() + *it()).satisfies(r()->req()) == kleen::F ){
	     NOTRACE( std::cerr << "skip " << *it() << '\n'; )
	     ++it();
           }
	   NOTRACE( if( it()!=it().end() ){ std::cerr << "satisfied:" << *it() << "\n"; } )
	   NOTRACE( if( it()==it().end() ){ std::cerr << "satisfy:end()" << "\n"; } )
	 }
         auto operator++(){
         /**/NOTRACE(std::cerr << __PRETTY_FUNCTION__ << '\n';)
	  ++it();
	  satisfy();
	  if( it() != it().end() ){
           /**/NOTRACE(std::cerr << "split()\n";)
          }
	NOTRACE(std::cerr << __FUNCTION__ << '=' << *this << '\n';)
          return *this;

        }
          auto operator*(){
          return *it();
        }
	bool operator!=(const iterator &e)const{ return it()!=e.it(); }
        template<typename E>
        bool operator!=(E e)const{
         /**/NOTRACE(std::cerr << __PRETTY_FUNCTION__ << '\n';)
         /**/NOTRACE(std::cerr << "this=" << (void *)this << '\n';)
         /**/NOTRACE(std::cerr << "r()=" << /*(void*)*/ r() << '\n';)
         /**/NOTRACE(std::cerr << "r()->v().use_count()=" << r()->v().use_count() << '\n';)
         /**/NOTRACE(std::cerr << "r()->v().get()=" << (void *)r()->v().get() << '\n';)
         /**/NOTRACE(std::cerr << "it().r()=" << /*(void*)*/ it().r() << '\n';)
          assert(it().r());
         /**/NOTRACE(std::cerr << "it().r()->vertex().get()=" << (void *)it().r()->vertex().get() << '\n';)
         /**/NOTRACE(std::cerr << "it().r()->vertex().use_count()=" << it().r()->vertex().use_count() << '\n';)
         /**/NOTRACE(std::cerr << "it().r()->path()=" << it().r()->path() << '\n';)
          return it() != e;
        }
        template<class E>
        bool operator==(const E &e)const{
          return !(*this != e);
        }
        iterator(const iterator &) = default;
        iterator &operator=(const iterator &other) = default; // lldb error: call to implicitly-deleted copy constructor of 'qtl::store::lattice::kve::iterator'
        //copy constructor of 'iterator' is implicitly deleted because field '_it' has a deleted copy constructor
        //copy constructor of 'iterator' is implicitly deleted because base class 'std::__1::tuple<int, qtl::store::splits::iterator>' has a deleted copy constructor
        //copy constructor is implicitly deleted because 'tuple<int, qtl::store::splits::iterator>' has a user-declared move assignment operator
     	inline std::ostream &write(std::ostream &os=std::cout )const{
	  os << "{"<< path()<< ",";
	  if( it()==_r->k().end() ){
  	    os << "end()";
 	  }else{
	    os << *it();
	  }
	  os << "}";
	  return os;
        }
      }; // end class kve::iterator
      friend std::ostream &operator<<(std::ostream &os, const iterator &o){
	o.write(os);
        return os;
     }

#  if 1
      iterator begin(){
       /**/NOTRACE(std::cerr << __PRETTY_FUNCTION__ << '\n';)
       /**/NOTRACE(std::cerr << "this=" << (void *)this << '\n';)
       /**/NOTRACE(std::cerr << "v().use_count()=" << v().use_count() << '\n';)
       /**/NOTRACE(std::cerr << "v().get()=" << (void *)v().get() << '\n';)
        //	     auto it=iterator(this); ///
        iterator it(std::shared_ptr<kve>(this));
       /**/NOTRACE(std::cerr << "&it=" << (void *)&it << '\n';)
       /**/NOTRACE(std::cerr << "it.r()=" << /*(void*)*/ it.r() << '\n';)
       /**/NOTRACE(std::cerr << "it.it().r()=" << /*(void*)*/ it.it().r() << '\n';)
       /**/NOTRACE(assert(it.it().r());)
       /**/NOTRACE(std::cerr << "it.it().r()->vertex().get()=" << (void *)it.it().r()->vertex().get() << '\n';)
       /**/NOTRACE(std::cerr << "it.it().r()->vertex().use_count()=" << it.it().r()->vertex().use_count() << '\n';)

        if(it != it.end() && (path() + *it).satisfies(req()) == kleen::F){
          ++it;
        }
        return it;
      }
#  endif
      std::ostream & write(std::ostream &os)const{
	os << "{/*k:*/" << k() << ", req:" << req() << "}";
        return os;
      }
      friend std::ostream &operator<<(std::ostream &os, const kve &o){
	o.write(os);
        return os;
     }      
    }; // end class kve
#endif
//#if 1
#define BASE_T std::vector<kve::iterator>
    class stack : public BASE_T {
      using base_t = BASE_T;
#undef BASE_T
    public:
      using base_t::base_t;
      std::shared_ptr<vertex> l;
      mutable path::requirements      predicate;
      class leaf {
      public:
        using iterator = range<rows>;
      };
      class branch {
      public:
        using iterator = kve::iterator;
      };
      leaf::iterator   li;
      //    mutable branch::iterator bi;
      mutable path p;
      //	 auto index(){ return base_t::back().index(); }
      auto it()const{ 
	if( empty() ){
             NOTRACE( std::cerr << __PRETTY_FUNCTION__ << " empty()\n"; )
              return branch::iterator();
        }
        return back();
      }
      auto& it(){
	if( empty() ){
             NOTRACE( std::cerr << __PRETTY_FUNCTION__ << " empty()\n"; )
        }
	return back();
      }
      #if 0
      auto path()const{
	if( empty() ){ return p;}
        p=back().path();
	return back().path();
      }
      #endif
      auto path()const{
	return p;
      }
      auto& path(){
	return p;
      }
      auto path(const class path &p){
	path()=p;
	l=root[path()];
	return l;
      }
      auto req0()const{
	return predicate;
      }
      auto req()const{
	if( empty() ){
	  NOTRACE( std::cerr << __PRETTY_FUNCTION__ << " empty()\n"; )
            return predicate;
        }else{
	  //predicate=it().r()->req();
          return it().r()->req();
	}
      }
      auto& req(){
	if( empty() ){
	  NOTRACE( std::cerr << __PRETTY_FUNCTION__ << " empty()\n"; )
            return predicate;
        }else{
	  //predicate=it().r()->req();
          return it().r()->req();
	}
      }
      bool is_leaf() const {
        return l->is_leaf();
      }
      auto leaf_it() const {
        assert(is_leaf());
        return li;
      }
      auto &leaf_it(){
        assert(is_leaf()); 
        return li;
      }
      bool push_back(const branch::iterator &b, bool e = false){
         TRACE(std::cerr << __PRETTY_FUNCTION__ << "(" << b << ", " << e << ") !" << empty() << "&&" << is_leaf() << '\n';)
	 TRACE( std::cerr << *this << "\n"; )
	   if( empty() ){
	     l=root[{}];
	     if( !is_leaf() ){
	       if(  b.it() != b.it().end() ){
		 NOTRACE( std::cerr << "root=" << root << '\n'; )
               }
	       assert( b.it() != b.it().end() );
	       base_t::push_back(b);
	       assert( b.it() != b.it().end() );
   	       NOTRACE( std::cerr << path() << " <- " << path() << "+" << *b.it() << '='; )
	       path() += *b.it();
	       l=root[path()];
	       NOTRACE( std::cerr << *this << '\n'; )
	       return true;
             }
	   }
           if( is_leaf() ){
	    NOTRACE( std::cerr << "leaf()\n"; )
	    leaf_it() = &(l->log());
            return false;
           }
	   NOTRACE( if( !empty() ){ std::cerr << "it():" << it() << "\n"; } ) //7404

   	  base_t::push_back( kve::iterator(p, req()) );

	   NOTRACE( std::cerr << "*this:" << *this << '\n'; )
	   NOTRACE( std::cerr << "b:"  << b << '\n'; )
	   NOTRACE(  std::cerr << b.it() << '\n'; ) 
	   assert( b.it() != b.it().end() );
	 NOTRACE( std::cerr << path() << " <- " << path() << "+" << *b.it() << '='; ) //7434
	   auto p=path()+*b.it();
	  NOTRACE( std::cerr << p << "\n"; )

	  NOTRACE( std::cerr << "l:" << l << " <- "; )
	    path(p);
	NOTRACE( std::cerr << l << "\n"; )

	assert(root[path()]==l);

        if(is_leaf()){
	  NOTRACE( if( is_leaf() ){ std::cerr << "li.r():" << li.r() << "\n";  } )
          leaf_it() = &(l->log());
	  TRACE( std::cerr << "leaf_it():"  << leaf_it() << "\n"; )
	  TRACE(std::cerr << "stack():" << *this << '\n'; )
          return false;
        } else {
          req()  = e ? path::requirements(req().prefix()) : req();
          NOTRACE( std::cerr << __LINE__ << " req(): " << req() << '\n'; );
  	//NOTRACE( std::cerr << __LINE__ << *this << '\n'; )
	  NOTRACE( std::cerr << "branch_it():"  << it() << "\n"; )
	  TRACE(std::cerr << "stack():" << *this << '\n'; )
          return true;
        }
      }
      leaf::iterator push_to_leaf(){	
	TRACE(std::cerr << __PRETTY_FUNCTION__ <<  '\n';)
	  //assert(root[path()]==l);
	while( !is_leaf() ){ 
	  if( it()==it().end() ){
	      NOTRACE( std::cerr << *this << "end()\n"; );
	      return nullptr;
	  }
	  NOTRACE(std::cerr << __LINE__ <<  '\n';)
  	  NOTRACE( std::cerr << *this << '\n'; )
  	  NOTRACE( std::cerr << l << '\n'; )
  	  NOTRACE( std::cerr << *l << '\n'; )
  	  NOTRACE( std::cerr << "it():" << it() << '\n'; )
  	  NOTRACE( std::cerr << "it().it()" << it().it() << '\n'; )
  	  NOTRACE( std::cerr << "it().it().it():" << it().it().it() << '\n'; )
  	  assert( it() != it().end() );
  	  NOTRACE( std::cerr << "*it()" << *it() << '\n'; )
          auto p=path() + *it();
  	  NOTRACE( std::cerr << p << '=' <<path() << "+" << *it() << '\n'; )
	  if( auto e=(p).satisfies(req()); e !=  kleen::F ){
	     NOTRACE(std::cerr << __LINE__ <<  '\n';)
	      l=root[p];
	      NOTRACE( std::cerr << "*l:" << l << '\n' << *l << "\n"; )
	      if( !is_leaf() ){
      	        base_t::push_back(/*bi = */kve::iterator(p, e==kleen::T?path::requirements(req().prefix()) : req()));
              }
              path()=p;
  	      //assert(root[path()]==l);
  	      //assert( it() != it().end() ); // fail
	  }else if( it()!=it().end() ){
	    NOTRACE( std::cerr << "skip it():" << *it() << '\n'; );
	      ++it();
  	      //assert(root[path()]==l);
	    NOTRACE( std::cerr << "++it():" << *it() << '\n'; );
	    NOTRACE(std::cerr << __LINE__ <<  '\n';)
	   }else{
	    NOTRACE( std::cerr << __LINE__ << *this << "end()\n"; );
	      return nullptr;
          }
	}
	assert(root[path()]==l);
	leaf_it()=&(l->log());
	NOTRACE( std::cerr << "leaf_it():"  << leaf_it() << "\n"; )
        NOTRACE(std::cerr << "stack():" << *this << '\n'; )
	return leaf_it();
      }
      bool pop_back(){
	TRACE(std::cerr << __PRETTY_FUNCTION__ << '\n';)
        if( empty() ){
	  NOTRACE(std::cerr << "assert failed !stack().empty()" << '\n';)
	  path()={};
          return false;
        }
	TRACE(std::cerr << *this << '\n'; )
	NOTRACE( std::cerr << "it:{" << it()  << " <- "; ) 
        base_t::pop_back();
        if( empty() ){
	  path({});
	  TRACE( std::cerr << "empty()}\n"; ) 
          return false;
        }
	TRACE( std::cerr << it() << "}\n"; );
	  NOTRACE( std::cerr << "it().it().r():" << it().it().r()  << "\n"; )	  
	  NOTRACE( std::cerr << "it().it().r()->vertex():" << it().it().r()->vertex()  << "\n"; )	  
	  NOTRACE( std::cerr << "*it().it().r()->vertex():" << *it().it().r()->vertex()  << "\n"; ) 

	  TRACE( std::cerr << "l:{" << l  << " <- "; ) 
        l         = it().r()->v();
	  TRACE( std::cerr << l  << "}" << '\n'; ) 

	  NOTRACE( std::cerr << "*it().it().r()->vertex():" << *it().it().r()->vertex()  << "\n"; ) 
	  TRACE( std::cerr << "path():" << path() << "\n"; )
       return true;
      }
      std::optional<leaf::iterator> operator++(){
	  TRACE(std::cerr << __PRETTY_FUNCTION__ << '\n'; )
	if( empty() ){ return {}; }

	while( it() == it().end() /*|| is_leaf()*/ ){
	  NOTRACE(std::cerr << "it():" << it() << " is_leaf():" << is_leaf() << '\n'; )
	  NOTRACE( std::cerr << "it().it():"  << it().it() << '\n');
  	  NOTRACE( if( it() == it().end() ){ std::cerr << "end()\n"; }else{ std::cout << *it() << '\n'; } )
          pop_back();
	  NOTRACE( if( !empty() ){ std::cerr << "it():"  << it() << '\n'; }else{  std::cerr << "it(): empty()" << "\n"; } )
	  NOTRACE( if( empty() || it() == it().end() ){ std::cerr << "end()\n"; }else{ std::cout << *it() << '\n'; } )
	  if( empty() ){ return {}; }

       }
       NOTRACE( std::cerr << "is_leaf():" << is_leaf() << '\n'; );
       NOTRACE( std::cerr << "*l:" << *l << '\n'; );
       NOTRACE( std::cerr << "it()!=it().end() : " <<  std::boolalpha << (it() != it().end())  << '\n'; )
       if( empty() || it()==it().end() ){ 
	   NOTRACE( std::cerr << "end(" << size() << ")\n"; ); 
	   return {};
       }else{
	   NOTRACE( std::cerr << *it() << ")\n"; ); 
       }
       NOTRACE( std::cerr << "path():" << path() <<  "\n"; )
	 
       TRACE( std::cerr << "it():" << it() << '\n'; ) 
       TRACE( std::cerr << "it()!=it().end() : " <<  std::boolalpha << (it() != it().end())  << '\n'; )
       ++it();
       TRACE( std::cerr << "++it():" << it() << '\n'; )
       TRACE( std::cerr << "it()!=it().end() : " <<  std::boolalpha << (it() != it().end())  << '\n'; )

       if( it()==it().end() ){
	 return operator++();
       }
       //push_back(*it());
       TRACE( std::cerr << "path():" << path() <<  " <- "; )
       path() = it().path() + *it();
       TRACE( std::cerr << path() << "=" << it().path() << " + " << *it() << "\n"; )
       l      = root[path()];
      
       push_to_leaf();
      
       if( it()!=it().end() ){
	 return leaf_it();
       }
       return operator++();
      }
      inline auto& write(std::ostream &os=std::cout )const{

	os << "path=" << path() << '\n';
        os << "ptr<vertex>-" << l << '\n';;
	if( l ){ os << "*vertex=" << *l << '\n'; }
	if( is_leaf() ){
	  os << "leaf::iterator=" << leaf_it() << '\n';
        }else if( stack().size() ){
	  os << "branch::iterator=" << it() << '\n';
        }
	os << "stack: size()=" << size() ;
        if( size() > 1 ){ os << ": {\n"; }
	for( auto x:*(base_t*)this ){
	  os << /*x.path() << ":" <<*/ x << "\n";
        }
        if( size()>1 ){ os << "}//stack"; }
	os << "\n";
	return os;
      }
      friend std::ostream& operator<<(std::ostream &os, const stack &o){
	o.write(os);
        return os;
     }

    }; // end class stack

#define BASE_T std::tuple<stack,  range<rows>>
    class iterator : public BASE_T {
      using base_t = BASE_T;
#undef BASE_T
    public:
      using base_t::base_t;
      mutable sample<row>pass;
      mutable sample<row> fail;
      //qtl::tree::optree::hints mutable hints;
      auto stack() const {
        return std::get<0>(*this);
      };
      auto &stack(){
        NOTRACE(std::cerr << __PRETTY_FUNCTION__ << '\n';)
        return std::get<0>(*this);
      };
      auto v() const {
        return stack().l;
      };
      auto it() const { // leaf iterator
        return std::get<1>(*this);
      };
      auto &it(){
        NOTRACE(std::cerr << __PRETTY_FUNCTION__ << '\n';)
        return std::get<1>(*this);
      };
      auto req() const {
        return stack().req();
      }
      auto path()const{
        return stack().path();
      }
      /*std::vector<lex::string>*/ auto operator*(){
       NOTRACE(std::cerr << __PRETTY_FUNCTION__ << stack().predicate << '\n';)

	  assert( it()!=it().end() );

	auto ret=(*it()) + path();
       	while( ret.size() && ret.back()== nullptr/* "\0xff\0xff\0xff"*/ ){ ret.pop_back(); }
        return ret;
      }

      //    iterator(nullptr_t p){}

    iterator(const path::requirements &r, bool lval=false) :  base_t({}, {}){
	/**/TRACE(std::cerr << __PRETTY_FUNCTION__ << "(" << r << ")" << '\n';)
       /**/NOTRACE(std::cerr << (void *)root[{}].get() << '\n';)
        //auto p=root[{}];
	  //TRACE(std::cerr << "p.get()=" << (void *)p.get() << '\n';)
	  stack().path({});
	  NOTRACE( std::cerr << "stack().path()" << stack().path() << "\n"; )
	  NOTRACE( std::cerr << "*root[" << path() << "]:" << *root[path()] << "\n"; )
	  NOTRACE( std::cerr << "*stack().l" << *stack().l << "\n"; )
        //stack().l         = root[stack().path()];
	    stack().req() = r;  // 7290
        stack().predicate = r;
	//TRACE( std::cerr << "stack().req():" << stack().req() << "\n"; );
       /**/NOTRACE(std::cerr << stack().l << '\n');
        //TRACE(std::cerr << "p.get()=" << (void *)p.get() << '\n';)
	/**/NOTRACE(std::cerr << stack().l << " -> " << *(stack().l) << '\n');
        assert(stack().l);

        while(!stack().is_leaf()){
          TRACE( std::cerr << __LINE__ << " r:" <<r );
	  TRACE( std::cerr << "*root[" << path() << "]:" << *root[path()] << "\n"; ) // 7291. 7359
	  TRACE( std::cerr << "stack().path()" << stack().path() << "\n"; )
	  TRACE( std::cerr << "*stack().l" << *stack().l << "\n"; )
          TRACE( std::cerr << kve::iterator(path(), r) << '\n'; )
	    stack().push_back(kve::iterator(stack().path(), r)); // 7393
        }

       /**/NOTRACE(std::cerr << (void *)&(stack().l->log()) << '\n';)
       NOTRACE(std::cerr << (stack().l->log()) << '\n';)
        it() = &(stack().l->log());
	NOTRACE( std::cerr << "stack().req():" << stack().req() << "\n"; );
	if( !lval ){
	  while( !stack().empty() && it()==it().end() ){
	     NOTRACE( std::cerr << __LINE__ << " end()" << '\n'; )
	     it()=(++stack()).value_or(nullptr);
	  }
	  NOTRACE(
             if( it()!=it().end() ){
	       std::cerr << "(" << path() << " + " << *it() << " = " << (path() , *it()) << ").satisfies(" << stack().req() << ")" << '\n';
             }
           )
	  if(it() != it().end() && (path() , *it()).satisfies(stack().req0()) != kleen::T){
	    fail.push_back(*it());
	    NOTRACE( std::cerr << "fail(" << *it() << ")\n"; )
            ++*this;
          }
	}else{
	  NOTRACE( std::cerr << stack().l->log() << "\n"; )
        }
       /**/TRACE(std::cerr << "it().r():" << (void *)it().r() << '\n';)
    }
    iterator operator++(){
        TRACE( std::cerr << __PRETTY_FUNCTION__ << '\n'; )
        while(1){
	  NOTRACE( std::cerr << "it():" << it() << "\n"; )
	  assert( it()!=it().end() );
	  ++it();
	  NOTRACE( std::cerr << "++it():" << it() << "\n"; ) // ++it():{0x606000003638,end()}
          NOTRACE( std::cerr << "it() == it().end():" << (it() == it().end()) << '\n'; )
	while( it()==it().end() ){
	  NOTRACE( std::cerr << "stack().size() : " << stack().size() << '\n'; )
          split(*this); //
	  pass.clear(); fail.clear();
	  NOTRACE( std::cerr << "stack().size() : " << stack().size() << '\n'; )
	    if( stack().empty() ){
              return *this;
	    }
	  it()=(++stack()).value_or(nullptr);
	  NOTRACE( std::cerr << it().r() << "->begin()" << &it().r()->begin()[0]<< "->end()" << &it().r()->end()[0] << ",end()-it()=" << it().end()-it().it() << "\n"; )

	  NOTRACE( std::cerr << "stack().size() : " << stack().size() << '\n'; )  // leaf_it():{0x6060000036f8,end()} // stack().size() : 1
	    if( stack().empty() ){
	      it()=nullptr;
              return *this;
	    }
        }
	assert( it()!=it().end() ); 
	TRACE( std::cout << it() << '\n'; )
	if( (path() , *it()).satisfies(stack().req0()) != kleen::T){
          fail.push_back(*it());
	  NOTRACE( std::cerr<<"fail(" << *it() << ")\n"; )
	    //   return ++*this;  // // //
        } else {
	  NOTRACE( std::cerr<<"pass(" << *it() << ")\n"; )
          pass.push_back(*it());
          return *this;
        }
	}
      }
      bool operator!=(nullptr_t n){
	NOTRACE( std::cerr << __PRETTY_FUNCTION__ << '\n'; )
        NOTRACE( std::cerr << stack().size() << ":"<< it().end()-it().it() << '\n'; )
        return !stack().empty() || it() != it().end();
      }
      auto end()const{ return nullptr; }
      inline auto &write(std::ostream &os=std::cout)const{
	if( it() == it().end() ){
	  os << "end()";
	}else{
	  os << *it();
	}
	return os;
      }
      friend std::ostream &operator<<(std::ostream &os, const iterator &o){
        o.write(os);
        return os;
      }
    }; // end class lattice::iterator;
    //#endif

    operator lex_t()const{
      NOTRACE( std::cout << __PRETTY_FUNCTION__ << '\n'; )
      lex_t ret;
      std::vector<lex::tuple<path::lex_t,vertex::lex_t>>r;
      //      for( auto x:*this ){
      for( auto x:*m ){
	path::lex_t p=x.first;
	vertex::lex_t v=*x.second;;
        NOTRACE( std::cout << "{p,v}:" << "{" << p << "," << v << "}"  << '\n'; )
	lex::tuple<path::lex_t,vertex::lex_t>l{p,v};
        NOTRACE( std::cout << "l:" << l << '\n'; )
	r.push_back(l);
      }
      ret=r;
      return ret;
    }
  lattice(const base_t& b):base_t(b){  }
  lattice(const lex_t& t){
      NOTRACE( std::cout << __PRETTY_FUNCTION__ << "(" << t << ")" << '\n'; )
      std::vector<lex::tuple<path::lex_t,vertex::lex_t>>v;
      base_t b;
      //      lex::vector<lex::string>lvls=t;
      std::vector<lex::tuple<path::lex_t,vertex::lex_t>>svls=t;
      //      v=t; 
      for( auto x:svls ){
        NOTRACE( std::cout << "x=(" << qtl::type_name<decltype(x)>() << ")" << x << '\n'; )
	  lex::tuple<path::lex_t,vertex::lex_t> lt=x;
          NOTRACE( std::cout << "lt=" << lt << '\n'; )
	  std::tuple<path::lex_t,vertex::lex_t> st=lt; 
          NOTRACE( std::cout << "std::get<0>(st)._this.__size" << std::get<0>(st)._this.size() << '\n'; )
          NOTRACE( std::cout << "st=" << "{" << std::get<0>(st) << "," << std::get<1>(st) << "}" << '\n'; )
          auto [lp,lv]=st;
	  NOTRACE( std::cout << "[(" << qtl::type_name<decltype(lp)>() << ")" << lp << ",(" << qtl::type_name<decltype(lv)>() << ")" << lv << "]\n"; )
	  path p=lp;
	  if( lv._this.data()[0]=='f' ){
	    auto a=1;
	  }
	  vertex v=lv;
          NOTRACE( std::cout << "{p,v}:" << "{" << p << "," << v << "}" << '\n'; )
	  auto sv=std::make_shared<vertex>(v);
	  b[p]=sv;
      }   
      //      NOTRACE( std::cout << r << '\n'; )
      *this=b;
    }
    inline std::ostream &write(std::ostream &os=std::cout, int indent = 0)const{
      os << *(base_t *)this;
      return os;
    }
     void save(const std::string &f="store.dat"){
     NOTRACE( std::cout << __PRETTY_FUNCTION__ << "(" << f << ")" << '\n'; )
     std::ofstream o(f, std::ios::binary);
     NOTRACE( std::cout << "*this="<< *this << '\n'; )
     NOTRACE( std::cout << "*m=" << *m << '\n'; )
       auto l=(lex_t)*this;
       //       auto l=(lex_t)*m;
     NOTRACE( std::cout << "l=" << l << '\n'; )
       auto s=(std::string)l;
     NOTRACE( std::cout << "s=" << qtl::visible(s) << '\n'; )
       //       auto v=(std::string_view)l;
       //     NOTRACE( std::cout << "v=" << qtl::visible(v) << '\n'; )
     o.write(s.data(),s.size());
    }
    void get(const std::string &f="store.dat"){
     NOTRACE( std::cout << __PRETTY_FUNCTION__ << "(" << f << ")" << '\n'; )
       std::ifstream i(f, std::ios::binary | std::ios::ate);
     //     std::streambuf sb;
     //     std::stringbuf sb;
     //     std::basic_streambuf<char> sb;
     // i>>sb;
     i.seekg(0,std::ios_base::end);
     //i.seekg(0,std::istringstream::end);
     auto size=i.tellg();
     if( size<=0 ){
       *(base_t*)this=lattice();
       return;
     }
     std::string s1(size,'\0');
     i.seekg(0,std::ios_base::beg);
     //i.seekg(0,std::istrstream::beg);

     i.read(s1.data(),size);
     NOTRACE( std::cerr << "s1:" << qtl::visible(s1) <<  '\n';  )
     auto r=(lex::raw)s1;

     NOTRACE( std::cerr << "r:" << qtl::visible(r) <<  '\n';  )
       buf=(lex_t)r;
     //*(base_t*)this=lattice(*buf);
       *(base_t*)m=lattice(*buf);
       NOTRACE( std::cerr << *this << '\n'; )
	 if( NOTRACE(1+)0 ){
	   std::string b=(std::string)*buf;
	   std::string s2=(lex_t)*this;
           if( s2!=b ){
	     std::cout << "*this" << *this << '\n';
             std::cerr << "buf:" << qtl::visible(*buf) <<  '\n'; 
	     std::cout << "s2:" << qtl::visible(s2) << '\n';
           }
	   assert(s2==(std::string)*buf);
	 }
    }

  }; // end class lattice;
  friend std::ostream &operator<<(std::ostream &os, const lattice &o){
    for( auto x:*(o.m) ){
      os << "{" << x.first << " -> " << x.second << "},\n";
    }
    return os;
  }

  static inline lattice root;

  static inline std::shared_ptr<class lattice> rp = std::make_shared<class lattice>(root);

  friend std::ostream &operator<<(std::ostream &os, const store &o){
    //    o.rp.get()->write(os);
    root.write(os);
    return os;
  }

#if 1
  static std::optional<boundary_t> partcount0(int i, const sample<row> &p, const sample<row> &f, const std::set<boundary_t> &B){
    NOTRACE( std::cerr << __PRETTY_FUNCTION__ << "(\n" << i << ","  << p << ",\n" << f << "," << B << ")" << '\n'; )

    if( p.empty()||f.empty() ){ return {}; }
    std::optional<boundary_t> ret;
    int                       p0 = -1, p1;
    int                       f0, f1;
    boundary_t b0;
    // want all pass on one side, at least one failure on the other sidr
    for(auto b : B){
      auto lt = [b, i](const row &r){ return r.size()<=i || r[i] < b; };
      p1      = std::count_if(p.begin(), p.end(), lt);
      f1      = std::count_if(f.begin(), f.end(), lt);
      if( p1==0 && f1>0 ){  // some fail | all pass
        NOTRACE(std::cerr << p1 << "," << f1 << "," << b << '\n'; )
	b0=b; // possible higher bound
        p0=p1;
        f0=f1;
      }
      if( p0 == 0 && p1 > 0 ){
	ret = b0; // highest possible low bound
        NOTRACE(std::cerr << p0 << "," << p1 << "," << f1 << "," << b << '\n';)
        p0=p1;
      }
      if( p1 == p.size() && f1 < f.size() ){ // all pass | some fail
        NOTRACE(std::cerr << p1 << "," << f1 << "," << b << '\n';)
        if( p0<=0 || f0 < f.size() - f1 ){ // if no low bound or high bound isolates more failures
          ret = b;  // lowest possible high bound
        }
        break;
      }
    }
    NOTRACE( if( ret ){ std::cerr << *ret << '\n'; } )
    return ret;
  }

  static std::tuple<int,boundary_t> partcount(int i, const sample<row> &p, const sample<row> &f, const std::set<boundary_t> &B){
    NOTRACE( std::cerr << __PRETTY_FUNCTION__ << "(\n" << i << ","  << p << ",\n" << f << "," << B << ")" << '\n'; )
    std::tuple<int,boundary_t> ret={-1,{}};
    using val_t=std::remove_cv_t<std::remove_reference_t<decltype(p[0][i])>>;
    if( p.empty()||f.empty() ){ return ret; }
    std::vector<val_t> pi;
    std::vector<val_t> fi;
    fi.reserve(f.size());
    for( auto x:f ){
      if( i<x.size() ){ fi.push_back( x[i] ); }
    }
    if( f.size()<=0 ){ return ret; }
    pi.reserve(p.size());
    for( auto x:p ){
      if( i<x.size() ){ pi.push_back( x[i] ); }
    }
    std::vector<decltype(pi.begin())>pp;
    std::vector<decltype(fi.begin())>fp;
    pp.reserve(B.size());
    fp.reserve(B.size());
    NOTRACE( std::cerr << pi.size() << ":" << fi.size() << "\n"; )
    for( auto b:B ){
      auto bp=[b](val_t x){ return x<b; };
      pp.push_back(std::partition(pp.empty()?pi.begin():pp.back(),pi.end(),bp));
      fp.push_back(std::partition(fp.empty()?fi.begin():fp.back(),fi.end(),bp));
      NOTRACE( std::cerr << b <<":"<< pp.back()-pi.begin() << ":" << fp.back()-fi.begin() << "\n"; )
	if( pp.back()==pi.begin() && fp.back()!=fi.begin() ){
	  ret={fp.back()-fi.begin(),b};
	}else if( pp.back()==pi.end() && fp.back()!=fi.end() ){
	  if( std::get<0>(ret) < fi.end()-fp.back() ){
	    ret={fi.end()-fp.back(),b};
          }
	  break;
	}
    }
    NOTRACE( std::cerr << __FUNCTION__ << "=" << ret << '\n'; )
    return ret;
  }
#endif
  //     auto partcount(int i,const rows &p,const rows &f,const lex::string &s){
  //       return partcount(i,p,f,std::array<boundary_t,2>{{{s,infi},{s,supre}}});
  //     }

  static std::optional<boundary_t> choosesplit(int i, const lattice::iterator &ki, const std::set<boundary_t> &B){
    NOTRACE( std::cerr << __PRETTY_FUNCTION__ << "(" << i << "," << ki.stack().predicate.expr()->stringify() << "," << B << ")" << "\n"; );
    std::optional<boundary_t> ret;
    path p=ki.path();
    for(auto b : B){
     NOTRACE( std::cerr << b << "\n"; );
      auto s0= p + segment(i, interval({}, b)) ;
      auto s1= p + segment(i, interval(b,{})) ;
      auto l0=ki.stack().predicate.expr().admits(s0);
      auto l1=ki.stack().predicate.expr().admits(s1);
      NOTRACE( std::cerr << s0 <<":"<< l0 << ", " << s1 << ":"<< l1 << "\n"; );
    }
    return ret;
  }

  static void split(const lattice::iterator &ki){
   NOTRACE(std::cerr << __PRETTY_FUNCTION__ << '\n';);

    constexpr int threshhold = 2;
    if(ki.fail.size() <= threshhold){
      return;
    }
   NOTRACE(std::cerr << "ki.path():" << ki.path()<< '\n';);
   NOTRACE(std::cerr << "confluence[ki.path()]:" << confluence[ki.path()] << '\n';);

    auto hints=ki.req().expr()->hints();
    int i = 0;
#if 1
    auto p=ki.req().prefix();
    for(auto x : p )
#else // 
    for( auto x:ki.req().prefix() )
#endif
    {
      NOTRACE(std::cerr << "p:" << i << ":" << x << '\n';);
      hints[i].insert({x,infi});
      hints[i].insert({x,supre});
      ++i;
    }

    for(auto [i, x] : 
#if 0
     hints 
#else
         ki.req().expr()->hints() 
#endif
    ){
      NOTRACE(std::cerr << "hint:" << i << ":" << x << '\n';);
      if( i<ki.path().size() && ki.path()[i].is_point() ){ continue; }
      choosesplit( i%ki.path(),ki,x);
      NOTRACE( partcount0(i%ki.path(), ki.pass, ki.fail, x) );
     if( auto b = partcount(i%ki.path(), ki.pass, ki.fail, x); std::get<0>(b)>0 ){
        split(ki, i, std::get<1>(b));
        return;
     }
    }
  }
  
  static void split(const lattice::iterator &ki, int i, const boundary_t &b){
    TRACE( std::cerr << __PRETTY_FUNCTION__ << '(' << ki << "," << i << ',' << b << ")\n"; )
    TRACE( std::cerr << "root:" << root << '\n'; )
    TRACE( std::cerr << "ki.path():" << ki.path() << '\n'; )
    TRACE( std::cerr << "ki.stack():" << ki.stack() << '\n'; )
      boundary_t lb={};
      boundary_t ub={};
      path p=ki.path();
    NOTRACE( std::cerr << "p:" << p << '\n'; )
    NOTRACE( std::cerr << "p.size():" << p.size() << "\n"; )
    NOTRACE( std::cerr << "confluence[p]:" << confluence[p] << "\n"; )

      if( ki.stack().size() ){
        NOTRACE( std::cerr << "ki.stack().it()" << ki.stack().it() << '\n'; )
        NOTRACE( std::cerr << "*ki.stack().it()" << *ki.stack().it() << '\n'; )
        NOTRACE( std::cerr << "ki.stack().it().it().it()" << ki.stack().it().it().it().it() << '\n'; )
	
	p += *ki.stack().it();
        NOTRACE( std::cerr << p << " = " << ki.path() << "+" << *ki.stack().it()<< '\n'; )
      }
    auto r  = std::get<0>(*ki.v());
    auto j=i%p;
    auto rp = std::partition(r.begin(), r.end(), [b, j](const row &r){ return r.size()<=j || r[j] < b; });
    auto r0 = rows(r.begin(), rp);
    auto r1 = rows(rp, r.end());
    auto v0 = std::make_shared<vertex>(r0);
    auto v1 = std::make_shared<vertex>(rows(rp, r.end()));

   /**/NOTRACE(std::cerr << "r0:" << r0 << '\n';)
   /**/NOTRACE(std::cerr << "r1:" << r1 << '\n';)

      auto s0= p + segment(i, interval({}, b)) ;
      auto s1= p + segment(i, interval(b,{})) ;
      NOTRACE( std::cerr << s0 << " = " << p <<"+"<< segment(i, interval({}, b)) << '\n'; )
      NOTRACE( std::cerr << s1 << " = " << p <<"+"<< segment(i, interval(b, {})) << '\n'; )

      root[s0] = std::make_shared<vertex>(r0);
      root[s1] = std::make_shared<vertex>(r1);

      NOTRACE(std::cerr << "root:" << root << '\n';)
      NOTRACE( std::cerr << "stack().path():" <<  stack().path() << '\n'; );

     if( !ki.stack().empty()){
      auto pp=ki.stack().back().path();
      NOTRACE( std::cerr << "ki.stack().back().path():" << pp << '\n'; )
      
       NOTRACE( std::cerr << root[pp] << '\n'; );
       NOTRACE( std::cerr << *root[pp] ; );

       NOTRACE( std::cerr << std::get<1>(*root[pp]) << "[" << i << ']' << '\n'; );

       auto v=std::get<1>(*root[pp]);

	NOTRACE(
	       std::cerr << pp << ", " << i << ", " << b << " " << p << "\n";
	       if( p.size() > i ){
		 std::cerr << (b.ma()<qtl::sign(0)?p[i].u():p[i].l()) << " " << b[nullptr] << "\n";
	       }
	       
	 )
	 if( p.size() > i && (b.ma()<qtl::sign(0)?p[i].u():p[i].l())==b[nullptr] ){
#ifndef NOELIDECOLUMN
	   auto n=i%p;
	   for( auto &x:(b.ma()<qtl::sign(0))?r1:r0 ){
	     if( x.size()>n && x[n]==b.value() ){
	       x.erase(x.begin()+n);
             }
           }
#endif
	 }

       if( v.size()>i && !v[i].div().empty() ){
         //while( v.size()<=i ){ v.push_back({}); }
         NOTRACE( std::cerr << v[i] << '\n'; );
         NOTRACE( std::cerr << v[i].div() << '\n'; );
         v[i].div().insert(b);
         NOTRACE( std::cerr << "s0" << p + segment(i, interval({}, b)) << '\n'; )
         NOTRACE( std::cerr << "s1"  << p + segment(i, interval(b, {})) << '\n'; )

          root[p + segment(i, interval({}, b))] = std::make_shared<vertex>(r0);
          root[p + segment(i, interval(b, {}))] = std::make_shared<vertex>(r1);

         root[pp]=std::make_shared<vertex>(v);

         NOTRACE( std::cerr << std::get<1>(*root[pp])[i] << '\n'; );
          NOTRACE( std::cerr << *root[pp] << '\n'; );
          NOTRACE(std::cerr << "root=" << root << '\n';)
	 return;
       }
    }

    NOTRACE( std::cerr << "s0" << p + segment(i, interval({}, b)) << '\n'; )
    NOTRACE( std::cerr << "s1"  << p + segment(i, interval(b, {})) << '\n'; )

#if 1
      NOTRACE( std::cerr << interval(lex::string(std::string(""))) << '\n'; )
      if( b==interval(lex::scalar(std::string("")))++ ){
#ifndef NOELIDECOLUMN
	   auto n=i%p;
	   for( auto &x:r0 ){
	     if( x.size()>n && x[n]==b.value() ){
	       x.erase(x.begin()+n);
             }
           }
#endif
      }
#endif      
    root[p + segment(i, interval({}, b))] = std::make_shared<vertex>(r0);
    root[p + segment(i, interval(b, {}))] = std::make_shared<vertex>(r1);

    auto v = std::vector<splits>(i + 1);
    v[i]   = { {}, { b } };
    root[p] = std::make_shared<vertex>(v);
    NOTRACE(std::cerr << "["<< p << "]" << '\n';)
    NOTRACE(std::cerr << "root="<< root << '\n';)
}

   static void dumpsharedmap(){
     auto m=root.m;
     std::cerr << "m=" << m << "\n"; 
	std::cerr << m->size() << ": " << m->max_size() << "\n"; 
	for( auto x:*m ){
	     std::cerr << '{' << x.first << ", " << x.second  << "}" << '\n';
	}
       std::cerr << (*m)[{}] << "\n";
   }
   static void dumpconfluence(){
   NOTRACE( std::cout << "confluence:" << qtl::store::confluence << "\n"; );
   NOTRACE( operator<<(std::cout << "confluence:",qtl::store::confluence) << "\n"; ); 
  }

   class lval {
     //      std::vector<lex::string>prefix;
     //      path prefix;
     //      std::shared_ptr<class trie> t;
   public:
     std::shared_ptr<class lattice> l;
     path::requirements  predicate;
     //lval(const std::vector<std::string> &v={}):prefix(v){}
     //     lval(const std::vector<lex::string> &v={},const expr &e={}):prefix(v),predicate(e),t(tp){
     auto prefix(){
       return predicate.prefix();
     }
   lval(const row &p,  const optexpr &e):lval(path::requirements(p,e)){}
   lval(const path::requirements &r) : predicate(r), l(rp){
       NOTRACE(std::cerr << __PRETTY_FUNCTION__ << "(" << &tr << " : " << t.use_count() << ":" << t.get() << ")\n";)
       NOTRACE(std::cerr << __PRETTY_FUNCTION__ << "(";
	       for(auto x
		   : v){ std::cerr << x << ", "; } std::cerr
	       << ")\n";)
      /**/NOTRACE(if(e.has_value()){ std::cerr << __PRETTY_FUNCTION__ << '(' << e.value().stringify() << ")\n"; })
     }
   lval(const expr &e)
       : predicate((std::vector<lex::scalar>){}, e)
       , l(rp){
       //t=&trie;
     }
     operator std::vector<std::shared_ptr<class lattice>>() const {
       std::vector<std::shared_ptr<class lattice>> ret;
       for(auto x : *l){
	 ret.push_back(l);
       }
       return ret;
     }
     auto operator[](const expr &e){
      /**/NOTRACE(std::cerr << __PRETTY_FUNCTION__ << '(' << e.stringify() << ')' << '\n';)
       auto ret = *this;
      /**/NOTRACE(
	   if(ret.predicate.expr().has_value()){
	     std::cerr << ret.predicate.expr()->stringify() << ' := ' << '\n';
	   })
       ret.predicate.expr() = e;
      /**/NOTRACE(std::cerr << ret.predicate.expr().has_value() << ": " << ret.predicate.expr()->stringify() << '\n';)
       return ret;
     }
     auto operator[](const row &v){
       auto ret               = *this;
       ret.predicate.prefix() = ret.predicate.prefix() + v;
       return ret;
     }
     void operator=(const row  &v){
      /**/NOTRACE(std::cerr << __PRETTY_FUNCTION__ << "(";
	     std::cerr << predicate << ",";
	     std::cerr << v;
	     std::cerr << ")\n";
      )
       NOTRACE(std::cerr << "l=" << l << '\n';)
       NOTRACE(std::cerr << "*l=" << *l << '\n';)

      if( v.size() ){     
	auto ll=(*this)[v];

	NOTRACE( std::cerr << "ll.predicate=" << ll.predicate << '\n'; );
	NOTRACE( std::cerr << "ll.l=" << ll.l << '\n'; );
       NOTRACE(std::cerr << "l=" << l << '\n';)
       NOTRACE(std::cerr << "*l=" << *l << '\n';)
	NOTRACE(std::cerr << "l->size()=" << l->size() << '\n';)
	//	return ll=row();
	ll=row();
	NOTRACE( std::cerr << "ll.l=" << ll.l << '\n'; );
       NOTRACE(std::cerr << "l=" << l << '\n';)
       NOTRACE(std::cerr << "*l=" << *l << '\n';)
	NOTRACE(std::cerr << "l->size()=" << l->size() << '\n';)
	return;
      }
      auto p = lattice::iterator(predicate,true);
       NOTRACE(std::cerr << "root:" << root << "\n";);
       NOTRACE(std::cerr << "v " << v << " p.path() " << p.path() << "\n";);
       NOTRACE(std::cerr << "*p.v()" << *p.v() << "\n";);
       NOTRACE(std::cerr << "v%p.path() " << (prefix() + v) % p.path() << "\n";);
       assert(p.v()->is_leaf());
       NOTRACE(std::cerr << "std::get<0>(*p.v()):" << std::get<0>(*p.v()) << "\n";);
       std::get<0>(*p.v()).push_back((prefix() + v) % p.path());
       NOTRACE(std::cerr << "std::get<0>(*p.v()):" << std::get<0>(*p.v()) << "\n";);
       NOTRACE(std::cerr << "*p.v()" << *p.v() << "\n";);
       NOTRACE(std::cerr << "root:" << root << "\n";);
     }
     void operator=(const std::nullptr_t &n){
      auto p = lattice::iterator(predicate,true);
      NOTRACE(std::cerr << __PRETTY_FUNCTION__ << '\n'; )
       assert(p.v()->is_leaf());
      NOTRACE( std::cerr << *p.v() << '\n'; )
      auto d=(prefix()) % p.path();
      NOTRACE( std::cerr <<  prefix() << " % " << p.path()<< "=" << d << "\n"; );
      auto v= std::get<0>(*p.v());
      v.erase(std::remove(v.begin(),v.end(),d),v.end());
      NOTRACE( std::cerr << *p.v() << '\n'; )
      std::get<0>(*p.v())=v;
      NOTRACE( std::cerr << *p.v() << '\n'; )
     }
 #define BASE_T lattice::iterator
     class iterator : public BASE_T {
       using base_t = BASE_T;
 #undef BASE_T
     public:
       using base_t::base_t;
     }; // end class lval::iterator
     iterator begin(){
      /**/NOTRACE(std::cerr << __PRETTY_FUNCTION__ << '(' << predicate << ')' << '\n');
       NOTRACE(std::cerr << iterator(predicate).stack().predicate << '\n';)
       NOTRACE(std::cerr << l << '\n';)
       NOTRACE(std::cerr << *l << '\n';)
       return iterator(predicate);
     }
     auto end(){
       return nullptr;
     }
   }; /* end class lval */

public:
   /* index by expression or vector */
   lval operator[](const expr &e){
    return lval(e);
  }
  lval operator[](const std::vector<lex::scalar> &v){
    assert(!lval(v).predicate.expr().has_value());
    return lval(v);
  }
  lval operator[](const std::string &v){
    std::vector<lex::scalar> c={v};
    return lval(c);
  }
  template<class V=std::vector<lex::string>>
    lval operator[](const V &v){
    std::vector<lex::scalar> c;
    c.reserve(v.size());
    for( auto x:v ){
      c.push_back(x);
    }
    assert(!lval(c).predicate.expr().has_value());
    return lval(c);
  }
  lval operator[](const lex::string &s){
    std::vector<lex::scalar> c;
    c.reserve(1);
    c.push_back(s);
    assert(!lval(c).predicate.expr().has_value());
    return lval(c);
  }
  auto begin(){
   /**/NOTRACE(std::cerr << __PRETTY_FUNCTION__ << '\n';)
    return root.begin();
  }
  auto end(){
    return root.end();
  }
  inline std::ostream& write(std::ostream &os, int indent = 0)const{
    os << root;
    return os;
  }
  inline std::ostream& write()const{
    return write(std::cout);
  }
  void save(const std::string &f="store.dat"){
    NOTRACE( std::cerr << __PRETTY_FUNCTION__ << '\n'; );
    NOTRACE( std::cerr << "rp=" << rp << '\n'; );
    NOTRACE( std::cerr << "*rp=" << *rp << '\n'; );
    NOTRACE( std::cerr << "this=" << this << '\n'; );
    NOTRACE( std::cerr << "*this=" << *this << '\n'; );
    rp->save(f);
 }

  void get(const std::string &f="store.dat"){ rp->get(f); }
}; // end class store
    //using expr=store::expr;
// using optexpr=store::optexpr;
    kleen store::path::pathspec::satisfies(const requirements &r) const {
       /**/NOTRACE(std::cerr << __PRETTY_FUNCTION__ << '(' << r << ')' << '\n';)
	NOTRACE( std::cerr << *this << '\n';)
       	auto ret = satisfies(r.prefix());
        if(ret == kleen::F){
          return ret;
        }
        NOTRACE(std::cerr << "=" << ret << "&&" << satisfies(r.expr()) << '\n';)
	return ret & satisfies(r.expr());
    }

}; // end namespace qtl
#ifndef NDEBUG
/// Explicitly instantiate any STL stuff you need in order to debug
template class std::vector<qtl::store::lattice::kve::iterator>; // stack
template class std::vector<lex::scalar>; // row
//__attribute__(used)
#endif

#if __INCLUDE_LEVEL__ + !defined __INCLUDE_LEVEL__ < 1 + defined TEST_H
namespace qtl{
#define BASE_T std::vector<row>
 class model:public BASE_T{
 public:
  using base_t=BASE_T;
#undef BASE_T
  using base_t::base_t;
  static inline base_t base;
  using requirements=store::path::requirements;
#define BASE_T std::tuple<model::base_t*,requirements>
  class lval:public BASE_T{
     public:
     using base_t=BASE_T;
    #undef BASE_T
     auto& predicate(){ return std::get<1>(*(base_t*)this); }
     auto& base(){ return std::get<0>(*(base_t*)this); }
     auto& cont(){ return std::get<0>(*(base_t*)this); }

     lval(const requirements &p):base_t(&model::base,p){}
     lval(const row  &p):base_t(&model::base,{p,{}}){}
     lval(const expr &p):base_t(&model::base,{(std::vector<lex::scalar>){},p}){}
     auto operator[](const store::optexpr &e){
       auto ret=*this;
       if( e && predicate().expr() ){
         ret.predicate().expr()=/* predicate().expr().value() & */e.value();
       }
     }
     auto operator[](const row &r){
       auto ret=*this;
       ret.predicate().prefix() =  ret.predicate().prefix() + r;
     }
     auto begin(){
       return iterator(this/*,model::base.begin()*/);
     }
     auto end(){
       return iterator(nullptr);
     }
     #define BASE_T std::tuple<lval*,model::base_t::iterator>
     class iterator:public BASE_T{
     public:
      using base_t=BASE_T;
       using base_t::base_t;
      #undef BASE_T
       auto& it(){ return std::get<1>(*(base_t*)this); }
       auto it()const{ return  std::get<1>(*(base_t*)this); }
       auto r()const{ return  std::get<0>(*(base_t*)this); }
       auto& r(){ return  std::get<0>(*(base_t*)this); }
       auto end()const{ return model::base.end(); }
       auto operator*()const{
	 return *it();
       }
       bool satisfy(){
         NOTRACE( std::cerr << __PRETTY_FUNCTION__ << '\n'; )
	   if( r()->predicate().expr().has_value() && r()->predicate().expr().value().cachevalue.has_value() ){
	     NOTRACE( std::cerr << r()->predicate().expr().value() << '\n'; );
          }
         while( it() != end()  && !r()->predicate().admits(*it()) ){
           ++it();
         }
	 if( it() != end() ){
	   NOTRACE( std::cerr << *it() << " satisfies\n"; )
	 }else{
      	   NOTRACE( std::cerr << "satisfy:end()\n"; )
	 }
	 return it() != end();
       }
       auto operator++(){
         ++it();
    	 satisfy();
       }
       auto operator!=(nullptr_t n)const{
	 return it()!=end();
       }
       auto operator!=(const iterator &e)const{
	 return it()!=e.it();
       }
       auto operator!=(const model::base_t::iterator &e)const{
	 return it()!=e;
       }
       template<typename T>
       auto operator==(const T& t)const{
	 return !(*this!=t);
       }
       iterator(lval*l):base_t(l,l->cont()->begin()){
	 satisfy();
       }
    }; // end class model::lval::iterator
     void operator=(const row &r){
       cont()->push_back(predicate().prefix()+r);
     }
     void operator=(nullptr_t n){
       for( auto p=cont()->begin(); p != cont()->end(); ){
	 if( predicate().admits(*p) ){
           p=cont()->erase(p);
         }else{
	   ++p;
         }
       }
     }
     lval operator[]( const expr &t ){
       lval ret=*this;
       ret.predicate().expr()=t;
       return ret;
     }
  }; // end class model::lval
  lval operator[]( const requirements &r ){
    return lval(r);
  }
 }; // end class model
  class test{
  public:
    model m;
    store s;
    class lval{
  public:
      model::lval m;
     store::lval s;
      class iterator{
      public:
        model::lval::iterator m;  
        store::lval::iterator s;  
        std::set<row>srows;
        std::set<row>mrows;
        iterator(const model::lval::iterator &m, const store::lval::iterator &s):m(m),s(s){}
        //iterator(nullptr_t n):m(n),s(n){}
        auto operator*(){ 
	 assert( s!=s.end() );
	 NOTRACE( std::cerr << "*s=" << *s; );
	 assert( m!=m.end() );
         return *m;
        }
        auto operator++(){
	  NOTRACE( std::cerr << __PRETTY_FUNCTION__ << '\n'; )
	    assert( m!=m.end() );
	    assert( s!=s.end() );
	  NOTRACE( std::cerr << "*m=" << *m << '\n'; )
	  NOTRACE( std::cerr << "*s=" << *s << '\n'; )
	    /**/assert( srows.size()==mrows.size() );
  	  srows.insert(*s); mrows.insert(*m);
	  NOTRACE( std::cerr << mrows.size() << " " << srows.size() << '\n'; )
	  if( srows.size()!=mrows.size() ){
	    WARN( std::cerr << srows << " != " << mrows << "\n"; )
          }
	  assert( srows.size()==mrows.size() );
          ++s; ++m; 
 	  NOTRACE( std::cerr << (m.it()!=m.end()) << "," << (s.it()!=s.end()) << "," << std::distance(m.it(),m.end()) << '\n'; ) 
 	  NOTRACE( std::cerr << (m!=m.end()) << "," << (s!=s.end()) << "," << std::distance(m.it(),m.end()) << '\n'; ) 
	  if( m!=m.end() && !(s!=s.end()) ){ WARN( std::cerr << "*++m=" << *m << "\n"; ) }
	  if( s!=s.end() && !(m!=m.end()) ){ WARN( std::cerr << "*++s=" << *s << "\n"; ) }
          return m;
        } 
        auto operator!=(const iterator&e){
	  NOTRACE( std::cerr << __PRETTY_FUNCTION__ << '\n'; )
	    NOTRACE( std::cerr << std::distance(m.it(),e.m.it()) << '\n'; )
	    NOTRACE( std::cerr << std::distance(m.it(),m.end()) <<  '\n'; )
	  if( !(m!=e.m && s!=e.s) ){
            if( srows!=mrows ){
	      WARN( std::cerr << srows << " != " << mrows << "\n"; )
            }
	    assert( srows==mrows );
            if( !(m!=m.end()) ){
                    if( s!=s.end() ){ 
		      WARN( std::cout << "m==m.end() but *s=" << *s << "\n" );
                       assert(!(s!=s.end())); 
                      } 

               mrows.clear();
            }
	    if( !(s!=s.end()) ){
                 if( m!=m.end() ){
		   WARN( std::cout << "s==s.end() but *m=" << *m << "\n" );
                       assert(!(m!=m.end()));
                  } 
                  srows.clear();
               }
	  }
	  NOTRACE( std::cerr << (s!=e.s) << ", " << (s!=s.end()) << '\n' );
	  NOTRACE( std::cerr << "=" << (m!=e.m) << '\n' );
          return m!=e.m;
        }
        auto operator!=(nullptr_t){
	  NOTRACE( std::cerr << __PRETTY_FUNCTION__ << '\n'; )
	  NOTRACE( std::cerr << std::distance(m.it(),m.end()) <<  std::boolalpha << (s.it()!=s.end()) << '\n'; )
	  if( !(m/*.it()*/!=m.end() && s/*.it()*/!=s.end()) ){
            if( srows!=mrows ){
	      TRACE( std::cerr << srows << " != " << mrows << "\n"; )
            }
            if( !(m!=m.end()) ){
                    if( s!=s.end() ){ 
		      TRACE( std::cerr << "m==m.end() but *s=" << *s << "\n"; )
   		      TRACE( std::cerr << "mrows=" << mrows << "\n"; )
   		      TRACE( std::cerr << "srows=" << srows << "\n"; )
                       assert(!(s!=s.end())); 
                      } 
               mrows.clear();
	    }else if( !(s!=s.end()) ){
                 if( m!=m.end() ){
		   TRACE( std::cerr << "s==s.end() but *m=" << *m << "\n" );
   		      TRACE( std::cerr << "mrows=" << mrows << "\n"; )
   		      TRACE( std::cerr << "srows=" << srows << "\n"; )
                   TRACE(std::cerr << "store::root=="<< store::root << '\n';)
                       assert(!(m!=m.end()));
                  } 
                  srows.clear();
	    }else{
	      assert( srows==mrows );
            }
	  }
	  NOTRACE( std::cerr << "=" << (m/*.it()*/!=m.end()) << '\n' );
          return m/*.it()*/!=m.end();
	}
      }; // end class test::lval::iterator
      auto begin(){ return iterator(m.begin(),s.begin()); }
      auto   end(){ return nullptr; }
      template<typename T>
       lval(const T&t):m(t),s(t){}
       auto operator=(const row &r){ m=r;  s=r; return *this; }
       lval operator[](const expr &t){ lval ret=*this; ret.m=ret.m[t]; ret.s=ret.s[t]; return ret; }
    }; // end class test::lval
     template<typename T> 
       lval operator[](T t){ return lval(t); }
     lval operator[](const row &t){ return lval(t); }
     lval operator[](const expr &t){ return lval(t); }
     lval operator[](const store::path::requirements &t){ return lval(t); }

     inline std::ostream &write(std::ostream &os=std::cerr, int indent = 0)const{ 
      os << "{model=" << m.base << ",\nstore=" << s << "\n}"; 
      return os;
    }
  }; // end class test
  std::ostream &operator<<(std::ostream &os, const test &o){
    o.write(os);
    return os;
  }

}; // end namespace qtl
qtl::test pl;
#include "randstream.h"
qtl::randstream rnd(std::cin);
//qtl::randstream rnd;

lex::string rndletter(const std::string &l/*="etaoinshrdlcumwfgypbvkjxqz "*/){
 TRACE( std::cout << __PRETTY_FUNCTION__  << '\n'; )      
  lex::string ret;
  ret += l[std::min(rnd(0,l.size()-1),rnd(0,l.size()-1))];
 TRACE( std::cout << __FUNCTION__ << "=" << ret << '\n'; )      
  return ret;
}

char rndletter(const std::vector<std::tuple<char,double>>&v={
  {'j', 0}, // 1856837400
  {'q', 0.000658531709342035}, // 2645182452
  {'z', 0.00159665182779566}, // 2869381994
  {'x', 0.00261428483608388}, // 8033735169
  {'k', 0.00546346772804997}, // 15871199801
  {'w', 0.0110922257088303}, // 18823068302
  {'b', 0.0177678707297557}, // 19927128290
  {'v', 0.0248350731972909}, // 31403271727
  {'p', 0.0359723166679975}, // 43988161156
  {'y', 0.0515728204907197}, // 53654593742
  {'g', 0.0706015471476535}, // 54403556552
  {'f', 0.0898958952098342}, // 55683243089
  {'m', 0.109644087083343}, // 61046421702
  {'c', 0.131294342745273}, // 80192822158
  {'u', 0.159734913929734}, // 88471612094
  {'d', 0.191111577250123}, // 112406534620
  {'l', 0.230976816599849}, // 127031390303
  {'h', 0.276028795085596}, // 148835849712
  {'s', 0.328813775893403}, // 182348473613
  {'a', 0.393484086834081}, // 199634202053
  {'r', 0.464284821632451}, // 202746627058
  {'t', 0.53618938520912}, // 211685951157
  {'i', 0.611264300901314}, // 215475320731
  {'o', 0.68768312534012}, // 215513933277
  {'n', 0.764115643808311}, // 240780091585
  {'e', 0.849508864176989}, // 424334265762
  {'_', 1}, // 721541698508
 }){
  return std::get<0>(v[ rnd( (std::function<double(int)>) [&v](int i){ return std::get<1>(v[i]); },0,(int)v.size()-1) ]);
}
  static const  std::vector<std::tuple<char,double>>firstletter={
  {'x', 0}, // 197814014
  {'z', 0.000274154653028423}, // 310614372
  {'q', 0.000704641723480882}, // 1618537734
  {'k', 0.00294780762414442}, // 3329551342
  {'j', 0.00756230370785633}, // 3699092905
  {'y', 0.012688955310458}, // 5597531872
  {'v', 0.0204466938910204}, // 6021346133
  {'u', 0.0287918056779773}, // 8756866813
  {'g', 0.0409281338085724}, // 12144016371
  {'n', 0.0577587846165732}, // 16865281870
  {'l', 0.0811327377850096}, // 17849942291
  {'e', 0.105871352792167}, // 20666094536
  {'r', 0.134512933145365}, // 20923663849
  {'d', 0.163511484292535}, // 23443194288
  {'m', 0.196001906310384}, // 28321763080
  {'f', 0.235253640671078}, // 29853272312
  {'h', 0.276627926278868}, // 31166010615
  {'p', 0.3198215638461}, // 32017544261
  {'b', 0.364195360020605}, // 32739156230
  {'c', 0.409569253584481}, // 38719371814
  {'w', 0.463231255231875}, // 40841702982
  {'s', 0.519834640824769}, // 49164447263
  {'i', 0.587972694889644}, // 50371493120
  {'o', 0.657783619503091}, // 56635008916
  {'a', 0.736275283994706}, // 71583139170
  {'t', 0.835483880418196}, // 118705240355
    //  {'*', 1}, // 721541698508
  };
  static const  std::vector<std::tuple<char,double>>aftervowel={
  {'j', 0}, // 702148282
  {'h', 0.000519554901361867}, // 1818119757
  {'q', 0.00186487334082534}, // 2032787446
  {'z', 0.00336903522239805}, // 2428152560
  {'k', 0.00516574767779129}, // 6569807805
  {'x', 0.0100270795838817}, // 7822482438
  {'y', 0.0158153285843685}, // 11337207914
  {'w', 0.0242043001028907}, // 14326768892
  {'b', 0.0348053984596582}, // 15246312147
  {'i', 0.0460869132147805}, // 20061499829
  {'g', 0.060931428016039}, // 22589847566
  {'p', 0.077646794733881}, // 23221457180
  {'v', 0.0948295212375595}, // 26182493335
  {'e', 0.114203267625326}, // 27090173912
  {'u', 0.134248652618128}, // 29278739085
  {'o', 0.155913467283739}, // 31946059253
  {'a', 0.17955196657169}, // 33020986674
  {'f', 0.203985858833612}, // 46064930414
  {'m', 0.238071622469343}, // 46832599605
  {'c', 0.272725423229188}, // 55767848946
  {'d', 0.313990865303292}, // 59732713813
  {'l', 0.358190110717368}, // 77839834065
  {'t', 0.415787726985074}, // 109117254393
  {'s', 0.496529082901449}, // 115130853334
  {'r', 0.581720203753118}, // 148247229494
  {' ', 0.691415800354927}, // 190766841729
  {'n', 0.832573800991643}, // 226266786604
    //  {'*', 1}, // 1351441936472
  };
  static const  std::vector<std::tuple<char,double>>afterconsonant={
  {'x', 0}, // 211252731
  {'z', 9.64728913117558e-05}, // 441229434
  {'q', 0.000297969359680866}, // 612395006
  {'j', 0.000577632096224139}, // 1154689118
  {'w', 0.00110494455865891}, // 4496299410
  {'b', 0.0031582718800749}, // 4680816143
  {'v', 0.00529586255109894}, // 5220778392
  {'k', 0.00768003805496109}, // 9301391996
  {'f', 0.011927709433364}, // 9618312675
  {'m', 0.0163201091382358}, // 14213822097
  {'n', 0.0228111424507874}, // 14513304981
  {'p', 0.0294389407631623}, // 20766703976
  {'c', 0.0389224819514656}, // 24424973212
  {'g', 0.0500766467985644}, // 31813708986
  {'y', 0.0646050293416057}, // 42317385828
  {'l', 0.083930131290618}, // 49191556238
  {'d', 0.106394464111029}, // 52673820807
  {'r', 0.130449044450104}, // 54499397564
  {'u', 0.155337311848399}, // 59192873009
  {'s', 0.182368951047552}, // 67217620279
  {'t', 0.213065255633602}, // 102568696764
  {'h', 0.259905354164218}, // 147017729955
  {'a', 0.327044015121221}, // 166613215379
  {'o', 0.403131356299122}, // 183567874024
  {'i', 0.486961389999597}, // 195413820902
  {'e', 0.576201118240997}, // 397244091850
  {' ', 0.757610778205498}, // 530774856779
    //  {'*', 1}, // 2189762617535
  };

lex::string rndstring(){
 TRACE( std::cout << __PRETTY_FUNCTION__  << '\n'; )      
  lex::string ret;
  char c=' ';
 constexpr double l=4;
  double p=exp(-l);
  double s=p;
  double u=rnd(0,262143)/262144.0;
  int i=0;
  while( ret.empty()|| s<u ){
    s += (p *= l/++i);
    switch( c ){
    case ' ':{
      c=rndletter( firstletter );
    };break;
    case 'a':case 'e':case'i':case'o':case'u':{
      c=rndletter( aftervowel );
    };break;
    default:{
      c=rndletter( afterconsonant );
    };break;
    }
    ret += c;
  }
 TRACE( std::cout << __FUNCTION__ << "=" << ret << '\n'; )      
  return ret;
}

qtl::row rndrow(){
 TRACE( std::cout << __PRETTY_FUNCTION__  << '\n'; )      
  qtl::row ret;
  for( auto i=rnd(0,3)+rnd(0,3); i>0; --i ){
    ret.push_back(rndstring());
   TRACE( std::cout << "ret=" << ret  << '\n'; )      
  }
  if( ret.size() > 1  && ret[0].size()>1  ){
    std::string s=ret[0];
   TRACE( std::cout << "s=" << s  << '\n'; )      
    auto p=std::find(s.begin(),s.end(),' ');
    if( p==s.end() ){ --p; }
     s.erase(p,s.end());
   TRACE( std::cout << "s=" << s  << '\n'; )      
    ret[0]=s;
   TRACE( std::cout << "ret=" << ret  << '\n'; )      
  }
 TRACE( std::cout << __FUNCTION__ << "=" << ret << '\n'; )      
  return ret;
}

int main( int argc, char *argv[] ){ 
std::ifstream in;
 std::streambuf *cinbuf;
if( argc==2 && std::strcmp(argv[1],"dict")==0 ){
  std::cout << "\"@\"\n\"?\"\n";
   exit(0);
   for( auto i='@'; i<='_'; ++i ){ std::cout << '"' <<  i << '"'<< '\n'; }
   exit(0);
 }else if(  argc==2 && std::strcmp(argv[1],"fuzz")==0 ){
  for( auto i='@'; i<='_'; ++i ){ std::cout << i;  }
   std::cout << '\n';
   exit(0);
 }
if( argc==1 || argv[1][0] == '<' ){
    if( argc>1 && argv[1][0] == '<' ){
      in = std::ifstream(argv[1][1]?argv[1]+1:argv[2]);
      cinbuf = std::cin.rdbuf(); 
      std::cin.rdbuf(in.rdbuf()); //redirect std::cin
  }
}
  using namespace lex::literals;
#if 0
  {
//    TRACE( std::cout << std::boolalpha << (std::is_same_v<lex::string,lex::interval<lex::string>>) << "\n"; )
{
    auto t=lex::tuple{"AA\0BB"_s,"YY\0ZZ"_s};
    TRACE( std::cout << decltype(t)::depth << "\n"; )
    TRACE( std::cout << "lt:" << qtl::visible(t._this) << "\n" );
    decltype(t)::std_t st=t;
    auto [t0,t1]=st;
    TRACE( std::cout << qtl::visible(t0._this)<< "," << qtl::visible(t1._this) << "\n" );
    auto lv=lex::vector{"c\0d"_s,"c\0d"_s};
    TRACE( std::cout << decltype(lv)::depth << "\n"; )
    TRACE( std::cout << "lv:" << qtl::visible(lv._this) << "\n" );
    auto sv=(decltype(lv)::std_t)lv;
    TRACE( std::cout << sv << "\n" );

    auto b0 = --"c\0d"_s;
    TRACE( std::cout << b0 << "\n" );
    TRACE( std::cout << "b0.value()._this:"<< qtl::visible(b0.value()._this) << "\n" );
    decltype(b0)::lex_t l0=b0;
    TRACE( std::cout << "l0._this" << qtl::visible(l0._this) << "\n" );
 
   auto b1 = "c\0d"_s++;
    TRACE( std::cout << b1 << "\n" );
    TRACE( std::cout << "b1.value()._this:"<<qtl::visible(b1.value()._this) << "\n" );
    decltype(b0)::lex_t l1=b1;
    TRACE( std::cout << "l1._this" << qtl::visible(l1._this) << "\n" );
}


{    
    auto cd_ef = ("cd"_s <= x::x <= "ef"_s);
    TRACE( std::cout << "cd_ef=" << cd_ef << '\n'; )
     lex::interval li=cd_ef;
    TRACE( std::cout << "lex::interval="<<qtl::visible(li._this) << '\n'; )
      qtl::interval qi=li;
    TRACE( std::cout << "qtl::interval="<<(qtl::interval)li << '\n'; )
    NOTRACE( std::cout << qtl::type_name<decltype(cd_ef)>() << '\n'; )
 }
#endif
#if 0
    auto cd_ef = ("cd"_s < x::x < "ef"_s);
    TRACE( std::cout << "cd_ef=" << cd_ef << '\n'; )
     lex::interval li=cd_ef;
    TRACE( std::cout << "lex::interval="<<qtl::visible(li._this) << '\n'; )
      qtl::interval qi=li;
    TRACE( std::cout << "qtl::interval="<<(qtl::interval)li << '\n'; )
    NOTRACE( std::cout << qtl::type_name<decltype(cd_ef)>() << '\n'; )
//exit(1);
     qtl::store::path p{{"ab"_s},{cd_ef},(1 <= x::x < 2)};
      TRACE( std::cout << qtl::type_name<decltype(p)>() << '\n'; )
    TRACE( std::cout << "p=" << p << '\n'; )
    qtl::store::path::lex_t lp=p;
    TRACE( std::cout << lp << '\n'; )
    qtl::store::path p2=lp;
    TRACE( std::cout << "p2=" << lp << '\n'; )
    assert(!qtl::store::path::lexicographical_less()(p2,p));
    assert(!qtl::store::path::lexicographical_less()(p,p2));
    //    exit(1);
  }
#endif
  std::set<qtl::row> uniq;  
#if 0
  pl[{ "12"_s, "34"_s }] = { "56"_s, "78"_s };
  uniq.insert({"12"_s, "34"_s,"56"_s, "78"_s});
  std::cerr << &pl << "\n";
  pl[{ "12"_s }] = { "56"_s, "78"_s, "12"_s };
  uniq.insert({"12"_s, "56"_s, "78"_s, "12"_s});
  std::cerr << &pl << "\n";
  pl[{ "12"_s, "34"_s, "56"_s }] = {{ "xyz"_s }};
  uniq.insert({"12"_s, "56"_s, "78"_s, "xyz"_s});
  pl[{ "foo"_s }]                = {{ "bar"_s }};
  uniq.insert({"foo"_s,"bar"_s});
  std::cerr << "save:" << qtl::store::root << "\n";
  qtl::store::root.save();
  qtl::store::root.get();
  std::cerr << "get:" << qtl::store::root << "\n";
  qtl::store::root.save("s2");
#else
  {
    static std::vector<qtl::row> data={
      {"A0"_s,"a1"_s,"12"_s},
      {"A0"_s,"a1"_s,"22"_s},
      {"A0"_s,"a1"_s,"32"_s},
      {"A0"_s,"a1"_s,"42"_s},

      {"A0"_s,"b1"_s,"12"_s},
      {"A0"_s,"b1"_s,"22"_s},
      {"A0"_s,"b1"_s,"32"_s},

      {"B0"_s,"a1"_s,"12"_s},
      {"B0"_s,"a1"_s,"22"_s},
      {"B0"_s,"a1"_s,"32"_s},

      {"B0"_s,"b1"_s,"12"_s},
      {"B0"_s,"b1"_s,"22"_s},
      {"B0"_s,"b1"_s,"32"_s},
      {"B0"_s,"b1"_s,"42"_s},

    };
    for( auto d:data ){
      pl[qtl::row()]=d;
    }
    static std::vector<qtl::expr>queries={
      (qtl::expr(qtl::op::column,std::string("1")) < qtl::expr(qtl::op::lit,qtl::interval("b"_s))),
      (qtl::expr(qtl::op::column,std::string("0")) <= qtl::expr(qtl::op::lit,qtl::interval("A0"_s))),
      (qtl::expr(qtl::op::column,std::string("1")) <  qtl::expr(qtl::op::lit,qtl::interval("b"_s))),
      qtl::expr(qtl::op::lit,qtl::kleen::T),
    };
    for( auto q:queries ){
      TRACE( std::cout << "root:" << qtl::store::root << "\n"; );
      std::cout <<  q.stringify() << "\n"; 
      for( auto r: pl[q] ){
	std::cout << r << "\n";	
      }
     TRACE( qtl::store::dumpconfluence(); )
    }
    exit(0);
  }
#endif

#if 0
  std::cerr << &pl << "\n";
  std::cerr << __LINE__ << '\n';
  auto l = pl[{ "12"_s }];
  auto b = l.begin();
  //  assert(b.it().r());
  // auto e = l.end();
  // std::cerr << (b != e) << '\n';
  for( auto x : pl[{ "12"_s }] ){ 
    //    std::cerr << x.first << ":" << x.second  << "\n";
    std::cerr << x << "\n";
  }
#endif

  std::vector<std::string>ops={"<","<=","=",">=",">","!="};
  std::deque<long>pos={0,0,0};
  while( !std::cin.eof() ){
    auto r=rndrow();
    TRACE( std::cout << "r=" << r << '\n'; )
      if( r.size()>1 && !uniq.count(r) ){
	  int n=1+(rnd(0,rnd(0,std::max(r.size(),(size_t)2)-2)));
          auto k=qtl::row(r.begin(),r.begin()+n);
          auto v=qtl::row(r.begin()+n,r.end());
          std::cout << '[' << k <<  "]=" <<  v << '\n';
	  pl[k]=v;      
	  uniq.insert(r);
    }else if( pl.m.base.size() ){
	int o=rnd(0,ops.size()+1);
	int m=rnd(0,pl.m.base.size()-1);
        r=pl.m.base[m];
        TRACE( std::cout << "m[" << m << "]==" << r << '\n'; )
	if( o>ops.size() ){
	    TRACE( std::cout << '[' << r <<  "]=nullptr" << '\n'; )
	     //pl[r]=nullptr;      
	    //if( rnd(0,log(pl.m.base.size())) > r.size() ){
	    // int n=rnd(0,r.size()-1); 
	    //  NOTRACE( std::cout << '[' << "col."<<n << "=" << r[n] <<  "]=nullptr" << '\n'; )
	    //}
	}else if( o==ops.size() ){
	    int n=rnd(1,r.size()-1); 
	    while( ((r[n]=rndstring()), uniq.count(r)) ){}
	    //n=1+(rnd(0,rnd(0,std::max(r.size(),(size_t)2)-2)));
          auto k=qtl::row(r.begin(),r.begin()+n);
          auto v=qtl::row(r.begin()+n,r.end());
          std::cout << '[' << k <<  "]=" <<  v << '\n';
	  pl[k]=v;      
	  uniq.insert(r);
	}else{
    	    int n=rnd(0,r.size()-1); 
            auto k=qtl::row(r.begin(),r.begin()+(n>1?rnd(0,n-1):0));
	    //int o=rnd(0,ops.size()-1);
	    auto op=ops[o];

    	    std::cout << "pl[" << k << "][ " << "col."<<n << op << r[n] <<  " ]" << '\n';

	    std::ostringstream s;
            s<<n;
	    qtl::expr col=qtl::expr(qtl::op::column,s.str());
	    qtl::expr val=qtl::expr(qtl::op::lit,qtl::interval(r[n]));

	    NOTRACE( std::cout << col.stringify() << op << val.stringify() << "\n"; )
	      qtl::op cmp=std::vector<qtl::op>{qtl::op::less,qtl::op::less_equal,qtl::op::equal_to,qtl::op::greater_equal,qtl::op::greater,qtl::op::not_equal_to}[o];

	    qtl::expr e(cmp,{col,val});
	    NOTRACE( std::cout << e.stringify() << "\n"; )
	    NOTRACE( pl.write(); )
	    for( auto r:pl[k][ e ] ){
		std::cout << r << "\n";
            }
	}
    }
    TRACE( std::cout << "m.size()=" << pl.m.base.size() << '\n'; )
      if( pl.m.base.size()==10 ){
	;TRACE( std::cerr << "debughook\n"; );
      }
      NOTRACE(  pl.s.write(); )
      if( rnd(1.0/sqrt(1.0+pl.m.base.size())) ){
	qtl::store::root.save("s.tmp");
	qtl::store::root.get("s.tmp");
      }

    pos.pop_front();
    pos.push_back(rnd.tellg());
    static auto p2=[](long n){ return n & -n; };
    static auto m2=[](long n){ return p2(n)-1; };
    static auto next_power_2=[](long n){
      n -= 1;
      n |= n>>1;
      n |= n>>2;
      n |= n>>4;
      n |= n>>8;
      return n+1;
    };

    long m=m2(pos[0])|m2(pos[1])|3;
    long s=next_power_2(pos.back()&m)+(pos.back()&~m);

    TRACE( std::cout << pos << "->" << s << "=" << std::oct << s << std::dec << '\n'; )

    TRACE( std::cout << m2(pos[0]) << "|" << m2(pos[1]) << "|3 = " << m << '\n'; )
    TRACE( std::cout << "np2(" << (pos.back()&m) << ")+" << (pos.back()&~m) << "\n"; )
    rnd.seekg(s);
    pos.back()=s;
  }
//  TRACE( pl.s.write(); )
//  qtl::store::root.save("s.");
TRACE( std::cout << "root:" << qtl::store::root << "\n"; );
#if 0
TRACE( std::cout << "confluence:" << qtl::store::confluence << "\n"; ); // 
/*
./qtl/store.h:3338:35: error: use of overloaded operator '<<' is ambiguous (with operand types 'basic_ostream<char, std::__1::char_traits<char> >' and 'std::map<path, std::optional<std::vector<splits> > >')
./qtl/out.h:602:23: note: candidate function [with T = std::__1::map<qtl::store::path, std::__1::optional<std::__1::vector<qtl::store::splits, std::__1::allocator<qtl::store::splits> > >, std::__1::less<qtl::store::path>, std::__1::allocator<std::__1::pair<const qtl::store::path, std::__1::optional<std::__1::vector<qtl::store::splits, std::__1::allocator<qtl::store::splits> > > > > >, $1 = void, $2 = std::__1::__map_iterator<std::__1::__tree_iterator<std::__1::__value_type<qtl::store::path, std::__1::optional<std::__1::vector<qtl::store::splits, std::__1::allocator<qtl::store::splits> > > >, std::__1::__tree_node<std::__1::__value_type<qtl::store::path, std::__1::optional<std::__1::vector<qtl::store::splits, std::__1::allocator<qtl::store::splits> > > >, void *> *, long> >]
static  std::ostream& operator<<(std::ostream& os, const T& obj)
                      ^
./qtl/store.h:647:26: note: candidate function [with T = std::__1::map<qtl::store::path, std::__1::optional<std::__1::vector<qtl::store::splits, std::__1::allocator<qtl::store::splits> > >, std::__1::less<qtl::store::path>, std::__1::allocator<std::__1::pair<const qtl::store::path, std::__1::optional<std::__1::vector<qtl::store::splits, std::__1::allocator<qtl::store::splits> > > > > >, $1 = void, $2 = std::__1::__map_iterator<std::__1::__tree_iterator<std::__1::__value_type<qtl::store::path, std::__1::optional<std::__1::vector<qtl::store::splits, std::__1::allocator<qtl::store::splits> > > >, std::__1::__tree_node<std::__1::__value_type<qtl::store::path, std::__1::optional<std::__1::vector<qtl::store::splits, std::__1::allocator<qtl::store::splits> > > >, void *> *, long> >]
    friend std::ostream &operator<<(std::ostream &os, const T &obj)

*/
#else
  qtl::store::dumpconfluence();
#endif

//qtl::store::operator<<(std::cout << "confluence:", qtl::store::dumpconfluence());
//qtl::operator<<(std::cout << "confluence:", qtl::store::dumpconfluence());
#if 0
operator<<(std::cout << "confluence:", qtl::store::dumpconfluence());
/*
./qtl/store.h:3342:1: error: no matching function for call to 'operator<<
 */

#endif
    TRACE( qtl::store::dumpsharedmap(); )    

//  std::cout << "done\n";
operator<<(std::cout, "done\n");
}
#endif
