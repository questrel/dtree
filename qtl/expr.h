 #include "interval.h"
#include "tree.h"
#define BOOST_SPIRIT_X3_DEBUG
#include <boost/spirit/home/x3.hpp>
#include <boost/spirit/home/x3/binary/binary.hpp>
#include <boost/spirit/home/x3/directive/repeat.hpp>
#include <boost/spirit/include/support_utree.hpp>
#if 1
// https://stackoverflow.com/questions/18158376/getting-boostspiritqi-to-use-stl-containers
#include <boost/fusion/adapted/std_tuple.hpp>
#include <boost/fusion/include/std_tuple.hpp>
#include <boost/fusion/adapted/std_pair.hpp>
#include <boost/fusion/include/std_pair.hpp>
#include <boost/fusion/adapted/boost_tuple.hpp>
#include <boost/fusion/include/boost_tuple.hpp>
#endif
#include <boost/range/adaptor/reversed.hpp>

namespace qtl{
  namespace x3 = boost::spirit::x3;
  using x3::int_;
  using x3::long_;
  using x3::char_;
  using x3::lit;
  using x3::double_;
  using x3::bin;
  using x3::hex;
  using x3::oct;
  using x3::raw;
  using x3::lexeme;
  using x3::attr;
  using x3::omit;
  using x3::repeat;
  using x3::string;
  using x3::alpha;
  using x3::alnum;

  //  x3::rule<struct _, 
  //    std::tuple<std::string, int>
  //   >
	static const auto digit=char_('0','9');
	static const auto digitx=char_("0123456789abcdefABCDEF");
	static const auto digito=char_('0','7');
	static const auto digitb=char_('0','1');
        static const auto digits =+(digit  >> *( char('\'') >> *digit ) );
        static const auto digitsx=+(digitx >> *( char('\'') >> *digitx ) );
        static const auto digitso=+(digito >> *( char('\'') >> *digito ) );
        static const auto digitsb=+(digitb >> *( char('\'') >> *digitb ) );
	static const auto d_num=(char_('.') >> digits) | (digits >> -( char_('.') >> -digits ) );
	//	static const auto e_num=d_num >> -( char_("eE") >> -char_("+-") >>  digits );
	static const auto e_num=double_;
	//	static const auto x_num=char_('0')>>char_("xX")>>digitsx;
	static const auto x_num=(char_('0')>>char_("xX"))>hex;
	//	static const auto o_num=char_('0')>>             digitso;
	static const auto o_num=char_('0')>>             oct;
	//	static const auto b_num=char_('0')>>char_("bB")>digitsb;
	static const auto b_num=(char_('0')>>char_("bB"))>bin;
	static const auto to_string=[](auto& ctx){ _val(ctx)=std::string(_attr(ctx).begin(),_attr(ctx).end()); };
	static const auto copy_attr=[](auto& ctx){ _val(ctx)=_attr(ctx); };

#define as_string( p ) 	( x3::rule<struct as_string, std::string>{}= ( raw[ p ] [ to_string ] ) )
//#define copy_expr( p ) 	( x3::rule<struct _, qtl::expr>{}= ( p [ copy_attr ] ) )
#define copy_expr( p ) 	( p [ copy_attr ] )
#define TYPENAME(t) boost::core::demangle(std::string(typeid(t).name()).c_str())

using expr_rule_t = x3::rule<struct expr_rule, qtl::expr>;
static auto const num_rule=x3::rule<struct num_rule, qtl::expr>{} = 
	 (x3::rule<struct int_rule,  boost::fusion::deque<std::string, int>>{}=
        (as_string( x_num ) >> attr(16))
      | (as_string( b_num ) >> attr( 2))
      | (as_string( o_num ) >> attr( 8))
      | (as_string( e_num ) >> attr( 0))
	) [ ([](auto& ctx){
	       NOTRACE(	  std::cerr << "num_rule "<< __PRETTY_FUNCTION__ << "\n"; )
	       //	  std::cout << "type(ctx): " << qtl::type_name<decltype(ctx)>() << "\n";
	       NOTRACE( std::cout << "type(_attr): " <<  qtl::type_name<decltype(_attr(ctx))>() << "\n"; )
	       NOTRACE( std::cout << "type(_val): " << qtl::type_name<decltype(_val(ctx))>() << "\n"; )
	       //    	  std::cout << "_where:" <<  _where(ctx) << "\n";
	       //    	  std::cout << "_attr:" <<  _attr(ctx) << "\n";
	       //	  _val(ctx)=lex::number(lex::number::sem(std::string( boost::fusion::at_c<0>(_attr(ctx)).begin(),boost::fusion::at_c<0>(_attr(ctx)).end())));
	       NOTRACE( std::cerr << "string=" << std::string( boost::fusion::at_c<0>(_attr(ctx)).begin(),boost::fusion::at_c<0>(_attr(ctx)).end()) << '\n'; )
	       NOTRACE( std::cerr << "sem=" << lex::number::sem(std::string( boost::fusion::at_c<0>(_attr(ctx)).begin(),boost::fusion::at_c<0>(_attr(ctx)).end())) << "\n"; )
	       NOTRACE({
		   //		   auto e=qtl::expr(qtl::operation(qtl::op::lit,qtl::interval(lex::number::sem(std::string( boost::fusion::at_c<0>(_attr(ctx)).begin(),boost::fusion::at_c<0>(_attr(ctx)).end())))));
		   // error: no matching conversion for functional-style cast from 'qtl::operation<basic_interval<number, void> >' (aka 'qtl::operation<qtl::basic_interval<lex::number, void> >') to 'qtl::expr' (aka 'tree<operation<basic_interval<lex::number> > >')
		   qtl::expr e;
		   std::cerr << "expr=" << e << "\n";
		   _val(ctx)=e;
		   std::cerr << __LINE__ << "\n";
                   NOTRACE( std::cerr << "_val=" << _val(ctx) << "\n"; )
		 })
	       _val(ctx)=qtl::expr(
                           qtl::operation(
                             qtl::op::lit,
                             qtl::interval(
                                lex::number::sem(
                                  std::string(
                                    boost::fusion::at_c<0>(_attr(ctx)).begin(),boost::fusion::at_c<0>(_attr(ctx)).end()
                                  )
                                )
                             )
                          )
                );
		NOTRACE( std::cerr << "_val=" << _val(ctx) << "\n"; )
	 }) ] ;

	  static auto qchar_rule=x3::rule<struct as_string, std::string>{}=( as_string( (!char_("\\'")>>char_) | (char('\\') >> char_) ) )[
     ([](auto& ctx){
       NOTRACE(  std::cerr << "qchar_rule "<< "\n"; )
       //TRACE( std::cout << "type(_val): " << qtl::type_name<decltype(_val(ctx))>() << "\n"; )
       NOTRACE( std::cerr << "type(_attr): " << qtl::type_name<decltype(_attr(ctx))>() << "\n"; )
    // boost::variant<boost::detail::variant::over_sequence<boost::mpl::l_item<mpl_::long_<2l>, char, boost::mpl::l_item<mpl_::long_<1l>, char, boost::mpl::l_end> > > >
       NOTRACE( std::cout << std::string(_attr(ctx).begin(),_attr(ctx).end()) << '\n'; );

#if 0
       TRACE( std::cerr << "which(): " << _attr(ctx).which() << "\n"; )
       TRACE( std::cerr << "type(get<>()): " <<
        qtl::type_name<decltype(
          boost::get< 
               boost::detail::variant::over_sequence<boost::mpl::l_item<mpl_::long_<2l>, char, boost::mpl::l_item<mpl_::long_<1l>, char, boost::mpl::l_end>  > >
          >(_attr(ctx)) 
        )>() << '\n'; )
	//      boost::detail::variant::over_sequence<boost::mpl::l_item<mpl_::long_<2l>, char, boost::mpl::l_item<mpl_::long_<1l>, char, boost::mpl::l_end> > >
#else
       NOTRACE( std::cout << std::string(_attr(ctx).begin(),_attr(ctx).end()) << '\n'; );
#endif
        if( _attr(ctx).size()>1 ){
          using namespace std::string_literals;
	  switch(  _attr(ctx)[1] ){
	    case '0':{  _val(ctx)="\0"s; };break;
	    case 'Z':{  _val(ctx)="\032"s; };break;
	    case 'b':{  _val(ctx)="\b"s; };break;
	    case 'n':{  _val(ctx)="\n"s; };break;
	    case 'r':{  _val(ctx)="\r"s; };break;
	    case 't':{  _val(ctx)="\t"s; };break;
	    case '%':case '_':{  _val(ctx)=_attr(ctx); };break;
	    default:{ _val(ctx)=_attr(ctx)[1]; };break;
          }
        }else{
	  _val(ctx)=std::string(1,_attr(ctx)[0]);
        }
})];
 
