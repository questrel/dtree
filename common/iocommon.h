////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// common routines for input/output from/to the console
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef IOCOMMON_H
#define IOCOMMON_H
#include <cstdlib>
#include <cstring>
#include <ioerror.h>
#include <iostream>
#include <sstream>

#ifdef DEBUG
#define my_assert(x) if (!(x)) *(new std::stringstream) << "assert " #x " failed" << Throw
#else
#define my_assert(x) (0)
#endif


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// input routines
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static bool input(const char *name, unsigned long long &value) {
  char buf[256];
  std::cout << "enter " << name << " [" << value << "] (q = quit): ";
  std::cin.getline(buf, sizeof(buf));
  if (*buf) {
    if (*buf == 'q')
      return false;
    value = atoll(buf);
    std::cout << name << " set to " << value << std::endl;
  }
  return true;
}

static bool input(const char *name, unsigned int &value) {
  char buf[256];
  std::cout << "enter " << name << " [" << value << "] (q = quit): ";
  std::cin.getline(buf, sizeof(buf));
  if (*buf) {
    if (*buf == 'q')
      return false;
    value = atol(buf);
    std::cout << name << " set to " << value << std::endl;
  }
  return true;
}

static bool input(const char *name, unsigned short &value) {
  char buf[256];
  std::cout << "enter " << name << " [" << value << "] (q = quit): ";
  std::cin.getline(buf, sizeof(buf));
  if (*buf) {
    if (*buf == 'q')
      return false;
    value = atoi(buf);
    std::cout << name << " set to " << value << std::endl;
  }
  return true;
}

static bool input(const char *name, double &value) {
  char buf[256];
  std::cout << "enter " << name << " [" << value << "] (q = quit): ";
  std::cin.getline(buf, sizeof(buf));
  if (*buf) {
    if (*buf == 'q')
      return false;
    value = atof(buf);
    std::cout << name << " set to " << value << std::endl;
  }
  return true;
}

// name can include a parenthesized list of comma separated options 
// if a single character is input and it matches the first letter of one of the options
// the option is copied to the value buffer
static bool input(const char *name, char *value) {
  char buf[256];
  std::cout << "enter " << name << " [" << value << "] (q = quit): ";
  std::cin.getline(buf, sizeof(buf));
  if (*buf) {
    bool found = false;
    if (buf[1] == 0) { // abbreviation
      if (buf[0] == 'q')
	return false;
      for (const char *p = strchr(name, '('); p; p = strchr(p, ',')) {
	while (*p && *++p == ' ');
	if (*p == buf[0]) {
	  size_t l = 0;
	  while (p[l] != ',' && p[l] != ')')
	    ++l;
	  memcpy(value, p, l);
	  value[l] = 0;
	  found = true;
	  break;
	}
      }
    }
    if (!found)
      strcpy(value, buf);
    std::cout << name << " set to " << value << std::endl;
  }
  return true;
}

// value is displayed and optionally selected, no edits are allowed
static bool input(const char *name, const std::string &value) {
  char buf[256];
  std::cout << "using " << name << " [" << value << "] (y = yes): ";
  std::cin.getline(buf, sizeof(buf));
  if (*buf) {
    if (buf[0] != 'y')
      return false;
    std::cout << name << " set to " << value << std::endl;
  }
  return true;
}

// value is displayed, if enter is pressed no edits are made, otherwise new value is copied to value (unless it is the single letter 'q')
static bool input(const char *name, std::string &value) {
  std::cout << "using " << name << " [" << value << "] (q = quit): ";
  std::string line;
  if (!getline(std::cin, line) || line == "q")
    return false;
  if (!line.empty())
    value = line;
  std::cout << name << " set to " << value << std::endl;
  return true;
}

static bool input(const char *name, bool &value) {
  char buf[256];
  std::cout << "enter " << name << " [" << (value ? "y" : "n") << "] (y, n, or q = quit): ";
  std::cin.getline(buf, sizeof(buf));
  if (*buf) {
    if (buf[0] == 'q' && buf[1] == 0)
      return false;
    value = (buf[0] == 'y' && buf[1] == 0);
    std::cout << name << " set to " << (value ? "y" : "n") << std::endl;
  }
  return true;
}

#endif // IOCOMMON_H
