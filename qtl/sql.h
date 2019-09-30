#include "expr.h"
#include "store.h"
#define BOOST_NSPIRIT_X3_DEBUG
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
#include <boost/optional/optional_io.hpp>

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
  using x3::no_case;

#define as_string( p ) 	( x3::rule<struct as_string, std::string>{}= ( raw[ p ] [ to_string ] ) )
//#define copy_expr( p ) 	( x3::rule<struct _, qtl::expr>{}= ( p [ copy_attr ] ) )
#define copy_expr( p ) 	( p [ copy_attr ] )
#define TYPENAME(t) boost::core::demangle(std::string(typeid(t).name()).c_str())


 static std::map<std::string,qtl::interval> dictionary;
 static auto const assignment_rule=x3::rule<struct assignment, std::pair<std::string,qtl::expr>>{}=
    (id_rule >> lit("=") >> expr_rule)
  [ ([](auto& ctx){
         using boost::fusion::at_c;
	 NOTRACE( std::cerr << "assignment_rule " << __LINE__ << "\n"; )
	 NOTRACE( std::cout << "type(_val): " << qtl::type_name<decltype(_val(ctx))>() << "\n"; )
	 NOTRACE( std::cout << "type(_attr): " << qtl::type_name<decltype(_attr(ctx))>() << "\n"; )
	 if(  at_c<0>( _attr(ctx) ).identifier ){
	    _val(ctx).first  = *at_c<0>( _attr(ctx) ).identifier;
	 }else{
	   assert( at_c<0>( _attr(ctx) ).identifier );
         }
	 _val(ctx).second = at_c<1>( _attr(ctx) );
      })]
;

static auto const assignment_list=
  ( assignment_rule % ',') 
  [ ([](auto& ctx){
	 NOTRACE( std::cerr << "assignment_list " << __LINE__ << "\n"; )
	 NOTRACE( std::cerr << "type(_val): " << qtl::type_name<decltype(_val(ctx))>() << "\n"; )
	 NOTRACE( std::cerr << "type(_attr): " << qtl::type_name<decltype(_attr(ctx))>() << "\n"; )
	 for( auto x:_attr(ctx) ){
	   std::cerr << "{" << x.first << ", " << x.second << " = " << x.second.eval(dictionary) << "}" << '\n';
	   dictionary[x.first]=x.second.eval(dictionary);
	   //std::cerr << "dictionary=" << dictionary << '\n';
         }
      })]
  ;
 static auto const set_clause=
   (no_case[lit("SET")] >
 assignment_list )
;

static auto const alias_list=
  ( assignment_rule % ',') 
  [ ([](auto& ctx){
	 NOTRACE( std::cerr << "alias_list " << __LINE__ << "\n"; )
	 NOTRACE( std::cerr << "type(_val): " << qtl::type_name<decltype(_val(ctx))>() << "\n"; )
	 NOTRACE( std::cerr << "type(_attr): " << qtl::type_name<decltype(_attr(ctx))>() << "\n"; )
	 for( auto x:_attr(ctx) ){
	   std::cerr << "{" << x.first << ", " << x.second << " = " << x.second.eval(dictionary) << "}" << '\n';
	   dictionary[x.first]=x.second;
	   //	   TRACE( std::cerr << "dictionary=" << dictionary << '\n'; )
         }
      })]
  ;
 static auto const alias_clause=
   (no_case[lit("ALIAS")] >
 alias_list )
;

static inline qtl::store file;