	  static auto const quoted_lit_rule=expr_rule_t{}=
    (+( lexeme[lit("'") > *qchar_rule > lit("'")] ) )[
 ([](auto& ctx){
       NOTRACE( std::cerr << "quoted_lit_rule "<< "\n"; )
       NOTRACE( std::cerr << "type(_attr): " << qtl::type_name<decltype(_attr(ctx))>() << "\n"; )
       NOTRACE( std::cerr << "type(_val): " << qtl::type_name<decltype(_val(ctx))>() << "\n"; )
       std::string s;
       for( auto v:_attr(ctx) ){
	 for( auto x:v ){ 
	 	 s += x;
         }
       }
       NOTRACE( std::cerr << "s=" << qtl::visible(s) << '\n'; )
       NOTRACE( std::cerr << "lex::string=" << lex::string(s) << '\n'; )
       NOTRACE( std::cerr << "(std::string) lex::string=" << qtl::visible((std::string)lex::string(s)) << '\n'; )
       NOTRACE( std::cerr << "number(lex::string).raw=" << lex::number(lex::string(s)).raw() << '\n'; )
       NOTRACE( std::cerr << "(std::string)number(lex::string).raw=" << qtl::visible((std::string)lex::number(lex::string(s)).raw()) << '\n'; )
       _val(ctx)=expr(op::lit,interval(lex::string(s)));
       NOTRACE( std::cerr << "_val(ctx)=" << _val(ctx) << '\n'; )
  })
   ]
 ;
      static auto const lit_rule=expr_rule_t{} = ((num_rule|quoted_lit_rule)/**/>>-lit("_s")/**/)
#if 0
      [([](auto& ctx){
	TRACE( std::cerr << "lit_rule "<< "\n"; )
	TRACE( std::cerr << "type(_attr): " << qtl::type_name<decltype(_attr(ctx))>() << "\n"; )
	TRACE( std::cerr << "type(_val): " << qtl::type_name<decltype(_val(ctx))>() << "\n"; )
	 _val(ctx)= boost::get<
	 qtl::expr
	 >(_attr(ctx));
	  })]
#endif
    ;

 static auto const id_char= (alnum|char_('_') );

 //static x3::rule<struct _, qtl::expr> const expr_rule;
 static expr_rule_t const expr_rule;

#if 0
#else
 static auto const list_rule=
   x3::rule<struct list_rule, std::vector<qtl::expr>>{}=
   (-(expr_rule % lit(",")))
     [ ([](auto& ctx){
		NOTRACE(	  std::cerr << "list_rule " << "\n"; )
		NOTRACE( std::cout << "type(_attr): " <<  qtl::type_name<decltype(_attr(ctx))>() << "\n"; )
		NOTRACE( std::cout << "type(_val): " << qtl::type_name<decltype(_val(ctx))>() << "\n"; )
		//TRACE(  std::cout << "_val=" << _val(ctx) << '\n'; )
		//		TRACE(  std::cout << "_attr=" << _attr(ctx) << '\n'; )
		if( _attr(ctx) ){
  		  _val(ctx)=*_attr(ctx);
                }else{ 
		  _val(ctx)={};
                }
		// TRACE(  std::cout << "_val=" << _val(ctx) << '\n'; )
     })]
   ;

 static auto const name_rule=
   as_string( lexeme[(lit("x::x")>>!id_char) | lit("?") | (alpha >> *id_char) | (char_('"') > *(char_ - char_('"')) > char_('"'))]  );

 static auto const col_rule=expr_rule_t{} = 
   (lit("column") >> lit("(") >> digits >> lit(")") )[ ([](auto& ctx){ 
       using boost::fusion::at_c;
	 NOTRACE( std::cerr << "col_rule " << "\n"; )
  	 NOTRACE( std::cout << "type(_val): " << qtl::type_name<decltype(_val(ctx))>() << "\n"; )
         NOTRACE( std::cout << "type(_attr): " <<  qtl::type_name<decltype(_attr(ctx))>() << "\n"; )
	 NOTRACE( std::cerr << _attr(ctx).size() << "\n"; )
       std::string c;
         NOTRACE( std::cout << "at_c<0>(_attr[0])=" << at_c<0>(_attr(ctx)[0]) << "\n"; )
       for( auto x:_attr(ctx) ){
         NOTRACE( std::cout << qtl::type_name<decltype(x)>()  << "\n"; )
	 NOTRACE( std::cout << qtl::type_name<decltype(at_c<0>(x))>()  << "\n"; )
	 NOTRACE( std::cout << at_c<0>(x) << "\n"; )
         c.push_back(at_c<0>(x));
       }
         _val(ctx)=qtl::expr(op::column,c);
	 NOTRACE( std::cerr << "_val=" << _val(ctx) << "\n"; )
   }) ];

