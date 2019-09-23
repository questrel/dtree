#pragma once
#include "out.h"
#include "string.h"
#include "optional"
#include <experimental/type_traits>

namespace lex{
  using std::nullptr_t;
#if 0
template <typename... T> // https://ideone.com/Rihfre
std::ostream& operator<<(std::ostream& os, const std::tuple<T...>& obj){
  using  namespace qtl::literals;
  os /* << boost::core::demangle(typeid(obj).name()) << */ << "std::tuple{"_r;   
  print(os,obj,std::make_index_sequence<sizeof...(T)>());
  os << "}"_r;
  return os;
}
#endif



#define BASE_T std::pair<const std::vector<T0>&,const std::vector<T1>&>
 template<typename T0,typename T1>
  class pairvec:public BASE_T{
 public:
  using base_t=BASE_T;
  #undef BASE_T
  using base_t::base_t;
   pairvec(const std::vector<T0>&t0,const std::vector<T1>&t1):base_t(t0,t1){
   }
   std::pair<T0,T1> operator[](int i){
    std::pair<T0,T1> ret(std::get<0>(*this).size()?std::get<0>(*this)[i]:T0(),std::get<1>(*this).size()?std::get<1>(*this)[i]:T1());
    return ret;
  }
   //   using iterator0_t=/*const*/ typename std::vector<T0>::iterator;
   //   using iterator1_t=/*const*/ typename std::vector<T1>::iterator;
   using iterator0_t=decltype( std::declval<const std::vector<T0>&>().begin() );
   using iterator1_t=decltype( std::declval<const std::vector<T1>&>().begin() );
   #define BASE_T std::pair<iterator0_t,iterator1_t>
   class iterator:public BASE_T{
   public:
     using base_t=BASE_T;
     #undef BASE_T
     base_t ends;
     iterator(const std::pair<const std::vector<T0>&,const std::vector<T1>&>&p):iterator(std::get<0>(p),std::get<1>(p)){     }
   iterator(const std::vector<T0>&v0,const std::vector<T1>&v1):base_t(static_cast<iterator0_t>(v0.begin()),(v1.begin())),ends(v0.end(),v1.end()){     }
     iterator(const std::pair<const std::vector<T0>&,const std::vector<T1>&>&p,nullptr_t):iterator(std::get<0>(p),std::get<1>(p),nullptr){ }

     iterator(const std::vector<T0>&v0,const std::vector<T1>&v1,nullptr_t):base_t(v0.end(),v1.end()),ends(v0.end(),v1.end()){ }
     std::pair<T0,T1> operator*()const{
       return {std::get<0>(*this)!=std::get<0>(ends)?*std::get<0>(*this):T0(), 
	       std::get<1>(*this)!=std::get<1>(ends)?*std::get<1>(*this):T1()};
     }
     iterator operator++(){
       if( std::get<0>(*this)!=std::get<0>(ends) ){ ++std::get<0>(*this); }
       if( std::get<1>(*this)!=std::get<1>(ends) ){ ++std::get<1>(*this); }
       return *this;
     }
     std::optional<iterator0_t> i0(){ if( std::get<0>(*this)!=std::get<0>(ends) ){ return std::get<0>(*this); }else{ return {}; } }
     std::optional<iterator1_t> i1(){ if( std::get<1>(*this)!=std::get<1>(ends) ){ return std::get<1>(*this); }else{ return {}; } }
     bool operator!=( const iterator&e )const{
       return std::get<0>(*this)!=std::get<0>(ends)
  	   && std::get<1>(*this)!=std::get<1>(ends);
     }  
   }; // end class pairvec::iterator
   iterator begin()const{ return iterator(*this); }
   iterator end()const{ return iterator(*this,nullptr); }
}; // end class pairvec

#define BASE_T std::tuple<const std::vector<T>&...>
template<typename... T>
  class tuplevec:public BASE_T{
 public:
  using base_t=BASE_T;
#undef BASE_T
  using base_t::base_t;
  tuplevec(const std::vector<T>&...t):base_t(t...){
  }
#if 0
  template<std::size_t... Is>
    auto tup=(int i,std::index_sequence<Is...>){
    return std::make_tuple((i<size()?std::get<Is>(*this)[i]:T())...);
  };
#endif
    template<std::size_t... Is>
    struct dummy{
      auto tup(const base_t &t,int i,std::index_sequence<Is...>){
	return std::make_tuple((i<t.size()?std::get<Is>(t)[i]:T())...);
      }
     };
  std::tuple<T...> operator[](int i){
    return dummy<sizeof...(T)>::tup(this,i,std::index_sequence_for<T...>{});
  }
}; // end class tuplevec

template<typename... T>
  constexpr int min_depth() {
  return std::min({T::depth...});
 }
#if 0
 struct container:public string{
  using base_t=string;
  using base_t::base_t;
  //  using std_t=std::vector<T>;
  //  static inline constexpr int depth=T::depth-1;
  //  std::optional<std_t> mutable cache;
  container(const string& s):base_t(  s ){
    for( auto x:l ){
      base_t::operator +=( string(x,depth));
    }
  }
 vector(const std::string_view &s):string(s){
    NOTRACE( std::cout << __PRETTY_FUNCTION__ << std::endl; )
  }

