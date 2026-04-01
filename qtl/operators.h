#if 1
//X(name,arity,operator,prefix|postfix|left infix|right infix,precedence after)
#define multiplies_identity 1
#define divides_identity 1
#define plus_identity 0
#define minus_identity 0
#define logical_or_identity 0
#define logical_and_identity interval()
#define OP_TABLE \
X(lit,0,,lit,lit) \
X(name,0,,name,lit)			 \
X(column,0,col,column,lit)		 \
X(function,1,(),func,lit)		 \
/*X(interval,3,lit)*/			 \
/*X(fact,1,!,post,name)*/		 \
X(negate,01,-,preop,/*fact*/name)		 \
/*X(abs,1,abs,pre,fact)*/		 \
X(pow,2,^,right2,/*fact*/negate)		 \
X(multiplies,2,*,left_,pow)			 \
X(divides,2,/,left_,pow)			 \
X(plus,2,+,left_,multiplies)			 \
X(minus,2,-,left_,multiplies)			 \
X(equal_to,2,==,in2,plus)			 \
X(not_equal_to,2,!=,in2,plus)			 \
X(less,2,<,in2,plus)			 \
X(greater,2,>,in2,plus)			 \
X(greater_equal,2,>=,in2,plus)			 \
X(less_equal,2,<=,in2,plus)			 \
X(complement,1,~,preop,complement)		 \
/* interval: lo <= x::x < hi  (x::x = "x such that", set-builder notation) */ \
X(interval,2,<= x::x <,interval,interval)		 \
X(logical_not,1,!,preop,equal_to)		 \
 /*X(bit_and,02,&,left,equal_to)*/		 \
 /*X(bit_or,02,|,left,bit_and)*/		 \
 X(logical_and,2,&&,left_,logical_and)	 \
X(logical_or,2,||,left_,logical_or) \
// end define OP_TABLE

/* [interval syntax footnote]
   lo <= x::x < hi reads as {x : lo ≤ x < hi} in set-builder notation — "the set of
   all x such that lo ≤ x < hi".  C++ :: is borrowed to suggest the mathematical
   "such that" colon; it carries no scope-resolution meaning here.  Conventional
   bracket syntax [lo,hi) would be clearer but is unparseable as a C++ expression;
   (lo,hi) parses only in limited contexts; so lo <= x::x < hi was chosen as the
   closest parseable approximation of standard mathematical interval notation. */
#else
//X(name,arity,name,operator,prefix|postfix|left|right,precedence after)
#define OP_TABLE \
X(lit,0,,lit,lit) \
X(name,0,,name,lit) \
X(fact,1,!,post,name)			 \
X(neg,1,-,pre,fact)			 \
X(abs,1,abs,pre,fact)		 \
X(pow,1,^,right,fact)		 \
X(mul,2,*,left,pow)			 \
X(div,2,/,left,pow)			 \
X(add,2,+,left,mul)			 \
X(sub,2,-,left,mul)			 \
X(eq,2,==,left,add)			 \
X(ne,2,!=,non,add)			 \
X(lt,2,<,non,add)			 \
X(gt,2,>,non,add)			 \
X(ge,2,>=,non,add)			 \
X(le,2,<=,non,add)			 \
X(Not,1,~,pre,eq)			 \
X(And,2,&,left,eq)			 \
X(Or,2,|,left,And)			 \
// end define OP_TABLE
#endif