 static auto const id_rule=expr_rule_t{} = 
   //   (name_rule >> -( lit("(") >>  expr_rule  /* >> (expr_rule % lit(",")) */ >> lit(")") ) )[ ([](auto& ctx){ 
     (name_rule >> -( lit("(") >  list_rule >  lit(")") ) )[ ([](auto& ctx){ 
       using boost::fusion::at_c;
		NOTRACE(	  std::cerr << "id_rule " << "\n"; )
		NOTRACE( std::cout << "type(_attr): " <<  qtl::type_name<decltype(_attr(ctx))>() << "\n"; )
		NOTRACE( std::cout << "type(_val): " << qtl::type_name<decltype(_val(ctx))>() << "\n"; )
                NOTRACE( std::cout << "at_c<0>:" << qtl::type_name<decltype( at_c<0>(_attr(ctx)) )>() << "\n"; )
           	NOTRACE( std::cout << "at_c<1>:" << qtl::type_name<decltype( at_c<1>(_attr(ctx)) )>() << "\n"; )
       if(  at_c<1>(_attr(ctx)) ){
	 NOTRACE( std::cout << "type(*at_c<1>) " << qtl::type_name<decltype( *at_c<1>(_attr(ctx)) )>() << "\n"; )
         NOTRACE( std::cout << "at_c<1>.size()=" <<  at_c<1>(_attr(ctx))->size() << "\n"; )
	   //	   _val(ctx)=qtl::expr(qtl::operation(qtl::op::function,at_c<0>(_attr(ctx))),{*at_c<1>(_attr(ctx))});
	   _val(ctx)=qtl::expr(qtl::operation(qtl::op::function,at_c<0>(_attr(ctx))),*at_c<1>(_attr(ctx)));
	   //           _val(ctx)=qtl::expr(qtl::op::lit,lex::number::sem("123"));
       }else{

		//    	  std::cout << "_where:" <<  _where(ctx) << "\n";
		//    	  std::cout << "_attr:" <<  _attr(ctx) << "\n";
		//	  _val(ctx)=lex::number(lex::number::sem(std::string( boost::fusion::at_c<0>(_attr(ctx)).begin(),boost::fusion::at_c<0>(_attr(ctx)).end())));

       //	_val(ctx)=qtl::expr(qtl::op::lit,lex::number::sem("123"));
       _val(ctx)=qtl::expr(qtl::op::name,at_c<0>(_attr(ctx)));
       }
		NOTRACE( std::cerr << "_val=" << _val(ctx) << "\n"; )
	}) ];
#endif

#if 1
 static auto const atom_rule=
   expr_rule_t{}=
(col_rule | lit_rule | id_rule | '(' > expr_rule > ')')
#if 0
  [ ([](auto& ctx){
	 NOTRACE( std::cerr << "atom_rule " << __LINE__ << "\n"; )
	 NOTRACE( std::cout << "type(_val): " << qtl::type_name<decltype(_val(ctx))>() << "\n"; )
	 NOTRACE( std::cout << "type(_attr): " << qtl::type_name<decltype(_attr(ctx))>() << "\n"; )
	  using boost::fusion::at_c;
	 //	 TRACE( std::cout << "at_c<0>:" << qtl::type_name<decltype( at_c<0>(_attr(ctx)) )>() << "\n"; )
	 //	TRACE( std::cout << "at_c<1>:" << qtl::type_name<decltype( at_c<1>(_attr(ctx)) )>() << "\n"; )
#if 1
	 _val(ctx)= boost::get<
	 qtl::expr
	 >(_attr(ctx));
#endif
	 
	}) ]
#endif
   ;
#endif

static expr fold(enum op o, const std::vector<expr> &v){
       expr e(o,v);
       for( auto x:v ){
         if( !x.cachevalue ){ return e; }
       }
       NOTRACE( std::cerr << __PRETTY_FUNCTION__ << "(" << (int)o << ")\n"; )
	 return expr(o,e.eval(),v);
}

 static x3::rule<struct fact_rule, qtl::expr> const fact_rule;
 static auto const fact_def=
   fact_rule=atom_rule >> (lit('!')[ ([](auto& ctx){
       //             _val(ctx)=qtl::expr(qtl::op::fact,_attr(ctx));
	 }) ]);

#if 0
 static auto const neg_rule=expr_rule_t{}=atom_rule[ 
	    ([](auto& ctx){
	      //    _val(ctx)=_attr(ctx);
	      //	     _val(ctx)=(qtl::expr)_attr(ctx);
	 NOTRACE( std::cerr << "neg_rule::atom " << __LINE__ << "\n"; )
	 NOTRACE( std::cout << "type(_attr): " << qtl::type_name<decltype(_attr(ctx))>() << "\n"; )
	 NOTRACE( std::cout << "type(_val): " << qtl::type_name<decltype(_val(ctx))>() << "\n"; )
	 NOTRACE( std::cerr << "attr=" << _attr(ctx)<< "\n"; )
	 NOTRACE( std::cerr << "val=" << _val(ctx)<< "\n"; )
	 _val(ctx)=_attr(ctx);
      } ) ]
        | (lit('-') >> atom_rule)[ 
//      | (lit('-') >> neg_rule)[ 
      ([](auto& ctx){
		      NOTRACE( std::cerr << "neg_rule::- " << __LINE__ << "\n"; )
	 NOTRACE( std::cout << "type(_attr): " << qtl::type_name<decltype(_attr(ctx))>() << "\n"; )
	 NOTRACE( std::cout << "type(_val): " << qtl::type_name<decltype(_val(ctx))>() << "\n"; )
			  NOTRACE( std::cerr << "attr=" << _attr(ctx)<< "\n"; )
			  NOTRACE( std::cerr << "val=" << _val(ctx)<< "\n"; )
		      //		     qtl::expr a=_attr(ctx);
		      //    	                 TRACE( std::cerr << "a=" << a << "\n"; )
		      qtl::expr v=_attr(ctx);
		       NOTRACE( std::cerr << "v=" << v << "\n"; )
//		      qtl::expr n=qtl::expr(qtl::op::negate,{{v}});
//		      _val(ctx)=n;
                      _val(ctx)= fold(qtl::op::negate,{{v}});
		      NOTRACE( std::cerr << "val=" << _val(ctx)<< "\n"; )
		      //		     _val(ctx)=_attr(ctx);
		      //  TRACE( std::cerr << "val=" << _val(ctx)<< "\n"; )
      } ) ]
 ;
#else
 static auto const neg_count=x3::rule<struct neg_count, int>{}=
   (*char_('-'))[
	       ([](auto& ctx){
         NOTRACE( std::cerr << "neg_count " << __LINE__ << "\n"; )
        NOTRACE( std::cout << "type(_val): " << qtl::type_name<decltype(_val(ctx))>() << "\n"; )
	 NOTRACE( std::cout << "type(_attr): " << qtl::type_name<decltype(_attr(ctx))>() << "\n"; )
	NOTRACE( std::cout << "_attr=" << _attr(ctx) << "\n"; )
	 NOTRACE( std::cout << "_attr.size=" << _attr(ctx).size() << "\n"; )
	 //	 _val(ctx)=expr(op::lit,_attr(ctx).size());
	  _val(ctx)=_attr(ctx).size();
		})
	       ];
   static auto const plus_count=x3::rule<struct plus_count, std::pair<char,int>>{}=
   (+char_('+'))[
	       ([](auto& ctx){
         NOTRACE( std::cerr << "neg_count " << __LINE__ << "\n"; )
        NOTRACE( std::cout << "type(_val): " << qtl::type_name<decltype(_val(ctx))>() << "\n"; )
	 NOTRACE( std::cout << "type(_attr): " << qtl::type_name<decltype(_attr(ctx))>() << "\n"; )
	NOTRACE( std::cout << "_attr=" << _attr(ctx) << "\n"; )
	 NOTRACE( std::cout << "_attr.size=" << _attr(ctx).size() << "\n"; )
	 //	 _val(ctx)=expr(op::lit,_attr(ctx).size());
	 _val(ctx)={_attr(ctx).size(),'+'};
		})
	       ];
#undef PRE_PAIR
#ifdef PRE_PAIR
   static auto const minus_count=x3::rule<struct minus_count, std::pair<int,char>>{}=
#else
   static auto const minus_count=x3::rule<struct minus_count, int>{}=
#endif
   (+char_('-'))[
	       ([](auto& ctx){
         NOTRACE( std::cerr << "neg_count " << __LINE__ << "\n"; )
        NOTRACE( std::cout << "type(_val): " << qtl::type_name<decltype(_val(ctx))>() << "\n"; )
	 NOTRACE( std::cout << "type(_attr): " << qtl::type_name<decltype(_attr(ctx))>() << "\n"; )
	NOTRACE( std::cout << "_attr=" << _attr(ctx) << "\n"; )
	 NOTRACE( std::cout << "_attr.size=" << _attr(ctx).size() << "\n"; )
	 //	 _val(ctx)=expr(op::lit,_attr(ctx).size());
#ifdef PRE_PAIR	 
	 _val(ctx)={_attr(ctx).size(),'-'};
#else
	 _val(ctx)=attr(ctx).size();
#endif
		})
	       ];

template<char C>
#ifdef PRE_PAIR
  //  static auto const char_count=x3::rule<struct minus_count, std::pair<int,char>>{}=
  static auto const char_count=x3::rule<struct minus_count, std::pair<char,int>>{}=
#else
   static auto const char_count=x3::rule<struct minus_count, int>{}=
#endif
   (+char_(C))[
	       ([](auto& ctx){
         NOTRACE( std::cerr << "neg_count " << __LINE__ << "\n"; )
        NOTRACE( std::cout << "type(_val): " << qtl::type_name<decltype(_val(ctx))>() << "\n"; )
	 NOTRACE( std::cout << "type(_attr): " << qtl::type_name<decltype(_attr(ctx))>() << "\n"; )
	NOTRACE( std::cout << "_attr=" << _attr(ctx) << "\n"; )
	 NOTRACE( std::cout << "_attr.size=" << _attr(ctx).size() << "\n"; )
	 //	 _val(ctx)=expr(op::lit,_attr(ctx).size());
#ifdef PRE_PAIR	 
	 //	 _val(ctx)={_attr(ctx).size(),C};
	 _val(ctx)={C,_attr(ctx).size()};
#else
	 _val(ctx)=_attr(ctx).size();
#endif
		})
	       ];