static auto const insert_clause=
  (no_case[ lit("INSERT") > lit("INTO") ] > id_rule > no_case[ lit("VALUES") ] > list_rule)
  [ ([](auto& ctx){
	 TRACE( std::cerr << "insert_clause" << __LINE__ << "\n"; )
	 NOTRACE( std::cout << "type(_val): " << qtl::type_name<decltype(_val(ctx))>() << "\n"; )
	 NOTRACE( std::cout << "type(_attr): " << qtl::type_name<decltype(_attr(ctx))>() << "\n"; )
         using boost::fusion::at_c;
         NOTRACE( std::cout << "type(at_c<0>): " << qtl::type_name<decltype( at_c<0>( _attr(ctx) ) )>() << '\n'; );
         NOTRACE( std::cout << "type(at_c<1>): " << qtl::type_name<decltype( at_c<1>( _attr(ctx) ) )>() << '\n'; );
	 if( at_c<0>( _attr(ctx) ).identifier ){
	   TRACE ( std::cout <<  "into " << *at_c<0>( _attr(ctx) ).identifier << '\n'; )
	 }
	 std::vector<lex::string> v;
	 for( auto x: at_c<1>( _attr(ctx) ) ){
	   NOTRACE( std::cout <<  x  << '\n'; );
           auto y=x.eval(dictionary);
	   NOTRACE( std::cout <<  y  << '\n'; );
           v.push_back(y.l().value().raw());
         }
	 NOTRACE( std::cout << v << '\n'; )
	 file[ *at_c<0>( _attr(ctx) ).identifier ]=v;
	 NOTRACE( std::cout << file << '\n'; );
      })]
  ;

 static auto const where_clause=
   (no_case[lit("WHERE")] > expr_rule)
;

inline static const  std::map<std::string,expr> cols={
                                              {"col0",expr(op::column,"0"s)},
                                              {"col1",expr(op::column,"1"s)},
    					      {"col2",expr(op::column,"2"s)},
					      {"col3",expr(op::column,"3"s)},
					      {"col4",expr(op::column,"4"s)},
					      {"col5",expr(op::column,"5"s)},
					      {"col6",expr(op::column,"6"s)},
					      {"col7",expr(op::column,"7"s)},
					      {"col8",expr(op::column,"8"s)},
					      {"col9",expr(op::column,"9"s)},
};
static auto const select_clause=
  //  (  (lit("SELECT") >> (list_rule | (lit("*")>>attr('*')) ) ) >>  lit("FROM") >> id_rule >> -where_clause  )
  //  (lit("SELECT") > (list_rule | string("*") ) >> lit("FROM") >> id_rule >> -where_clause)
  (no_case[lit("SELECT")] > (string("*") | list_rule) >> no_case[lit("FROM")] >> id_rule >> -where_clause)
  [ ([](auto& ctx){
	 TRACE( std::cerr << "select_clause" << __LINE__ << "\n"; )
	 NOTRACE( std::cout << "type(_val): " << qtl::type_name<decltype(_val(ctx))>() << "\n"; )
	 NOTRACE( std::cout << "type(_attr): " << qtl::type_name<decltype(_attr(ctx))>() << "\n"; )
         using boost::fusion::at_c;
         NOTRACE( std::cout << "type(at_c<0>): " << qtl::type_name<decltype( at_c<0>( _attr(ctx) ) )>() << '\n'; );
         NOTRACE( std::cout << "type(at_c<1>): " << qtl::type_name<decltype( at_c<1>( _attr(ctx) ) )>() << '\n'; );
         //TRACE( std::cout << "type(at_c<2>): " << qtl::type_name<decltype( at_c<2>( _attr(ctx) ) )>() << '\n'; );
	 NOTRACE( std::cout << "which: " << at_c<0>( _attr(ctx)).which() << '\n'; )
	 if( at_c<0>( _attr(ctx)).which() == 0 ){
	   std::cout << "*\n";
         }else{
	   for( auto x: boost::get<std::vector<qtl::expr>>( at_c<0>(_attr(ctx)) ) ){
	     std::cout << x.stringify() << '\n';
           }
         }
	 if( at_c<1>( _attr(ctx) ).identifier ){
	 	std::cout << "from " << *at_c<1>( _attr(ctx) ).identifier << '\n';
	 }else{
	   assert(  at_c<1>( _attr(ctx) ).identifier );
         }
         if(  at_c<2>( _attr(ctx) ) ){
	   std::cout << "where " << at_c<2>( _attr(ctx) )->stringify() <<'\n';
         }
	 NOTRACE( std::cout << file << '\n'; );
	 #if 0
	 for( auto  x:file[{*at_c<1>( _attr(ctx) ).identifier}] ){
           TRACE( std::cout << __LINE__ << "\n"; )
           std::vector<interval> args;
           if(  at_c<2>(_attr(ctx)) || at_c<0>( _attr(ctx)).which() != 0 ){
	     args.push_back( *at_c<1>( _attr(ctx) ).identifier );
	     for( auto y:x ){
	       args.push_back(y);
             }
	   }
	   if( !at_c<2>(_attr(ctx)) || at_c<2>(_attr(ctx))->bind( cols ).eval( dictionary, args ) ){
	     if( at_c<0>( _attr(ctx)).which() == 0 ){
	       for( auto y:x ){
	         std::cout << y << ',';
               }
	     }else{
	      for( auto x: boost::get<std::vector<qtl::expr>>( at_c<0>(_attr(ctx)) ) ){
	        std::cout << x.bind(cols).eval(dictionary,args)  << ',';
               }
             }
            std::cout << '\n';
	   }
         }
         #else
         auto v=file[ *at_c<1>( _attr(ctx) ).identifier ];
         if( at_c<2>(_attr(ctx)) ){
           v=v[at_c<2>(_attr(ctx))->bind( cols )];
         }
	 TRACE(std::cout << v.predicate << '\n'; )
	 for( auto r:v ){
	   NOTRACE( continue; /* profile just the query with no output */)
	    NOTRACE( std::cout << __LINE__ << "\n"; )
	    if( at_c<0>( _attr(ctx)).which() == 0 ){
	      int i=0;
	      for( auto x:r ){
		if( i>1 ){ std::cout << ", "; }
		if( i!=0 ){ std::cout << x; }
		++i;
              }
	    }else{
	      auto p=store::path()+r;
	      NOTRACE( std::cout << p << '\n');
	      for( auto e: boost::get<std::vector<qtl::expr>>( at_c<0>(_attr(ctx)) ) ){
	        std::cout << e.bind(cols).eval(dictionary,p)  << ',';
              }
            }
           std::cout << '\n';
         }
         #endif
	 NOTRACE( std::cout << file << '\n'; );
      })]
  ;