  template<bool is_const_iterator = true>
  class _iterator  /*: public std::iterator<std::forward_iterator_tag,PointerType>*/{
  public:
    typedef typename std::conditional<is_const_iterator, base_t::const_iterator, base_t::iterator>::type base_iterator;
    typedef typename std::conditional<is_const_iterator, const char, char>::type base_value;
    typename base_iterator::base_iterator  p;
    typename base_iterator::base_iterator  e0;
    typename base_iterator::base_iterator  e1;
    typename base_iterator::base_iterator  e;

  T operator *(){
    return T(std::string_view(p,e0-p));
  }
    _iterator operator++(){
      base_iterator b;
      std::tie(b.p,b.e)={e1,e};
      e0=b+=string::iterator::skipeof(depth);
      std::tie(p,e1)={b.p,b.e};
      return *this;
   }
  bool operator!=(const _iterator&i){
    return p!=i.p;
  }

}; // end class contaner::_iterator;  
  typedef  _iterator<false> iterator;
  typedef _iterator<true> const_iterator;
  //  template<typename _iterator = const_iterator>
  const_iterator begin()const{
    const_iterator ret;
    typename const_iterator::base_iterator b;
    b=base_t::begin();
    ret.p=b.p;
    ret.e=b.e;
    ret.e0=(b+=string::iterator::skipeof(depth));
    ret.e1=b.p;
    return ret;
  }
  const_iterator end()const{
    const_iterator ret;
    ret.p=ret.e0=ret.e1=ret.e=(base_t::end()).e;
    return ret;
  }
  operator std_t()const{
    NOTRACE( std::cout << __PRETTY_FUNCTION__ << std::endl; )
    if( !cache ){
      assert( !"not implemented" );
    }
  assert( !base_t::_opt || base_t::_opt->data()==base_t::_this->data() );
    return *cache;
  }
   
}; // end class container
#endif