   static auto const pre_count=//x3::rule<struct pre_count, std::vector<std::pair<int,char>>>{}=
	//   static auto const pre_count=x3::rule<struct pre_count, std::vector<std::pair<char,int>>>{}=
#ifdef PRE_PAIR
     (*(plus_count|minus_count))
#else
     (*( (char_count<'-'> >> attr('-')) | (char_count<'+'> >> attr('+')) ) )
       //     (*( (attr('-') >> char_count<'-'>) | ( attr('+') >>char_count<'+'>) ) )
#endif
;

   static auto const pre_rule=expr_rule_t{}=(
   (
#if 0
    neg_count >> atom_rule
#else
    pre_count >> atom_rule
#endif
   )[ 
     ([](auto& ctx){
       using boost::fusion::at_c;
        NOTRACE( std::cerr << "pre_rule " << __LINE__ << "\n"; )
        NOTRACE( std::cout << "type(_val): " << qtl::type_name<decltype(_val(ctx))>() << "\n"; )
	NOTRACE( std::cout << "type(_attr): " << qtl::type_name<decltype(_attr(ctx))>() << "\n"; )
#if 0
       if( at_c<0>(_attr(ctx))&1 ){
         _val(ctx)=fold(op::negate,{{at_c<1>(_attr(ctx))}});
       }else{
         _val(ctx)=at_c<1>(_attr(ctx));
       }
#else
       NOTRACE( std::cerr << "at_c<0>(_attr).size()=" << at_c<0>(_attr(ctx)).size() << "\n"; )
       if( at_c<0>(_attr(ctx)).size()==0 ){
         _val(ctx)=at_c<1>(_attr(ctx));
       }else{
	 expr e=at_c<1>(_attr(ctx));
	 for (auto i : boost::adaptors::reverse(at_c<0>(_attr(ctx))) ){
	   NOTRACE( std::cout << "type(i): " <<  qtl::type_name<decltype(i)>() << '\n'; )
	   NOTRACE( std::cout << "type(i): " <<  qtl::type_name<decltype(i)>() << '\n'; )
	   NOTRACE( std::cout <<  qtl::type_name<decltype(  boost::get<boost::fusion::deque<int, char>>(i) )>() <<'\n'; )
	   NOTRACE( std::cout <<  qtl::type_name<decltype(  at_c<0>(boost::get<boost::fusion::deque<int, char>>(i)) )>() <<'\n'; )
	   NOTRACE( std::cout <<  at_c<0>(boost::get<boost::fusion::deque<int, char>>(i))  <<'\n'; )
	   NOTRACE( std::cout <<  qtl::type_name<decltype(  at_c<1>(boost::get<boost::fusion::deque<int, char>>(i)) )>() <<'\n'; )
	   NOTRACE( std::cout <<   at_c<1>(boost::get<boost::fusion::deque<int, char>>(i)) <<'\n'; )
	   switch(  at_c<1>(boost::get<boost::fusion::deque<int, char>>(i))){
           case '-':{ 
	     if( at_c<0>(boost::get<boost::fusion::deque<int, char>>(i))&1 ){
                    e=fold(op::negate,{{e}});
	     }
	   };break;
           case '+':{};break;
	     //	   devault:{ std::cerr << "unknown prefix" << i.second << '\n'; }
	   }
	 }
	 _val(ctx)=e;
       }
#endif
  } ) ]
);

   //   static const auto neg_count_atom_rule= x3::rule<struct neg_atom, std::tuple<int,expr>>()=  neg_count >> atom_rule;

   static auto const neg_rule=expr_rule_t{}=(
   (
    neg_count >> atom_rule
   )[ 
     ([](auto& ctx){
       using boost::fusion::at_c;

       if( at_c<0>(_attr(ctx))&1 ){
         _val(ctx)=fold(op::negate,{{at_c<1>(_attr(ctx))}});
       }else{
         _val(ctx)=at_c<1>(_attr(ctx));
       }
  } ) ]
);
#endif

