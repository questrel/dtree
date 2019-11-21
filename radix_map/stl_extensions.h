#ifndef STL_EXTENSIONS_H
#define STL_EXTENSIONS_H

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// algorithm extensions to standard template library
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "ioerror.h"

//
// lengthof, endof - from Extended STL, Volume 1, p. 35 - works even if operator[] or operator* exist for class
//

template <int N>
struct ss_array_size_struct
{
  unsigned char c[N];
};

template <class T, int N>
ss_array_size_struct<N> const &ss_static_array_size(T (&)[N]);

#define lengthof(x) sizeof(ss_static_array_size(x).c)
#define endof(x) ((x) + lengthof(x))

//
// incr, decr - increment and decrement values through wrapping bounded range
//

template <typename Type>
Type incr(Type &value, const Type max, const Type min = 0) { return (value == max - 1) ? (value = min) : ++value; }

template <typename Type>
Type decr(Type &value, const Type max, const Type min = 0) { return (value == min) ? (value = max - 1) : --value; }

//
// valarray with iterators
//

#include <algorithm> // for lexicographical_compare()
#include <valarray>

template<typename Type>
class valarray_with_iterator : public std::valarray<Type> {
public:
  // constructors: replicate base class constructors since they are are not inherited
  valarray_with_iterator() : std::valarray<Type>() {}
  explicit valarray_with_iterator(size_t count_) : std::valarray<Type>(count_) {}
  valarray_with_iterator(const Type &t, size_t count_) : std::valarray<Type>(t, count_) {}
  valarray_with_iterator(const Type *p, size_t count_) : std::valarray<Type>(p, count_) {}
  valarray_with_iterator(const std::valarray<Type> &v) : std::valarray<Type>(v) {}
  // iterator
  typedef typename valarray<Type>::value_type *iterator;
  iterator begin() { return &((*this)[0]); }
  iterator end() { return &((*this)[size()]); }
  typedef const typename valarray<Type>::value_type *const_iterator;
  const_iterator begin() const { return const_cast<valarray_with_iterator *>(this)->begin(); }
  const_iterator end() const { return const_cast<valarray_with_iterator *>(this)->end(); }
  // comparison operator
  bool operator<(const valarray_with_iterator &x) const {
    return std::lexicographical_compare(begin(), end(), x.begin(), x.end());
  }
};

//
// for_each
//

// apply a member function
template<typename InputIterator, typename Tp, typename Ret>
void
for_each(InputIterator first, InputIterator last, Ret (Tp::*f)())
{
  while (first != last)
    mem_fun_ref(f)(*first++);
  return;
}

// apply a member function that takes an argument by value
template<typename InputIterator, typename Tp, typename Ret, typename Arg>
void
for_each(InputIterator first, InputIterator last, Ret (Tp::*f)(Arg), Arg a)
{
  while (first != last)
    mem_fun_ref(f)(*first++, a);
  return;
}

// apply a member function that takes an argument by reference
template<typename InputIterator, typename Tp, typename Ret, typename Arg>
void
for_each(InputIterator first, InputIterator last, Ret (Tp::*f)(Arg &), Arg &a)
{
  while (first != last)
    mem_fun_ref(f)(*first++, a);
  return;
}

// apply a member function of an object
template<typename InputIterator, typename Function, typename Base>
Function
for_each_indirect(InputIterator first, InputIterator last, Function f, Base *b)
{
  while (first != last)
    (b->*f)(*first++);
  return f;
}

// apply a member function of an object that takes an argument
template<typename InputIterator, typename Function, typename Base, typename Arg>
Function
for_each_indirect(InputIterator first, InputIterator last, Function f, Base *b, Arg a)
{
  while (first != last)
    (b->*f)(*first++, a);
  return f;
}

// apply a member function of an object in backward order
template<typename InputIterator, typename Function, typename Base>
Function
for_each_indirect_backward(InputIterator first, InputIterator last, Function f, Base *b)
{
  while (first != last--)
    (b->*f)(*last);
  return f;
}

// apply a function to each second half of a pair in a range from a map
template<typename InputIterator, typename Functor>
void
for_each_second(InputIterator first, InputIterator last, Functor f)
{
  while (first != last)
    f(first++->second);
  return;
}

// apply a member function to each second half of a pair in a range from a map
template<typename InputIterator, typename Tp, typename Ret>
void
for_each_second(InputIterator first, InputIterator last, Ret (Tp::*f)())
{
  while (first != last)
    mem_fun_ref(f)(first++->second);
  return;
}