 //https://stackoverflow.com/questions/1005476/how-to-detect-whether-there-is-a-specific-member-variable-in-class
  template<typename T> struct HasShallow { 
    struct Fallback { int Shallow; }; // introduce member name "x"
    struct Derived : T, Fallback { };

    template<typename C, C> struct ChT; 

    template<typename C> static char (&f(ChT<int Fallback::*, &C::Shallow>*))[1]; 
    template<typename C> static char (&f(...))[2]; 

    static bool const value = sizeof(f<Derived>(0)) == 2;
  }; 
template< class T >
  inline constexpr bool HasShallow_v = HasShallow<T>::value;
 template <typename T/*,int Shallow_0=T::depth*/>
class vector:public string{
  using base_t=string;
 public:
  using base_t::base_t;
  using std_t=std::vector<T>;
  static inline constexpr int depth=T::depth-1;
#if 0
  template <typename S>
  using Shallow_type = decltype(std::declval<S>().Shallow);
  //  template< typename S,typename=std::enable_if_t<std::experimental::is_detected_v<Shallow_type,S>> >
  //  template< typename S,typename=decltype(std::declval<S>().Shallow) >
  //  template< typename S,typename=std::enable_if_t<std::experimental::is_detected_v< decltype(std::declval<S>().Shallow) >> >
  template< typename S,typename=std::enable_if_t<std::experimental::is_detected_v< S::Shallow >> >
  class maybeShallow{
    static inline constexpr int Shallow=S::Shallow;
  }; // end class maybeShallow;
  template<typename S,typename=void >
  class maybeShallow{
    static inline constexpr int Shallow=depth;
  }; // end class maybeShallow;
  //  static inline constexpr int Shallow=std::experimental::is_detected_v<decltype(Shallow_type,T)>?T::Shallow:T::depth;
  static inline constexpr int Shallow=maybeShallow<T>::Shallow;
#endif
  //  static inline constexpr auto Eof=eof(Depth>1?T::depth:Depth);
  //  #define  Eof eof(Shallow)
  std::optional<std_t> mutable cache;
  /**/ vector():base_t(){}
 vector(const std::initializer_list<T> l):base_t(  /* (... + string(v,T::depth) )*/ ),cache(l){
    for( auto x:l ){
      if constexpr ( HasShallow_v<T> ){
        base_t::operator+=(x);
      }else{
	base_t::operator+=( string(x,eof(T::depth)) );
      }
    }
  }
 vector(const raw &r):string(r){}
#if 0
  vector(const std::string_view &s):string(s){
  }
#endif
 vector(const std::vector<T>&v):cache(v){
    for( auto x:v ){
      if constexpr ( HasShallow_v<T> ){
        base_t::operator+=(x);
      }else{
	base_t::operator+=( string(x,eof(T::depth)) );
      }
    }
  }
 template<class S>
 vector(const std::vector<S>&v){
   NOTRACE( std::cerr << __PRETTY_FUNCTION__ << "\n"; )
   cache=std_t();
   for( auto x:v ){
      NOTRACE( std::cerr << x << '\n'; );
      NOTRACE( std::cerr << (T)x << '\n'; );
      push_back((T)x);
      NOTRACE( std::cerr << cache->size() << '\n'; );
      NOTRACE( std::cerr << cache->back() << '\n'; );
   }
 }
  template<bool is_const_iterator = true>
  class _iterator  /*: public std::iterator<std::forward_iterator_tag,PointerType>*/{
  public:
    typedef typename std::conditional<is_const_iterator, base_t::const_iterator, base_t::iterator>::type base_iterator;
    typedef typename std::conditional<is_const_iterator, const char, char>::type base_value;
    typename base_iterator::base_iterator  p;  // start of current element
  typename base_iterator::base_iterator  e0;   // end of current element
  typename base_iterator::base_iterator  e1;   // start of next element
  typename base_iterator::base_iterator  e;  // end of vector
     T operator *(){
       NOTRACE( std::cerr << __PRETTY_FUNCTION__ << std::endl; );
       NOTRACE ( std::cerr << std::string_view(p,e0-p) << std::endl; );
       NOTRACE ( std::cerr << T(std::string_view(p,e0-p)) << std::endl; );
       return T(std::string_view(p,e0-p));
     }
     _iterator operator++(){
      NOTRACE( std::cout << __PRETTY_FUNCTION__ << std::endl; );
      base_iterator b;
      NOTRACE( std::cout << (void*)p << ", " << (void*) e0 <<","<< (void*) e1 << ", " << (void*) e<< std::endl; );
      p=e1;
      std::tie(b.P(),b.E())={p,e};
      if constexpr ( HasShallow_v<T> ){
	  e0=(b+=string::iterator::skipeof(T::Shallow));
      }else{
	  e0=(b+=string::iterator::skipeof(T::depth));
      }
      e1=b.P();
      if constexpr ( HasShallow_v<T> ){
	  e0=e1; 
      }
      NOTRACE( std::cout << (void*)p << ", " << (void*) e0 <<","<< (void*) e1 << ", " << (void*) e<< std::endl; );
      return *this;
    }
    bool operator!=(const _iterator&i)const{
    NOTRACE( std::cout << __PRETTY_FUNCTION__ << '\n'; );
    NOTRACE( std::cout << (void*)p << "," << (void*)i.p << '\n'; );
    return p!=i.p;
    }
  }; // end class vector::_iterator;  
  typedef  _iterator<false> iterator;
  typedef _iterator<true> const_iterator;
  //  template<typename _iterator = const_iterator>
  const_iterator begin()const{
    /**/NOTRACE( std::cout << __PRETTY_FUNCTION__ << std::endl; );
    const_iterator ret;
    typename const_iterator::base_iterator b;
    if constexpr ( HasShallow_v<T> ){
	b=base_t::begin(string::iterator::skipeof(T::Shallow));
      }else{
        b=base_t::begin(string::iterator::skipeof(T::depth));
      }
    ret.p=b.P();
    ret.e=b.E();
    if constexpr ( HasShallow_v<T> ){
	ret.e0=(b+=string::iterator::skipeof(T::Shallow));
    }else{
	ret.e0=(b+=string::iterator::skipeof(T::depth));
    }
    ret.e1=b.P();
    if constexpr ( HasShallow_v<T> ){
	  ret.e0=ret.e1; 
    }
    return ret;
  }
  const_iterator end()const{
    const_iterator ret;
    ret.p=ret.e0=ret.e1=ret.e=(base_t::end()).E();
    return ret;
  }
   operator std_t()const{
     NOTRACE( std::cout << __PRETTY_FUNCTION__ << std::endl; )
    if( !cache ){
      cache=std_t();
      NOTRACE( std::cout << cache->size() << std::endl; )
	auto e=end();
      for( auto p=begin();p!=e;++p ){
	NOTRACE( std::cout << *p << std::endl; )
	 cache->push_back(*p);
       }
     }
     return *cache;
   }