 static auto const product_rule=expr_rule_t{}=
   //   (pre_rule >> *(char_("*/")>> pre_rule))
      (pre_rule > *(char_("*/")> pre_rule))
     [ ([](auto& ctx){ 	
     using boost::fusion::at_c;
	NOTRACE( std::cerr << "product_rule: " << __LINE__ << "\n"; )
      NOTRACE( std::cerr << "type(_val): " << qtl::type_name<decltype(_val(ctx))>() << "\n"; )
       NOTRACE( std::cerr << "type(_attr): " << qtl::type_name<decltype(_attr(ctx))>() << "\n"; )
     NOTRACE( std::cerr << "type(at<0>_attr): " << qtl::type_name<decltype(at_c<0>(_attr(ctx)))>() << "\n"; )
     NOTRACE( std::cerr << "type(at<1>_attr): " << qtl::type_name<decltype(at_c<1>(_attr(ctx)))>() << "\n"; )
     NOTRACE( std::cerr << "type((at<1>_attr)[0]): " << qtl::type_name<decltype((at_c<1>(_attr(ctx)))[0])>() << "\n"; )
     NOTRACE( std::cerr << "at<0>_attr= " << at_c<0>(_attr(ctx)) << "\n"; )
     NOTRACE( std::cerr << "at<1>_attr.size=" << at_c<1>(_attr(ctx)).size() << "\n"; )
	if( at_c<1>(_attr(ctx)).size() ){
	        qtl::expr e;
    	        std::vector<qtl::expr> a;
		a.reserve(  at_c<1>(_attr(ctx)).size()+1 );
		a.push_back( at_c<0>(_attr(ctx)));
		char o=at_c<0>(at_c<1>(_attr(ctx))[0]);
         	for( auto i: at_c<1>(_attr(ctx)) ){
		  if( at_c<0>(i) != o ){
//		    e=qtl::expr(o=='*'?op::multiplies:op::divides,a);
		    e=fold(o=='*'?op::multiplies:op::divides,a);
		    a={e};
		    o=at_c<0>(i);
		  }
		  a.push_back( at_c<1>(i) );
		}
//		_val(ctx)=qtl::expr(o=='*'?op::multiplies:op::divides,a);  
		_val(ctx)=fold(o=='*'?op::multiplies:op::divides,a);  
	}else{
	    _val(ctx) =  boost::fusion::at_c<0>(_attr(ctx));
	}
     NOTRACE( std::cerr << "_val=" <<  _val(ctx)  << '\n'; );
      } ) ]
     ;
 static auto const sum_rule=expr_rule_t{}=
   //   (product_rule >> *(char_("+-")>> product_rule))
      (product_rule > *( char_("+-")> product_rule) )
     [ ([](auto& ctx){ 	
     using boost::fusion::at_c;
	NOTRACE( std::cerr << "sum_rule: " << __LINE__ << "\n"; )
      NOTRACE( std::cerr << "type(_val): " << qtl::type_name<decltype(_val(ctx))>() << "\n"; )
       NOTRACE( std::cerr << "type(_attr): " << qtl::type_name<decltype(_attr(ctx))>() << "\n"; )
     NOTRACE( std::cerr << "type(at<0>_attr): " << qtl::type_name<decltype(at_c<0>(_attr(ctx)))>() << "\n"; )
     NOTRACE( std::cerr << "type(at<1>_attr): " << qtl::type_name<decltype(at_c<1>(_attr(ctx)))>() << "\n"; )
     NOTRACE( std::cerr << "type((at<1>_attr)[0]): " << qtl::type_name<decltype((at_c<1>(_attr(ctx)))[0])>() << "\n"; )
     NOTRACE( std::cerr << "at<0>_attr= " << at_c<0>(_attr(ctx)) << "\n"; )
     NOTRACE( std::cerr << "at<1>_attr.size=" << at_c<1>(_attr(ctx)).size() << "\n"; )
	if( at_c<1>(_attr(ctx)).size() ){
	        qtl::expr e;
    	        std::vector<qtl::expr> a;
		a.reserve(  at_c<1>(_attr(ctx)).size()+1 );
		a.push_back( at_c<0>(_attr(ctx)));
		char o=at_c<0>(at_c<1>(_attr(ctx))[0]);
         	for( auto i: at_c<1>(_attr(ctx)) ){
		  if( at_c<0>(i) != o ){
		    e = fold(o=='+'?op::plus:op::minus,a);
		    //e=qtl::expr(o=='+'?op::plus:op::minus,a);
		    a={e};
		    o=at_c<0>(i);
		  }
		  a.push_back( at_c<1>(i) );
		}
		//_val(ctx)=qtl::expr(o=='+'?op::plus:op::minus,a);  
                _val(ctx)=fold(o=='+'?op::plus:op::minus,a);  
	}else{
	    _val(ctx) =  boost::fusion::at_c<0>(_attr(ctx));
	}
     NOTRACE( std::cerr << "_val=" <<  _val(ctx)  << '\n'; );
      } ) ]
     ;


#define CMP_TABLE \
  X0("<=",less_equal) \
  X("<>",not_equal_to) \
  X("!=",not_equal_to) \
  X("=",equal_to) \
  X("==",equal_to) \
  X(">=",greater_equal) \
  X("<",less) \
  X(">",greater) \
//
static struct cmp_sym_ : x3::symbols<unsigned>
   {
     cmp_sym_()
       {
        add
#define X0(o,n) (o,(int)op::n)
#define X(o,n) X0(o,n)
 CMP_TABLE
#undef X0
#undef X
	  ;
       }

   }cmp_sym ;
using namespace std::string_literals;
#if 0
 static auto const cmp_op=
   ///*as_string*/
   // x3::rule<struct cmp_op,  boost::fusion::deque<std::string, enum op>>{}=
 (
#if 0
#define X0(o,n) (raw[lit(o)] >> attr(op::n))
#define X0(o,n) (as_string(lit(o)) >> attr(op::n))
#else
#define X0(o,n) (string(o) /* >> attr(op::n) */)
#endif
#define X(o,n) |X0(o,n)
CMP_TABLE
#undef X0
#undef X
  );
#else
 static auto const cmp_op=cmp_sym;
#endif
static auto const cmp_rule=expr_rule_t{}=
  ( 
   //   sum_rule >>
      sum_rule >
   -  
   *  
   (
     (
      cmp_op
     )
     > sum_rule
   )
 )
     [ ([](auto& ctx){ 	
     using boost::fusion::at_c;
       NOTRACE( std::cerr << "cmp_rule: " << __LINE__ << "\n"; )
      NOTRACE( std::cerr << "type(_val): " << qtl::type_name<decltype(_val(ctx))>() << "\n"; )
      NOTRACE( std::cerr << "type(_attr): " << qtl::type_name<decltype(_attr(ctx))>() << "\n"; )
     NOTRACE( std::cerr << "type(at<0>_attr): " << qtl::type_name<decltype(at_c<0>(_attr(ctx)))>() << "\n"; )
     NOTRACE( std::cerr << "type(at<1>_attr): " << qtl::type_name<decltype(at_c<1>(_attr(ctx)))>() << "\n"; )
     NOTRACE(  std::cerr << "at<1>(_attr).size()" << at_c<1>(_attr(ctx))->size() << "\n"; )
     NOTRACE(
        if( at_c<1>(_attr(ctx))->size()==2 ){
	  std::cerr << "(at_c<1>((*at_c<1>(_attr(ctx)))[0]))=" << at_c<1>((*at_c<1>(_attr(ctx)))[0]) << '\n';
	  std::cerr << "(at_c<1>((*at_c<1>(_attr(ctx)))[1]))=" << at_c<1>((*at_c<1>(_attr(ctx)))[1]) << '\n';
	  std::cerr << "(at_c<1>((*at_c<1>(_attr(ctx)))[0])).o=" << (int) at_c<1>((*at_c<1>(_attr(ctx)))[0]).o << '\n';
	  std::cerr << "(at_c<1>((*at_c<1>(_attr(ctx)))[0])).identifier=" <<  (bool)at_c<1>((*at_c<1>(_attr(ctx)))[0]).identifier << '\n';
 	  std::cerr << "*(at_c<1>((*at_c<1>(_attr(ctx)))[0])).identifier="     <<  *at_c<1>((*at_c<1>(_attr(ctx)))[0]).identifier << '\n';
	  std::cerr << "(at_c<1>((*at_c<1>(_attr(ctx)))[0])).identifier==x::x" << (*at_c<1>((*at_c<1>(_attr(ctx)))[0]).identifier=="x::x") << '\n';
	}
	  )
     static auto make_cmp_interval=[](auto &ctx,const interval &l,enum op cl, enum op cu, const interval &u){
	  NOTRACE( std::cerr << __LINE__ << '\n'; )
	  NOTRACE( std::cerr << (void*)&ctx << '\n'; )
	  NOTRACE(  std::cerr << "_val(ctx)=" << _val(ctx) << '\n'; )
		 boundary<lex::number> lb,ub;
		 if( cl == op::less ){
		   lb={l.u().value(),1};
		 }else if( cl == op::less_equal ){
		   lb={l.l().value(),-1};
                 }else{
		   std::cerr << "lower bound must be < or <=\n";
                 }
		 if( cu == op::less ){
		   ub={u.l().value(),-1};
		 }else if( cu == op::less_equal ){
		   ub={u.u().value(),1};
                 }else{
		   std::cerr << "upper bound must be < or <=\n";
                 }
	  NOTRACE( std::cerr << __LINE__ << '\n'; )
	  NOTRACE(  std::cerr << "lb=" << lb << '\n'; )
	  NOTRACE(  std::cerr << "ub=" << ub << '\n'; )
	  NOTRACE(  std::cerr << "interval=" << interval(lb,ub) << '\n'; )
	  NOTRACE(  std::cerr << "expr=" <<  expr(op::lit,interval(lb,ub)) << '\n'; )
	  NOTRACE(  std::cerr << "_val(ctx)=" << _val(ctx) << '\n'; )

  	  _val(ctx) =  expr(op::lit,interval(lb,ub));
	  NOTRACE(  std::cerr << "_val(ctx)=" << _val(ctx) << '\n'; ) // (x::x<1) && (x::x<=3)
	  NOTRACE( std::cerr << __LINE__ << '\n'; )
   };
	     if(  at_c<1>(_attr(ctx))->size()==0 ){
	       _val(ctx) = at_c<0>(_attr(ctx));
	     }else if( at_c<1>(_attr(ctx))->size()==2
		       && at_c<1>((*at_c<1>(_attr(ctx)))[0]).o==op::name
		       && at_c<1>((*at_c<1>(_attr(ctx)))[0]).identifier
                       && (*at_c<1>((*at_c<1>(_attr(ctx)))[0]).identifier=="x::x" || *at_c<1>((*at_c<1>(_attr(ctx)))[0]).identifier=="?" )
             ){ // () <? x::x <? () 
	       NOTRACE( std::cerr <<  __LINE__ << "\n"; )
#if 1
   	       if(  /* at_c<0>(_attr(ctx)).o==op::lit &&*/  at_c<0>(_attr(ctx)).cachevalue 
		    && /* at_c<1>((*at_c<1>(_attr(ctx)))[1]).o==op::lit && */  at_c<1>((*at_c<1>(_attr(ctx)))[1]).cachevalue
	       ){
	         make_cmp_interval(ctx, *at_c<0>(_attr(ctx)).cachevalue, (enum op)at_c<0>((*at_c<1>(_attr(ctx)))[0]),  (enum op)at_c<0>((*at_c<1>(_attr(ctx)))[1]),  *at_c<1>((*at_c<1>(_attr(ctx)))[1]).cachevalue );
               }else{
		 auto a=at_c<0>(_attr(ctx));
		 auto b=at_c<1>((*at_c<1>(_attr(ctx)))[1]);
		 switch( (enum op)at_c<0>((*at_c<1>(_attr(ctx)))[0]) ){
		 case op::less_equal: {};break;
		 case op::less: { 
		    if( /* a.o==op::lit  && */ a.cachevalue ){
		      a.cachevalue = interval(a.cachevalue->l(),a.cachevalue->l());
                    }else{
		      a = expr(op::complement,{{at_c<0>(_attr(ctx))}});
                    }
                 };break;
		 default: {
                      std::cerr << "lower bound must be < or <=\n";
                 };break;
		 }

		 switch( (enum op)at_c<0>((*at_c<1>(_attr(ctx)))[1]) ){
		 case op::less: {};break;
		 case op::less_equal: { 
		    if( /* b.o==op::lit  && */  b.cachevalue ){
		      b.cachevalue = {b.cachevalue->u(),b.cachevalue->u()};
                    }else{
		      b = expr(op::complement,{{b}});
                    }
                 };break;
		 default: {
                      std::cerr << "upper bound must be < or <=\n";
                 };break;
		 }
		 _val(ctx) = expr(op::interval,{a,b});
		 NOTRACE( std::cout << __LINE__ << '\n'; )
               }
#else
	       auto a=at_c<0>(_attr(ctx)).eval();
	       auto b=at_c<1>((*at_c<1>(_attr(ctx)))[1]).eval();
	       NOTRACE( std::cout << "a=" << a << '\n'; )
	       NOTRACE( std::cout << "b=" << b << '\n'; )
	       make_cmp_interval(ctx, a, (enum op)at_c<0>((*at_c<1>(_attr(ctx)))[0]),  (enum op)at_c<0>((*at_c<1>(_attr(ctx)))[1]),  b );
#endif
	     }else if(  at_c<1>(_attr(ctx))->size()==1 ){
	       if( at_c<0>(_attr(ctx)).o==op::name && at_c<0>(_attr(ctx)).identifier
		   && (at_c<0>(_attr(ctx)).identifier=="x::x" ||at_c<0>(_attr(ctx)).identifier=="?")
               ){  // x::x <? (expr)
  	         NOTRACE( std::cerr <<  __LINE__ << "\n"; )
		 if( at_c<1>((*at_c<1>(_attr(ctx)))[0]).cachevalue ){ }
		 auto a=at_c<1>((*at_c<1>(_attr(ctx)))[0]).eval();
		 NOTRACE( std::cerr <<  __LINE__ << "\n"; )
		   switch( at_c<0>((*at_c<1>(_attr(ctx)))[0]) ){
		   case (int)op::equal_to:{ _val(ctx) = expr(op::lit,a); };break;
		   case (int)op::not_equal_to:{
		     _val(ctx) = expr(op::lit,interval(a.u(),a.l()));
                   };break;
		   default:{ 
  	            NOTRACE( std::cerr <<  __LINE__ << "\n"; )
    	            NOTRACE( std::cerr << (void*)&ctx << '\n'; )
 	            NOTRACE(  std::cerr << "_val(ctx)=" << _val(ctx) << '\n'; )
		      //		      make_cmp_interval(ctx, interval(), op::less_equal, (enum op)at_c<0>((*at_c<1>(_attr(ctx)))[0]), *at_c<1>((*at_c<1>(_attr(ctx)))[0]).cachevalue );
		      make_cmp_interval(ctx, interval(), op::less_equal, (enum op)at_c<0>((*at_c<1>(_attr(ctx)))[0]), a );
       	            NOTRACE(  std::cerr << "_val(ctx)=" << _val(ctx) << '\n'; )
                   };break;
		   }
  	         NOTRACE( std::cerr <<  __LINE__ << "\n"; )
	       }else if( 
                  at_c<1>((*at_c<1>(_attr(ctx)))[0]).o==op::name && at_c<1>((*at_c<1>(_attr(ctx)))[0]).identifier
                  && (at_c<1>((*at_c<1>(_attr(ctx)))[0]).identifier=="x::x" || at_c<1>((*at_c<1>(_attr(ctx)))[0]).identifier=="?")
               ){  // (expr) <? x::x 
 		 auto a=at_c<0>(_attr(ctx)).eval();
		   switch( at_c<0>((*at_c<1>(_attr(ctx)))[0]) ){
		   case (int)op::equal_to:{ _val(ctx) = expr(op::lit,a); };break;
		   case (int)op::not_equal_to:{
		     _val(ctx) = expr(op::lit,interval( a.u(),a.l()));
                   };break;
		   default:{ 
  	            NOTRACE( std::cerr <<  __LINE__ << "\n"; )
    	            NOTRACE( std::cerr << (void*)&ctx << '\n'; )
 	            NOTRACE(  std::cerr << "_val(ctx)=" << _val(ctx) << '\n'; )
		      //		      make_cmp_interval(ctx, *at_c<0>(_attr(ctx)).cachevalue, (enum op)at_c<0>((*at_c<1>(_attr(ctx)))[0]), op::less, interval());
		      make_cmp_interval(ctx, a, (enum op)at_c<0>((*at_c<1>(_attr(ctx)))[0]), op::less, interval());
       	            NOTRACE(  std::cerr << "_val(ctx)=" << _val(ctx) << '\n'; )
                   };break;
		   }
  	         NOTRACE( std::cerr <<  __LINE__ << "\n"; )
               }else{

  	         NOTRACE( std::cerr <<  __LINE__ << "\n"; )
 	         NOTRACE( std::cerr << (void*)&ctx << '\n'; )
	         _val(ctx) = expr((enum op)at_c<0>((*at_c<1>(_attr(ctx)))[0]), {at_c<0>(_attr(ctx)),at_c<1>((*at_c<1>(_attr(ctx)))[0])} );
	       }
             }else{
	       // () <? () <? () ... 
	       expr l=at_c<0>(_attr(ctx));
               std::vector<qtl::expr> a;
	       for( auto i:*at_c<1>(_attr(ctx)) ){
		 a.push_back( expr((enum op)at_c<0>(i),{l,at_c<1>(i)}) );
		 l=at_c<1>(i);
	       }
//	       _val(ctx) = expr(op::logical_and,a);
	       _val(ctx) = fold(op::logical_and,a);
             }
             NOTRACE( std::cerr << __LINE__ <<  " _val=" <<  _val(ctx)  << '\n'; );
	 } ) ]
    ;

#if 1
 static auto const not_count=x3::rule<struct not_count, int>{}=
   (*lexeme[x3::string("NOT") >> !id_char])[
	       ([](auto& ctx){
         NOTRACE( std::cerr << "NOT_count " << __LINE__ << "\n"; )
        NOTRACE( std::cout << "type(_val): " << qtl::type_name<decltype(_val(ctx))>() << "\n"; )
	 NOTRACE( std::cout << "type(_attr): " << qtl::type_name<decltype(_attr(ctx))>() << "\n"; )
	 //TRACE( std::cout << "_attr=" << _attr(ctx) << "\n"; )
	 NOTRACE( std::cout << "_attr.size=" << _attr(ctx).size() << "\n"; )
	 //	 _val(ctx)=expr(op::lit,_attr(ctx).size());
	  _val(ctx)=_attr(ctx).size();
		})
	       ];