static auto const delete_clause=
  (no_case[lit("DELETE") > lit("FROM")]> id_rule >> -where_clause)
  [ ([](auto& ctx){
	 TRACE( std::cerr << "delete_clause" << __LINE__ << "\n"; )
	 NOTRACE( std::cout << "type(_val): " << qtl::type_name<decltype(_val(ctx))>() << "\n"; )
	 NOTRACE( std::cout << "type(_attr): " << qtl::type_name<decltype(_attr(ctx))>() << "\n"; )
         using boost::fusion::at_c;
         NOTRACE( std::cout << "type(at_c<0>): " << qtl::type_name<decltype( at_c<0>( _attr(ctx) ) )>() << '\n'; );
         NOTRACE( std::cout << "type(at_c<1>): " << qtl::type_name<decltype( at_c<1>( _attr(ctx) ) )>() << '\n'; );
	 if( at_c<0>( _attr(ctx) ).identifier ){
	   TRACE( std::cout <<  "FROM " << *at_c<0>( _attr(ctx) ).identifier << '\n'; 
		  if( at_c<1>(_attr(ctx)) ){
                    std::cout <<  "WHERE " << at_c<1>( _attr(ctx) )->stringify() << '\n'; 
		  }
	   )
	   std::vector<std::vector<lex::string>>del;
 	   for( auto  x:file[ *at_c<0>( _attr(ctx) ).identifier ] ){	   
              std::vector<interval> args;
	      //args.push_back( *at_c<0>( _attr(ctx) ).identifier );
	      for( auto y:x ){
	         args.push_back(y);
              }
	      NOTRACE(  std::cerr << "args:" << args << '\n'; );
	      NOTRACE(  std::cerr << (!at_c<1>(_attr(ctx)) || at_c<1>(_attr(ctx))->bind( cols ).eval( dictionary, args )) << '\n'; );
	      if( !at_c<1>(_attr(ctx)) || at_c<1>(_attr(ctx))->bind( cols ).eval( dictionary, args ) ){
		std::vector<lex::string>d;
	        //d.push_back( *at_c<0>( _attr(ctx) ).identifier );
	        for( auto y:x ){
	          d.push_back(y);
                }
		del.push_back(d);
	      }
	   }
	   NOTRACE(  std::cerr << "del:" << del << '\n'; );
           for( auto d:del ){
	     NOTRACE( std::cout << "delete(" << d << ")\n"; )
 	     file[d]=nullptr;
 	     NOTRACE( std::cout << file << '\n'; );
           }
	 }
      })]
  ;

 static auto const sql_rule=
   ( (select_clause | set_clause | alias_clause | insert_clause | delete_clause) > (lit(";")|x3::eoi) )
   ;

} // end namespace qtl

