#pragma once
#include <random>
#include <queue>
#include "out.h"
namespace qtl{
  class randstream{
    std::istream &in=std::cin;
    long pos=0;
    static constexpr auto rand_1=(std::numeric_limits<long long>::max()>>1)+1;
    static constexpr auto rand_2=(rand_1)/2;
    static constexpr auto rand_4=(rand_2)/2;
    std::array<long long,2> bounds={0,rand_1}; // we have enough random information to select the range [bounds[0],bounds[1]-1] out of [0,rand_1-1]
    std::queue<uint_fast8_t> pre={};
    long pre_=0; // we're in the middle of the range, so we don't know what the next bit will be, but we know the following bits will be the opposite to stay near the middle
    std::random_device rd;
    static inline std::default_random_engine gen;
    bool norm(){
      // try to keep bounds from getting so small we lose information from rounding
      // this is the same technique used in Arithmetic Coding
      NOTRACE( assert(bounds[0]<bounds[1]); )
      while( bounds[1]-bounds[0]<rand_2 ){
	//assert(bounds[0]<bounds[1]);
	if( bounds[1]<=rand_2){   // expand lower range
  	  for( auto &x:bounds ){ x += x; }
	  //assert(bounds[0]<bounds[1]);
	  pre.push(0);
	  for( ; pre_>0; --pre_ ){ pre.push(1); }      
        }else if( rand_2<=bounds[0] ){  // expand upper range
	  for( auto &x:bounds ){ x += x-(rand_1); }
	  //assert(bounds[0]<bounds[1]);
	  pre.push(1);
	  for( ; pre_>0; --pre_ ){ pre.push(0); }      
        }else if( rand_4<=bounds[0] && bounds[1]<=(rand_1-rand_4) ){ // expand middle range
	  pre_+=1;
  	  for( auto &x:bounds ){ x += x-rand_2; }
	  //assert(bounds[0]<bounds[1]);
        }else{ // can't expand range
          NOTRACE( std::cerr << "norm:" << std::hex << bounds << ", " << pre << ", " << pre_ << "\n";)
	  return 0;
        }
      }
      return pre.size(); // return true when we have full bits of information ready to use
    }
    void flush(){ // throw away accumulated entropy
      bounds={0,rand_1};
      pre={};
      pre_=0;
    }
    void put(double p){ // add a fraction of a bit of information (1-lg(max(p,1-p)) bits)
      norm();
      //auto b0=bounds;
      if( p<0.5 ){
	bounds[0] = (bounds[0]+bounds[1]+(bounds[0]-bounds[1])*p/(1-p))/2;  
	//assert(bounds[0]<bounds[1]);
      }else/* if( p>0.5 )*/{
	bounds[1] = (bounds[1]+bounds[0]+(bounds[1]-bounds[0])*(1-p)/p)/2;
	//assert(bounds[0]<bounds[1]);
      }
    }
    void put1(uint_fast8_t i){ // add another bit of random information
      //assert(bounds[0]<bounds[1]);
      norm();
      //assert(bounds[0]<bounds[1]);
      switch(i){
      case 0:{ bounds[1]-=(bounds[1]-bounds[0])/2; };break;
      case 1:{ bounds[0]+=(bounds[1]-bounds[0])/2; };break;
      default:{ throw std::domain_error(std::string(__PRETTY_FUNCTION__)+"(0|1)" ); };break;
      }
      //assert(bounds[0]<bounds[1]);
    }
    void put6(uint_fast8_t n){
      NOTRACE( std::cerr << __PRETTY_FUNCTION__ << "(0x" << std::hex << (int)n << ")\n"; )
      NOTRACE( std::cerr << std::hex << bounds << ", " << pre <<  ", " << pre_ << "\n";  )
	for( auto i=5;i>=0;--i ){ put1((n>>i)&1); }
      NOTRACE( std::cerr << std::hex << bounds << ", " << pre << ", " << pre_ << "\n";  )
    }
    std::optional<int> get6(){
      // get 6 bits (so it can be human readable )
      if( in.eof() ){ return {}; }
      int ret=in.get();
      ++pos;
      if( ret == '\n' ){ return get6(); }// skip eol (to allow human readability )
      if( ret=='\025' ){
         flush(); // ctrl-U, flush buffer
         return get6();
      }
      return ret&0x3f;
    }
    int seed(){  // add more random information, from input, or fallback to std::random
      std::optional<int>s;
      put6( (s=get6())?*s:/*std::uniform_int_distribution<>(0,(1<<CHAR_BIT)-1)(gen) */0x32 );
      return pre.size();
    }    
    int rand01(){ // return a random bit
      if( pre.size() ){ auto ret=pre.front(); pre.pop(); if( pre.empty() && pre_ ){  pre.push(!ret); pre_-=1; } return ret; }
      norm() || seed();
      return rand01();
    }
  public:
    long tellg(){
      return pos;
    }
    void seekg(long n){
      if( n<=pos ){ return; }
      flush();
      for( /*pos=pos*/;pos<n;++pos ){
        in.get();
      }
    }
    randstream(std::istream &s=std::cin):in(s){
      gen = std::default_random_engine(rd());
    }
    int uniform_int_distribution(long b=rand_1-1){ // Produces random integer values i, uniformly distributed on the closed interval [0, b]
     if( b==0 ){ return 0; }
     auto b1= rand01();
     auto b2= uniform_int_distribution(b/2);
     if( (b&1)==0  && b2==(b1?0:b/2) ){ put1(b1); } // we wasted a bit when our sub-ranges overlapped,  put it back
     return (b1?(b+1)/2:0)+b2;
    }
    int uniform_int_distribution( int a,int b){
      // Produces random integer values i, uniformly distributed on the closed interval [a, b]
      return a+uniform_int_distribution(b-a);
    }
    bool bernoulli_distribution(double p){ // return true with probability p
      if( p<=0 ){ return false; }else if( p>=1 ){ return true; }
      if( rand01() ){ 
	return p<0.5?(put(p),false):bernoulli_distribution(p+(p-1));
      }else{
	return p>0.5?(put(p),true):bernoulli_distribution(p+p);
      }
    }
    auto operator()(int a,int b){ return uniform_int_distribution(a,b); }
    auto operator()(double p){ return bernoulli_distribution(p); }