  static auto const not_rule=
 expr_rule_t{}=
    (
     (  not_count  >> cmp_rule  )
  [
   ([](auto& ctx){
     using boost::fusion::at_c;
     NOTRACE( std::cerr << "not_rule\n"; )
      NOTRACE( std::cerr << "type(_val): " << qtl::type_name<decltype(_val(ctx))>() << "\n"; )
      NOTRACE( std::cerr << "type(_attr): " << qtl::type_name<decltype(_attr(ctx))>() << "\n"; )
#if 0
     //  (  ( *(lit("NOT")>>attr(expr())) ) >> cmp_rule  )
     //  boost::fusion::deque<std::__1::vector<qtl::optree<qtl::basic_interval<lex::number, void> >>, qtl::optree<qtl::basic_interval<lex::number, void> > >
#endif
#if 0
     //     (  ( *(lit("NOT")>>attr(1)) ) >> cmp_rule  )
     //   boost::fusion::deque<qtl::optree<qtl::basic_interval<lex::number, void> >, std::__1::vector<boost::fusion::deque<char, qtl::optree<qtl::basic_interval<lex::number, void> > >> >
     type(at<0>_attr): qtl::optree<qtl::basic_interval<lex::number, void> >
     type(at<1>_attr): std::__1::vector<boost::fusion::deque<char, qtl::optree<qtl::basic_interval<lex::number, void> > >>
#endif
#if 0
     // (  not_count  >> cmp_rule  )
     //     type(_attr): boost::fusion::deque<int, qtl::optree<qtl::basic_interval<lex::number, void> > >
     //     type(at<0>_attr): int
     //type(at<1>_attr): qtl::optree<qtl::basic_interval<lex::number, void> >
#endif
     NOTRACE( std::cerr << "type(at<0>_attr): " << qtl::type_name<decltype(at_c<0>(_attr(ctx)))>() << "\n"; )
     NOTRACE( std::cerr << "type(at<1>_attr): " << qtl::type_name<decltype(at_c<1>(_attr(ctx)))>() << "\n"; )
     NOTRACE( std::cerr << "at<0>_attr= " << at_c<0>(_attr(ctx)) << "\n"; )
     if( at_c<0>(_attr(ctx))&1 ){
       _val(ctx)=fold(op::complement,{{at_c<1>(_attr(ctx))}});
     }else{
       _val(ctx)=at_c<1>(_attr(ctx));
     }
   })
  ]
     )
  ;
#endif
static auto const and_rule=expr_rule_t{}=
  //    (cmp_rule >> *((lit("AND")|lit("&&")) >> cmp_rule))
    (not_rule >> *((lit("AND")|lit("&&")) > not_rule))
     [ ([](auto& ctx){ 	
     using boost::fusion::at_c;
      NOTRACE( std::cerr << "and_rule: " << __LINE__ << "\n"; )
      NOTRACE( std::cerr << "type(_val): " << qtl::type_name<decltype(_val(ctx))>() << "\n"; )
      NOTRACE( std::cerr << "type(_attr): " << qtl::type_name<decltype(_attr(ctx))>() << "\n"; )
     NOTRACE( std::cerr << "type(at<0>_attr): " << qtl::type_name<decltype(at_c<0>(_attr(ctx)))>() << "\n"; )
     NOTRACE( std::cerr << "type(at<1>_attr): " << qtl::type_name<decltype(at_c<1>(_attr(ctx)))>() << "\n"; )
     NOTRACE( std::cerr << "type((at<1>_attr)[0]): " << qtl::type_name<decltype((at_c<1>(_attr(ctx)))[0])>() << "\n"; )
     NOTRACE( std::cerr << "at<0>_attr= " << at_c<0>(_attr(ctx)) << "\n"; )
     NOTRACE( std::cerr << "at<1>_attr.size=" << at_c<1>(_attr(ctx)).size() << "\n"; )
	if( at_c<1>(_attr(ctx)).size() ){
	        qtl::expr e;
    	        std::vector<qtl::expr> a;
		a.reserve(  at_c<1>(_attr(ctx)).size()+1 );
		a.push_back( at_c<0>(_attr(ctx)));
         	for( auto i: at_c<1>(_attr(ctx)) ){
		  NOTRACE( std::cerr << "type(i): " << qtl::type_name<decltype(i)>() << "\n"; )
		    NOTRACE( std::cerr << "type(get<>(i)): " << qtl::type_name<decltype(boost::get<qtl::expr>(i))>() << "\n"; )
		    NOTRACE( std::cerr << "a.push_back(" <<  boost::get<qtl::expr>(i) << ")\n"; )
		    a.push_back( boost::get<qtl::expr>(i) );
		}
//		_val(ctx)=qtl::expr(op::logical_and,a);  
		_val(ctx)=fold(op::logical_and,a);  
		NOTRACE( std::cout << __LINE__ << " val(ctx)=" << _val(ctx) << '\n' ; );
	}else{
	    _val(ctx) =  boost::fusion::at_c<0>(_attr(ctx));
	}
     NOTRACE( std::cerr << "_val=" <<  _val(ctx)  << '\n'; );
    } ) ]
   ;

static auto const or_rule=expr_rule_t{}=
  (and_rule >> *((lit("OR")|lit("||")) >and_rule))
     [ ([](auto& ctx){ 	
     using boost::fusion::at_c;
	NOTRACE( std::cerr << "or_rule: " << __LINE__ << "\n"; )
	if( at_c<1>(_attr(ctx)).size() ){
	        qtl::expr e;
    	        std::vector<qtl::expr> a;
		a.reserve(  at_c<1>(_attr(ctx)).size()+1 );
		a.push_back( at_c<0>(_attr(ctx)));
         	for( auto i: at_c<1>(_attr(ctx)) ){
		  NOTRACE( std::cerr << "type(i): " << qtl::type_name<decltype(i)>() << "\n"; )
		    NOTRACE( std::cerr << "type(get<>(i)): " << qtl::type_name<decltype(boost::get<qtl::expr>(i))>() << "\n"; )
		    NOTRACE( std::cerr << "a.push_back(" <<  boost::get<qtl::expr>(i) << ")\n"; )
		    a.push_back( boost::get<qtl::expr>(i) );
		}
//		_val(ctx)=qtl::expr(op::logical_or,a);  
		_val(ctx)=fold(op::logical_or,a);  
	}else{
	    _val(ctx) =  boost::fusion::at_c<0>(_attr(ctx));
	}
     NOTRACE( std::cerr << "_val=" <<  _val(ctx)  << '\n'; );
    } ) ]
   ;

#if 1
    auto expr_rule_def = or_rule;