   template<class S>
   operator std::vector<S>()const{
     NOTRACE( std::cerr << __PRETTY_FUNCTION__<< "(S=" <<  qtl::type_name<S>() << ")" << '\n' );
     std::vector<S>ret;
     for( auto x:(std_t)*this ){
       NOTRACE( std::cerr << qtl::pretty_function(x) << "\n"; )
       NOTRACE( std::cerr << qtl::pretty_function(qtl::pretty_function(x)) << "\n"; )
       NOTRACE( std::cerr << "(" << qtl::type_name<decltype(x)>() << ")" << x << '\n'; )
	 //TRACE( std::cerr << "(" << qtl::type_name<S>() << ")" << (S)x << '\n'; ) //  error: invalid operands to binary expression ('basic_ostream<char, std::__1::char_traits<char> >' and 'std::__1::vector<std::__1::basic_string<char>, std::__1::allocator<std::__1::basic_string<char> > >')
       ret.push_back((S)x);
     }
     return ret;
   }
  void push_back(const T &t){
    if( cache ){
      cache->push_back(t);
    }
    NOTRACE( std::cerr << __PRETTY_FUNCTION__ << "(" << t << ")" << '\n' );
    if constexpr ( HasShallow_v<T> ){
      base_t::operator+=(t);
    }else{
      base_t::operator +=( string(t,eof(T::depth)) );    
    }
  }
  template<class S> void push_back(const S &t){ push_back((T)t); }

  template<class S> 
  vector& operator=(const std::vector<S>& s){
    for( auto x:s ){
      push_back((T)x);
    }
    return *this;
  }
  
  int compare(const vector& t)const{
    return base_t::compare(t);
  }

  bool operator==(const vector& t)const{
    return compare(t)==0;
  }

  bool operator<(const vector& t)const{
    return compare(t)<0;
  }
  std::string raw(){
    return base_t::std_();
  }
  //  #undef Eof
}; // end class vector // lex::vector
#define BASE_T vector<T/*,Shallow*/>
 template <typename T/*,int Shallow=T::depth*/>
 class set:public BASE_T{
 public:
 using base_t=BASE_T;
 #undef BASE_T
  using base_t::base_t;
  using std_t=std::set<T>;
  static inline constexpr int depth=T::depth-1;
  set( const std_t &s ){
    for( auto x:s ){
      //      base_t::operator +=( string(x,eof(T::depth)) );
      base_t::push_back( x );
    }
  }
  template< class C, typename = std::enable_if_t< std::is_convertible_v<C,T>> >
 set( const std::set<C> &s ){
    NOTRACE( std::cerr << __PRETTY_FUNCTION__ << '(' << s << ')' << std::endl; )
    for( auto x:s ){
      NOTRACE( std::cerr << "push_back(" << x << ')' << std::endl; )
      base_t::push_back( x );
      NOTRACE( std::cerr << "*this:" << *this << std::endl; )
    }
  }
 set( const lex::raw &r):base_t(r){}
  template< class C, typename = std::enable_if_t< std::is_convertible_v<C,T>> >
  set( const lex::set<C>& s):base_t((lex::raw)s){}
  operator std_t()const{
    NOTRACE( std::cerr << __PRETTY_FUNCTION__ << std::endl; )
    typename base_t::std_t v=*this;
    std_t ret;
    for( auto x:v ){
      ret.insert(x);
    }
    return ret;
  }
  //  using value_std_t=typename T::std_t;
  // using std_std_t=std::set<value_std_t>;
  template<class S>
    operator std::set<S>()const{
    NOTRACE( std::cerr << __PRETTY_FUNCTION__ << std::endl; )
    std::set<S> ret;
    NOTRACE( std::cerr << "(" <<  qtl::type_name<typename base_t::std_t>() << ")" << static_cast<typename base_t::std_t>(*this)  << std::endl; )
    for( auto x:static_cast<typename base_t::std_t>(*this) ){
     NOTRACE( std::cerr << "(" << qtl::type_name<decltype(x)>() << ")" << x << std::endl; )
      NOTRACE( std::cerr << "(" << qtl::type_name<S>() << ")" << static_cast<S>(x) << std::endl; )
      ret.insert( static_cast<S>(x) );
    }
    return ret;
  }
}; // end class lex::set

#if 1
// https://codereview.stackexchange.com/questions/173564/implementation-of-static-for-to-iterate-over-elements-of-stdtuple-using-c17?utm_medium=organic&utm_source=google_rich_qa&utm_campaign=google_rich_qa
 template <class Tup, class Func, std::size_t ...Is>
   constexpr void static_for_impl(Tup&& t, Func &&f, std::index_sequence<Is...> )
   {
     ( f(std::integral_constant<std::size_t, Is>{}, std::get<Is>(t)),... );
   }

 template <class ... T, class Func >
   constexpr void static_for(std::tuple<T...>&t, Func &&f)
 {
   static_for_impl(t, std::forward<Func>(f), std::make_index_sequence<sizeof...(T)>{});
 }

 int main()
 {
   auto t = std::make_tuple( 1, 22, 3, 4 );

   std::size_t weighted = 0;
   static_for(t, [&] (auto i, auto w) { weighted += (i+1) * w; });

   std::cout << "Weighted: " << weighted << std::endl;

   return 0;
 }
#endif

#if 1
 // https://stackoverflow.com/questions/1198260/iterate-over-tuple?utm_medium=organic&utm_source=google_rich_qa&utm_campaign=google_rich_qa
 //#include <tuple>
 //#include <utility> 

