#ifndef RADIX_MAP_H
#define RADIX_MAP_H

//
// Copyright (C) 2000 - 2019, Questrel, Inc.
//
// Radix map with persistence 
// 
//
// Assumes a C++ compiler with the standard template library.
// Uses POSIX compliant routines.  See below for routines used.
//
// TODO List
// 
// Node expansion project
//
// Allow remainder nodes to have children.
// Add infix nodes.
// Simplify and speed up insert() by adding parent pointer to node.
// Make advance() constant-time by adding next pointer to node.
//
// Data update project
//
// Add garbage collection.
// Implement erase(), shrink nodes, and coalescing allocate().
// Add list of allocation sizes (NB shorten_remainder).
//
// Other short projects
//
// Test if writing zeroes in set_length() avoids file fragmentation.
// On create, test if faster to MAP_PRIVATE and write() in ~radix_map().
// Convert POSIX calls to ISO calls.
//
// Other long projects
//
// Support regular expressions.
// Maintain max and longest across update and remove of values.
// Convert between byte orders.
// Make multithreaded using hazard pointers.
//

#include <cstring>              // for memcpy(), memmove(), memset()
#include <fcntl.h>              // for open(), etc.
#include <regex.h>              // for regcomp(), regerror(), regexec(), regfree(), regex_t, regmatch_t
#include <sys/stat.h>           // for fstat()
#include <sys/mman.h>           // for mmap(), munmap(), msync()
#include <sys/sem.h>            // for semget(), semop(), semctl()
#include <unistd.h>             // for ftruncate(), lseek(), sysconf(), write()

#include "iocommon.h"           // for my_assert
#include "ioerror.h"            // for Throw
#include "shared_memory_allocator.h"

//
// includes required for Standard Library routines
//

#include <algorithm>            // for std::fill(), std::copy(), std::copy_backward(), std::binary_search(), std::lower_bound(), std::upper_bound()
#include <iostream>             // for std::cerr (debug messages)
#include <fstream>              // for std::ifstream, std:ofstream
#include <map>                  // used in database methods
#include <set>                  // used in database methods
#include <sstream>              // for std::stringstream (exception message work area)
#include <stdexcept>            // for std::exception
#include <utility>              // for std::pair, std::make_pair()
#include <vector>               // used in database methods

namespace questrel {

////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
//  constants, types, data structures, utility routines
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////
// type definitions
////////////////////////////////////////////////////////////////////////////

// offsets are measured from root node in units of min_size
// TODO: convert between different endians
#ifdef BIT_64
typedef unsigned long offset; // 64-bit offset
#else
typedef unsigned int offset; // 32-bit offset
#endif
typedef unsigned long long time_stamp_t;
constexpr offset bit_mask(unsigned short bit_number) { return offset(1) << bit_number; }

////////////////////////////////////////////////////////////////////////////
// constants
////////////////////////////////////////////////////////////////////////////

enum : offset {

  /////////////////////////////////////////////////////////////////////////
  // option_flags bit definitions (16 bits) = flags specifying permanent options
  /////////////////////////////////////////////////////////////////////////
  unique =      bit_mask(0),  // unique key values
  sort_values = bit_mask(1),  // sort key values
  dictorder =   bit_mask(2),  // skip non-alphanumeric characters and sort digits as first letter of names (e.g., '1' == 'o', '2' == 't', etc.)
  dtree =       bit_mask(3),  // allow NUL in strings, terminate a string with two NULs, make lowercase letters common characters
  /////////////////////////////////////////////////////////////////////////
  // state_flags bit definitions (16 bits) = flags specifying run-time state
  /////////////////////////////////////////////////////////////////////////
  //
  // open mode 
  //
  read_only =   bit_mask(0),  // radix_map is read-only
  read_write =  bit_mask(1),  // radix_map is read-write
  create =      bit_mask(2),  // radix_map is to be initialized (this implies read_write)
  at_end =      bit_mask(3),  // radix_map is to refer to end of file (this implies create)
  //
  // flags from open(), close()
  //
  opened =      bit_mask(4),  // file was opened
  temp_file =   bit_mask(5),  // temporary file was opened
  multiuser =   bit_mask(6),  // make thread safe
  have_lock =   bit_mask(7),  // semaphore has been acquired
  //
  // flags from map(), unmap(), remap()
  //
  mapped =      bit_mask(8),  // data is currently mapped

  /////////////////////////////////////////////////////////////////////////
  // constants related to machine environment
  /////////////////////////////////////////////////////////////////////////

  bits_in_offset = 8 * sizeof(offset), // number of bits in offset
  ceiling_lg_bits_in_offset = (bits_in_offset <= 32 ? 5 : 6), // ceiling(lg(number of bits in offset))
  number_of_common_characters = bits_in_offset - ceiling_lg_bits_in_offset - 1, // bits in occupancy mask
  min_size = 8, // minimum size of storage block
  lg_min_size = 3, // lg(min_size)
  lg_max_count = bits_in_offset - lg_min_size, // lg(max number of blocks)
  max_count = bit_mask(lg_max_count),
  max_depth = 200, // maximum depth of index TODO: remove this
  max_key_length = 2 * max_depth // maximum length of key TODO: remove this
};


////////////////////////////////////////////////////////////////////////////
// function returning time stamp
////////////////////////////////////////////////////////////////////////////

static unsigned long microseconds = 0;
static time_t last_time;

// 64-bit time stamp
// comprises 44 bits of time_t followed by 20 bits of "microseconds" (which are not accurate)
// guaranteed to be unique as long as not called more than 2^20 = 1048576 times in one second
time_stamp_t timestamp() {
  time_t current_time = time(0);
  if (current_time == last_time)
    ++microseconds;
  else {
    microseconds = 0;
    last_time = current_time;
  }
  return (current_time << 20) + microseconds;
}

////////////////////////////////////////////////////////////////////////////
// class describing character set
////////////////////////////////////////////////////////////////////////////
class character_set_t {
  //
  // data and simple access members
  //

  //
  // lookup table mapping characters onto radix positions
  //
  unsigned char radix_by_character[256];
  unsigned char character_by_radix[256];
  unsigned short number_of_characters;
  bool end_at_double_NUL;

public:

  unsigned char invalid_radix;

  size_t size() { return number_of_characters; }

  //
  // swap the contents of this character set with passed character set
  //

  void swap(character_set_t &x) {
    for (unsigned char *p = radix_by_character, *q = x.radix_by_character; p != radix_by_character + 256; )
      std::swap(*p++, *q++);
    for (unsigned char *p = character_by_radix, *q = x.character_by_radix; p != character_by_radix + 256; )
      std::swap(*p++, *q++);
    std::swap(number_of_characters, x.number_of_characters);
    std::swap(end_at_double_NUL, x.end_at_double_NUL);
    /* TODO
    typedef std::array<char, sizeof(character_set_t)> Array;
    std::swap<Array>(Array(this), Array(&x));
    */
  }

  //
  // returns radix position for character
  //
  unsigned char toradix(unsigned char c) const {
    return radix_by_character[c];
  }

  //
  // returns character equivalent of radix position
  //
  unsigned char tocharacter(unsigned char r) const {
    return character_by_radix[r];
  }

  //
  // returns true if p is at the end of the key
  //
  bool at_end(const char *p) const {
    return (*p == 0) ? (!end_at_double_NUL || (p[1] == 0)) : false;
  }

  //
  // constructor
  //
  character_set_t() {}

  character_set_t(unsigned short option_flags) { init(option_flags); }
  //
  // initialize
  // index all graphical characters, reserving low radix values for commonly indexed characters
  //
  void init(unsigned short option_flags) {
    // compute number_of_characters
    if (option_flags & dtree) {
      end_at_double_NUL = true;
      number_of_characters = 256; // all characters are valid
    } else {
      end_at_double_NUL = false; // end at single NUL
      number_of_characters = 0;
      for (unsigned char c = 0; c != 255; ++c)
        if (isprint(c)) // includes 0x20 (space)
          ++number_of_characters;
    }
    // invalid radix = one more than any radix
    invalid_radix = number_of_characters;
    // set all radices to invalid
    memset(radix_by_character, invalid_radix, sizeof(radix_by_character));
    // fill in radix values for lowercase letters (common characters for index compression)
    my_assert(number_of_common_characters > 'z' - 'a');
    unsigned char next_radix = 0;
    for (unsigned char c = 'a'; c <= 'z'; ++c)
      radix_by_character[c] = next_radix++;
    if (option_flags & dictorder) { // if dictsort, convert upper case letters and digits to lower case letters
      for (unsigned char c = 'A'; c <= 'Z'; ++c)
        radix_by_character[c] = radix_by_character[tolower(c)];
      for (unsigned char c = '0'; c <= '9'; ++c)
        radix_by_character[c] = radix_by_character["zottffssen"[c - '0']]; // index digits by the first letter of their name
    } else { // if not dictorder, fill in other radix values
      // fill in radix values for remaining common characters
      for (unsigned short i = 0; i != 256 && next_radix < number_of_common_characters; ++i)
        if (radix_by_character[i] == invalid_radix && (isalpha(i) || strchr(" -_/.%", i)))
          radix_by_character[i] = next_radix++;
      // fill in remaining radix values
      for (unsigned short i = 0; i != 256; ++i)
        if (radix_by_character[i] == invalid_radix)
          radix_by_character[i] = next_radix++;
    }
    // set all characters to invalid character
    memset(character_by_radix, 0, sizeof(character_by_radix));
    // fill in valid character values
    for (unsigned short i = 0; i != 256; ++i)
      if (radix_by_character[i] != invalid_radix)
        character_by_radix[radix_by_character[i]] = i;
  }

  ////////////////////////////////////////////////////////////////////////////
  // key comparison, length, and match length routines
  // minimal caseless letter-only comparison
  // execute minimum possible numbers of dereferences and comparisons 
  // uses toradix() function and invalid_radix value
  ////////////////////////////////////////////////////////////////////////////

  //
  // returns true if x < y
  //
  bool key_less(const char* const &x, const char* const &y) const {
    unsigned char c1, c2, r1, r2;
    for (const char *s1 = x, *s2 = y;;) {
      while ((c1 = *s1++) && (r1 = toradix(c1)) == invalid_radix);
      while ((c2 = *s2++) && (r2 = toradix(c2)) == invalid_radix);
      if (!c1 || !c2)
	return c1 < c2;
      if (r1 != r2)
	break;
    }
    return r1 < r2;
  }

  //
  // returns true if x < y
  // allows use of character_set_t as a runtime sorting routine
  //
  bool operator()(const char* const  &x, const char* const &y) const { return key_less(x, y); }

  //
  // returns true if x == y
  //
  bool key_equal(const char* const &x, const char* const &y) const {
    unsigned char c1, c2, r1, r2;
    for (const char *s1 = x, *s2 = y;;) {
      while ((c1 = *s1++) && (r1 = toradix(c1)) == invalid_radix);
      while ((c2 = *s2++) && (r2 = toradix(c2)) == invalid_radix);
      if (!c1 || !c2)
        return c1 == c2;
      if (r1 != r2)
        break;
    }
    return false;
  }

  //
  // returns number of letters in string
  //
  unsigned short key_length(const char *s) const {
    unsigned short n = 0;
    for (unsigned char c; (c = *s++); )
      if (toradix(c) != invalid_radix)
        ++n;
    return n;
  }

  //
  // copy valid characters of source into target
  //
  void key_copy(char *target, const char *source) const {
    for (unsigned char c, radix; (c = *source++); )
      if ((radix = toradix(c)) != invalid_radix)
        *target++ = tocharacter(radix);
    *target = 0;
  }

  //
  // skips second argument forward over matching characters in first
  // returns length of match
  //
  unsigned short key_match_skip(const char *s1, const char *&s2ret, bool skip_invalid) const {
    unsigned short l;
    const char *s2 = s2ret;
    unsigned char c1, c2, r1, r2;
    for (l = 0; ; ++l) {
      while ((c1 = *s1++) && (r1 = toradix(c1)) == invalid_radix && skip_invalid);
      while ((c2 = *s2++) && (r2 = toradix(c2)) == invalid_radix && skip_invalid);
      if (!c1 || !c2 || r1 != r2)
        break;
    }
    s2ret = s2 - 1; // back up to mismatched letter
    return l;
  }

  //
  // returns strict radix match between first argument (NUL terminated) and second argument (unterminated)
  //
  int key_match_first_strict(const char *s1, const char *s2) const {
    unsigned char c1, c2;
    int r1, r2;
    for (;;) {
      r1 = static_cast<int>(toradix(c1 = *s1++));
      r2 = static_cast<int>(toradix(c2 = *s2++));
      if (!c1 || r1 != r2)
        break;
    }
    return !c1 ? 0 : r1 - r2;
  }

  //
  // arguments are composed of separator-delimited segments and are either NUL terminated or of known length
  // returns true if keys match up to a segment boundary, false if they mismatch at a non-segment boundary
  // set difference to <0, 0, >0 if first argument is less than, equal to, greater than second argument
  // if true is returned, set count to number of matched segments
  //
  bool key_match_count_segments(const char *s1, const char *s1_end, const char *s2, const char *s2_end, char separator, int &difference, size_t &count) const {
    bool retval = false;
    count = 0;
    unsigned char c1, c2;
    int r1, r2;
    for (;;) {
      r1 = static_cast<int>(toradix(c1 = *s1++));
      r2 = static_cast<int>(toradix(c2 = *s2++));
      if (!c1 || s1 - 1 == s1_end) { // at end of s1
        if (!c2 || s2 - 1 == s2_end) { // at end of s2
          ++count;
          retval = true;
          difference = 0; // s1 == s2
        } else if (c2 == separator) { // s2 is at segment boundary
          ++count;
          retval = true;
          difference = -1; // s1 < s2
        }
        break; // return
      }
      if (!c2 || s2 - 1 == s2_end) { // at end of s2
        if (c1 == separator) { // s1 is at segment boundary
          ++count;
          retval = true;
          difference = 1; // s1 > s2
        }
        break; // return
      }
      if (r1 != r2) // strings do not match in middle of segment
        break; // return
      if (c1 == separator) // at end of segment
        ++count;
    }
    return retval;
  }

};

////////////////////////////////////////////////////////////////////////////
// class describing key comparison
////////////////////////////////////////////////////////////////////////////
class key_comparison_t {

////////////////////////////////////////////////////////////////////////////
// data members
// NB. swap() must be kept in sync with data members
////////////////////////////////////////////////////////////////////////////

  //
  // compiled regular expression
  //
  //regex_t regex;

  //
  // status of regular expression
  //
  int regex_status;

public:
  void swap(key_comparison_t &x) {
    //std::swap(regex, x.regex);
    //std::swap(regex_status, x.regex_status);
  }


////////////////////////////////////////////////////////////////////////////
// constructors and destructors
////////////////////////////////////////////////////////////////////////////
public:
  key_comparison_t() : regex_status(0 /* REG_BADPAT */ ) {}

  key_comparison_t(const key_comparison_t &k) {
    //regex = x.regex;
    //regex_status = x.regex_status;
  }

  ~key_comparison_t() {
    //if (regex_status == 0)
      //regfree(&regex);
  }

////////////////////////////////////////////////////////////////////////////
// methods
////////////////////////////////////////////////////////////////////////////
  
  //
  // test for regular expression
  //

  bool is_regular_expression(const char *string) {
    return strcspn(string, "^$*?[]") != strlen(string);
  }

  //
  // initializer
  //

  void init(const char *regex_) {
    /*
    regex_status = regcomp(&regex, regex_, REG_EXTENDED | REG_NOSUB);
    if (regex_status) {
      std::stringstream error;
      char errbuf[256];
      regerror(regex_status, &regex, errbuf, sizeof(errbuf));
      error << errbuf << Throw;
    }
    */
  }

  //
  // validity operator
  //

  operator bool(void) {
    return regex_status == 0;
  }

  //
  // comparison
  //

  bool match(const char *key) {
    return regex_status == 0 /* && regexec(&regex, key, 0, 0, 0) == 0*/;
  }

};

////////////////////////////////////////////////////////////////////////////
// index entry
////////////////////////////////////////////////////////////////////////////

// entry layout for 32-bit offset
// ----------------------------------------------------------------------
// | type (3 bits) |           offset  (29 bits)                        |
// ----------------------------------------------------------------------
// |  single value or value list offset (max(32, 8 * sizeof(value))     |
// ----------------------------------------------------------------------
//
// entry layout for 64-bit offset
// ----------------------------------------------------------------------
// | type (3 bits) |           offset  (61 bits)                        |
// ----------------------------------------------------------------------
// |  single value or value list offset (max(64, 8 * sizeof(value))     |
// ----------------------------------------------------------------------
//
// types of entry
// child
// ----------------------------------------------------------------------
// | type |                     child offset                            |
// ----------------------------------------------------------------------
// |               single value or value list offset                    |
// ----------------------------------------------------------------------
//
// remainder
// ----------------------------------------------------------------------
// | type |                    remainder offset                         |
// ----------------------------------------------------------------------
// |               single value or value list offset                    |
// ----------------------------------------------------------------------
//

template <typename Value>
struct Entry {

  typedef Entry<Value> entry;

  //
  // data fields
  //

private:
  // no more than lg_min_size bits available for type
  enum {
    child = 1, // nonzero to catch uninitialized memory references
    child_value,
    child_list,
    remainder_value, // NB. there is no pure remainder type
    remainder_list
  };
  struct {
    offset type : lg_min_size;
    offset entry_offset : lg_max_count;
  } s;
  char data[(sizeof(offset) > sizeof(Value)) ? sizeof(offset) : sizeof(Value)]; // single value or offset of value list