    BOOST_SPIRIT_DEFINE(expr_rule)

      static auto const start_rule= expr_rule_t{}=
      expr_rule;
#else
    //  static auto const start_rule =expr_rule_t{}=expr_rule;

#if 0
 static auto const start_rule =expr_rule_t{}=
    expr_rule=or_rule;
#else
 static auto const parse_expr_rule =expr_rule_t{}=
    expr_rule=or_rule;
 static auto const start_rule= expr_rule_t{}=
   parse_expr_rule;
#endif
#endif
} // end namespace qtl

#if __INCLUDE_LEVEL__ + !defined __INCLUDE_LEVEL__ < 1+defined TEST_H
#include "bounds.h"
namespace qtl{

 void parse_test(const std::string &s){
     //        std::tuple<std::string,int> result;
     //     boost::fusion::deque<double, int> result;
	     //	   TRACE( std::cerr << "type(atom_rule)=" <<   qtl::type_name<decltype(atom_rule)>() << "\n"; )
   using namespace lex::literals;
	   qtl::expr result;
	  try{
	    auto b=s.begin();
	    auto e=s.end();
#if 1
	    //	    std::cout <<  qtl::type_name<decltype(b)>() << "\n"; 
	    while( b!=e ){

	     boost::spirit::x3::ascii::space_type space;
             auto p=phrase_parse( b,e, start_rule,space,result );

       // std::cout << "type(result): " << qtl::type_name<decltype(result)>() << "\n";
       //      std::cout << "at<0>" << boost::fusion::at_c<0>(result) << "\n";
      //         std::cout << "at<1>" << boost::fusion::at_c<1>(result) << "\n";
       //		      TRACE( std::cerr << p << "," << "{" << std::get<0>(result) << "," << std::get<1>(result) << "}" << '\n'; )
  	      if( !p ){ break; }
	       std::cout << "result: " << result << '\n';
               std::cout << "stringify: " << result.stringify() << std::endl;
               std::cout << "eval: " << result.eval({{"a",1_s},{"b",2_s},{"c",3_s}}) << std::endl;
	       auto b=result.bind({
		   {"col1",1_column},{"col2",2_column},{"col3",3_column},
		 });
               if( b ){
		 std::cout << "bind: " << ((expr)b) << std::endl;
		 std::cout << "stringify: " << ((expr)b).stringify() << std::endl;
                 std::cout << "eval: " << ((expr)b).eval({},{{1_s<=x::x<2_s,3_s},{}}) << std::endl;
               }
	    }
	    if( b!=e ){
	      std::cout << "parse ended at " << qtl::visible(std::string(b,e)) << '\n';
            }
            std::cout << '\n';
#endif
	  }

	  catch (x3::expectation_failure<std::string::const_iterator> const& e) 
	    { 
	      std::cout << "expectation_failure: " << e.what() << " : " << e.which() << " : " << *e.where() << std::endl; 
	      std::cout << "Expected: " << e.which() << " at '" << std::string(e.where(), s.end()) << "'\n";
	      //	      std::cout << "expected: "; print_info(x.what_);
	      //	      std::cout << "got: \"" << std::string(x.first, x.last) << '"' << std::endl;
	    } 
	  //	  std::cout << '\n';
  }
} // end namespace qtl
#if 0
#include "display_attribute_type.hpp"
#else
namespace tools
{
  namespace spirit = boost::spirit;