 template<std::size_t I = 0, typename FuncT, typename... Tp>
   inline typename std::enable_if<I == sizeof...(Tp), void>::type
   for_each(std::tuple<Tp...> &, FuncT) // Unused arguments are given no names.
   { }

 template<std::size_t I = 0, typename FuncT, typename... Tp>
   inline typename std::enable_if<I < sizeof...(Tp), void>::type
   for_each(std::tuple<Tp...>& t, FuncT f)
   {
     f(std::get<I>(t));
     for_each<I + 1, FuncT, Tp...>(t, f);
   }
#endif

 template <typename... T>
 struct tuple:public string{
  public:
   using base_t=string;
   using std_t=std::tuple<T...>;
   std::optional<std_t> mutable cache;
   static inline constexpr int depth=std::min({-1,T::depth...})-1;

#if 1
tuple():base_t(){} //
 /*
./qtl/container.h:305:2: error: multiple overloads of 'tuple' instantiate to the same signature 'void ()'
 tuple(const T&... t):base_t( (... + string(t,eof(T::depth)) ) ),cache({t...}){
 ^
./qtl/ioerror.h:22:30: note: in instantiation of template class 'lex::tuple<>' requested here
    os << strerror(errno) << std::endl;
                             ^
./qtl/container.h:303:2: note: previous declaration is here
 tuple():base_t(){} //
 ^
1 error generated.
*/
#else
 /*
./qtl/container.h:345:9: error: no matching constructor for initialization of 'lex::tuple<lex::vector<lex::interval<lex::string, void> >, lex::tuple<lex::vector<lex::set<lex::basic_boundary<lex::number, qtl::sign, qtl::lim::inf, void> > >, lex::vector<lex::vector<lex::string> > > >::std_t' (aka 'tuple<lex::vector<lex::interval<lex::string, void> >, lex::tuple<lex::vector<lex::set<lex::basic_boundary<lex::number, qtl::sign, qtl::lim::inf, void> > >, lex::vector<lex::vector<lex::string> > > >')
/usr/include/c++/v1/tuple:617:23: note: candidate template ignored: requirement '_CheckArgsConstructor<true>::template __enable_default<lex::vector<lex::interval<lex::string, void> >, lex::tuple<lex::vector<lex::set<lex::basic_boundary<lex::number, qtl::sign, qtl::lim::inf, void> > >, lex::vector<lex::vector<lex::string> > > >()' was not satisfied [with _Dummy = true]
 */
#endif
// tuple(const T&... t):base_t( (... + string(t,std::remove_reference_t<decltype(t)>::depth) ) ){}
/*
./qtl/container.h:508:2: error: multiple overloads of 'tuple' instantiate to the same signature 'void ()'
 tuple(const T&... t):base_t( (... + string(t,eof(T::depth)) ) ),cache({t...}){
 ^
./qtl/store.h:2840:46: note: in instantiation of template class 'lex::tuple<>' requested here
 TRACE( std::cout << __PRETTY_FUNCTION__  << std::endl; )      
                                             ^
./qtl/container.h:488:2: note: previous declaration is here
 tuple():base_t(){} //
 ^

*/
 tuple(const T&... t/*,std::enable_if_t<sizeof...(t)!=0,int>=0*/):base_t( (... + string(t,eof(T::depth)) ) ),cache({t...}){
     NOTRACE( std::cerr  << __PRETTY_FUNCTION__ << std::endl; )
       NOTRACE( (..., (std::cerr << t << ", " )); )
	 /*fatal*/// TRACE( std::cout << base_t::_this << std::endl; )
       //       TRACE( std::cout << cache << std::endl; )
       //       TRACE( if( cache ){ std::cout << *cache << std::endl; } )
       NOTRACE( std::cerr << "!" << !!base_t::_opt << "||";  )
       NOTRACE(
	 if( !base_t::_opt ){
	   std::cerr << " base_t::_opt->data()==base_t::_this.data()*/\n"; 
          }else{
             std::cerr << (void*)base_t::_opt->data() << "==" << (void*)base_t::_this.data() << "\n";
	     std::cerr << *base_t::_opt << '\n';
		  ///*crash*/ std::cerr << base_t::_this << '\n';
          }
       )
     view_();
	 NOTRACE( std::cerr << (void*)base_t::_opt->data() << "==" << (void*)base_t::_this.data() << "\n"; )
	      NOTRACE(
		    std::cerr << (void*)base_t::std_().data() << "==" << (void*)base_t::_this.data() << "\n";
	     std::cerr << *base_t::_opt << '\n';
		    )
		assert( !base_t::_opt || std_().data()==base_t::_this.data() );
  }
 tuple( const std::tuple<T...>& t ):cache(t){
    NOTRACE( std::cout << __PRETTY_FUNCTION__ << std::endl; )
     base_t::clear();
     NOTRACE( std::cout << base_t::_this << std::endl; )
     //     TRACE( std::cout << *cache << std::endl; )
  }