#if __INCLUDE_LEVEL__ + !defined __INCLUDE_LEVEL__ < 1+defined TEST_H
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
	//	std::cout << typeid(type).name() << std::endl;
		std::cout << typeid(type).name() ;
		std::cout << '\n';
      }

    template <typename T>
      void display_attribute_of_parser(std::ostream& os, T const &) 
      {
        // Report invalid expression error as early as possible.
        // If you got an error_invalid_expression error message here,
        // then the expression (expr) is not a valid spirit qi expression.
        BOOST_SPIRIT_ASSERT_MATCH(spirit::qi::domain, T);

        typedef typename attribute_of_parser<T>::type type;
        os << typeid(type).name() << /*std::endl*/ '\n';
      }
}
#endif
#if 0
 void parse_test(const std::string &s){
   namespace x3 = boost::spirit::x3;

     //        std::tuple<std::string,int> result;
     //     boost::fusion::deque<double, int> result;
	     //	   TRACE( std::cerr << "type(atom_rule)=" <<   qtl::type_name<decltype(atom_rule)>() << "\n"; )
	   qtl::expr result;
	  try{
	    auto b=s.begin();

	    //	    std::cout <<  qtl::type_name<decltype(b)>() << "\n"; 
	    while( b!=e ){

	     boost::spirit::x3::ascii::space_type space;
#if 0
 static auto const sql_rule =expr_rule_t{}=
   expr_rule;
	     auto p=phrase_parse( b,e, sql_rule,space,result );
#else
	     TRACE( std::cout << __LINE__ << '\n'; )
	       auto p=phrase_parse( b,e, sql_rule,space/*,result*/ );
#endif
       // std::cout << "type(result): " << qtl::type_name<decltype(result)>() << "\n";
       //      std::cout << "at<0>" << boost::fusion::at_c<0>(result) << "\n";
      //         std::cout << "at<1>" << boost::fusion::at_c<1>(result) << "\n";
       //		      TRACE( std::cerr << p << "," << "{" << std::get<0>(result) << "," << std::get<1>(result) << "}" << '\n'; )
  	      if( !p ){ break; }
	      //	       std::cout << "result: " << result << '\n';
	      //               std::cout << result.stringify() << std::endl;
	      //               std::cout << result.eval({{"a",1},{"b",2},{"c",3}}) << std::endl;
              //   std::cout << '\n' << std::endl;
	    }
	    if( b!=e ){
	      std::cout << "parse ended at " << std::string(b,e) << '\n';
            }
	  }

	  catch (x3::expectation_failure<std::string::const_iterator> const& e) 
	    { 
	      std::cout << "expectation_failure: " << e.what() << " : " << e.which() << " : " << *e.where() << std::endl; 
	      std::cout << "Expected: " << e.which() << " at '" << std::string(e.where(), s.end()) << "'\n";
	      //	      std::cout << "expected: "; print_info(x.what_);
	      //	      std::cout << "got: \"" << std::string(x.first, x.last) << '"' << std::endl;
	    } 
	  std::cout << '\n';
  }