  template <typename Expr, typename Iterator = spirit::unused_type>
    struct attribute_of_parser
    {
      typedef typename spirit::result_of::compile<
      spirit::qi::domain, Expr
        >::type parser_expression_type;

      typedef typename spirit::traits::attribute_of<
      parser_expression_type, spirit::unused_type, Iterator
        >::type type;
    };

    template <typename T>
      void display_attribute_of_parser(T const &) 
      {
        // Report invalid expression error as early as possible.
        // If you got an error_invalid_expression error message here,
        // then the expression (expr) is not a valid spirit qi expression.
        BOOST_SPIRIT_ASSERT_MATCH(spirit::qi::domain, T);

        typedef typename attribute_of_parser<T>::type type;
#if 0
//	std::cout << typeid(type).name() << std::endl;
/*
./qtl/container.h:363:2: error: multiple overloads of 'tuple' instantiate to the same signature 'void ()'
tuple(const T&... t):base_t( (... + string(t,eof(T::depth)) ) ),cache({t...}){
 ^
./qtl/expr.h:953:38: note: in instantiation of template class 'lex::tuple<>' requested here
        std::cout << typeid(type).name() << std::endl;
                                            ^
*/
#else
	std::cout << typeid(type).name() << '\n';
#endif
      }

    template <typename T>
      void display_attribute_of_parser(std::ostream& os, T const &) 
      {
        // Report invalid expression error as early as possible.
        // If you got an error_invalid_expression error message here,
        // then the expression (expr) is not a valid spirit qi expression.
        BOOST_SPIRIT_ASSERT_MATCH(spirit::qi::domain, T);

        typedef typename attribute_of_parser<T>::type type;
	//        os << typeid(type).name() << std::endl;
        os << typeid(type).name() << '\n';
      }
}
#endif
const char *__asan_default_options() {
  return "include_if_exists=asan_options";
}

int main(int argc, char *argv[]){
#if 0
{
auto qp=[](const std::string &s){
#if 1
  {
    auto b=s.begin();
    auto e=s.end();
    boost::spirit::x3::ascii::space_type space;
    qtl::expr result;
    //   
    static auto const start_rule =qtl::expr_rule_t{}=/*qtl::expr_rule=*/  qtl::not_rule;
    //    auto p=phrase_parse( b,e, qtl::not_rule,boost::spirit::x3::ascii::space_type()/*,result*/ );
    auto p=phrase_parse( b,e, start_rule,space,result );
    if( s.begin() != b ){ std::cerr << "parsed " << std::string(s.begin(),b) << '\n' ; }
    if( b!= e ){ std::cerr << "skiped " << std::string(b,e) << '\n' ;  }
    std::cerr << "result: " << result << '\n';
}
#endif
};
using namespace std::string_literals;
//	qp("'abc\\0\\'\\\"\\b\\n\\r\\t\\Z\\\\\\x\\B' 'def' 'ghi'  ;"s);
#if 0
	qp("-1"s);
	qp("--1"s);
	qp("---1"s);
	qp("- 1"s);
	qp("- -1"s);
	qp("- - 1"s);
	qp("- - -1"s);
	qp("1"s);
#endif
#if 1
	qp("NOT 11 xx ##");
	qp("NOT NOT 22 xx ##");
	qp("NOT NOT NOT 33 zz ##");
	qp("10 aa ##");
#endif
#if 0
  qp("1");
  qp("1*2");
  qp("1*2*4");
  qp("1*2*3*4");
#endif

}
#endif

if( argc==1 || argv [1][0] == '<' ){
  std::ifstream in;
  std::streambuf *cinbuf;
  if( argc>1 && argv[1][0] == '<' ){
    in = std::ifstream(argv[1][1]?argv[1]+1:argv[2]);
    cinbuf = std::cin.rdbuf(); 
    std::cin.rdbuf(in.rdbuf()); //redirect std::cin
  }
  int tellg=0;
  std::string i;
  while( std::cin.good() && tellg<1000 ){
   std::getline(std::cin, i);
   TRACE( std::cout << i << '\n'; )
   qtl::parse_test(i);
  }
  std::cout << "test passed\n";
  exit(0);
 }
 if( std::strcmp(argv[1],"dict")==0 ){
   std::cout << "\"++\"\n";
   std::cout << "\"--\"\n";
   exit(0);
 }
 std::cout << "'aa' 'b\\\\\\'b'\n";
 std::cout << "1*2*3+4*5*6+7*8*9\n";
 std::cout << "1+2 <= x::x < 3+4\n";
 std::cout << "1+2 <= x::x AND x::x < 3+4 ;;\n";
 std::cout << "- - -(3<=x::x<7)\n";
 std::cout << "a b a < b\n";
 std::cout << "3+col1\n";
 std::cout << "0 OR NOT col1 < col2\n";
 std::cout << "NOT 1<2\n";
 std::cout << "ABS( -3 )\n";
 std::cout << "ABS( -3<=x::x<2 )\n";
 std::cout << "ABS( NOT -3<=x::x<2 )\n";
 }
#endif