// apply a member function that takes an argument to each second half of a pair in a range from a map
template<typename InputIterator, typename Tp, typename Ret, typename Arg>
void
for_each_second(InputIterator first, InputIterator last, Ret (Tp::*f)(Arg), Arg a)
{
  while (first != last)
    mem_fun_ref(f)(first++->second, a);
  return;
}

//
// find
//

// find an elements in a collection
template<typename Tp, typename El>
bool
find(const Tp &coll, const El &element)
{
  return find(coll.begin(), coll.end(), element) != coll.end();
}

//
// find_if
//

// find an iterator satisfying a functor that takes the iterator as an argument
template <typename InputIterator, typename Functor>
InputIterator
find_if_iterator(InputIterator first, InputIterator last, Functor f) {
  for ( ; first != last; ++first)
    if (f(first))
      break;
  return first;
}

//
// copy_until
//

// copy until a terminating value is found, which is not copied; return end of target 
template <typename InputIterator, typename OutputIterator, typename Type>
OutputIterator
copy_until(InputIterator source, OutputIterator target, Type terminator) {
  for (Type save; (save = *source) != terminator; ++source)
    *target++ = save;
  return target;
}

//
// transform elements in range by applying member function to elements in another range
//

template <class InputIterator, class OutputIterator, typename Functor>
  void transform_indirect(InputIterator first, InputIterator last, OutputIterator to, Functor f)
{
  while (first != last) ((*to++).*f)(*first++); // ((&(*to++))->*f)(*first++); // mem_fun_ref(f)(*to++, *first++);
}

//
// get_index
//

// find the index of an element in an array
template<typename Ret, typename Tp, typename El>
Ret
get_index(const Tp &array, const El &element) {
  typename Tp::size_type retval = find(array.begin(), array.end(), element) - array.begin();
  if (retval == array.size()) {
    std::stringstream error;
    error << "unable to find " << element << Throw;
  }
  return static_cast<Ret>(retval);
}

//
// print
//

// print all elements in range, using optional separator between elements
template<typename InputIterator>
void
print_range(std::ostream &os, InputIterator first, InputIterator last, const char *sep="")
{
  for (const char *p = ""; first != last; p = sep)
    os << p << *first++;
}

// print all elements in a collection, using optional separator between elements
template<typename Tp>
void
print(std::ostream &os, const Tp &coll, const char *sep="")
{
  print_range(os, coll.begin(), coll.end(), sep);
}

//
// sum
//

// sum a range
template<typename Tp, typename InputIterator>
Tp
sum(InputIterator first, InputIterator last)
{
  Tp sum = 0;
  while (first != last)
    sum += *first++;
  return sum;
}

// sum a member of a range
template<typename Tp, typename InputIterator, typename Ret>
Ret
sum(InputIterator first, InputIterator last, Ret Tp::*mem)
{
  Ret sum = 0;
  while (first != last)
    sum += (*first++).*mem;
  return sum;
}

// sum a member (referenced by function) of a range
template<typename Tp, typename InputIterator, typename Ret>
Ret
sum(InputIterator first, InputIterator last, Ret &(Tp::*f)())
{
  Ret sum = 0;
  while (first != last)
    sum += mem_fun_ref(f)(*first++);
  return sum;
}

//
// scale
//

// scale a member of a range to sum to a given total; return scale factor
template<typename Tp, typename InputIterator, typename Ret>
Ret
scale_member(InputIterator first, InputIterator last, Ret Tp::*mem, Ret total)
{
  Ret scale_factor = total / sum(first, last, mem);
  while (first != last)
    (*first++).*mem *= scale_factor;
  return scale_factor;
}

// scale a member (referenced by function) of a range to sum to a given total; return scale factor
template<typename Tp, typename InputIterator, typename Ret>
Ret
scale_member_function(InputIterator first, InputIterator last, Ret &(Tp::*f)(), Ret total)
{
  Ret scale_factor = total / sum(first, last, f);
  while (first != last)
    mem_fun_ref(f)(*first++) *= scale_factor;
  return scale_factor;
}

//
// transform
//

// find the "middle" of a set of values computed by a function over a range
template<typename Ret, typename InputIterator, typename Function>
Ret
transformed_middle_second(InputIterator first, InputIterator last, Function f)
{
  Ret min;
  Ret max;
  if (first != last)
    min = max = f(first++->second);
  while (first != last) {
    Ret val = f(first++->second);
    if (val < min) min = val;
    else if (val > max) max = val;
  }
  return (min + max) / 2;
}

#endif // STL_EXTENSIONS_H
