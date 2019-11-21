#ifndef ECMA_H
#define ECMA_H

#include <cctype>		// for isspace()
#include <cstring>		// for strtok()
#include <string>
#include <vector>

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  ECMA routines for strings
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//
// strstr with a stopper
//

static const char *strstr(const char *s1, const char *s2, char stop) {
  for (size_t l = strlen(s2); *s1; ++s1) {
    if (*s1 == stop)
      return 0;
    if (!memcmp(s1, s2, l))
      return s1;
  }
  return 0;
}

//
// return a string with white space stripped off the ends
//
static std::string strip(const char *s) {
  while (*s && isspace(*s))
    ++s;
  const char *e = 0;
  for (const char *p = s; *p; ++p)
    if (!isspace(*p))
      e = p;
  return std::string(s, e ? e - s + 1 : 0);
}

//
// split a string into a given number of stripped pieces at delimiting string (cf. strtok)
//
static bool split(std::string &s, const char *tokens, std::string &p1, std::string &p2) {
  bool retval = true;
  size_t count = 0;
  for (char *p = (char *)s.c_str(); retval && (p = strtok(p, tokens)); p = 0)
    switch (++count) {
    case 1:
      p1 = strip(p);
      break;
    case 2:
      p2 = strip(p);
      break;
    default:
      retval = false;
      break;
    }
  return retval;
}

//
// split a string into a variable number of pieces at delimiting tokens (cf. strtok)
//
static void split(const char *p, std::vector<std::string> &vec, const char *tokens) {
  vec.clear();
  for (const char *start = 0; ; ++p)
    if (strchr(tokens, *p) || *p == 0) {
      if (start != 0) {
	vec.push_back(std::string(start, p - start));
	start = 0;
      }
      if (*p == 0)
	break;
    } else {
      if (start == 0)
	start = p;
    }
}
#endif // ECMA_H