  tuple( const std::string_view &v):base_t(v){ }
  tuple( const raw &v):base_t(v){ }

   // tuple(const T&... t):base_t( (... + string<1>(t)) ){}
   //  operator std::tuple<T...>()const{

   operator std_t()const{
     NOTRACE( std::cout << __PRETTY_FUNCTION__ << std::endl; )
       assert( !base_t::_opt || base_t::_opt->data()==base_t::_this.data() );
       if( !cache ){
	 //cache=std_t();
	 std_t s;
    NOTRACE( std::cerr << "std::get<0>(*cache)._this.__size:" << std::get<0>(s)._this.size() << "\n"; ) 
	 auto p=base_t::begin();
	 int i=0;
	 NOTRACE( std::cerr << "p.P():" << (void*) p.P()<< "\n"; )
	 static_for( s, [&](auto i, auto w){ 
	   NOTRACE( std::cerr << __PRETTY_FUNCTION__ << "(" << i << ", w)\n"; )
	   auto e=p;
	   NOTRACE( std::cerr << (void*) e.P() << "\n"; )
	   auto e0=e+=lex::string::iterator::skipeof(std::tuple_element_t<i,std_t>::depth);
   std::get<i>(s)=lex::raw(std::string_view(p.P(),e0-p.P()));
   NOTRACE( std::cerr << "(" << qtl::type_name<decltype(w)>() << ")" << std::get<i>(s) << "\n"; )
     //   TRACE( std::cerr << s << "\n"; ) // ./qtl/container.h:485:21: error: invalid operands to binary expression ('std::__1::ostream' (aka 'basic_ostream<char>') and 'lex::tuple<lex::string, lex::string>::std_t' (aka 'tuple<lex::string, lex::string>'))

	   //	   TRACE( std::cerr << std::get<i>(s) << "\n"; )
	     //TRACE( std::cerr << "e0=" << (void*)e0 << "\n"; )
           NOTRACE( 
		 if( e0==p.P() ){
		   std::cerr << "string_view(" << (void*)p.P() << "," << e0-p.P() << ")" << '\n';
		 }else{
                   std::cerr << "string_view(" << p.P() << "," << e0-p.P() << ")" << '\n';
                 }
           )
	     NOTRACE( std::cerr << qtl::visible(std::string_view(p.P(),e0-p.P())) << "\n"; )
#if 0
	     {
	       decltype(w) xx=lex::raw(std::string_view(p.P(),e0-p.P()));
   	      TRACE( std::cerr << "xx(" << qtl::type_name<decltype(xx)>() << ")" << xx << "\n"; )
	     }
           w=std::string_view(p.P(),e0-p.P()); 
	   TRACE( std::cerr << "w(" << qtl::type_name<decltype(w)>() << ")" << w << "\n"; )
	     TRACE( std::cerr << s << "\n"; )	   
	   //TRACE( std::cerr << "w._this.__size:" << w._this.size() << "\n"; ) 
#endif
 	    p=e;
	   });
	 //TRACE( std::cerr << s << "\n"; ) // error: invalid operands to binary expression ('std::__1::ostream' (aka 'basic_ostream<char>') and 'lex::tuple<lex::string, lex::string>::std_t' (aka 'tuple<lex::string, lex::string>'))
	 cache=s;
       }
       //TRACE( std::cerr << "*cache" << *cache << "\n"; ) // ./qtl/container.h:420:37: error: invalid operands to binary expression ('basic_ostream<char, std::__1::char_traits<char> >' and 'std::__1::optional<std::__1::tuple<lex::string, lex::string> >::value_type' (aka 'std::__1::tuple<lex::string, lex::string>'))
    NOTRACE( std::cerr << "std::get<0>(*cache)._this.__size:" << std::get<0>(*cache)._this.size() << "\n"; ) 
    return *cache;
   }
 }; // end struct tuple

#if 1 // copied from g++ std::tuple  /usr/local/include/c++/8.0.0/tr1/tuple
// Adds a reference to a non-reference type.
  template<typename _Tp>
    struct __add_ref
    { typedef _Tp& type; };

  template<typename _Tp>
    struct __add_ref<_Tp&>
    { typedef _Tp& type; };

/// Gives the type of the ith element of a given tuple type.
 template<int __i, typename _Tp>
   struct tuple_element;
 /**
  * Recursive case for tuple_element: strip off the first element in
  * the tuple and retrieve the (i-1)th element of the remaining tuple.
  */
 template<int __i, typename _Head, typename... _Tail>
   struct tuple_element<__i, tuple<_Head, _Tail...> >
   : tuple_element<__i - 1, tuple<_Tail...> > { };

