/*

array iterator templates
Copyright (C) 2000-2010 Questrel, Inc.

*/
#ifndef ARRAY_ITERATOR
#define ARRAY_ITERATOR

#include <iterator>

namespace questrel {

// generic array iterator template
template<typename T>
class array_iterator : public std::iterator<std::random_access_iterator_tag, T> {
private:
  typename std::iterator<std::random_access_iterator_tag, T>::pointer current;
public:
  array_iterator() {}
  array_iterator( typename std::iterator<std::random_access_iterator_tag, T>::pointer current_) : current(current_) {}
  typename std::iterator<std::random_access_iterator_tag, T>::pointer operator->() const { return current; }
  typename std::iterator<std::random_access_iterator_tag, T>::value_type &operator*() const { return *current; }
  typename std::iterator<std::random_access_iterator_tag, T>::value_type &operator[](typename std::iterator<std::random_access_iterator_tag, T>::difference_type i) const { return current[i]; }
  array_iterator &operator++() { ++current; return *this;}
  array_iterator operator++(int) { return array_iterator(current++); }
  array_iterator &operator--() { --current; return *this; }
  array_iterator operator--(int) { return array_iterator(current--); }
  array_iterator operator+(typename std::iterator<std::random_access_iterator_tag, T>::difference_type i) { return array_iterator(current + i); }
  array_iterator operator-(typename std::iterator<std::random_access_iterator_tag, T>::difference_type i) { return array_iterator(current - i); }
  array_iterator &operator+=(typename std::iterator<std::random_access_iterator_tag, T>::difference_type i) { current += i; return *this; }
  array_iterator &operator-=(typename std::iterator<std::random_access_iterator_tag, T>::difference_type i) { current -= i; return *this; }
  bool operator==(const array_iterator &a) const { return current == a.current; }
  bool operator!=(const array_iterator &a) const { return !operator==(a); } // requires only that == is defined for T*
  bool operator<(const array_iterator &a) const { return current < a.current; }
  bool operator>=(const array_iterator &a) const { return !operator<(a); } // requires only that < is defined for T*
  bool operator>(const array_iterator &a) const { return a.current < current; } // requires only that < is defined for T*
  bool operator<=(const array_iterator &a) const { return !operator>(a); } // requires only that < is defined for T*
};

// generic array iterator template
template<typename T>
class new_array_iterator : public std::iterator<std::random_access_iterator_tag, T> {
private:
  typename std::iterator<std::random_access_iterator_tag, T> current;
public:
  new_array_iterator() {}
  new_array_iterator( typename std::iterator<std::random_access_iterator_tag, T> current_) : current(current_) {}
  typename std::iterator<std::random_access_iterator_tag, T>::pointer operator->() const { return current.operator->(); }
  typename std::iterator<std::random_access_iterator_tag, T>::value_type &operator*() const { return *current; }
  typename std::iterator<std::random_access_iterator_tag, T>::value_type &operator[](typename std::iterator<std::random_access_iterator_tag, T>::difference_type i) const { return current[i]; }
  new_array_iterator &operator++() { ++current; return *this;}
  new_array_iterator operator++(int) { return new_array_iterator(current++); }
  new_array_iterator &operator--() { --current; return *this; }
  new_array_iterator operator--(int) { return new_array_iterator(current--); }
  new_array_iterator operator+(typename std::iterator<std::random_access_iterator_tag, T>::difference_type i) { return new_array_iterator(current + i); }
  new_array_iterator operator-(typename std::iterator<std::random_access_iterator_tag, T>::difference_type i) { return new_array_iterator(current - i); }
  new_array_iterator &operator+=(typename std::iterator<std::random_access_iterator_tag, T>::difference_type i) { current += i; return *this; }
  new_array_iterator &operator-=(typename std::iterator<std::random_access_iterator_tag, T>::difference_type i) { current -= i; return *this; }
  bool operator==(const new_array_iterator &a) const { return current == a.current; }
  bool operator!=(const new_array_iterator &a) const { return !operator==(a); } // requires only that == is defined for T*
  bool operator<(const new_array_iterator &a) const { return current < a.current; }
  bool operator>=(const new_array_iterator &a) const { return !operator<(a); } // requires only that < is defined for T*
  bool operator>(const new_array_iterator &a) const { return a.current < current; } // requires only that < is defined for T*
  bool operator<=(const new_array_iterator &a) const { return !operator>(a); } // requires only that < is defined for T*
};

// generic array const_iterator template
template<typename T>
class array_const_iterator : public array_iterator<const T> { };

// generic array reverse iterator template
template<typename T>
class array_reverse_iterator : public std::iterator<std::random_access_iterator_tag, T> {
private:
  typename std::iterator<std::random_access_iterator_tag, T>::pointer current;
public:
  array_reverse_iterator() {}
  array_reverse_iterator(typename std::iterator<std::random_access_iterator_tag, T>::pointer current_) : current(current_) {}
  typename std::iterator<std::random_access_iterator_tag, T>::pointer operator->() const { return current; }
  typename std::iterator<std::random_access_iterator_tag, T>::value_type &operator*() const { return *current; }
  typename std::iterator<std::random_access_iterator_tag, T>::value_type &operator[](typename std::iterator<std::random_access_iterator_tag, T>::difference_type i) const { return current[-i]; }
  array_reverse_iterator &operator++() { --current; return *this;}
  array_reverse_iterator operator++(int) { return array_reverse_iterator(current--); }
  array_reverse_iterator &operator--() { ++current; return *this; }
  array_reverse_iterator operator--(int) { return array_reverse_iterator(current++); }
  array_reverse_iterator operator+(typename std::iterator<std::random_access_iterator_tag, T>::difference_type i) { return array_reverse_iterator(current - i); }
  array_reverse_iterator operator-(typename std::iterator<std::random_access_iterator_tag, T>::difference_type i) { return array_reverse_iterator(current + i); }
  array_reverse_iterator &operator+=(typename std::iterator<std::random_access_iterator_tag, T>::difference_type i) { current -= i; return *this; }
  array_reverse_iterator &operator-=(typename std::iterator<std::random_access_iterator_tag, T>::difference_type i) { current += i; return *this; }
  bool operator==(const array_reverse_iterator &a) const { return current == a.current; }
  bool operator!=(const array_reverse_iterator &a) const { return !operator==(a); } // requires only that == is defined for T*
  bool operator<(const array_reverse_iterator &a) const { return a.current < current; }
  bool operator>=(const array_reverse_iterator &a) const { return !operator<(a); } // requires only that < is defined for T*
  bool operator>(const array_reverse_iterator &a) const { return current < a.current; } // requires only that < is defined for T*
  bool operator<=(const array_reverse_iterator &a) const { return !operator>(a); } // requires only that < is defined for T*
};

// generic array reverse_const_iterator template
template<typename T>
class array_reverse_const_iterator : public array_reverse_iterator<const T> { };

// distance() and advance() function templates
template<typename T>
typename array_iterator<T>::difference_type distance(const array_iterator<T> &first, const array_iterator<T> &last) { return last.operator->() - first.operator->(); };

template<typename T>
void advance(array_iterator<T> &a, typename array_iterator<T>::difference_type i) { a += i; };

template<typename T>
typename array_const_iterator<T>::difference_type distance(const array_const_iterator<T> &first, const array_const_iterator<T> &last) { return last.operator->() - first.operator->(); };

template<typename T>
void advance(array_const_iterator<T> &a, typename array_const_iterator<T>::difference_type i) { a += i; };

template<typename T>
typename array_reverse_iterator<T>::difference_type distance(const array_reverse_iterator<T> &first, const array_reverse_iterator<T> &last) { return first.operator->() - last.operator->(); };

template<typename T>
void advance(array_reverse_iterator<T> &a, typename array_reverse_iterator<T>::difference_type i) { a -= i; };

template<typename T>
typename array_reverse_const_iterator<T>::difference_type distance(const array_reverse_const_iterator<T> &first, const array_reverse_const_iterator<T> &last) { return first.operator->() - last.operator->(); };

template<typename T>
void advance(array_reverse_const_iterator<T> &a, typename array_reverse_const_iterator<T>::difference_type i) { a -= i; };

}; // namespace questrel

#endif // ARRAY_ITERATOR