#elsif 0
namespace qtl{
} // end namespace qtl
using qtl::parse_test;
#else
void parse_test(const std::string &s){
   namespace x3 = boost::spirit::x3;
   using qtl::start_rule;
     //        std::tuple<std::string,int> result;
     //     boost::fusion::deque<double, int> result;
	     //	   TRACE( std::cerr << "type(atom_rule)=" <<   qtl::type_name<decltype(atom_rule)>() << "\n"; )
	   qtl::expr result;
	  try{
	    auto b=s.begin();
	    auto e=s.end();
	    //	    std::cout <<  qtl::type_name<decltype(b)>() << "\n"; 
	    while( b!=e ){

	     boost::spirit::x3::ascii::space_type space;
#if 0
      TRACE( std::cout << __LINE__ << '\n'; )
		static auto const sql_rule =qtl::assignment_list;
       //       /usr/local/include/boost/spirit/home/x3/nonterminal/rule.hpp:32:9: error: static_assert failed due to requirement '!is_same<decltype(get<expr_rule>(context)), unused_type>::value' "BOOST_SPIRIT_DEFINE undefined for this rule."
       auto p=phrase_parse( b,e, sql_rule,space/*,result*/ );
#else
      NOTRACE( std::cout << __LINE__ << '\n'; )
       auto p=phrase_parse( b,e, qtl::sql_rule,space/*,result*/ );
#endif
       // std::cout << "type(result): " << qtl::type_name<decltype(result)>() << "\n";
       //      std::cout << "at<0>" << boost::fusion::at_c<0>(result) << "\n";
      //         std::cout << "at<1>" << boost::fusion::at_c<1>(result) << "\n";
       //		      TRACE( std::cerr << p << "," << "{" << std::get<0>(result) << "," << std::get<1>(result) << "}" << '\n'; )
  	      if( !p ){ break; }
	      //	       std::cout << "result: " << result << '\n';	  
    //               std::cout << result.stringify() << std::endl;
	      //               std::cout << result.eval({{"a",1},{"b",2},{"c",3}}) << std::endl;
	      std::cout << '\n' /*<< std::endl*/; // error: multiple overloads of 'tuple' instantiate to the same signature 'void ()'
	    }
	    if( b!=e ){
	      std::cout << "parse ended at " << std::string(b,e) << '\n';
            }
	    //	    for( auto x:qtl::file ){
	    //               std::cout << x << "\n";
	    //            }
	  }

	  catch (x3::expectation_failure<std::string::const_iterator> const& e) 
	    { 
	      std::cout << "expectation_failure: " << e.what() << " : " << e.which() << " : " << *e.where() << /*std::endl*/ '\n';
	      std::cout << "Expected: " << e.which() << " at '" << std::string(e.where(), s.end()) << "'\n";
	      //	      std::cout << "expected: "; print_info(x.what_);
	      //	      std::cout << "got: \"" << std::string(x.first, x.last) << '"' << std::endl;
	    } 
	  std::cout << '\n';
  }

#endif
int main(int argc, char *argv[]){
if( argc==1 || argv [1][0] == '<' ){
  std::ifstream in;
  std::streambuf *cinbuf;
  if( argc>1 && argv[1][0] == '<' ){
    in = std::ifstream(argv[1][1]?argv[1]+1:argv[2]);
    cinbuf = std::cin.rdbuf(); 
    std::cin.rdbuf(in.rdbuf()); //redirect std::cin
  }
  qtl::file.get("sql.dat");
  int tellg=0; 
 std::string i;
 while( std::cin.good() /* && tellg<1000*/ ){
   std::getline(std::cin, i);
   TRACE( std::cout << i << '\n'; )
     i += '\n';
   parse_test(i);
   //   qtl::store::root.save("sql.test.dat");
  }
 qtl::file.save("sql.dat");
  std::cout << "test passed\n";
  exit(0);
 }
 if( std::strcmp(argv[1],"dict")==0 ){
   //std::cout << "\"++\"\n";
   //std::cout << "\"--\"\n";
   exit(0);
 }
 // std::cout << "SET a=1,b=2,c=a+b;\n";
 // std::cout << "SELECT a,b FROM table WHERE x<3\n";
 // std::cout << " INSERT INTO  bar VALUES 1,2,3,'foo' ; SET a=1,b=2; DELETE FROM table WHERE a==b;  SELECT * FROM X ;\n";
  std::cout << "INSERT INTO  bar VALUES 3,2,1,'foo' ; "
            << "SET a=1,b=2,c=a+b; "
            << "INSERT INTO  bar VALUES a,b,c,'abc' ; "
            << "INSERT INTO  bar VALUES 2,4,8,'del' ; "
            << " SELECT col1+col2, col4  FROM bar WHERE col1+col2<=col3 ;"
            << "DELETE  FROM bar WHERE col4='del' ;"
            << " SELECT col1+col2, col4  FROM bar WHERE col1+col2<=col3 ;"
            << "\n";
 }
#endif