  //
  // access to data fields
  //

public:
  bool is_empty() const { // NB: is_empty() <=> !has_child() && !has_value()
    return s.type == child && s.entry_offset == 0;
  }
  void copy(const entry *e) {
    s = e->s;
    memcpy(data, e->data, sizeof(data));
  }
  void make_empty() {
    s.type = child;
    s.entry_offset = 0;
  }
  bool has_child() const {
    return (s.type == child || s.type == child_value || s.type == child_list) &&
      s.entry_offset != 0; // no child node has node offset == 0
  }
  bool has_value() const {
    return s.type != child;
  }
  bool has_single_value() const {
    return s.type == child_value || s.type == remainder_value;
  }
  bool has_value_list() const {
    return s.type == child_list || s.type == remainder_list;
  }
  bool is_remainder() const {
    return s.type == remainder_value || s.type == remainder_list;
  }
  void remove_remainder() {
    s.type = (s.type == remainder_list) ? child_list : child_value;
    set_remainder_offset(0);
  }
  offset get_child_offset() const {
    return s.entry_offset;
  }
  void set_child_offset(offset o) {
    s.entry_offset = o;
  }
  offset get_remainder_offset() const {
    return s.entry_offset;
  }
  void set_remainder_offset(offset o) {
    s.entry_offset = o;
  }
  void make_pure_child(offset o) {
    s.type = child;
    set_child_offset(o);
  }
  void make_pure_value(const Value &value) {
    s.type = child_value;
    set_child_offset(0);
    set_value(value);
  }
  void copy_pure_value(entry *e) {
    s.type = child_value;
    set_child_offset(0);
    copy_value(e);
  }
  void make_child_value(offset o, const Value &value) {
    s.type = child_value;
    set_child_offset(o);
    set_value(value);
  }
  void make_remainder(offset r, const Value &value) {
    s.type = remainder_value;
    set_remainder_offset(r);
    set_value(value);
  }
  void copy_remainder(offset r, entry *e) {
    s.type = remainder_value;
    set_remainder_offset(r);
    copy_value(e);
  }
  Value &get_value() {
    return reinterpret_cast<Value &>(data);
  }
  void set_value(const Value &value_) {
    if (s.type == child)
      s.type = child_value;
    get_value() = value_;
  }
  void remove_value() {
    s.type = child;
  }
  offset &get_value_list_offset() {
    return reinterpret_cast<offset &>(data);
  }
  void set_value_list_offset(offset o) {
    if (s.type == child_value)
      s.type = child_list;
    else if (s.type == remainder_value)
      s.type = remainder_list;
    get_value_list_offset() = o;
  }
  void copy_value(entry *e) {
    if (e->has_single_value())
      set_value(e->get_value());
    else
      set_value_list_offset(e->get_value_list_offset());
  }
  void convert_list_to_value(const Value &value) {
    if (s.type == remainder_list)
      s.type = remainder_value;
    else if (s.type == child_list)
      s.type = child_value;
    else {
      std::stringstream error;
      error << "convert_list_to_value: not a list, type = " << s.type << Throw;
    }
    set_value(value);
  }

};

////////////////////////////////////////////////////////////////////////////
// index node
////////////////////////////////////////////////////////////////////////////
//
// layout of nodes for 32-bit offset
// index node
// ----------------------------------------------------------------------
// | lg(count) (5 bits) | expanded flag (1) | occupancy mask (26 bits)  |
// ----------------------------------------------------------------------
// |    array of entries (if expanded, number of characters, else 26)   |
// ----------------------------------------------------------------------
//
// value list
// ----------------------------------------------------------------------
// | lg(count) (5 bits)  |        number of values (27 bits)            |
// ----------------------------------------------------------------------
// |           array of values (max 2^27 - 1 = 134,216,731)             |
// ----------------------------------------------------------------------
//
// remainder
// ----------------------------------------------------------------------
// | lg(count) (5 bits)  |         reference count (27 bits)            |
// ----------------------------------------------------------------------
// |               array of bytes (max 2^31 - 4)                        |
// ----------------------------------------------------------------------
//
// layout of nodes for 64-bit offset
// index node
// ----------------------------------------------------------------------
// | lg(count) (6 bits) | expanded flag (1) | occupancy mask (57 bits)  |
// ----------------------------------------------------------------------
// |    array of entries (if expanded, number of characters, else 57)   |
// ----------------------------------------------------------------------
//
// value list
// ----------------------------------------------------------------------
// | lg(count) (6 bits)  |        number of values (58 bits)            |
// ----------------------------------------------------------------------
// |               array of values (max 2^58 - 1)                       |
// ----------------------------------------------------------------------
//
// remainder
// ----------------------------------------------------------------------
// | lg(count) (6 bits)  |         reference count (58 bits)            |
// ----------------------------------------------------------------------
// |                array of bytes (max 2^63 - 8)                       |
// ----------------------------------------------------------------------
//
template <typename Value>
struct Node {

  typedef Node<Value> node;
  typedef Entry<Value> entry;

  //
  // data fields
  //

  union {
    // NB.  These bit fields all must be typed as unsigned long so that
    // Microsoft VC++ 7.0 will pack them all into one offset.
    struct {
      offset c : ceiling_lg_bits_in_offset; // lg(length / min_size)
      offset e: 1; // expanded flag
      offset m : bits_in_offset - ceiling_lg_bits_in_offset - 1; // occupancy mask
      //entry a[] entry list
    } index_node;
    struct {
      offset c : ceiling_lg_bits_in_offset; // lg(length / min_size)
      offset m : bits_in_offset - ceiling_lg_bits_in_offset; // count
      //Value v[] value list
    } value_list_node;
    struct {
      offset c : ceiling_lg_bits_in_offset; // lg(length / min_size)
      offset m : bits_in_offset - ceiling_lg_bits_in_offset; // reference count
      //char r[] remainder
    } remainder_node;
  } u;

  //
  // access to universal data fields
  //

  unsigned short get_lg_count() const { return u.index_node.c; }
  void set_lg_count(unsigned short l) { u.index_node.c = l; }
  offset size() const {
    return min_size << get_lg_count();
  }
  offset capacity(offset units = 1) const {
    return (size() - sizeof(node)) / units;
  }
  char *body() const { return const_cast<char *>(reinterpret_cast<const char *>(this)) + sizeof(u); }

  //
  // access to index node data fields
  //

  offset get_occupancy_mask() const { return u.index_node.m; }
  void set_occupancy_mask(offset m1) { u.index_node.m = m1; }
  void add_occupancy_mask(offset m1) { u.index_node.m |= m1; }
  void remove_occupancy_mask(offset m1) { u.index_node.m &= ~m1; }
  bool is_expanded() const { return u.index_node.e == 1; }
  void set_expanded_flag(bool is_expanded) { u.index_node.e = (is_expanded ? 1 : 0); }
  entry *entry_array(void) const { return reinterpret_cast<entry *>(body()); }

  //
  // access to value list data fields
  //

  offset get_count() const { return u.value_list_node.m; }
  void set_count(unsigned long m1) { u.value_list_node.m = m1; }
  void incr_count() { ++u.value_list_node.m; }
  void decr_count() { --u.value_list_node.m; }
  Value *get_list() const { return reinterpret_cast<Value *>(body()); }
  Value &value(offset i) const { return (*(get_list() + i)); }

  //
  // access to remainder data fields
  //

  offset get_reference_count() const { return u.remainder_node.m; }
  void set_reference_count(unsigned long m1) { u.remainder_node.m = m1; }
  void incr_reference_count() { ++u.remainder_node.m; }
  char *get_remainder() const { return body(); }

  //
  // count set bits in mask
  //

  unsigned short count_bits(offset mask) const {
    unsigned short retval = 0;
    while (mask) {
      mask &= (mask - 1); // zero low order bit
      ++retval;
    }
    return retval;
  }

  //
  // return number of used entries if not expanded, otherwise number of entries that can fit in node
  //

  unsigned short number_of_entries(void) const
  {
    if (is_expanded())
      return std::min<offset>(256, capacity(sizeof(entry))); // cannot be longer than largest radix = largest unsigned char = 255
    else
      return count_bits(get_occupancy_mask());
  }

  //
  // return number of entries with masks < given mask
  //

  unsigned short number_of_entries(unsigned long mfind) const
  {
    // mask off all bits >= mfind
    return count_bits(get_occupancy_mask() & (mfind - 1));
  }

  //
  // return radix associated with passed entry in node
  //

  unsigned char get_radix(const entry *e) const
  {
    if (is_expanded())
      return e - entry_array();
    else {
      offset m = get_occupancy_mask();
      unsigned char radix = 0;
      for (entry *e1 = entry_array(); m; ++radix, m >>= 1)
        if (m & 1)
          if (e == e1++)
            return radix;
    }
    std::stringstream error;
    error << "get_radix: entry not in node" << Throw;
    return 0; // not reached
  }

  //
  // return first non-empty entry in node, or NULL if none
  //

  entry *first_entry() const
  {
    entry *retval = 0;
    if (!is_expanded()) {
      if (get_occupancy_mask() != 0)
        retval = entry_array();
    } else {
      for (entry *e = entry_array(), *e1 = e + number_of_entries(); e != e1; ++e)
        if (!e->is_empty()) {
          retval = e;
          break;
        }
    }
    return retval;
  }

  //
  // copy data fields (except for lg_count, which is maintained by allocate())
  //

  void copy(node *old) {
    set_count(old->get_count());
    memcpy(reinterpret_cast<char *>(entry_array()), reinterpret_cast<char *>(old->entry_array()), old->endof_node() - reinterpret_cast<char *>(old->entry_array()));
  }

  //
  // return the end of the node
  //

  const char *endof_node() const { return reinterpret_cast<const char *>(this) + (min_size << get_lg_count()); }

  //
  // return the point halfway through the node
  //

  const char *halfway() const { return reinterpret_cast<const char *>(this) + (min_size << get_lg_count()) / 2; }

  //
  // return the end of the used array
  //

  entry *end_array() const { return entry_array() + number_of_entries(); }

  //
  // check if at end of array
  //

  bool at_end(entry *e) const { return e >= end_array(); }

  //
  // return bytes left in node
  //

  offset room() const { 
    if (endof_node() < reinterpret_cast<char *>(end_array())) {
      std::stringstream error;
      error << "room: end of node - end of array = " << endof_node() - reinterpret_cast<char *>(end_array()) << Throw;
    }
    return endof_node() - reinterpret_cast<char *>(end_array());
  }

  //
  // check if there are length bytes left in node
  //

  bool is_room(offset length) const { return length <= room(); }

  //
  // make room in front of given entry
  //

  void make_room(entry *e, offset length) {
    long l = endof_node() - (reinterpret_cast<char *>(e) + length);
    if (l < 0) {
      std::stringstream error;
      error << "make_room: l = " << l << " < 0" << Throw;
    }
    if (l > 0)
      memmove(reinterpret_cast<char *>(e) + length, reinterpret_cast<char *>(e), l);
  }

  //
  // unmake room at given entry
  // TODO: recover space if node is less than half full
  //

  void unmake_room(entry *e, offset length) {
    int l = endof_node() - (reinterpret_cast<char *>(e) + length);
    if (l < 0) {
      std::stringstream error;
      error << "unmake_room: l < 0" << Throw;
    }
    if (l > 0)
      memcpy(reinterpret_cast<char *>(e), reinterpret_cast<char *>(e) + length, l);
  }


  //
  // copy a pure value entry at given entry
  //

  void copy_pure_value_entry(entry *e1, entry *e2) {
    e1->copy_pure_value(e2);
  }

  //
  // copy a remainder entry at given entry
  //

  void copy_remainder_entry(entry *e1, offset r, entry *e2) {
    e1->copy_remainder(r, e2);
  }

};

////////////////////////////////////////////////////////////////////////////
// index position
////////////////////////////////////////////////////////////////////////////

template <typename Value>
struct Position {
  typedef Entry<Value> entry;
  typedef Node<Value> node;
  node *current_node;
  entry *current_entry;
  Position(node *n) : current_node(n), current_entry(n->entry_array()) {}
  Position(node *n, entry *e) : current_node(n), current_entry(e) {}
  Position() {}
};

////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
//  iterator
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////
// iterator base class
////////////////////////////////////////////////////////////////////////////

//
// typedefs used in iterators
//

template<typename Value>
struct iterator_base : public std::iterator<std::forward_iterator_tag, std::pair<const char *, Value> > {

////////////////////////////////////////////////////////////////////////////
// data members
// NB. copy and swap() for iterator_template must be kept in sync with data members
////////////////////////////////////////////////////////////////////////////

  //
  // beginning of index
  //

  char *base;

  //
  // depth of position in index
  //

  unsigned short depth;

  //
  // key of current position
  //

  char key[max_key_length];

  //
  // value list current pointer and end
  // these are maintained by:
  // 
  // find_value() - set based on terminal entry found
  // advance() - set based on terminal entry found or call find_value()
  //
  // find_entry() - set based on entry at top iterator position
  // add_value() - set to point to last value in value list
  //

  Value *value_list_pointer;
  Value *value_list_end;

  //
  // constructor
  //

  iterator_base() : base(0), depth(0), value_list_pointer(0) {}

};

////////////////////////////////////////////////////////////////////////////
// iterator comparison operators
// defined here so iterators and const_iterators can be compared
////////////////////////////////////////////////////////////////////////////

template<typename Value>
inline bool operator==(const iterator_base<Value> &x,
                       const iterator_base<Value> &y) {
  return
    x.base == y.base &&
    x.depth == y.depth &&
    (x.depth < 2 || // 0 = end(), 1 = begin()
    (!memcmp(x.key, y.key, x.depth - 1) && // keys match
    x.value_list_pointer == y.value_list_pointer)); // value list pointers match
};

template<typename Value>
inline bool operator!=(const iterator_base<Value> &x,
                       const iterator_base<Value> &y) {
  return !operator==(x, y);
};

////////////////////////////////////////////////////////////////////////////
// iterator class template
////////////////////////////////////////////////////////////////////////////

template <typename Value>
struct iterator_template : public iterator_base<Value> {

////////////////////////////////////////////////////////////////////////////
// typedefs and using declarations required by C++ Standard [temp/dep]/3 (cf. http://gcc.gnu.org/gcc-3.4/changes.html)
// names invisible in base template classes with parameters shared with derived template class
////////////////////////////////////////////////////////////////////////////
  typedef typename std::iterator<std::forward_iterator_tag, std::pair<const char *, Value> >::value_type value_type;
  typedef typename std::iterator<std::forward_iterator_tag, std::pair<const char *, Value> >::reference reference;
  typedef typename std::iterator<std::forward_iterator_tag, std::pair<const char *, Value> >::pointer pointer;

  using iterator_base<Value>::base;
  using iterator_base<Value>::depth;
  using iterator_base<Value>::key;
  using iterator_base<Value>::value_list_pointer;
  using iterator_base<Value>::value_list_end;

////////////////////////////////////////////////////////////////////////////
// type defintions
////////////////////////////////////////////////////////////////////////////

  typedef Node<Value> node;
  typedef Entry<Value> entry;
  typedef Position<Value> position;

////////////////////////////////////////////////////////////////////////////
// data members
// NB. swap() must be kept in sync with data members
////////////////////////////////////////////////////////////////////////////

  //
  // stack of index positions
  //

  position refs[max_depth];

  //
  // key comparator
  //

  key_comparison_t key_comparison;

  //
  // character set
  //

  const character_set_t *character_set;

  //
  // swap the contents of this iterator_template with passed iterator_template
  //

  void swap(iterator_template &x) {
    std::swap(base, x.base);
    std::swap(depth, x.depth);
    for (int i = 0; i != sizeof(key)/sizeof(*key); ++i)
      std::swap(key[i], x.key[i]);
    for (int i = 0; i != sizeof(refs)/sizeof(*refs); ++i)
      std::swap(refs[i], x.refs[i]);
    key_comparison.swap(x.key_comparison);
    std::swap(character_set, x.character_set);
    std::swap(value_list_pointer, x.value_list_pointer);
    std::swap(value_list_end, x.value_list_end);
  }


////////////////////////////////////////////////////////////////////////////
// routines to initialize, access, and compare iterator information
////////////////////////////////////////////////////////////////////////////

  void init(node *root, const character_set_t *character_set_, const char *key = 0) {
    base = (char *)root;
    depth = 0;
    push(position(root)); // null key
    set_value_list_pointer(0);
    character_set = character_set_;
    if (key && key_comparison.is_regular_expression(key))
      key_comparison.init(key);
  }

  const char *get_key(void) const {
    entry *e = top_entry();
    // TODO: cast away const because key is being modified
    if (!e->is_remainder())
      const_cast<char *>(key)[depth - 1] = 0;
    else {
      const char *remainder = get_node(e->get_remainder_offset())->get_remainder();
      size_t l = strlen(remainder);
      if (depth + l > max_key_length) {
        std::stringstream error;
        error << "get_key: key length = " << depth + l << " is more than maximum " << max_key_length << Throw;
      }
      memcpy(const_cast<char *>(key) + depth - 1, remainder, l + 1);
    }
    return key;
  }

  Value *get_value_ptr(void) const {
    entry *e = top_entry();
    if (e->has_single_value())
      return &e->get_value();
    else {
      if (!e->has_value_list()) {
        std::stringstream error;
        error << "get_value_ptr: !e->has_value_list()" << Throw;
      }
      if (value_list_pointer == 0) {
        std::stringstream error;
        error << "get_value_ptr: value_list_pointer == 0" << Throw;
      }
      return value_list_pointer;
    }
  }

////////////////////////////////////////////////////////////////////////////
// STL type definitions and operators
////////////////////////////////////////////////////////////////////////////

public:

  iterator_template() : character_set(0) {}

  iterator_template(char *b) : character_set(0) { base = b; }

  iterator_template(node *root, const character_set_t *character_set_) {
    init(root, character_set_);
  }

  iterator_template(const iterator_template &x) {
    base = x.base;
    depth = x.depth;
    strcpy(key, x.key);
    for (int i = 0; i != sizeof(refs)/sizeof(*refs); ++i)
      refs[i] = x.refs[i];
    key_comparison = x.key_comparison;
    character_set = x.character_set;
    value_list_pointer = x.value_list_pointer;
    value_list_end = x.value_list_end;
  }

  //
  // pair to return to STL routines expecting one
  //

  mutable value_type dummy;

  reference operator*() const {
    dummy.first = get_key();
    dummy.second = *get_value_ptr();
    return dummy;
  }

  pointer operator->() const { return &(operator*()); }

  iterator_template &operator++() { advance(); return *this; }

  iterator_template operator++(int) {
    iterator_template temp(*this);
    advance();
    return temp;
  }

  // TODO: implement decrement
  iterator_template operator+(int i) {
    iterator_template temp(*this);
    while (i-- > 0)
      temp.advance();
    return temp;
  }

////////////////////////////////////////////////////////////////////////////
// non-STL iterator functions
////////////////////////////////////////////////////////////////////////////
public:

  //
  // return the count of the number of values in the current entry
  //

  offset count() const { 
    if (depth == 0)
      return 0;
    else if (has_value_list())
      return get_node(top_entry()->get_value_list_offset())->get_count();
    else
      return 1;
  }

  //
  // return the address of the value or value list in the current entry
  //

  Value *value() const { 
    if (depth == 0)
      return 0;
    else if (!has_value_list())
      return &top_entry()->get_value();
    else // return value list
      return get_node(top_entry()->get_value_list_offset())->get_list();
  }

  //
  // return the offset of the value or value list in the current entry
  //

  offset address() const { 
    if (depth == 0)
      return 0;
    else
      return ((char *)value() - (char *)base) / min_size;
  }


////////////////////////////////////////////////////////////////////////////
// internal iterator functions
////////////////////////////////////////////////////////////////////////////

  //
  // get address from offset
  //

  node *get_node(offset o) const {
    if (base == 0) {
      std::stringstream error;
      error << "base == 0" << Throw;
    }
    return (node *)(base + o * min_size);
  }

  //
  // find find bit in mask
  //

  unsigned short first_bit(long m) {
    if (m == 0) {
      std::stringstream error;
      error << "m == 0" << Throw;
    }
    unsigned short c = 0;
    while ((m & 1) == 0) {
      m >>= 1;
      ++c;
    }
    return c;
  }

  //
  // get and set the current node
  //

  node *top_node() const {
    if (depth <= 0) {
      std::stringstream error;
      error << "top_node: depth <= 0" << Throw;
    }
    return refs[depth - 1].current_node;
  }
  void set_top_node(node *n) {
    if (depth <= 0) {
      std::stringstream error;
      error << "set_top_node: depth <= 0" << Throw;
    }
    refs[depth - 1].current_node = n;
  }

  //
  // get and set the current entry or aspects of it
  //

  entry *top_entry() const {
    if (base == 0) {
      std::stringstream error;
      error << "top_entry: base == 0" << Throw;
    }
    if (depth <= 0) {
      std::stringstream error;
      error << "top_entry: depth <= 0" << Throw;
    }
    return refs[depth - 1].current_entry;
  }
  bool check_top_entry() const {
    if (base == 0) {
      std::stringstream error;
      error << "check_top_entry: base == 0" << Throw;
    }
    if (depth <= 0) {
      std::stringstream error;
      error << "check_top_entry: depth <= 0" << Throw;
    }
    // NB: see find_entry (!found condition) for explanation of this check 
    return !at_end();
  }
  void set_top_entry(entry *e) {
    if (base == 0) {
      std::stringstream error;
      error << "set_top_entry: base == 0" << Throw;
    }
    if (depth <= 0) {
      std::stringstream error;
      error << "set_top_entry: depth <= 0" << Throw;
    }
    refs[depth - 1].current_entry = e;
  }
  bool has_value() const { return check_top_entry() && top_entry()->has_value(); }
  bool has_single_value() const { return check_top_entry() && top_entry()->has_single_value(); }
  bool has_value_list() const { return check_top_entry() && top_entry()->has_value_list(); }

  Value &get_value() const { return top_entry()->get_value(); }
  void set_value(const Value &v) const { return top_entry()->set_value(v); }

  //
  // return the parent entry of the current entry
  //

  entry *top_parent() const {
    if (depth <= 1) {
      std::stringstream error;
      error << "top_parent: depth <= 1" << Throw;
    }
    return refs[depth - 2].current_entry;
  }

  //
  // check if the current entry is at the end of the current node
  // root is preallocated so it is a special case (no occupancy mask)
  //

  bool at_end() const {
    if (depth == 1) { // root
      entry *e = top_node()->entry_array();
      return e->is_empty() || top_entry() > e;
    }
    return top_node()->at_end(top_entry());
  }

  //
  // push a position onto the stack
  //

  void push(const position &l) { 
    if (depth >= max_depth) {
      std::stringstream error;
      error << "push: depth >= max_depth" << Throw;
    }
    refs[depth++] = l;
    key[depth - 1] = 0; // terminate with NUL
  }

  //
  // push a position onto the stack and update current key
  //

  void push(const position &l, unsigned char radix) {
    push(l);
    if (depth > 1)
      key[depth - 2] = character_set->tocharacter(radix);
  }

  //
  // pop a position from the stack
  //

  void pop() { 
    --depth;
    if (depth > 0)
      key[depth - 1] = 0; // terminate with NUL
  }

  //
  // increment the iterator to the next entry in the current node
  // if there is no next entry, return end_array()
  // update current key directly, bypassing push() and pop()
  //

  void increment() { 
    node *n = top_node();
    entry *e = top_entry() + 1; // next entry
    if (n->is_expanded()) { // entries are not contiguous; find first non-empty
      for (entry *e1 = n->end_array(); e != e1; ++e)
        if (!e->is_empty())
          break;
    }
    set_top_entry(e);
    if (depth > 1 && e != n->end_array()) // NB: if at end of node key[depth - 2] is undefined
      key[depth - 2] = character_set->tocharacter(n->get_radix(e));
  }

  //
  // get the index of current entry in the current value list
  //

  offset get_value_list_index() const { 
    offset retval = 0;
    if (has_value_list()) {
      node *n = get_node(top_entry()->get_value_list_offset());
      retval = value_list_pointer - n->get_list();
    }
    return retval;
  }

  //
  // set the iterator to a given entry in the current value list
  //

  void set_value_list_pointer(offset i) { 
    if (!has_value_list())
      value_list_pointer = 0;
    else {
      node *n = get_node(top_entry()->get_value_list_offset());
      if (i > n->get_count()) {
        std::stringstream error;
        error << "i > n->get_count()" << Throw;
      }
      value_list_pointer = n->get_list() + i;
      value_list_end = n->get_list() + n->get_count();
    }
  }

  //
  // increment the iterator through the current value list
  // return true if not yet at end
  //

  bool increment_value_list_pointer() { 
    if (value_list_pointer == 0) {
      std::stringstream error;
      error << "value_list_pointer == 0" << Throw;
    }
    return ++value_list_pointer < value_list_end;
  }


  //
  // descend one level in index, if possible
  //

  bool follow_child() {
    bool retval = false;
    entry *e = top_entry();
    if (e->has_child()) {
      node *n = get_node(e->get_child_offset());
      e = n->first_entry();
      if (e) {
        push(position(n, e), n->get_radix(e));
        retval = true;
      }
    }
    return retval;
  }

  //
  // increment the iterator to the next terminal node (a node with a value)
  // if key_comparison is set, skip over terminals with keys that do not satisfy key comparison
  // return end() (depth == 0) if there is no such entry
  // if skip_value_list is true, skip over value lists
  //

  void advance(bool skip_value_list = false) {
    if (depth == 0) return; // cannot advance from end()
    if (has_value_list() && !skip_value_list)
      if (increment_value_list_pointer())
        return;
    //do { // if key_comparison is set, loop until key match
      do { // loop until value found or end of index
        if (at_end()) {
          pop();
          if (depth != 0)
            increment();
        } else if (!follow_child())
          increment();
      } while (depth != 0 && (at_end() || !has_value()));
      if (depth != 0)
        set_value_list_pointer(0);
    //} while (depth != 0 && key_comparison && !key_comparison.match(get_key())); // key is specified and does not match; go to next
  }

};

////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
// class definition proper
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////

template <typename Value>
class radix_map {

////////////////////////////////////////////////////////////////////////////
// STL type definitions
////////////////////////////////////////////////////////////////////////////
public:

  typedef const char *key_type;
  typedef Value mapped_type;
  typedef std::pair<const key_type, mapped_type> value_type;
  typedef offset size_type;
  typedef void difference_type;

  struct key_compare : public std::binary_function<key_type, key_type, bool> {
    const character_set_t &cs;
    key_compare(const character_set_t &cs_) : cs(cs_) {}
    bool operator()(const key_type &x, const key_type &y) const {
      return cs.key_less(x, y);
    }
  };

  struct value_compare : public std::binary_function<value_type, value_type, bool> {
    const character_set_t &cs;
    value_compare(const character_set_t &cs_) : cs(cs_) {}
    bool operator()(const value_type &x, const value_type &y) const {
      return cs.key_less(x.first, y.first);
    }
  };

  typedef iterator_template<Value> iterator;
  typedef iterator_template<const Value> const_iterator;

////////////////////////////////////////////////////////////////////////////
// non-STL type definitions
////////////////////////////////////////////////////////////////////////////
public:

  bool key_less(const key_type &x, const key_type &y) const {
      return character_set.key_less(x, y);
  }

  bool key_equal(const key_type &x, const key_type &y) const {
    return character_set.key_equal(x, y);
  }

  bool key_match_first(const key_type &x, const key_type &y) const {
    return !character_set.key_match_first_strict(x, y);
  }

  // return true if segmented keys match
  // optionally, end of y may be passed
  bool key_match_segments(const key_type &x, const key_type &y, char separator, const key_type &y_end = 0) const {
    int difference;
    size_t count;
    return character_set.key_match_count_segments(x, 0, y, y_end, separator, difference, count) && difference == 0;
  }

  // return true if keys match up to a segment boundary, set difference <0, 0, >0 if x <,=,> y
  // optionally, end of y may be passed
  bool key_match_prefix_segments(const key_type &x, const key_type &y, char separator, int &difference, const key_type &y_end = 0) const {
    size_t count;
    return character_set.key_match_count_segments(x, 0, y, y_end, separator, difference, count) && difference <= 0;
  }

  // return true if segmented keys match and x <= y
  // optionally, end of y may be passed
  bool key_match_prefix_segments(const key_type &x, const key_type &y, char separator, const key_type &y_end = 0) const {
    int difference;
    return key_match_prefix_segments(x, y, separator, difference, y_end);
  }

  // return number of matched segments assuming x <= y
  // optionally, end of y may be passed
  size_t key_match_count_segments(const key_type &x, const key_type &y, char separator, const key_type &y_end = 0) const {
    size_t retval = 0;
    int difference;
    size_t count;
    if (character_set.key_match_count_segments(x, 0, y, y_end, separator, difference, count) && difference <= 0)
      retval = count;
    return retval;
  }

  // return number of matched segments
  // optionally, end of y may be passed
  size_t key_match_count_prefix_segments(const key_type &x, const key_type &y, char separator, const key_type &y_end = 0) const {
    size_t retval = 0;
    int difference;
    size_t count;
    if (character_set.key_match_count_segments(x, 0, y, y_end, separator, difference, count))
      retval = count;
    return retval;
  }
////////////////////////////////////////////////////////////////////////////
// private type definitions
////////////////////////////////////////////////////////////////////////////
private:

  typedef Node<Value> node;
  typedef Entry<Value> entry;
  typedef Position<Value> position;

////////////////////////////////////////////////////////////////////////////
// index header
////////////////////////////////////////////////////////////////////////////
private:

struct header {
  char magic[4];
  struct {
      offset next[lg_max_count];
  } space; // free space table
  offset size;
  offset max;
  offset longest;
  offset record_list_offset;
  offset record_list_count;
  unsigned short endian;
  unsigned short min_size;
  unsigned short option_flags; // flags describing permanent index options
  node root;
};

////////////////////////////////////////////////////////////////////////////
// data members
// NB. new data members must be processed in swap()
////////////////////////////////////////////////////////////////////////////
private:

  unsigned short state_flags; // state flags
  character_set_t character_set; // character set
  header *data; // mapped address
  int fdesc; // file descriptor for file
  off_t start_offset; // offset into file
  offset mmap_len; // mapped length
  offset allocation_count; // number of blocks to extend file if needed
  int semaphore; // semaphore used for locking and unlocking critical sections
  int lock_level; // nesting level of locking TODO: detect overflow

public:

  //
  // swap the contents of this radix_map with passed radix_map
  //

  void swap(radix_map &x) {
    std::swap(state_flags, x.state_flags); // state flags
    character_set.swap(x.character_set); // character set
    std::swap(data, x.data); // mapped address
    std::swap(fdesc, x.fdesc); // file descriptor for file
    std::swap(start_offset, x.start_offset); // offset into file
    std::swap(mmap_len, x.mmap_len); // mapped length
    std::swap(allocation_count, x.allocation_count); // number of blocks to extend file if needed
  }

  //
  // contructors
  //

  radix_map(void) : state_flags(0), lock_level(0) {}

  radix_map(unsigned short option_flags) : state_flags(0), lock_level(0)
  {
    open(option_flags);
  }

  radix_map(const char *path, unsigned short mode_flags = read_only, unsigned short option_flags = 0, off_t start = 0, off_t end = 0) : state_flags(0), lock_level(0)
  {
    open(path, mode_flags, option_flags, start, end);
  }

  radix_map(int file_no, unsigned short mode_flags = read_only, unsigned short option_flags = 0, off_t start = 0, off_t end = 0) : state_flags(0), lock_level(0)
  {
    open(file_no, mode_flags, option_flags, start, end);
  }

  unsigned short get_option_flags(void) { return data->option_flags; }

  void set_option_flags(unsigned short option_flags_) { data->option_flags = option_flags_; }

  //
  // open radix_map from temporary file
  //

  void open(unsigned short option_flags = 0) {
    
    int fd;

    //
    // create file
    //
    fd = fileno(tmpfile());
    if (fd < 0) {
      std::stringstream error;
      error << "cannot create temporary file " << strerror(errno) << Throw;
    }
    state_flags |= opened | temp_file;
    open(fd, create, option_flags);
  }


  //
  // open radix_map from path
  //