    template<typename T>
    T operator()(
	   std::function<double(T)> cdf, // return random T from this cumulative distribution // P( rnd(cdf) < t ) = cdf(t) 
	   T a=0,T b= (T)std::numeric_limits<short>::max(), // within these bounds
	   double e=std::numeric_limits<float>::epsilon(),  // to this precison
	   double pa=0.0, double pb=1.0, 
 	  double r=0.5
    ){
      static constexpr T o=std::is_integral_v<T>?1:0;
      if constexpr ( std::is_integral_v<T> ){
         // externally, we follow the convention of std::random in returning real distributions over [a,b) and integer distributions over [a,b]
	 // but for consistency we use [a,b) internally for both
	 if( a==b ){ return a; }
         b+=o;
      }else{
      }
      T m=a+(b-a)*r;
      if( (pb-pa)<e ){ return m; }
      double pm=cdf(m);  
      double pr=(pm-pa)/(pb-pa);
      static constexpr double sqrt1_2=0.70710678118654757; //sqrt(0.5)
      // if( b-a > 10 ){ r = 1/pow(r,1/log2(pr)); } // if pr = r**(lg(pr)/lg(r)), we want r'**(lg(pr)/lg(r)) = 1/2 , so r' = 1/r**(1/lg(pr))
      if( b-a > 10 ){ r = sqrt1_2*pow(r,-0.5/log2(pr)); }else{ r=0.5; }  // geometric mean between r' and 1/2
      assert( r!=0 );
      if( operator()(pr) ){
	return operator()(cdf,a,m-o,e,pa,pm,r);
      }else{
	return operator()(cdf,m,b-o,e,pm,pb,r);
      }
    }
  }; // end class randstream;
}; // end namespact qtl
#if __INCLUDE_LEVEL__ + !defined __INCLUDE_LEVEL__ < 1 + defined TEST_H
int main(){
  auto r=qtl::randstream(std::cin);
  for( auto i=0;i<30;i+=1 ){
    std::cout << "r(0," << i << ") = " << r(i) << '\n';
  }
  exit(0);
}

#endif