 /**
  * Basis case for tuple_element: The first element is the one we're seeking.
  */
 template<typename _Head, typename... _Tail>
   struct tuple_element<0, tuple<_Head, _Tail...> >
   {
     typedef _Head type;
   };

#endif
 
 template <int i,typename... T>
  inline typename tuple_element<i, tuple<T...> >::type
   get(tuple<T...>){
   return {};
 }
}; // end namespace lex

template <typename... T>
std::ostream& operator<<(std::ostream& os, const lex::tuple<T...> &t){
  NOTRACE( std::cout << __PRETTY_FUNCTION__ << std::endl; )
    //    assert( !t.base_t._opt || t.base_t._opt->data()==t.base_t._this.data() );
#if 0
/*
/qtl/container.h:324:2: error: multiple overloads of 'tuple' instantiate to the same signature 'void ()'
    tuple(const T&... t):base_t( (... + string(t,eof(T::depth)) ) ),cache({t...}){
    ^
./qtl/container.h:440:70: note: in instantiation of template class 'lex::tuple<>' requested here
          std::cout  << (void*)t._this.data() << " + " << t._this.size() << std::endl;
                                                                            ^
./qtl/container.h:304:2: note: previous declaration is here
 tuple():base_t(){} //
 ^
*/
	  std::cout  << (void*)t._this.data() << " + " << t._this.size() << std::endl;
	  auto t_=t._this;
	  std::cout  << t._this << std::endl;
	  std::cout  << qtl::visible(t._this) << std::endl;
	  std::cout  << t._opt << std::endl;
  auto tt= t.operator std::tuple<T...>();
	  std::cout  << qtl::type<decltype(tt)>()  << std::endl;
	  std::cout << tt << std::endl;

        os << qtl::type<decltype(t)>() << '{' /**/<< (std::tuple<T...>)t/**/ << '}';
#endif
#if 1
    os << qtl::type<decltype(t)>() << '{' << t.operator std::tuple<T...>() << '}';
//  os << qtl::type<decltype(t)>() << '{' << t.operator typename std::remove_reference_t<decltype(t)>::std_t() << '}';
//  os << qtl::type<decltype(t)>() << '{' << typename std::remove_reference_t<decltype(t)>::std_t(t) << '}';
#else
    os << qtl::type<decltype(t)>() << '{' << qtl::stream_str( qtl::ios_base::fmtflags::none, t.operator std::tuple<T...>() ) << '}';
#endif
#if 0
os << qtl::type<decltype(t)>() << '{' /**/ << (typename std::remove_reference_t<decltype(t)>::base_t)t /**/ << '}';
#endif
  return os;
};
#if __INCLUDE_LEVEL__ + !defined __INCLUDE_LEVEL__ < 1+defined TEST_H
int main( int argc, char *argv[] ){
if( argc!=1 && argv[1][0] != '<' ){
  std::cout << "0000\t\t\t----\n";
  std::cout << "00000000-000	0	00000-ñ	000000	000000-\n";
  std::cout << "0	0ÿ>00000		00	0\n";
  exit(0);
}
#if 1
  {
    using namespace lex::literals;
    lex::string s0="abc\0def"_s;
    lex::string s1="ghi\0\0jkl"_s;
    TRACE(std::cout << qtl::visible(s0._this) <<"," <<  qtl::visible(s1._this) << '\n'; )
    lex::vector<lex::string>lv{s0,s1};
    NOTRACE(std::cout << "qtl::visible(lv._this:" << qtl::visible(lv._this) << '\n'; )
      decltype(lv)::std_t sv=lv;
    NOTRACE(std::cout << "sv:" << sv << '\n'; )

    lex::tuple<lex::string,lex::string>lt{s0,s1};
    NOTRACE(std::cout << "qtl::visible(lt._this):" << qtl::visible(lt._this) << '\n'; )
      decltype(lt)::std_t st=lt;
    NOTRACE(std::cout << "st:" << st << '\n'; )
    NOTRACE(std::cout << "lv:" << lv << '\n'; )
    NOTRACE(std::cout << "lt:" << lt << '\n'; )
      exit(0);
   }
#endif
 {

  static const auto re=std::basic_regex<char>("[[:punct:]]+|[^\t\n[:punct:]]*");
  std::vector<std::vector<std::string>>sss;
  std::vector<std::vector<lex::string>>sso;
  std::vector<lex::vector<lex::string>>soo;
  lex::vector<lex::vector<lex::string>>ooo;
  std::ifstream in;
  std::streambuf *cinbuf;
  if( argc>1 && argv[1][0] == '<' ){
    in = std::ifstream(argv[1][1]?argv[1]+1:argv[2]);
    cinbuf = std::cin.rdbuf(); 
    std::cin.rdbuf(in.rdbuf()); //redirect std::cin
  }
  while( std::cin.good() ){
    std::string s;
    std::getline(std::cin, s);
    std::smatch m;
    NOTRACE(std::cerr << qtl::visible(s) << "\n" );
    auto b=std::sregex_iterator(s.begin(), s.end(), re);
    auto e=std::sregex_iterator();
    std::vector<std::string>ss;
    std::vector<lex::string>so;
    lex::vector<lex::string>oo;
    for( auto i=b; i!=e; ++i ){
     NOTRACE( std::cerr << "push_back(" << qtl::visible(i->str()) << ")\n" );
      ss.push_back(i->str());
     NOTRACE( std::cerr << "ss=" << ss << "\n" );
    }
    ss.pop_back();
    for( auto i:ss ){
      lex::string o=i;
      NOTRACE( std::cout << "i=" << qtl::visible(i) << '\n'; )
	NOTRACE( std::cout << "o=" <<  o << '\n'; )
	so.push_back(o);
	NOTRACE( std::cout << "so=" <<  so << '\n'; )
        oo.push_back(o);
	NOTRACE( std::cout << "oo=" <<  so << '\n'; )
    }
    NOTRACE( std::cout << __LINE__ << '\n'; )    
    TRACE( std::cout << "ss=" << ss << '\n'; )
    TRACE( std::cout << "so=" << so << '\n'; )
    TRACE( std::cout << "oo=" << oo << '\n'; )
    {
      lex::vector<lex::string>t=ss;
      TRACE( std::cout << "lex::vector<lex::string>(ss)=" << t << '\n'; )
      assert(t==oo);
    }
    {
      std::vector<lex::string>t=oo;
      TRACE( std::cout << "std::vector<lex::string>(oo)=" << t << '\n'; )
      assert( t == so );
    }
    {
      std::vector<std::string>t=oo;
      TRACE( std::cout << "std::vector<std::string(oo)=" << t << '\n'; )
      assert( t == ss );
    }
    sss.push_back(ss);
    NOTRACE(  std::cerr << "sss=" << sss << '\n' );
    sso.push_back(so);
    NOTRACE(  std::cerr << "sso=" << sso << '\n' );
    soo.push_back(oo);
    NOTRACE(  std::cerr << "soo=" << soo << '\n' );
    ooo.push_back(oo);
    NOTRACE(  std::cerr << "ooo=" << ooo << '\n' );
  }
  //  exit(0);
  TRACE( std::cout << sss << '\n'; )
  TRACE( std::cout << ooo << '\n'; )
  NOTRACE( std::cerr << "(std::vector<std::vector<std::string>>)ooo = " << (std::vector<std::vector<std::string>>)ooo << '\n'; )
  assert( (std::vector<std::vector<std::string>>)ooo == sss ); 
#ifdef FUZZING
    exit(0);
#endif
 }
  using namespace std::string_literals;  

    lex::vector<lex::string> v{"a"s,"bb"s,"ccc"s};
    //std::cout << __LINE__ << '\n';
    std::cout << (lex::string)v << '\n';
    //std::cout << __LINE__ << '\n';
    std::cout << v << '\n';
    for( auto i:v ){
             std::cout << i << '\n';
    }
    //std::cout << __LINE__ << '\n';
    lex::vector<decltype(v)> vv{v,v};
    //std::cout << __LINE__ << '\n';
    std::cout << vv << '\n';
    //std::cout << __LINE__ << '\n';
    for( auto v:vv ){
       std::cout << v << '\n';
    }
    //std::cout << __LINE__ << '\n';
    lex::tuple<lex::string,lex::string> tt(std::tuple("123"s,"a\0c"s));
    //std::cout << __LINE__ << '\n';
  std::cout << (lex::string)tt << '\n';
  //std::cout << __LINE__ << '\n';
  //    std::cout << tt << '\n'; // ./qtl/container.h:382:15: error: use of overloaded operator '<<' is ambiguous (with operand types 'std::__1::ostream' (aka 'basic_ostream<char>') and 'lex::string::base_t' (aka 'basic_string_view<char>'))
    //std::cout << __LINE__ << '\n';
  std::cout << decltype(tt)::depth << '\n';
  //std::cout << __LINE__ << '\n';
#if 1
  lex::tuple<lex::string,lex::string> tp("123"s,"a\0c"s);
  //std::cout << __LINE__ << '\n';
  std::cout << std::tuple<std::string,std::string>{"std"s,"string"s} << '\n';
  //std::cout << __LINE__ << '\n';
#endif
  // lex::tuple<decltype(t),lex::string,decltype(t)> tt(t,"X\0Z"s,t);
  //    std::cout << (lex::string)tt << '\n';
  //     std::cout << tt << '\n';
  //    std::cout << decltype(tt)::depth << '\n';

}
#endif
