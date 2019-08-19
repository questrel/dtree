#ifndef TEST_H
#ifdef __clang__
#pragma message "clang++ -ggdb3 --std=c++1z -DERROR_INJECT=0 -DTEST_H=\"qtl/$1.h\" " __FILE__
#else
#pragma message "g++ -ggdb3 --std=c++17 -DERROR_INJECT=0 -DTEST_H=\"qtl/$1.h\" " __FILE__
#endif
#else
#ifdef ERROR_INJECT
#define XSTR(s) STR(s)
#define STR(s) #s
#pragma message "Warning: ERROR_INJECT " XSTR(ERROR_INJECT) 
#endif
#ifdef FUZZING 
#define TRACE(x)
#endif
#include TEST_H
#endif