  void open(const char *path, unsigned short mode_flags = read_only, unsigned short option_flags = 0, off_t start = 0, off_t end = 0) {
    int fd;
    struct stat st;

    my_assert(path != NULL);

    if (stat(path, &st) == 0 && !(mode_flags & (create | at_end))) {
      //
      // open file
      //
      if (mode_flags & read_write) {
        if (start != 0)
          std::cerr << "warning: non-contiguous allocation may occur" << /*std::endl*/'\n';
#pragma warning(suppress: 4996) // TODO: switch to ISO _open
        fd = ::open(path, O_RDWR);
        if (fd < 0) {
          std::stringstream error;
          error << "cannot open for write access " << path << " " << strerror(errno) << Throw;
        }
      } else {
        fd = ::open(path, O_RDONLY);
        if (fd < 0) {
          std::stringstream error;
          error << "cannot open for read access " << path << " " << strerror(errno) << Throw;
        }
      }
    } else {
      //
      // create file
      //
      if (mode_flags & read_only) {
        std::stringstream error;
        error << "cannot open for read access " << path << Throw;
      }
#pragma warning(suppress: 4996) // TODO: switch to ISO _open
      fd = ::open(path, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
      if (fd < 0) {
        std::stringstream error;
        error << "cannot create " << path << " " << strerror(errno) << Throw;
      }
      if (start != 0)
        std::cerr << "warning: offset = " << start << " is ignored" << /*std::endl*/'\n';
    }
    state_flags |= opened;
    open(fd, mode_flags, option_flags, start, end);
  }

  //
  // initialize or map radix_map from file number
  //

  void open(int file_no, unsigned short mode_flags, unsigned short option_flags = 0, off_t start = 0, off_t end = 0) {
    // TODO: Check at compile time.
    if (sizeof(offset) != 4 && sizeof(offset) != 8) {
      std::stringstream error;
      std::cerr << "sizeof(offset) = " << sizeof(offset) << " must be 4 or 8" << /*std::endl*/'\n';
    }
    if (sizeof(offset) == 4 && sizeof(header) != 152) { // check that compiler aligns things correctly
      std::stringstream error;
      error << "sizeof(header) = " << sizeof(header) << " must be " << 152 << Throw;
    }
    if (sizeof(offset) == 8 && sizeof(header) != 552) { // check that compiler aligns things correctly
      std::stringstream error;
      error << "sizeof(header) = " << sizeof(header) << " must be " << 552 << Throw;
    }
    if (bits_in_offset - ceiling_lg_bits_in_offset < number_of_common_characters) {
      std::stringstream error;
      error << "bits_in_offset - ceiling_lg_bits_in_offset < number_of_common_characters" << Throw;
    }
    //
    // acquire lock if multiuser
    //
    if (option_flags & multiuser)
      init_lock();
    //
    // set member data
    //
    fdesc = file_no;
    allocation_count = 0;
    state_flags |= mode_flags;
    if (mode_flags & create) {
      //
      // initialize file from beginning
      //
      start_offset = 0;
      init(option_flags);
    } else if (mode_flags & at_end) {
      //
      // initialize file from end
      //
      struct stat st;
      if (fstat(fdesc, &st)) {
        std::stringstream error;
        error << "unable to get file length" << Throw;
      }
      start_offset = st.st_size;
      unsigned long page_size = sysconf(_SC_PAGESIZE);
      if (start_offset % page_size != 0)
        start_offset += page_size - start_offset % page_size;
      init(option_flags);
    } else {
      //
      // map file
      //
      struct stat st;
      if (fstat(fdesc, &st)) {
        std::stringstream error;
        error << "unable to get file length" << Throw;
      }
      if (st.st_size == 0) { // new file
        start_offset = 0;
        init(option_flags);
      } else {
        if (start > st.st_size) {
          std::stringstream error;
          error << "start = " << start << " > length = " << st.st_size << Throw;
        }
        start_offset = start;
        if (end == 0)
          mmap_len = st.st_size - start_offset;
        else {
          if (end > st.st_size) {
            std::stringstream error;
            error << "end = " << end << " > length = " << st.st_size << Throw;
          }
          mmap_len = end - start_offset;
        }
        map();
        if (memcmp(data->magic, "COLE", 4)) {
          std::stringstream error;
          error << "incorrect magic number" << Throw;
        }
        if (data->endian != 1) {
          std::stringstream error;
          error << "byte order of file is incompatible with program" << Throw;
        }
        if (data->min_size != min_size) {
          std::stringstream error;
          error << "minimum block size in file = " << data->min_size <<
            " program compiled with minimum block size = " << min_size << Throw;
        }
        character_set.init(data->option_flags);
      }
    }
  }

  //
  // check if radix_map is open
  //

  bool is_open() {
    return (state_flags & opened) != 0;
  }

  //
  // get offset of mapping into file
  //

  off_t get_start_offset() const {
    return start_offset;
  }

  //
  // get size of mapping into file
  //

  off_t get_offset_length() const {
    return mmap_len;
  }

  //
  // start of mapping
  //

  char *get_begin() const {
    return reinterpret_cast<char *>(data);
  }

  //
  // end of mapping
  //

  char *get_end() {
    return reinterpret_cast<char *>(data) + mmap_len;
  }

  //
  // check that passed address is inside mapped region
  //
  bool check_address(node *n) { return reinterpret_cast<char *>(n) >= reinterpret_cast<char *>(get_root()) && reinterpret_cast<char *>(n) < get_end(); }; 
  bool check_address(entry *e) { return reinterpret_cast<char *>(e) >= reinterpret_cast<char *>(get_root()) && reinterpret_cast<char *>(e) < get_end(); }; 
  bool check_offset(offset o) { return check_address(get_node(o)); }

  //
  // get character set
  //

  const character_set_t *get_character_set() const {
    return &character_set;
  }

  //
  // close radix_map
  //

  ~radix_map() {
    close();
  }

  void close() {
    if ((state_flags & mapped) && (state_flags & (read_write | create | at_end)))
      trim();
    if (state_flags & have_lock)
      destroy_lock();
    if (state_flags & mapped)
      unmap();
    if ((state_flags & opened) && !(state_flags & temp_file))
#pragma warning(suppress: 4996) // TODO: switch to ISO _close()
      ::close(fdesc);
    state_flags = 0;
  }

  //
  // save radix_map to file
  //

  void save(char *path) {
    int fd = ::open(path, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
    if (fd < 0) {
      std::stringstream error;
      error << "cannot open " << path << " " << strerror(errno) << Throw;
    }
    int saved_lock_level = lock();
    if (write(fd, (char *) data, mmap_len) != mmap_len) {
      std::stringstream error;
      error << "cannot write to " << path << " " << strerror(errno) << Throw;
    }
    unlock(saved_lock_level);
#pragma warning(suppress: 4996) // TODO: switch to ISO _close()
    ::close(fd);
  }

  //
  // sync radix_map to file
  //

  void sync() {
    int saved_lock_level = lock();
    if (msync((void *) data, mmap_len, MS_SYNC)) {
      std::stringstream error;
      error << "cannot sync " << strerror(errno) << " writing..." << Throw;
    }
    unlock(saved_lock_level);
    /*
    if (write(fdesc, (char *) data, mmap_len) != mmap_len) {
      std::stringstream error;
      error << "cannot write index to file " << strerror(errno) << Throw;
    }
    */
  }

  //
  // remove all data
  //

  void clear() {
    unsigned short option_flags = data->option_flags;
    if (state_flags & mapped)
      unmap();
    // TODO: should set start_offset here?
    init(option_flags);
  }

private:

  //
  // manage lock
  //

  void init_lock() {
    semaphore = semget(3705, 1, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
    if (semaphore != -1)
      destroy_lock();
    semaphore = semget(3705, 1, IPC_CREAT | S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
    if (semaphore == -1) {
      std::stringstream error;
      error << "uanble to get semaphore: " << strerror(errno) <<  Throw;
    }
    state_flags |= have_lock; 
  }

  void destroy_lock() {
    if (semctl(semaphore, 0, IPC_RMID) == -1) {
      std::stringstream error;
      error << "unable to delete semaphore: " << strerror(errno) << Throw;
    }
    state_flags &= ~have_lock; 
  }

  public:

  int lock() {
    int retval = lock_level;
    if ((state_flags & have_lock) && lock_level++ == 0) {
      sembuf sops;
      sops.sem_flg = 0;
      sops.sem_num = 0;
      sops.sem_op = 0; // reserve semaphore
      int cc = semop(semaphore, &sops, 1);
      if (cc < 0) {
        if (cc == -2) {
          std::stringstream error;
          error << "lock reserve timed out" <<  Throw;
        } else {
          std::stringstream error;
          error << "unable to reserve lock: " << strerror(errno) <<  Throw;
        }
      }
      check_map();
    }
    return retval;
  }

  void unlock(int saved_lock_level) {
    if ((state_flags & have_lock) && --lock_level == 0) {
      if (lock_level != saved_lock_level) {
        std::stringstream error;
        error << "unlock: lock level " << lock_level << " should be " << saved_lock_level << Throw;
      }
      sembuf sops;
      sops.sem_flg = 0;
      sops.sem_num = 0;
      sops.sem_op = 1; // release semaphore
      int cc = semop(semaphore, &sops, 1);
      if (cc != 0) {
        std::stringstream error;
        error << "unable to release lock: " << strerror(errno) <<  Throw;
      }
    }
  }

  private:

  //
  // initialize
  //

  void init(unsigned short option_flags) {
    //
    // critical section
    //
    int saved_lock_level = lock();
    // root has room for one node containing one child entry and
    // whichever is larger, a value or a value list
    unsigned short c = lg_count(sizeof(node) + sizeof(entry));
    set_length(sizeof(header) - sizeof(node) + (min_size << c));
    map();
    memset((char *)data, 0, mmap_len);
    memcpy(data->magic, "COLE", 4);
    data->root.set_lg_count(c);
    clear_index();
    // TODO: support conversion between byte orders
    data->endian = 1;
    data->min_size = min_size;
    data->option_flags = option_flags;
    character_set.init(option_flags);
    unlock(saved_lock_level);
    //
    // end of critical section
    //
  }

  void clear_index() {
    //
    // critical section
    //
    int saved_lock_level = lock();
    data->root.entry_array()->make_pure_child(0);
    data->size = 0;
    data->max = 0;
    data->longest = 0;
    unlock(saved_lock_level);
    //
    // end of critical section
    //
  }

  //
  // manage option flags
  //

  void set_option_flag(unsigned short option_flags) {
    //
    // critical section
    //
    int saved_lock_level = lock();
    // root has room for one node containing one child entry and
    // whichever is larger, a value or a value list
    data->option_flags |= option_flags;
    character_set.init(data->option_flags);
    unlock(saved_lock_level);
    //
    // end of critical section
    //
  }

  void reset_option_flag(unsigned short option_flags) {
    //
    // critical section
    //
    int saved_lock_level = lock();
    // root has room for one node containing one child entry and
    // whichever is larger, a value or a value list
    data->option_flags &= ~option_flags;
    character_set.init(data->option_flags);
    unlock(saved_lock_level);
    //
    // end of critical section
    //
  }

  //
  // map file onto memory
  //

  void map() {
    if (state_flags & (read_write | create | at_end)) {
      data = (header *) mmap(0, mmap_len, PROT_READ | PROT_WRITE, MAP_SHARED, fdesc, start_offset);
      if ((void *) data == MAP_FAILED) {
        std::stringstream error;
        error << "cannot mmap for write access " << strerror(errno) << Throw;
      }
    } else {
      data = (header *) mmap(0, mmap_len, PROT_READ, MAP_PRIVATE, fdesc, start_offset);
      if ((void *) data == MAP_FAILED) {
        long p = sysconf(_SC_PAGESIZE);
        std::cerr << "SC_PAGE_SIZE = " << p <<
          " start_offset % SC_PAGE_SIZE = " << start_offset % p <<
          "cannot mmap " << strerror(errno) << Exit;
      }
    }
    state_flags |= mapped;
  }

  //
  // unmap file from memory
  //

  void unmap() {
    if (munmap((char *)data, mmap_len)) {
      std::stringstream error;
      error << "cannot munmap file " << strerror(errno) << Throw;
    }
    state_flags &= ~mapped;
  }

  //
  // set length of file
  //

  void set_length(offset length) {
    //
    // critical section
    //
    int saved_lock_level = lock();
    if (length <= mmap_len) {
      if (ftruncate(fdesc, start_offset + length)) {
        std::stringstream error;
        std::cerr << "unable to set file length to " << start_offset + length << ": " << strerror(errno) << /*std::endl*/'\n';
      }
    } else {
      if (lseek(fdesc, start_offset + mmap_len, SEEK_SET) != start_offset + mmap_len) {
        std::stringstream error;
        error << "unable to seek file to " << start_offset + mmap_len << ": " << strerror(errno) << Throw;
      }
      // TODO: Test if writing zeroes avoids file fragmentation.
      /*
      for (offset rem = length - mmap_len; rem; --rem)
#pragma warning(suppress: 4996)
        if (::write(fdesc, "", 1) != 1) {
	  std::stringstream error;
	  std::string save_error(strerror(errno));
	  error << "unable to write 1 byte to file at " << lseek(fdesc, 0, SEEK_CUR) << ": " << save_error << Throw;
	}
      */
      char buf[65536];
      memset(buf, 0, sizeof(buf));
      for (offset rem = length - mmap_len; rem; ) {
        offset l = rem;
        if (l > sizeof(buf))
          l = sizeof(buf);
#pragma warning(suppress: 4996)
        if (::write(fdesc, buf, l) != static_cast<int>(l)) {
          std::stringstream error;
          error << "unable to write to file: " << strerror(errno) << Throw;
        }
        rem -= l;
      }
    }
    unlock(saved_lock_level);
    //
    // end of critical section
    //
    mmap_len = length;
  }

  //
  // remap file
  // throw exception if system does not return previously mapped region of memory
  //

  void remap() {
    void *x = mmap(0, mmap_len, PROT_READ | PROT_WRITE, MAP_SHARED, fdesc, start_offset);
    if (x != (void *)data) {
      munmap(x, mmap_len);
      throw std::runtime_error("mmap address changed"); // routines catching this should call new_map() below
    }
    state_flags |= mapped;
  }

  //
  // check that remap exception was thrown and map to a new location
  //

  void new_map(const std::exception &ex, const char *routine) {
    if (strcmp(ex.what(), "mmap address changed"))
      throw;
    std::cerr << routine << ": change map from " << data;
    map();
    std::cerr << " to " << data << /*std::endl*/'\n';
  }


  //
  // check that map has correct length and if not map to a new location
  //

  void check_map() {
    if (state_flags & mapped) {
      struct stat st;
      if (fstat(fdesc, &st))
        throw std::runtime_error("unable to stat file");
      if (mmap_len != st.st_size) {
        unmap();
        mmap_len = st.st_size;
        map();
      }
    }
  }

////////////////////////////////////////////////////////////////////////////
// space management
////////////////////////////////////////////////////////////////////////////
private:

  //
  // compute free list index from size
  // currently equal to ceiling(log2(length / min_size))
  // TODO: Add list of allocation sizes.
  //

  unsigned short lg_count(offset length) {
    unsigned short c = 0;
    for (offset l = min_size; l < length; l *= 2)
      ++c;
    return c;
  }

  //
  // allocate a node
  // TODO: coalesce smaller nodes if necessary
  //

  node *allocate(unsigned short l) {
    if (l > lg_max_count) {
      std::stringstream error;
      error << "allocate: request " << l << " > max " << lg_max_count << Throw;
    }
    node *retval = 0;
    //
    // critical section
    //
    int saved_lock_level = lock();
    if (data->space.next[l]) {
      retval = get_node(data->space.next[l]);
      data->space.next[l] = *(offset *) retval;
    } else {
      for (unsigned short i = l + 1; !retval && i < lg_max_count; ++i)
        if (data->space.next[i]) {
          retval = get_node(data->space.next[i]);
          data->space.next[i] = *(offset *) retval;
          split(retval, l, i);
        }
      if (!retval) {
        if (allocation_count <= l) {
          if (l < 10)
            allocation_count = l + 10; // only extend every 1K allocates
          else
            allocation_count = l + 1;
        }
        if (allocation_count >= lg_max_count)
          allocation_count = lg_max_count - 1;
        data->space.next[allocation_count] = get_offset((node *)get_end()); // TODO: every index has a free list
        unmap();
        set_length(mmap_len + (min_size << allocation_count));
        //
        // if remap() throws, execution will not reach here, so caller must retry allocate(l)
        // therefore it is necessary to unlock before remap and lock again after
        //
        unlock(saved_lock_level);
        remap();
        lock();
        //
        // recursive call -- guaranteed not to throw again
        //
        retval = allocate(l);
      }
    }
    retval->set_lg_count(l);
    unlock(saved_lock_level);
    //
    // end of critical section
    //
    return retval;
  }

  //
  // unallocate a block of memory
  //

  void unallocate(offset o, offset lg_count) {
    //
    // critical section
    //
    int saved_lock_level = lock();
    if (data->space.next[lg_count] &&
        ((o > data->space.next[lg_count] && o < data->space.next[lg_count] + bit_mask(lg_count)) ||
         (data->space.next[lg_count] > o && data->space.next[lg_count] < o + bit_mask(lg_count)))) {
      std::stringstream error;
      error << "unallocate of overlapping node at offset " << o << Throw;
    }
    *reinterpret_cast<offset *>(get_node(o)) = data->space.next[lg_count];
    data->space.next[lg_count] = o;
    unlock(saved_lock_level);
    //
    // end of critical section
    //
  }

  //
  // unallocate a node
  //

  void unallocate(node *n) {
    unallocate(get_offset(n), n->get_lg_count());
  }

  //
  // unallocate a node, keeping free list sorted
  // TODO: truncate file
  //

  void unallocate_sorted(node *n) {
    //
    // critical section
    //
    int saved_lock_level = lock();
    unsigned short l = n->get_lg_count();
    offset o = get_offset(n), on;
    for (offset *op = &(data->space.next[l]);; op = (offset *)get_node(on)) {
      // if (o > (on = *op)) // sort decreasing
      if ((on = *op) == 0 || o < on) { // sort increasing
        *op = o;
        *(offset *)n = on;
        break;
      }
    }
    unlock(saved_lock_level);
    //
    // end of critical section
    //
  }

  //
  // split and return to free space nodes from lg_count [i1, i2)
  //

  void split(node *n, unsigned short i1, unsigned short i2) {
    //
    // critical section
    //
    int saved_lock_level = lock();
    for (unsigned short i = i1; i < i2; ++i) {
      node *x = (node *)((char *)n + (min_size << i));
      x->set_lg_count(i);
      unallocate(x);
    }
    unlock(saved_lock_level);
    //
    // end of critical section
    //
  }

  //
  // clear free lists
  //

  void clear_free_lists() {
    //
    // critical section
    //
    int saved_lock_level = lock();
    for (unsigned short l = 0; l != lg_max_count; ++l)
      data->space.next[l] = 0;
    unlock(saved_lock_level);
    //
    // end of critical section
    //
  }

  //
  // trim and unmap file
  //

  void trim() {
    //
    // critical section
    //
    int saved_lock_level = lock();
    char *end = get_end(); // end of memory
    for (bool found = true; found; ) {
      found = false;
      for (unsigned short l = 0; l != lg_max_count; ++l) {
        offset length = min_size << l;
        offset *op = &(data->space.next[l]);
        for (offset o, *on; (o = *op); op = on) {
          on = (offset *)get_node(o);
          if ((char *)on + length == end) { // if block is at end of memory
            *op = *on; // unchain it
            end -= length; // calculate new end of memory
            found = true; // loop again
          }
        }
      }
    }
    if (end == get_end()) // no change
      unlock(saved_lock_level);
    else {
      unmap();
      set_length(end - (char *)data);
      unlock(saved_lock_level);
      std::cerr << "file truncated to " << mmap_len << " bytes" << /*std::endl*/'\n';
    }
    //
    // end of critical section
    //
  }

public:

  //
  // allocate a block of memory
  //

  char *alloc(offset size) {
    char *retval = 0;
    short retry_count;
    for (retry_count = 0; retry_count <= 1; ++retry_count) {
      try {
          node *n = allocate(lg_count(sizeof(node) + size));
          retval = n->body();
          break;
      } catch (const std::exception &ex) {
        new_map(ex, "alloc");
      }
    }
    if (retry_count > 1) {
      std::stringstream error;
      error << "alloc failed " << strerror(errno) << Throw;
    }
    return retval;
  }

  //
  // free a block of memory
  //

  void free(char *block) {
    node *n = reinterpret_cast<node *>(block - sizeof(node));
    unallocate(n);
  }

  //
  // enlarge a block of memory
  //

  char *realloc(char *block, offset size) {
    char *retval = 0;
    //
    // critical section
    //
    int saved_lock_level = lock();
    node *n = (node *)(block - sizeof(node));
    if (n->capacity() >= size)
      retval = block;
    else {
      short retry_count;
      for (retry_count = 0; retry_count <= 1; ++retry_count) {
        try {
          node *new_n = allocate(lg_count(sizeof(node) + size));
          memcpy(new_n->body(), n->body(), n->capacity());
          unallocate(n);
          retval = new_n->body();
          break;
        } catch (const std::exception &ex) {
          offset n_offset = (char *)n - (char *)data;
          new_map(ex, "realloc");
          n = (node *)((char *)data + n_offset);
        }
      }
      if (retry_count > 1) {
        std::stringstream error;
        error << "realloc failed " << strerror(errno) << Throw;
      }
    }
    unlock(saved_lock_level);
    //
    // end of critical section
    //
    return retval;
  }

  //
  // returns amount of free space
  //

  offset free_space() {
    offset t = 0;
    //
    // critical section
    //
    int saved_lock_level = lock();
    for (unsigned short l = 0; l < lg_max_count; ++l)
      for (offset o = data->space.next[l]; o; o = *(offset *)get_node(o))
        t += min_size << l;
    unlock(saved_lock_level);
    //
    // end of critical section
    //
    return t;
  }

  //
  // returns offset of allocated block in file
  //
  offset get_offset(const char *block) const {
    return get_offset(reinterpret_cast<node *>(const_cast<char *>(block - sizeof(node))));
  }


  //
  // returns address of allocated block in file
  // NB. This is not the same as the node address because the node contains a header followed by the block
  //
  char *block(offset o) const {
    return get_node(o)->body();
  }

////////////////////////////////////////////////////////////////////////////
// record (and keys) interface
////////////////////////////////////////////////////////////////////////////

public:

  struct record_t {
    char magic[4];
    offset data;
    offset length;
    offset record_id;
    time_stamp_t create_time;
    offset create_user_id;
    time_stamp_t update_time;
    offset update_user_id;
    offset key_data;
    offset key_length;
  };

  class record_offset_sort_by_id_t : public std::binary_function<offset, offset, bool> {
    const radix_map &map;
  public:
    record_offset_sort_by_id_t(const radix_map &map_) : map(map_) {}
    bool operator ()(const offset &x, const offset &y) const {
      record_t *rx = reinterpret_cast<record_t *>(map.block(x));
      record_t *ry = reinterpret_cast<record_t *>(map.block(y));
      return rx->record_id < ry->record_id;
    }
  };

  //
  // returns record_t if record_offset offset is valid
  //

  record_t *check_active_or_deleted(offset record_offset) {
    record_t *retval = 0;
    if (check_offset(record_offset)) {
      record_t *record = reinterpret_cast<record_t *>(block(record_offset));
      if (!memcmp(record->magic, "COLE", 4) || !memcmp(record->magic, "DEAD", 4))
        retval = record;
    }
    return retval;
  }

  //
  // returns record_t if record_offset offset is valid and record is not deleted
  //

  record_t *check_active(offset record_offset) {
    record_t *retval = 0;
    if (check_offset(record_offset)) {
      record_t *record = reinterpret_cast<record_t *>(block(record_offset));
      if (!memcmp(record->magic, "COLE", 4))
        retval = record;
    }
    return retval;
  }


  //
  // returns true if records are used
  //
  bool use_records() {
    return data->record_list_offset != 0;
  }
  
  //
  // returns record
  //
  std::string get_record(offset record_id) {
    if (record_id == 0) {
      std::stringstream error;
      error << " record id is zero" << Throw;
    } else if (record_id > data->record_list_count) {
      std::stringstream error;
      error << " record id " << record_id << " is larger than max " << data->record_list_count << Throw;
    }
    offset *record_list = reinterpret_cast<offset *>(get_node(data->record_list_offset)->body());
    offset record_offset = record_list[record_id - 1];
    record_t *record = reinterpret_cast<record_t *>(block(record_offset));
    return std::string(block(record->data), record->length);
  }
  
  //
  //
  // returns offset of allocated record_t in file
  //
  offset add_record(const char *contents_, offset length, offset user_id, time_stamp_t timestamp) {
    offset retval = get_offset(alloc(sizeof(record_t)));
    char *contents = alloc(length);
    record_t *record = reinterpret_cast<record_t *>(block(retval)); // NB. must be after alloc() which may remap
    memcpy(contents, contents_, length);
    memcpy(record->magic, "COLE", 4);
    record->data = get_offset(contents);
    record->length = length;
    record->create_user_id = record->update_user_id = user_id;
    record->create_time = record->update_time = timestamp;
    //
    // critical section
    //
    int saved_lock_level = lock();
    if (!use_records()) { // initialize record use
      data->record_list_offset = get_offset(alloc(8000000 * sizeof(offset))); // DEBUG
      record = reinterpret_cast<record_t *>(block(retval)); // NB. must be after alloc() which may remap
      data->record_list_count = 0;
    }
    node *n = get_node(data->record_list_offset);
    if (data->record_list_count == n->capacity(sizeof(offset))) { // current record list is full
      n = double_index_node(n);
      record = reinterpret_cast<record_t *>(block(retval)); // NB. must be after double_index_node() which may remap
      n = get_node(data->record_list_offset); // NB. must be after double_index_node() which may remap
      data->record_list_offset = get_offset(n);
    }
    offset *record_list = reinterpret_cast<offset *>(n->body());
    record_list[data->record_list_count++] = retval;
    record->record_id = data->record_list_count; // NB. record ids are not sorted
    unlock(saved_lock_level);
    //
    // end of critical section
    //
    record->key_length = 0;        // force update_key to ignore current record contents
    return retval;
  }

  //
  // same as add_record() but also updates user and time
  //
  offset copy_record(const char *data, offset length, offset create_user_id, time_stamp_t create_timestamp, offset update_user_id, time_stamp_t update_timestamp) {
    offset retval = add_record(data, length, create_user_id, create_timestamp);
    record_t *record = reinterpret_cast<record_t *>(block(retval));
    record->update_user_id = update_user_id;
    record->update_time = update_timestamp;
    return retval;
  }

  //
  // updates keys associated with record in database
  //
  void update_keys(offset record_offset, const std::vector<key_type> &key_list) {
    record_t *record = reinterpret_cast<record_t *>(block(record_offset));
    typedef std::multiset<key_type, character_set_t> key_set_t; // multiset allows duplicate keys
    key_set_t keys_to_delete(character_set);
    for (const char *p = block(record->key_data), *e = p + record->key_length; p != e; p += strlen(p) + 1)
      keys_to_delete.insert(p);
    for (std::vector<key_type>::const_iterator i = key_list.begin(); i != key_list.end(); ++i) {
      key_set_t::iterator k = keys_to_delete.find(*i);
      if (k != keys_to_delete.end()) // new key matches current key; remove from list of keys to delete
        keys_to_delete.erase(k);
    }
    std::stringstream error_list;
    for (key_set_t::const_iterator i = keys_to_delete.begin(); i != keys_to_delete.end(); ++i) {
      bool erased = erase(std::make_pair(*i, record->record_id)) != 0; // erase current key
      record = reinterpret_cast<record_t *>(block(record_offset)); // NB. must be after erase() which may remap
      if (!erased) {
        if (!error_list.str().empty())
          error_list << "; ";
        error_list << "unable to erase key " << *i << " for record id " << record->record_id;
      }
    }
    std::string key_data;
    for (std::vector<key_type>::const_iterator i = key_list.begin(); i != key_list.end(); ++i) {
      key_data.append(*i);
      key_data.append(1, 0);
      iterator j(find(*i));
      if (j == end() || !find_value(j, record->record_id)) { // TODO: should check that number of identical keys match
        bool inserted = insert(std::make_pair(*i, record->record_id)).second; // insert new key
	record = reinterpret_cast<record_t *>(block(record_offset)); // NB. must be after insert() which may remap
        if (!inserted) { // insert failed
          if (!error_list.str().empty())
            error_list << "; ";
          error_list << "unable to insert key " << *i << " for record id " << record->record_id;
        }
      }
    }
    if (key_data.length() == 0) {
      if (record->key_length != 0) {
        free(block(record->key_data));
        record = reinterpret_cast<record_t *>(block(record_offset)); // NB. must be after free() which  may remap
      }
      record->key_data = 0;
    } else {
      char *new_key_data;
      if (record->key_length == 0)
        new_key_data = alloc(key_data.length());
      else
        new_key_data = realloc(block(record->key_data), key_data.length());
      record = reinterpret_cast<record_t *>(block(record_offset)); // NB. must be after re/alloc() which may remap
      memcpy(new_key_data, key_data.data(), key_data.length());
      record->key_data = get_offset(new_key_data);
    }
    record->key_length = key_data.length();
    if (!error_list.str().empty()) {
      std::stringstream error;
      error << "update_keys: " << error_list.rdbuf() << Throw;
    }
  }

  //
  // update data only in record_t in file
  //
  record_t *update_record(offset record_offset, const char *contents, offset length, offset user_id, time_stamp_t timestamp) {
    record_t *retval = reinterpret_cast<record_t *>(block(record_offset));
    char *new_contents = realloc(block(retval->data), length);
    retval = reinterpret_cast<record_t *>(block(record_offset)); // NB. must be after realloc() which may remap
    memcpy(new_contents, contents, length);
    retval->data = get_offset(new_contents);
    retval->length = length;
    retval->update_user_id = user_id;
    retval->update_time = timestamp;
    return retval;
  }

  //
  // mark record_t as deleted in file
  //
  void delete_record(offset record_offset, offset user_id, time_stamp_t timestamp) {
    record_t *record = reinterpret_cast<record_t *>(block(record_offset));
    free(block(record->data));
    record = reinterpret_cast<record_t *>(block(record_offset)); // NB. must be after free() which may remap
    memcpy(record->magic, "DEAD", 4); // mark record as deleted
    record->length = 0; // data is empty
    record->update_user_id = user_id; // save deleter
    record->update_time = timestamp; // save time of deletion
  }

  //
  // erase record_t in file
  //
  void erase_record(offset record_offset) {
    record_t *record = reinterpret_cast<record_t *>(block(record_offset));
    free(block(record->data));
    free(block(record_offset));
  }

////////////////////////////////////////////////////////////////////////////
// index maintenance
////////////////////////////////////////////////////////////////////////////

  // start of node space
  node *get_root() const {
    return &data->root;
  }

private:

  //
  // utility routines
  //

  // return address of node at given offset
  // NB. node address != byte address returned by block()
  node *get_node(offset o) const {
    return reinterpret_cast<node *>(reinterpret_cast<char *>(get_root()) + o * min_size);
  }

  // node offset of given node (NB. node offset != byte offset)
  offset get_offset(node *n) const {
    offset byte = (char *)n - (char *)get_root();
    if (byte % min_size != 0) {
      std::stringstream error;
      error << "node at byte offset " << byte << " is not aligned at " << min_size << " boundary" << Throw;
    }
    return byte / min_size;
  }

  //
  // double size of index node
  // NB: may throw but if it does it does not unallocate old node
  //

  node *double_index_node(node *old) {
    offset lg = old->get_lg_count();
    node *retval = allocate(lg + 1);
    retval->copy(old);
    unallocate(old);
    return retval;
  }

  //
  // increase size of index node to accommodate required number of entries
  // NB: may throw but if it does it does not unallocate old node
  //

  node *reallocate_index_node(node *old, offset count_of_entries) {
    offset lg = lg_count(sizeof(node) + count_of_entries * sizeof(entry));
    node *retval = allocate(lg);
    retval->copy(old);
    unallocate(old);
    return retval;
  }

  //
  // enlarge index node and adjust things that point to it
  // NB: may throw but if it does it does not modify index
  //

  void enlarge(node *&n, entry *&e, offset count_of_entries, entry *eparent) {
    if (eparent == 0) {
      std::stringstream error;
      error << "enlarge: eparent == 0" << Throw;
    }
    offset pos = e - n->entry_array();
    // NB: reallocate index node before modifying index
    n = reallocate_index_node(n, count_of_entries);
    e = n->entry_array() + pos;
    eparent->set_child_offset(get_offset(n));
  }

  //
  // return the remainder from an entry
  //

  char *get_remainder(entry *e) const {
    return get_node(e->get_remainder_offset())->get_remainder();
  }

  //
  // create a remainder node
  // return its offset
  // NB: discard invalid characters
  // NB: may throw
  //

  offset create_remainder(const char *remainder) {
    // include NUL in length
    node *n = allocate(lg_count(character_set.key_length(remainder) + 1 + sizeof(node)));
    character_set.key_copy(n->get_remainder(), remainder);
    return get_offset(n);
  }

  //
  // return radix of dropped first character
  //

  unsigned char dropped_character(offset r) {
    return character_set.toradix(*get_node(r)->get_remainder());
  }

  //
  // shorten a remainder node by dropping the first character
  // return its new offset or 0 if remainder is shortened to nothing
  // NB: does not throw
  //

  offset shorten_remainder(offset r) {
    node *n = get_node(r);
    char *p = n->get_remainder();
    char *q = p;
    while (*q++)
      q[-1] = *q;
    if (*p == 0) { // remainder shortened to nothing
        unallocate(n);
        r = 0;
    } else if (q <= n->halfway()) {
      if (n->get_lg_count()) {
        split(n, n->get_lg_count() - 1, n->get_lg_count());
        n->set_lg_count(n->get_lg_count() - 1);
        my_assert(q > n->halfway());
      } else {
        std::stringstream error;
        error << "shorten_remainder: n->get_lg_count() == 0" << Throw;
      }
    }
    return r;
  }

  //
  // convert to value list at the current iterator position
  // NB: may throw but if it does it does not modify entry
  //

  void convert_to_value_list(entry *e) {
    node *n1 = allocate(lg_count(sizeof(node) + sizeof(entry)));
    n1->set_count(1);
    n1->value(0) = e->get_value();
    e->set_value_list_offset(get_offset(n1));
  }

  //
  // find key remainder starting at root or passed iterator position 
  // NB: if remainder is not empty, iterator may point to an internal node
  // return 0 if the key is not found, unless insert is true
  // if insert is true, add index nodes as required
  // NB: calls that may throw because of insufficient resources must be made before index is invalidated
  // return 0 if the key is a duplicate and uniqueness is required
  // return entry and point iterator (if passed) at (key, value) in index
  // if entry already contains a single value, convert it to a value list
  //
  // TODO split find_entry() into two pieces, one const and one non-const
  //

  entry *find_entry(const key_type &key, bool insert = false, iterator *i = 0) {
    bool prefix = !(data->option_flags & dictorder); // unless using dictorder key ends with first invalid radix
    const char *p = key;
    /* TODO: regular expression
    if (i && i->key_comparison && *p == '^') { // regular expression beginning at the start of the key
      prefix = true; // consider key ended at first invalid_radix
      ++p; // skip over '^' operator
    }
    */
    node *n;
    entry *e, *eparent;
    if (i == 0) { // start at root
      n = get_root();
      my_assert(check_address(n));
      e = n->entry_array();
      my_assert(check_address(e));
      eparent = 0;
    } else { // use iterator as hint as to where to start 
      if (i->depth == 0)
        i->init(get_root(), &character_set);
      if (i->depth > 1) {
        unsigned short l = character_set.key_match_skip(i->key, p, !prefix); // skip over match
        i->depth = l + 1;
        i->key[l] = 0;
      }
      n = i->top_node();
      my_assert(check_address(n));
      e = i->top_entry();
      my_assert(check_address(e));
      if (i->depth == 1)
        eparent = 0;
      else
        eparent = i->top_parent();
    }
    // loop will exit with radix and found set
    unsigned char radix;
    bool found;
    for (found = true; found && !character_set.at_end(p); ++p) {
      if ((radix = character_set.toradix(*p)) == character_set.invalid_radix) {
        if (prefix)
          break;
        else
          continue;
      }
      if (e->has_child())
        n = get_node(e->get_child_offset());
      else {
        node *n1;
        if (e->is_remainder()) {
          if (character_set.key_equal(get_remainder(e), p))
            break; // value goes here
          if (!insert) {
            found = false;
            break;
          }
          // shorten remainder
          unsigned char radix_of_dropped_character = dropped_character(e->get_remainder_offset());
          if (radix_of_dropped_character < number_of_common_characters) { // dropped character is common
            // NB: allocate before modifying index
            n1 = allocate(lg_count(sizeof(node) + sizeof(entry)));
            n1->set_expanded_flag(false);
            // NB: shorten_remainder does not throw
            offset r = shorten_remainder(e->get_remainder_offset());
            if (r)
              n1->copy_remainder_entry(n1->entry_array(), r, e);
            else
              n1->copy_pure_value_entry(n1->entry_array(), e);
            n1->set_occupancy_mask(bit_mask(radix_of_dropped_character));
            e->remove_value();
            e->set_child_offset(get_offset(n1));
          } else { // dropped character is not common
            // NB: allocate before modifying index
            n1 = allocate(lg_count(sizeof(node) + (radix_of_dropped_character + 1) * sizeof(entry)));
            n1->set_expanded_flag(true);
            // initialize new entries
            for (entry *ep = n1->entry_array(); ep != n1->entry_array() + n1->number_of_entries(); ++ep)
              ep->make_empty();
            // NB: shorten_remainder does not throw
            offset r = shorten_remainder(e->get_remainder_offset());
            if (r)
              n1->copy_remainder_entry(n1->entry_array() + radix_of_dropped_character, r, e);
            else
              n1->copy_pure_value_entry(n1->entry_array() + radix_of_dropped_character, e);
            e->remove_value();
            e->set_child_offset(get_offset(n1));
          }
        } else if (!insert) {
          found = false;
          break;
        } else if (e->has_value()) {
          // NB: allocate before modifying index
          n1 = allocate(lg_count(sizeof(node) + sizeof(entry)));
          n1->set_occupancy_mask(0);
          n1->set_expanded_flag(false);
          e->set_child_offset(get_offset(n1));
        } else { // must be root
          if (eparent != 0) {
            std::stringstream error;
            error << "find_entry: eparent != 0" << Throw;
          }
          Value null;
          // NB: create_remainder before modifying index
          e->make_remainder(create_remainder(p), null);
          ++data->size;
          if (i) i->set_value_list_pointer(0);
          return e;
        }
        n = n1;
      }
      eparent = e;
      my_assert(check_address(n));
      e = n->entry_array();
      my_assert(check_address(e));
      if (n->is_expanded()) {
        e += radix;
        if (radix < n->capacity(sizeof(entry)))
          found = !e->is_empty();
        else
          found = false; // NB: e points at an unallocated entry where key < next key with value (see below)
      } else {
        if (radix < number_of_common_characters) { // common character
          offset mfind = bit_mask(radix);
          e += n->number_of_entries(mfind);
          found = (mfind & n->get_occupancy_mask()) != 0; // NB: if !found, e may point at incorrect or unallocated entry, where key < next key with value (see below)
        } else { // uncommon character
          e += radix; // NB: e points at an unallocated entry where key < next key with value (see below)
          found = false;
        }
      }
      if (i) 
        if (found || insert) i->push(position(n, e), radix); // point at found node or place to insert new key
    }
    my_assert(check_address(n));
    if (found) {
      my_assert(check_address(e));
      if (!e->has_value()) {
        if (!insert)
          e = 0;
        else {
          Value null;
          e->set_value(null);
          ++data->size;
        }
      } else if (insert) {
        if (data->option_flags & unique)
          e = 0;
        else if (e->is_remainder() && !character_set.key_equal(get_remainder(e), p)) {
          // push remainder down one level
          node *n1;
          unsigned char radix_of_dropped_character = dropped_character(e->get_remainder_offset());
          if (radix_of_dropped_character < number_of_common_characters) { // dropped character is common
            // NB: allocate before modifying index
            n1 = allocate(lg_count(sizeof(node) + sizeof(entry)));
            n1->set_expanded_flag(false);
            // NB: shorten_remainder does not throw
            offset r = shorten_remainder(e->get_remainder_offset());
            if (r)
              n1->copy_remainder_entry(n1->entry_array(), r, e);
            else
              n1->copy_pure_value_entry(n1->entry_array(), e);
            n1->set_occupancy_mask(bit_mask(radix_of_dropped_character));
          } else { // dropped character is not common
            // NB: allocate before modifying index
            n1 = allocate(lg_count(sizeof(node) + (radix_of_dropped_character + 1) * sizeof(entry)));
            n1->set_expanded_flag(true);
            // initialize new entries
            for (entry *ep = n1->entry_array(); ep != n1->entry_array() + n1->number_of_entries(); ++ep)
              ep->make_empty();
            // NB: shorten_remainder does not throw
            offset r = shorten_remainder(e->get_remainder_offset());
            if (r)
              n1->copy_remainder_entry(n1->entry_array() + radix_of_dropped_character, r, e);
            else
              n1->copy_pure_value_entry(n1->entry_array() + radix_of_dropped_character, e);
          }
          Value null;
          e->make_child_value(get_offset(n1), null); // replace with child
          ++data->size;
        } else if (e->has_single_value())
          // NB: convert_to_value_list may throw but if it does it does not modify entry
          convert_to_value_list(e);
      }
    } else { // not found
      //
      // NB: e may point at incorrect or unallocated entry where given key < next key with value in index (see above)
      // if e is off end of n->entry_array (i.e., n->at_end(e))
      //   then current stack must be poppped to find next key with value that is > given key (if any)
      // else e is within entry array (i.e., !n->at_end())
      //   if n is an expanded array then e->empty() and root of a tree with keys > given key may be in current n->entry_array 
      //   else if n is not expanded then e is the root of a tree with keys > given key
      //
      if (!insert)
        e = 0;
      else {
        // NB: create_remainder before modifying index
        // TODO: this data may be lost if later subsequent allocation throws
        offset remainder = character_set.key_length(p) ? create_remainder(p) : 0;
        if (n->is_expanded()) { // expanded
          if (radix >= n->capacity(sizeof(entry))) { // radix will not fit in entry array
            // NB: e points at unallocated entry (see above)
            offset old_length = n->capacity(sizeof(entry));
            // NB: enlarge does not invalidate index
            enlarge(n, e, radix + 1, eparent);
            // NB: e points at an allocated entry
            if (i) { // adjust iterator to point to reallocated node and entry
              my_assert(check_address(n));
              i->set_top_node(n);
              my_assert(check_address(e));
              i->set_top_entry(e);
            }
            // initialize new entries
            my_assert(check_address(n));
            for (entry *ep = n->entry_array() + old_length; ep != n->end_array(); ++ep)
              ep->make_empty();
          }
          // NB: e points at correct empty allocated entry
        } else { // compressed
          if (radix < number_of_common_characters) { // common character
            my_assert(check_address(n));
            // NB: e points at incorrect entry (see above)
            if (!n->is_room(sizeof(entry))) { // no room for one more entry
              // NB: enlarge does not invalidate index
              enlarge(n, e, n->number_of_entries() + 1, eparent);
              if (i) { // adjust iterator to point to reallocated node and entry
                my_assert(check_address(n));
                i->set_top_node(n);
                my_assert(check_address(e));
                i->set_top_entry(e);
              }
            }
            n->add_occupancy_mask(bit_mask(radix)); // update occupancy mask
            // NB. expanded flag is false
            n->make_room(e, sizeof(entry)); // move remainder of compressed entry array down
            // NB: e points at correct entry (see above)
            my_assert(check_address(e));
            e->make_empty();
            // NB: e points at empty correct entry (see above)
          } else { // uncommon character
            // expand compressed array
            // NB: e points at unallocated entry (see above)
            // NB: enlarge does not invalidate index
            enlarge(n, e, radix + 1, eparent);
            // NB: e points at an allocated entry
            if (i) { // adjust iterator to point to reallocated node and entry
              my_assert(check_address(n));
              i->set_top_node(n);
              my_assert(check_address(e));
              i->set_top_entry(e);
            }
            // copy compressed entries to new locations (NB: copy backwards)
            // NB: currently number_of_entries() = count of bits in occupancy mask
            my_assert(check_address(n));
            for (entry *ep = n->entry_array() + number_of_common_characters, *eq = n->entry_array() + n->number_of_entries(); ep-- != n->entry_array(); )
              if (n->get_occupancy_mask() & bit_mask(ep - n->entry_array())) {
                if (ep != --eq) // if not already in right place
                  ep->copy(eq); // copy entry with bit set in occupancy mask
              } else
                ep->make_empty(); // initialize otherwise
            n->set_expanded_flag(true);
            // initialize new entries
            // NB. now number_of_entries() = capacity(sizeof(entry))
            for (entry *ep = n->entry_array() + number_of_common_characters; ep != n->entry_array() + n->number_of_entries(); ++ep)
              ep->make_empty();
            // NB: e points at correct empty allocated entry
          }
        }
        // NB: e points at correct empty allocated entry
        Value null;
        // NB: remainder previously created before index modified
        my_assert(check_address(e));
        if (remainder)
          e->make_remainder(remainder, null);
        else
          e->make_pure_value(null);
        ++data->size;
      }
    }
    if (i) i->set_value_list_pointer(0);
    return e;
  }

////////////////////////////////////////////////////////////////////////////
// STL iterators
////////////////////////////////////////////////////////////////////////////
public:

  //
  // beginning iterator
  //

  iterator begin() {
    iterator i(get_root(), &character_set);
    if (!i.has_value())
      i.advance();
    return i;
  }

  const_iterator begin() const {
    const_iterator i(get_root(), &character_set);
    if (!i.has_value())
      i.advance();
    return i;
  }

  //
  // ending iterator
  //

  iterator end() {
    return iterator(reinterpret_cast<char *>(get_root()));
  }

  const_iterator end() const {
    return const_iterator(reinterpret_cast<char *>(get_root()));
  }

////////////////////////////////////////////////////////////////////////////
// STL capacity members
////////////////////////////////////////////////////////////////////////////
public:

  bool empty() const { return size() == 0; }
  offset size() const { return data->size; }
  offset max_size() const { return max_count; }

////////////////////////////////////////////////////////////////////////////
// STL element access
////////////////////////////////////////////////////////////////////////////
public:

  //
  // find or insert Value for key
  // high speed version not using an iterator
  // NB.  only adds one value, first in list if it exists already
  //

  Value &operator[](const key_type &key) {
    Value *retval = 0;
    //
    // critical section
    //
    int saved_lock_level = lock();
    short retry_count;
    for (retry_count = 0; retry_count <= 1; ++retry_count) {
      try {
        entry *e = find_entry(key, true);
        if (e->has_value_list())
          retval = &(get_node(e->get_value_list_offset())->value(0));
        else
          retval = &(e->get_value());
        break;
      } catch (const std::exception &ex) {
        new_map(ex, "operator[]");
      }
    }
    unlock(saved_lock_level);
    //
    // end of critical section
    //
    if (retry_count > 1) {
      std::stringstream error;
      error << "operator[] failed " << strerror(errno) << Throw;
    }
    return *retval;
  }

////////////////////////////////////////////////////////////////////////////
// non-STL modifiers
////////////////////////////////////////////////////////////////////////////
public:

  //
  // conditionally insert new key and value with hint by reference
  // high speed version used by range insert below
  // NB: if find_entry or add_value throw they do not invalidate index
  //

  bool insert(iterator &hintpos, const key_type &key, const Value &v) {
    bool retval = false;
    //
    // critical section
    //
    int saved_lock_level = lock();
    short retry_count;
    for (retry_count = 0; retry_count <= 1; ++retry_count) {
      try {
        if (!find_entry(key, true, &hintpos))
          break;
        add_value(hintpos, v);
        retval = true;
        break;
      } catch (const std::exception &ex) {
        new_map(ex, "fast hinted insert");
        hintpos.init(get_root(), &character_set);
      }
    }
    unlock(saved_lock_level);
    //
    // end of critical section
    //
    if (retry_count > 1) {
      std::stringstream error;
      error << "insert failed " << strerror(errno) << Throw;
    }
    return retval;
  }
  
  //
  // find a value at the current iterator position (does not reposition iterator)
  // NB: does not throw
  //

  bool find_value(const iterator &i, const Value &v) {
    bool retval = false;
    //
    // critical section
    //
    int saved_lock_level = lock();
    if (i.base == 0) {
      std::stringstream error;
      error << "find_value: i.base == 0" << Throw;
    }
    entry *e = i.top_entry();
    if (e->has_single_value())
      retval = v == e->get_value();
    else {
      if (!e->has_value_list()) {
        std::stringstream error;
        error << "find_value: !e->has_value_list()" << Throw;
      }
      node *n = get_node(e->get_value_list_offset());
      Value *begin = n->get_list(); 
      Value *end = begin + n->get_count(); // one past old last entry in list
      retval = (data->option_flags & sort_values) ? std::binary_search(begin, end, v) : std::find(begin, end, v) != end;
    }
    unlock(saved_lock_level);
    //
    // end of critical section
    //
    return retval;
  }

  //
  // add a value at the current iterator position
  // NB: may throw but if it does it does not modify value list
  //

  void add_value(iterator &i, const Value &v) {
    if (i.base == 0) {
      std::stringstream error;
      error << "add_value: i.base == 0" << Throw;
    }
    //
    // critical section
    //
    int saved_lock_level = lock();
    entry *e = i.top_entry();
    my_assert(check_address(e));
    if (e->has_single_value())
      e->set_value(v);
    else {
      if (!e->has_value_list()) {
        std::stringstream error;
        error << "add_value: !e->has_value_list()" << Throw;
      }
      node *n = get_node(e->get_value_list_offset());
      offset capacity = n->capacity(sizeof(Value));
      if (n->get_count() > capacity) {
        std::stringstream error;
        error << "add_value: value list overflow" << Throw;
      }
      if (n->get_count() == capacity) {
        offset current_index = i.get_value_list_index();
        // NB: double index node before setting new value list offset
        n = double_index_node(n);
        e->set_value_list_offset(get_offset(n));
        i.set_value_list_pointer(current_index);
      }
      Value *begin = n->get_list(); 
      Value *old_end = begin + n->get_count(); // one past old last entry in list
      if (data->option_flags & sort_values) { // insert v in sorted order in list
        Value *p;
        // start looking at end of list since keys may be sorted
        if (!(v < old_end[-1])) // STL requires only that Value type implements operator<
          p = old_end;
        else {
          p = std::upper_bound(begin, old_end, v);
          std::copy_backward(p, old_end, old_end + 1);
        }
        *p = v;
        i.set_value_list_pointer(p - begin);
      } else { // list is not in sorted order; insert v at end of list
        i.value_list_pointer = old_end;
        *i.value_list_pointer = v;
      }
      n->incr_count();
      ++data->size;
      if (n->get_count() > data->max) {
        data->max = n->get_count();
        data->longest = get_offset(i.top_node());
      }
    }
    unlock(saved_lock_level);
    //
    // end of critical section
    //
  }

  //
  // update a value at the current iterator position
  // NB: does not throw
  //

  void update_value(iterator &i, const Value &v_old, const Value &v_new) {
    if (i.base == 0) {
      std::stringstream error;
      error << "update_value: i.base == 0" << Throw;
    }
    //
    // critical section
    //
    int saved_lock_level = lock();
    entry *e = i.top_entry();
    if (e->has_single_value()) {
      if (e->get_value() != v_old) {
        std::stringstream error;
        error << "update_value: e->get_value() = " << e->get_value() << " != v_old = " << v_old << Throw;
      }
      e->set_value(v_new);
    } else {
      if (!e->has_value_list()) {
        std::stringstream error;
        error << "update_value: !e->has_value_list()" << Throw;
      }
      node *n = get_node(e->get_value_list_offset());
      Value *begin = n->get_list(); 
      Value *end = begin + n->get_count(); // one past last entry in list
      if (data->option_flags & sort_values) {  // maintain sort order of list
        Value *p_old = std::lower_bound(begin, end, v_old);
        if (p_old == end || *p_old != v_old) {
          std::stringstream error;
          error << "update_value: value " << v_old << " not found in list" << Throw;
        }
        Value *p_new = std::upper_bound(begin, end, v_new);
        if (p_new == end)
          --p_new;
        if (p_new < p_old)
          std::copy_backward(p_new, p_old, p_old + 1);
        else if (p_old < p_new)
          std::copy(p_old + 1, p_new + 1, p_old);
        *p_new = v_new;
        i.set_value_list_pointer(p_new - begin);
      } else { // list is not in sorted order; search entire list
        Value *p = std::find(begin, end, v_old);
        if (p == end || *p != v_old) {
          std::stringstream error;
          error << "update_value: value " << v_old << " not found in list" << Throw;
        }
        *p = v_new;
      }
    }
    unlock(saved_lock_level);
    //
    // end of critical section
    //
  }


  //
  // remove a value at the current iterator position
  // NB: does not throw
  //

  void remove_value(iterator &i) {
    remove_value(i, i->second);
  }

  void remove_value(iterator &i, const Value &v_old) {
    if (i.base == 0) {
      std::stringstream error;
      error << "remove_value: i.base == 0" << Throw;
    }
    //
    // critical section
    //
    int saved_lock_level = lock();
    entry *e = i.top_entry();
    if (e->has_single_value()) {
      if (e->is_remainder()) {
        node *n = get_node(e->get_remainder_offset());
        unallocate(n);
        e->remove_remainder(); // NB. a remainder cannot have a child since it is always a leaf node
      }
      e->remove_value();
      // do not remove key if entry has a child
      if (!e->has_child()) {
        // remove key
        // if current character is common, turn off bit in occupancy mask and unmake room for entry
        // else removing value is sufficient (no need to recover space since expanded nodes are of fixed length)
        node *n = i.top_node();
        if (!n->is_expanded()) {
          if (i.depth < 1) {
            std::stringstream error;
            error << "remove_value: i.depth " << i.depth << " < 1 " << Throw;
          }
          if (i.depth > 1) { // not root
            unsigned short r = character_set.toradix(i.key[i.depth - 2]);
            if (r >= number_of_common_characters) {
              std::stringstream error;
              error << "remove_value: character " << i.key[i.depth - 2] << " radix " << r << " in unexpanded node" << Throw;
            }
            n->unmake_room(e, sizeof(entry));
            n->remove_occupancy_mask(bit_mask(r));
          }
        }
      }
    } else {
      if (!e->has_value_list()) {
        std::stringstream error;
        error << "remove_value: !e->has_value_list()" << Throw;
      }
      node *n = get_node(e->get_value_list_offset());
      Value *begin = n->get_list(); 
      Value *end = begin + n->get_count(); // one past last entry in list
      Value *p = (data->option_flags & sort_values) ? std::lower_bound(begin, end, v_old) : std::find(begin, end, v_old);
      if (p == end || *p != v_old) {
        std::stringstream error;
        error << "remove_value: value not found in list" << Throw;
      }
      std::copy(p + 1, end, p);
      n->decr_count();
      if (n->get_count() == 1) { // convert value list to single value
        e->convert_list_to_value(*begin);
        unallocate(n);
      }
      // set value list pointer to entry following value in list, or end if deleted value was previously last, or zero if list is now a single value
      i.set_value_list_pointer(p - begin);
      --data->size;
      // TODO: this may invalidate data->max and data->longest
    }
    unlock(saved_lock_level);
    //
    // end of critical section
    //
  }

////////////////////////////////////////////////////////////////////////////
// STL modifiers
////////////////////////////////////////////////////////////////////////////
public:

  //
  // conditionally insert new (key, value) pair
  // NB: if find_entry or add_value throw they do not invalidate index
  //

  std::pair<iterator, bool> insert(const value_type &v) {
    std::pair<iterator, bool> retval;
    //
    // critical section
    //
    int saved_lock_level = lock();
    short retry_count;
    for (retry_count = 0; retry_count <= 1; ++retry_count) {
      try {
        retval.first.init(get_root(), &character_set);
        retval.second = find_entry(v.first, true, &retval.first) != 0;
        if (retval.second)
          add_value(retval.first, v.second);
        break;
      } catch (const std::exception &ex) {
        new_map(ex, "insert");
      }
    }
    unlock(saved_lock_level);
    //
    // end of critical section
    //
    if (retry_count > 1) {
      std::stringstream error;
      error << "insert failed " << strerror(errno) << Throw;
    }
    return retval;
  }
  
  //
  // conditionally insert new (key, value) pair with hint
  // NB: if find_entry or add_value throw they do not invalidate index
  //

  iterator insert(iterator hintpos, const value_type &v) {
    if (hintpos.character_set == 0)
      hintpos.init(get_root(), &character_set);
    //
    // critical section
    //
    int saved_lock_level = lock();
    short retry_count;
    for (retry_count = 0; retry_count <= 1; ++retry_count) {
      try {
        if (find_entry(v.first, true, &hintpos))
          add_value(hintpos, v.second);
        break;
      } catch (const std::exception &ex) {
        new_map(ex, "hinted insert");
        hintpos.init(get_root(), &character_set);
      }
    }
    unlock(saved_lock_level);
    //
    // end of critical section
    //
    if (retry_count > 1) {
      std::stringstream error;
      error << "insert failed " << strerror(errno) << Throw;
    }
    return hintpos;
  }
  
  //
  // insert all (key, value) pairs in range
  //

  template <class InputIterator>
  void insert(InputIterator i, InputIterator end) {
    //
    // critical section
    //
    int saved_lock_level = lock();
    for (iterator hintpos; i != end; ++i)
      insert(hintpos, i->first, i->second);
    unlock(saved_lock_level);
    //
    // end of critical section
    //
  }

  //
  // erase all matching keys
  // TODO: speed this up by bulk deleting value lists
  //

  size_type erase(const key_type &k) {
    size_type count = 0;
    //
    // critical section
    //
    int saved_lock_level = lock();
    std::pair<iterator, iterator> r(equal_range(k));
    erase(r.first, r.second);
    unlock(saved_lock_level);
    //
    // end of critical section
    //
    return count;
  }

  //
  // erase (key, value) pair at iterator
  //

  void erase(iterator position) {
    //
    // critical section
    //
    int saved_lock_level = lock();
    short retry_count;
    for (retry_count = 0; retry_count <= 1; ++retry_count) {
      try {
        remove_value(position);
        break;
      } catch (const std::exception &ex) {
        new_map(ex, "erase");
      }
    }
    unlock(saved_lock_level);
    //
    // end of critical section
    //
    if (retry_count > 1) {
      std::stringstream error;
      error << "erase failed " << strerror(errno) << Throw;
    }
  }
  
  //
  // erase (key, value) pairs in range [first, last)
  //

  void erase(iterator first, iterator last) {
    if (first.value_list_pointer == 0) {
      while (first != last)
        erase(first++);
    } else {
      offset count = 0;
      for (iterator i = first; i != last; ++i)
        ++count;
      while (count--)
        erase(first);
    }
  }
  
////////////////////////////////////////////////////////////////////////////
// non-STL map operations
////////////////////////////////////////////////////////////////////////////
public:

  //
  // find first key, value pair with key >= passed key
  // returns true only if strict equality (==)
  // fill in passed iterator
  // use non-operator prefix if key is a regular expression
  // used by all other routines
  //

  bool lower_bound(const_iterator &i, const key_type &key) const {
    // TODO remove casts by splitting find_entry() into two pieces: one const and one non-const 
    return const_cast<radix_map *>(this)->lower_bound(*reinterpret_cast<iterator *>(&i), key);
  }

  bool lower_bound(iterator &i, const key_type &key) {
    bool retval = false;
    //
    // critical section
    //
    int saved_lock_level = lock();
    i.init(get_root(), &character_set, key);
    bool found = find_entry(key, false, &i) != 0;
    if (i.has_value()) // if current iterator has a value, then it is the lower bound 
      retval = found; // return true if exact match
    else // if current iterator does not have a value
      i.advance(true); // advance to next smallest value
    /*
    if (!found || !i.has_value() || !character_set.key_equal(i->first, key))
      i.advance(true);
    else
      retval = true;
    my_assert(i.depth == 0 || i.has_value());
    */
    unlock(saved_lock_level);
    //
    // end of critical section
    //
    return retval;
  }

 
  //
  // find lowest key greater than given key
  //

  void upper_bound(const_iterator &i, const key_type &key) {
    // TODO remove casts by splitting find_entry() into two pieces: one const and one non-const 
    const_cast<radix_map *>(this)->upper_bound(*reinterpret_cast<iterator *>(&i), key);
  }

  void upper_bound(iterator &i, const key_type &key) {
    //
    // critical section
    //
    int saved_lock_level = lock();
    if (lower_bound(i, key))
      i.advance(true);
    unlock(saved_lock_level);
    //
    // end of critical section
    //
  }

  //
  // conditionally erase (key, value) pair
  //

  size_type erase(const value_type &v) {
    size_type retval;
    iterator i;
    //
    // critical section
    //
    int saved_lock_level = lock();
    if (find_entry(v.first, false, &i)) {
      erase(i, v.second);
      retval = 1;
    } else
      retval = 0;
    unlock(saved_lock_level);
    //
    // end of critical section
    //
    return retval;
  }

  //
  // erase given value at iterator
  //

  void erase(iterator position, const mapped_type &m) {
    short retry_count;
    for (retry_count = 0; retry_count <= 1; ++retry_count) {
      try {
        remove_value(position, m);
        break;
      } catch (const std::exception &ex) {
        new_map(ex, "erase");
      }
    }
    if (retry_count > 1) {
      std::stringstream error;
      error << "erase with value failed " << strerror(errno) << Throw;
    }
  }
  
////////////////////////////////////////////////////////////////////////////
// STL map operations
////////////////////////////////////////////////////////////////////////////
public:

  //
  // find value by key
  //

  iterator find(const key_type &key) {
    iterator i;
    if (lower_bound(i, key))
      return i;
    else
      return end();
  }

  const_iterator find(const key_type &key) const {
    const_iterator i;
    if (lower_bound(i, key))
      return i;
    else
      return end();
  }

  //
  // count values by key
  //

  size_type count(const key_type &key) const {
    const_iterator i;
    if (lower_bound(i, key))
      return i.count();
    else
      return 0;
  }

  //
  // find lowest key not less than given key
  //

  iterator lower_bound(const key_type &key) {
    iterator i;
    lower_bound(i, key);
    return i;
  }

  const_iterator lower_bound(const key_type &key) const {
    const_iterator i;
    lower_bound(i, key);
    return i;
  }

  //
  // find lowest key greater than given key
  //

  iterator upper_bound(const key_type &key) {
    iterator i;
    //
    // critical section
    //
    int saved_lock_level = lock();
    if (lower_bound(i, key))
      i.advance(true);
    unlock(saved_lock_level);
    //
    // end of critical section
    //
    return i;
  }

  const_iterator upper_bound(const key_type &key) const {
    const_iterator i;
    //
    // critical section
    //
    int saved_lock_level = lock();
    if (lower_bound(i, key))
      i.advance(true);
    unlock(saved_lock_level);
    //
    // end of critical section
    //
    return i;
  }

  //
  // return pair(lower_bound, upper_bound)
  //

  std::pair<iterator, iterator> equal_range(const key_type &key) {
    std::pair<iterator, iterator> retval;
    //
    // critical section
    //
    int saved_lock_level = lock();
    bool found = lower_bound(retval.first, key);
    retval.second = retval.first;
    if (found)
      retval.second.advance(true);
    unlock(saved_lock_level);
    //
    // end of critical section
    //
    return retval;
  }

  std::pair<const_iterator, const_iterator> equal_range(const key_type &key) const {
    return equal_range(key);
  }

////////////////////////////////////////////////////////////////////////////
// debugging routines
////////////////////////////////////////////////////////////////////////////
public:

  //
  // return range containing longest list in radix_map
  //

  std::pair<iterator, iterator> longest() {
    std::pair<iterator, iterator> retval;
    // TODO: retval.first.set_child_offset(data->longest);
    //
    // critical section
    //
    int saved_lock_level = lock();
    for (retval.first = begin(); retval.first != end(); retval.first.advance(true))
      if (get_offset(retval.first.top_node()) == data->longest)
        break;
    retval.second = retval.first;
    retval.second.advance(true);
    unlock(saved_lock_level);
    //
    // end of critical section
    //
    return retval;
  }

  
  //
  // dump contents of index
  //

  void dump(std::ostream &os, offset o, unsigned short depth, char *key) {
    offset bad_count = 0;
    node *n = get_node(o);
    os << (n->is_expanded() ? '+' : '-') << o << " (" << n->size() << "): ";
    entry *e1 = n->end_array();
    for (entry *e = n->entry_array(); e != e1; ++e)
      if (!e->is_empty())
        os << character_set.tocharacter(n->get_radix(e));
    os << /*std::endl*/'\n';
    key[depth + 1] = 0;
    std::set<entry *> bad;
    for (entry *e = n->entry_array(); e != e1; ++e) {
      if (!e->is_empty()) {
        key[depth] = character_set.tocharacter(n->get_radix(e));
        os << key;
        if (e->is_remainder()) {
          if (check_offset(e->get_remainder_offset()))
            os << "|" << get_node(e->get_remainder_offset())->get_remainder();
          else {
            os << "| bad remainder offset = " << e->get_remainder_offset();
            bad.insert(e);
          }
        }
        if (!bad.count(e)) {
          if (e->has_single_value()) {
            if (data->record_list_offset == 0)
              os << " value = " << e->get_value();
            else { // use records
              if (check_active_id(os, e->get_value()))
                os << " id = " << e->get_value();
              else
                bad.insert(e);
            }
          } else if (e->has_value_list()) {
            offset o = e->get_value_list_offset();
            if (check_offset(o)) {
              if (data->record_list_offset == 0) {
                os << " number of values = " << get_node(o)->get_count();
                node *n = get_node(o);
                os << " values = ";
                offset count = 0;
                const char *sep = "";
                for (Value *v = n->get_list(), *ve = v + n->get_count(); v != ve; ++v) {
                  os << sep << *v;
                  if (count++ == 10) {
                    os << "...";
                    break;
                  }
                  sep = ",";
                }
              } else { // use records
                os << " number of ids = " << get_node(o)->get_count();
                node *n = get_node(o);
                for (Value *v = n->get_list(), *ve = v + n->get_count(); v != ve; ++v) {
                  if (!check_active_or_deleted_id(os, *v)) {
                    bad.insert(e);
                    ++bad_count;
                  }
                }
              }
              os << " bad count = " << bad_count;
            } else {
              os << " bad value list offset = " << e->get_value_list_offset();
              bad.insert(e);
            }
          }
        }
        if (!bad.count(e)) {
          if (e->has_child()) {
            if (check_offset(e->get_child_offset()))
              os << " offset = " << e->get_child_offset();
            else
              os << " bad child offset = " << e->get_child_offset();
          }
        }
        os << /*std::endl*/'\n';
      }
    }
    os << /*std::endl*/'\n';
    for (entry *e = n->entry_array(); e != e1; ++e)
      if (!e->is_empty() && !bad.count(e)) {
        key[depth] = character_set.tocharacter(n->get_radix(e));
        if (e->has_child() && check_offset(e->get_child_offset()))
          dump(os, e->get_child_offset(), depth + 1, key);
      }
    key[depth] = 0;
  }

  void dump(std::ostream &os) {
    //
    // critical section
    //
    int saved_lock_level = lock();
    offset bad_count = 0;
    if (data->record_list_offset != 0) { // use records
      os << "number of records = " << data->record_list_count << /*std::endl*/'\n';
      for (offset *vb = reinterpret_cast<offset *>(block(data->record_list_offset)), *v = vb, *ve = vb + data->record_list_count; v != ve; ++v) {
        if (!check_active_or_deleted(*v)) {
          os << " bad record offset = " << *v << " at id = " << (v - vb) + 1 << /*std::endl*/'\n';
          ++bad_count;
        }
      }
      if (bad_count)
        os << " bad count = " << bad_count << /*std::endl*/'\n';
    }
    char buffer[max_depth];
    memset(buffer, 0, max_depth);
    os << "root (" << get_root()->get_lg_count() << ")" << /*std::endl*/'\n';
    dump(os, get_root()->entry_array()->get_child_offset(), 0, buffer);
    unlock(saved_lock_level);
    //
    // end of critical section
    //
  }

  //
  // list contents of database
  //

 // return tag in data with format [:ws:]* '<' [:ws:]* [^:ws:]+ [ |>]
  std::string get_tag(const char *data) {
    std::string retval;
    const char *p = data;
    while (isspace(*p))
      ++p;
    if (*p == '<') {
      ++p;
      while (isspace(*p))
        ++p;
      for (const char *begin = p; *p; ++p)
        if (*p == ' ' || *p == '>') {
          retval.assign(begin, p);
          break;
        }
    }
    return retval;
  }

 // return tag from record id
  std::string get_tag(offset id) {
    std::string retval;
    record_t *r = reinterpret_cast<record_t *>(block(id));
    if (check_offset(r->data))
      retval = get_tag(block(r->data));
    return retval;
  }

  // return username in data with format 'username' [:ws:]* '=' [:ws:]* ['|"] [^'|"]+ ['|"]
  // TODO: obsolete: return username in data with format '<username>' [^<>]* '</username>' 
  std::string get_username(const char *data) {
    std::string retval;
    const char *p = strstr(data, "username");
    if (p) {
      p += 8;
      while (isspace(*p))
        ++p;
      if (*p == '=') {
        ++p;
        while (isspace(*p))
          ++p;
        if (*p == '"' || *p == '\'') {
          char sep = *p++;
          const char *begin = p;
          while (*p && *p != sep)
            ++p;
          retval.assign(begin, p);
        }
      }
    }
    if (retval.empty()) { // TODO: obsolete
      p = strstr(data, "<username>");
      if (p) {
        p += 10;
        for (const char *begin = p; *p; ++p)
          if (*p == '<') {
            retval.assign(begin, p);
            break;
          }
      }
    }
    return retval;
  }

  // return usernmae in data with format 'username' [:ws:]* '=' [:ws:]* ['|"] [^'|"]+ ['|"]
  std::string get_username(offset id) {
    std::string retval;
    if (id == 0)
      retval = "admin";
    else {
      record_t *r = reinterpret_cast<record_t *>(block(id));
      if (check_offset(r->data))
            retval = get_username(block(r->data));
    }
    return retval;
  }

  // check that time is valid
  bool check_time(time_stamp_t time_stamp) {
    time_stamp_t min_time_old = (2010LL - 1970LL) * 24LL * 60LL * 60LL * (365LL * 4LL + 1LL) / 4LL;
    time_stamp_t max_time_old = (2050LL - 1970LL) * 24LL * 60LL * 60LL * (365LL * 4LL + 1LL) / 4LL;
    time_stamp_t min_time = min_time_old << 20;
    time_stamp_t min_time2 = min_time + (10LL * 30LL * 24LL * 60LL * 60LL << 20);
    time_stamp_t max_time = max_time_old << 20;
    return (time_stamp > min_time_old && time_stamp < max_time_old) || (time_stamp > min_time && time_stamp < max_time);
  }

  offset get_offset_from_id(std::ostream &os, offset id) {
    offset retval = 0;
    if (id == 0)
      os << "invalid id " << id << /*std::endl*/'\n';
    else if (id > data->record_list_count)
      os << "id " << id << " greater than max " << data->record_list_count << /*std::endl*/'\n';
    else {
      offset *record_list = reinterpret_cast<offset *>(block(data->record_list_offset));
      retval = record_list[id - 1];
    }
    return retval;
  }

  record_t *check_active_or_deleted_id(std::ostream &os, offset id) {
    record_t *retval = 0;
    offset o = get_offset_from_id(os, id);
    if (o)
      retval = check_active_or_deleted(o);
    if (retval) {
      if (retval) {
        if (retval->record_id != id) {
          os << "record id " << retval->record_id << " should be " << id << /*std::endl*/'\n';
          retval = 0;
        }
      }
    }
    return retval;
  }

  record_t *check_active_id(std::ostream &os, offset id) {
    record_t *retval = 0;
    offset o = get_offset_from_id(os, id);
    if (o)
      retval = check_active(o);
    if (retval) {
      if (retval->record_id != id) {
        os << "record id " << retval->record_id << " should be " << id << /*std::endl*/'\n';
        retval = 0;
      }
    }
    return retval;
  }

  // check that record by offset is formatted correctly
  record_t *check_active_record(std::ostream &os, offset record_offset, bool check_id = false) {
    record_t *retval = check_active(record_offset);
    if (retval) {
      retval = check_active_record(os, retval, check_id);
      if (!retval)
        os << "good offset " << record_offset << " but bad record" << /*std::endl*/'\n';
    }
    return retval;
  }

  // check that record_t is formatted correctly
  record_t *check_active_record(std::ostream &os, record_t *r, bool check_id = false) {
    bool retval = true;
    std::stringstream error;
    bool good_data = false;
    bool good_keys = false;
    if (!check_time(r->create_time)) {
      retval = false;
      error << "  bad create time: " << r->create_time << /*std::endl*/'\n';
    }
    if (r->update_time != 0 && !check_time(r->update_time)) {
      retval = false;
      error << "  bad update time: " << r->update_time << /*std::endl*/'\n';
    }
    if (r->create_user_id > 2 && r->create_user_id != r->record_id && check_id && !check_active_id(os, r->create_user_id)) {
      retval = false;
      error << "  bad create user: " << r->create_user_id << /*std::endl*/'\n';
    }
    if (r->update_user_id > 2 && r->update_user_id != r->record_id && check_id && !check_active_id(os, r->update_user_id)) {
      retval = false;
      error << "  bad update user: " << r->update_user_id << /*std::endl*/'\n';
    }
    if (!memcmp(r->magic, "COLE", 4)) { // active record
      std::string tag;
      if (!check_offset(r->data)) {
        retval = false;
        error << "  bad data offset: " << r->data << /*std::endl*/'\n';
      } else if (r->length > get_node(r->data)->capacity()) {
        retval = false;
        error << "  bad length: " << r->length << " capacity = " << get_node(r->data)->capacity() << /*std::endl*/'\n';
      } else if (*(block(r->data) + r->length - 1) != 0) {
        retval = false;
        error << "  bad data termination " << /*std::endl*/'\n';
      } else {
        good_data = true;
      }
      if (r->key_length == 0) {
        retval = false;
        error << "  no keys" << /*std::endl*/'\n';
      } else if (!check_offset(r->key_data)) {
        retval = false;
        error << "  bad key offset: " << r->key_data << /*std::endl*/'\n';
      } else if (r->key_length > get_node(r->key_data)->capacity()) {
        retval = false;
        error << "  bad key length: " << r->key_length << " capacity = " << get_node(r->key_data)->capacity() << /*std::endl*/'\n';
      } else if (*(block(r->key_data) + r->key_length - 1) != 0) {
        retval = false;
        error << "  bad key termination " << /*std::endl*/'\n';
      } else {
        good_keys = true;
        for (const char *p = block(r->key_data), *q = p + r->key_length; p < q; p += strlen(p) + 1);
      }
    }
    if (!retval) {
      os << error.rdbuf();
      if (good_data)
        os << block(r->data) << /*std::endl*/'\n';
      if (good_keys) {
        const char *sep = "";
        for (const char *p = block(r->key_data), *q = p + r->key_length; p < q; p += strlen(p) + 1) {
          os << sep << p;
          sep = ", ";
        }
        os << /*std::endl*/'\n';
      }
    }
    return retval ? r : 0;
  }

  class ctime_t {
    std::string work;
  public:
    ctime_t(time_stamp_t time_stamp) {
      time_t temp = time_stamp >> 20;
      work = ctime(&temp);
    }
    operator const char *() {
      return work.c_str();
    }
  };

  // show given record
  bool show_record(std::ostream &os, offset record_offset, bool check_id = true) {
    record_t *r = check_active_record(os, record_offset, check_id);
    if (r != 0) {
      os << "record id: " << r->record_id << /*std::endl*/'\n';
      os << "create time: " << ctime_t(r->create_time);
      os << "update time: " << ctime_t(r->update_time);
      os << "create user: " << r->create_user_id << /*std::endl*/'\n';
      os << "update user: " << r->update_user_id << /*std::endl*/'\n';
      os << block(r->data) << /*std::endl*/'\n';
      const char *sep = "";
      for (const char *p = block(r->key_data), *q = p + r->key_length; p < q; p += strlen(p) + 1) {
        os << sep << p;
        sep = ", ";
      }
      os << /*std::endl*/'\n';
    }
    return r != 0;
  }

  typedef std::set<offset, record_offset_sort_by_id_t> set_record_offset_sorted_by_id_t;

  // list all valid ids by scanning memory
  void list_from_memory(std::ostream &os, set_record_offset_sorted_by_id_t &offsets) {
    for (offset o = get_root()->size() / min_size; o != mmap_len / min_size; ++o) { // skip over root, which is part of the header and not an allocated node
      record_t *record = check_active_or_deleted(o);
      if (record && (!memcmp(record->magic, "DEAD", 4) || check_active_record(os, record)))
        offsets.insert(o);
    }
  }

  bool list_active_from_index(std::ostream &os, std::set<offset> &ids) {
    bool retval = true;
    for (iterator i = begin(); i != end(); ++i)
      if (check_active_id(os, i->second) != 0)
        ids.insert(i->second);
      else {
        os << "invalid id " << i->second << " in index at key = " << i->first << /*std::endl*/'\n';
        retval = false;
      }
    return retval;
  }

  // list all ids from the record list
  bool list_from_record_list(std::ostream &os, std::vector<offset> &offsets) {
    bool retval = true;
    for (offset id = 1; id <= data->record_list_count; ++id) {
      offset *record_list = reinterpret_cast<offset *>(block(data->record_list_offset));
      offset record_offset = record_list[id - 1];
      record_t *record = check_active_or_deleted(record_offset);
      if (!record) {
        os << "invalid offset = " << record_offset << " at id = " << id << /*std::endl*/'\n';
        retval = false;
      } else if (record->record_id != id) {
        os << "unmatched id = " << record->record_id << " at id = " << id << /*std::endl*/'\n';
        retval = false;
      } else 
        offsets.push_back(record_offset);
    }
    return retval;
  }

  //
  // examine contents of database; check database for errors
  //

  enum { active_record = 1, deleted_record = 2, index_node = 4, index_remainder_node = 8, free_block = 16 };
  typedef std::map<offset, int> examine_map_t;
  
  // examine all nodes by scanning the index
  bool examine_from_index_node(std::ostream &os, offset o, examine_map_t &map)
  {
    bool retval = true;
    node *n = get_node(o);
    for (entry *e = n->entry_array(), *e1 = n->end_array(); retval && e != e1; ++e) {
      if (!e->is_empty()) {
        if (e->is_remainder()) {
          offset o = e->get_remainder_offset();
          retval = check_offset(o);
          if (!retval)
            os << "invalid remainder offset " << o << " in entry " << e << /*std::endl*/'\n';
          else {
            examine_map_t::iterator i = map.find(o);
            retval = i != map.end();
            if (!retval)
              os << "map does not contain remainder offset " << o << " in entry " << e << /*std::endl*/'\n';
            else {
              retval = i->second == 0;
              if (!retval)
                os << "remainder offset " << o << " in entry " << e << " is already classified as " << i->second << /*std::endl*/'\n';
              else
                i->second |= index_remainder_node;
            }
          }
        }
        if (retval) {
          if (e->has_single_value()) {
            offset o = get_offset_from_id(os, e->get_value());
            if ((retval = check_active_record(os, o)) != 0) {
              examine_map_t::iterator i = map.find(o);
              retval = i != map.end();
              if (!retval)
                os << "record offset " << o << " not in map" << /*std::endl*/'\n';
              else {
                retval = i->second == active_record;
                if (!retval)
                  os << "record offset " << o << " is not classified as active record, is classified as " << i->second << /*std::endl*/'\n';
              }
            }
          } else if (e->has_value_list()) {
            offset o = e->get_value_list_offset();
            if (!(retval = check_offset(o)))
              os << "invalid value list offset " << o << " in entry " << e << /*std::endl*/'\n';
            else {
              node *n = get_node(o);
              for (Value *v = n->get_list(), *ve = v + n->get_count(); retval && v != ve; ++v) {
                offset o = get_offset_from_id(os, *v);
                if ((retval = check_active_record(os, o)) != 0) {
                  examine_map_t::iterator i = map.find(o);
                  retval = i != map.end();
                  if (!retval)
                    os << "record offset " << o << " in entry e " << e << " not in map" << /*std::endl*/'\n';
                  else {
                    retval = i->second == active_record;
                    if (!retval)
                      os << "record offset " << o << " in entry " << e << " is not classified as active record, is classified as " << i->second << /*std::endl*/'\n';
                  }
                }
              }
              if (!retval)
                os << "invalid value in list at offset " << o << " in entry " << e << /*std::endl*/'\n';
            }
          }
        }
        if (retval && e->has_child()) {
          offset o = e->get_child_offset();
          retval = check_offset(o);
          if (!retval)
            os << "invalid child offset " << o << " in entry " << e << /*std::endl*/'\n';
          else {
            examine_map_t::iterator i = map.find(o);
            retval = i != map.end();
            if (!retval)
              os << "index node offset " << o << " in entry e " << e << " not in map" << /*std::endl*/'\n';
            else {
              retval = i->second == 0;
              if (!retval)
                os << "index node offset " << o << " in entry " << e << " is not classified as index node, is classified as " << i->second << /*std::endl*/'\n';
              else {
                i->second |= index_node;
                retval = examine_from_index_node(os, o, map);
              }
            }
          }
        }
      }
    }
    return retval;
  }

  bool examine_from_index(std::ostream &os, examine_map_t &map) {
    return examine_from_index_node(os, get_root()->entry_array()->get_child_offset(), map);
  }

  bool examine_from_free_list(std::ostream &os, examine_map_t &map) {
    bool retval = true;
    for (unsigned short l = 0; l != lg_max_count; ++l) {
      for (offset o = data->space.next[l]; o; o = *reinterpret_cast<offset *>(get_node(o))) {
        examine_map_t::iterator i = map.find(o);
        retval = i == map.end();
        if (!retval)
          os << "unable to find free list offset " << o << " in map" << /*std::endl*/'\n';
        else {
          retval = i->second == 0;
          if (!retval)
            os << "free list offset " << o << " in free list " << l << " is classified as " << i->second << /*std::endl*/'\n';
          else
            i->second |= free_block;
        }
      }
    }
    return retval;
  }

  bool examine(std::ostream &os) {
    bool retval = true;
    //
    // critical section
    //
    int saved_lock_level = lock();
    examine_map_t map;
    for (offset o = get_root()->size() / min_size; o < mmap_len / min_size; ) { // skip over root, which is part of the header and not an allocated node
      map[o] = check_active_or_deleted(o) ? check_active_record(os, o) ? active_record : deleted_record : 0;
      node *n = get_node(o);
      o += n->size() / min_size;
    }
    retval = examine_from_index(os, map) && examine_from_free_list(os, map);
    for (examine_map_t::iterator i = map.begin(); i != map.end(); ++i)
      if (i->second != active_record
        && i->second != deleted_record
        && i->second != index_remainder_node
        && i->second != index_node
        && i->second != free_block
        ) {
        os << "offset " << i->first << " is unaccounted for" << /*std::endl*/'\n';
        retval = false;
      }
    unlock(saved_lock_level);
    //
    // end of critical section
    //
    return retval;
  }

  //
  // unload contents of database
  //

  void print(std::ostream &os, offset id) {
    if (check_active(id) != 0) {
      //
      // critical section
      //
      int saved_lock_level = lock();
      record_t *r = reinterpret_cast<record_t *>(block(id));
      os.write(block(r->data), r->length - 1);
      os << /*std::endl*/'\n';
      os << "**" << /*std::endl*/'\n';
      os << get_username(r->create_user_id) << /*std::endl*/'\n';
      os << r->create_time << /*std::endl*/'\n';
      os << get_username(r->update_user_id) << /*std::endl*/'\n';
      os << r->update_time << /*std::endl*/'\n';
      for (const char *p = block(r->key_data), *q = p + r->key_length; p < q; p += strlen(p) + 1)
        os << p << /*std::endl*/'\n';
      os << "***" << /*std::endl*/'\n';
      unlock(saved_lock_level);
      //
      // end of critical section
      //
    }
  }

  //
  // unload contents of database
  //

  void unload(std::ostream &os, const char *file_name) {
    std::ofstream of(file_name);
    if (!of.good())
      os << "unable to write to file: " << file_name << /*std::endl*/'\n';
    else {
      //
      // critical section
      //
      int saved_lock_level = lock();
      std::vector<offset> ids;
      list_from_record_list(of, ids);
      for (std::vector<offset>::iterator i = ids.begin(); i != ids.end(); ++i)
        print(of, *i);
      unlock(saved_lock_level);
      //
      // end of critical section
      //
    }
  }

  //
  // load contents of datbase
  //

  typedef std::map<std::string, offset> map_t; // mapping from username -> id
  typedef std::map<std::string, std::vector<offset> > xref_t; // mapping from username -> array of ids

  // load one record from stream
  offset load_record(std::ostream &os, const std::string &record, const std::string &tag, std::ifstream &is, xref_t &create_xref, xref_t &update_xref) {

    offset retval = 0;
    std::string create_user;
    if (!std::getline(is, create_user))
      os << "unable to read line with create user" << /*std::endl*/'\n';
    else {
      std::string create_time_str;
      if (!std::getline(is, create_time_str))
        os << "unable to read line with create time" << /*std::endl*/'\n';
      else {
        time_stamp_t create_time = atoll(create_time_str.c_str());
        if (create_time == 0)
          os << "invalid create time: " << create_time_str << /*std::endl*/'\n';
        else {
          std::string update_user;
          if (!std::getline(is, update_user))
            os << "unable to read line with update user" << /*std::endl*/'\n';
          else {
            std::string update_time_str;
            if (!std::getline(is, update_time_str))
              os << "unable to read line with update time" << /*std::endl*/'\n';
            else {
              time_stamp_t update_time = atoll(update_time_str.c_str());
              if (update_time == 0)
                os << "invalid update time: " << update_time_str << /*std::endl*/'\n';
              else {
                std::vector<std::string> key_data;
                std::vector<key_type> key_list;
                for (std::string key; std::getline(is, key); ) {
                  if (key == "***")
                    break;
                  if (key.compare(0, tag.length(), tag))
                    os << "key (" << key << ") does not match tag of record: " << record << /*std::endl*/'\n';
                  else {
                    key_data.push_back(key);
                    key_list.push_back(key_data.back().c_str());
                  }
                }
                if (key_list.empty())
                  os << "empty key list for record: " << record << /*std::endl*/'\n';
                else {
                  retval = copy_record(record.c_str(), record.length() + 1, 0, create_time, 0, update_time);
                  update_keys(retval, 0, key_list); // TODO: get record id
                  create_xref[create_user].push_back(retval);
                  update_xref[update_user].push_back(retval);
                }
              }
            }
          }
        }
      }
    }
    return retval;
  }

  // load database; load records then resolve create_user_ids and update_user_ids
  void load(std::ostream &os, const char *file_name) {
    std::ifstream is(file_name);
    if (!is.good())
      os << "unable to open file: " << file_name << /*std::endl*/'\n';
    else {
      //
      // critical section
      //
      int saved_lock_level = lock();
      map_t security_xref; // username -> id mapping
      xref_t create_xref; // create username -> id mapping
      xref_t update_xref; // update username -> id mapping
      
      for (std::string line; std::getline(is, line); ) {
        std::string record = line;
        while (std::getline(is, line))
          if (line == "**")
            break;
          else
            record += line;
        std::string tag = get_tag(record.c_str());
        offset id = load_record(os, record, tag, is, create_xref, update_xref);
        if (tag == "security") {
          std::string username = get_username(record.c_str());
          if (username.empty())
            os << "no username in security record: " << record << /*std::endl*/'\n';
          else
            security_xref[username] = id;
        }
      }
      // resolve create users
      for (xref_t::const_iterator i = create_xref.begin(); i != create_xref.end(); ++i) {
        for (std::vector<offset>::const_iterator j = i->second.begin(); j != i->second.end(); ++j) {
          record_t *r = reinterpret_cast<record_t *>(block(*j));
          map_t::const_iterator k = security_xref.find(i->first);
          if (k != security_xref.end())
            r->create_user_id = k->second;
          else
            os << "unable to find create user " << i->first << " for record " << block(r->data) << /*std::endl*/'\n';
        }
      }
      // resolve update users
      for (xref_t::const_iterator i = update_xref.begin(); i != update_xref.end(); ++i) {
        for (std::vector<offset>::const_iterator j = i->second.begin(); j != i->second.end(); ++j) {
          record_t *r = reinterpret_cast<record_t *>(block(*j));
          map_t::const_iterator k = security_xref.find(i->first);
          if (k != security_xref.end())
            r->update_user_id = k->second;
          else
            os << "unable to find update user " << i->first << " for record " << block(r->data) << /*std::endl*/'\n';
        }
      }
      unlock(saved_lock_level);
      //
      // end of critical section
      //
    }
  }

  //
  // fix index
  // NB.  Assumes that records are in use.
  //

  void fix(std::ostream &os)
  {
    if (data->record_list_offset == 0) { // records must be in use
      os << "unable to fix becasue record use is required" << /*std::endl*/'\n';
      exit(-1);
    }
    //
    // critical section
    //
    int saved_lock_level = lock();
    //
    // map offset -> length
    //
    std::map<offset, offset> m;
    offset total_allocated = 0;
    //
    // account for record list
    //
    offset record_list_length = get_node(data->record_list_offset)->size();
    m[data->record_list_offset] = record_list_length;
    total_allocated += record_list_length;
    //
    // get list of all record offsets sorted by record id
    //
    typedef set_record_offset_sorted_by_id_t list_t;
    list_t offsets(*this);
    list_from_memory(os, offsets);
    //
    // add all allocated record data to map
    //
    for (typename list_t::iterator i = offsets.begin(); i != offsets.end(); ++i) {
      record_t *r = reinterpret_cast<record_t *>(block(*i));
      m[*i] = get_node(*i)->size();
      total_allocated += get_node(*i)->size();
      if (r->length) {
        if (r->length >= get_node(r->data)->size()) {
          os <<
           "record at offset " << *i <<
           " data offset = " << r->data <<
           " stated length = " << r->length <<
           " but node length = " << get_node(r->data)->size() <<
           std::endl;
          show_record(os, *i);
          exit(-1);
        }
        m[r->data] = get_node(r->data)->size();
        total_allocated += get_node(*i)->size();
        if (r->key_length) {
          if (r->key_length >= get_node(r->key_data)->size()) {
            os <<
            "record at offset " << *i <<
            " key data offset = " << r->key_data <<
            " stated length = " << r->key_length <<
            " but node length = " << get_node(r->key_data)->size() <<
             std::endl;
            show_record(os, *i);
            exit(-1);
          }
          m[r->key_data] = get_node(r->key_data)->size();
          total_allocated += get_node(*i)->size();
        }
      }
    }
    //
    // check that allocated memory segments do not overlap
    //
    const char *last = reinterpret_cast<char *>(get_root()) + get_root()->size(); // skip over root, which is part of the header and not an allocated node
    for (std::map<offset, offset>::iterator i = m.begin(); i != m.end(); ++i) {
      if (reinterpret_cast<char *>(get_node(i->first)) < last) {
        os << "node at " << i->first << " for " << i->second << " bytes overlaps previous segment; fix fails" << /*std::endl*/'\n';
        show_record(os, i->first, false);
        return;
      }
      last = reinterpret_cast<char *>(get_node(i->first)) + i->second;
    }
    //
    // add all unallocated memory to free lists
    //
    clear_free_lists();
    char *p = reinterpret_cast<char *>(get_root()) + get_root()->size(); // skip over root, which is part of the header and not an allocated node
    for (std::map<offset, offset>::iterator i = m.begin(); i != m.end(); ++i) {
      if (reinterpret_cast<char *>(get_node(i->first)) < p) {
        os <<
          "node at " << i->first <<
          " for " << i->second <<
          " bytes overlaps next segment; skipping it " <<
          /*std::endl*/'\n';
        show_record(os, i->first, false);
        continue;
      }
      offset l = reinterpret_cast<char *>(get_node(i->first)) - p;
      while (l != 0) {
        my_assert(l >= min_size);
        node *n = reinterpret_cast<node *>(p);
        n->set_lg_count(lg_count(l));
        if (n->size() > l) {
          my_assert(n->get_lg_count() > 0);
          n->set_lg_count(n->get_lg_count() - 1);
        }
        my_assert(l >= n->size());
        l -= n->size();
        p += n->size();
        free(n->body());
      }
      p = reinterpret_cast<char *>(get_node(i->first)) + i->second;
    }
    //
    // remap to end of currently allocated memory
    //
    std::map<offset, offset>::reverse_iterator last_allocation = m.rbegin();
    offset new_length = reinterpret_cast<const char *>(get_node(last_allocation->first)) + last_allocation->second - reinterpret_cast<const char *>(data);
    unmap();
    set_length(new_length);
    map();
    //
    // add all key data to index and add record offset to record_list
    //
    clear_index();
    set_option_flag(sort_values);
    for (typename list_t::iterator i = offsets.begin(); i != offsets.end(); ++i) {
      record_t *r = reinterpret_cast<record_t *>(block(*i));
      if (check_active(*i)) {                                                          // record is not deleted
        std::string key_data(block(r->key_data), r->key_length); // copy key data since it may be moved by remap
        for (const char *p = key_data.c_str(), *q = p + r->key_length; p < q; p += strlen(p) + 1) {
          if (!insert(std::make_pair(p, r->record_id)).second)
            os << "unable to insert key " << p << " id " << r->record_id << " for record " << r->data << /*std::endl*/'\n';
          r = reinterpret_cast<record_t *>(block(*i)); // insert() may remap
        }
      }
      if (r->record_id != 0 && r->record_id <= data->record_list_count) { // if valid record id
        offset *record_list = reinterpret_cast<offset *>(block(data->record_list_offset));
        record_list[r->record_id - 1] = *i; // save record offset by id
      }
    }
    //
    // trim index
    //
    trim();
    unlock(saved_lock_level);
    //
    // end of critical section
    //
  }
};

template <typename Value>
std::ostream &operator<<(std::ostream &os, radix_map<Value> &x) {
  x.dump(os);
  return os;
}

} // end of namespace

#endif // RADIX_MAP_H
