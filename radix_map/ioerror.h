#ifndef IOERROR_H
#define IOERROR_H
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <iostream>
#include <sstream>
#include <errno.h>

struct ErrorType {
  int value;
  ErrorType(int value_) : value(value_) {}
};

const ErrorType Error(0);
const ErrorType Exit(1);
const ErrorType Throw(2);

static std::ostream &operator<<(std::ostream &os, const ErrorType &error_type) {
  switch (error_type.value) {
  case 0:
    os << strerror(errno) << std::endl;
    break;
  case 1:
    os << " (exiting)" << std::endl;
    exit(-1);
    break;
  case 2:
    throw std::runtime_error(((std::stringstream *)(&os))->str());
    break;
  }
  return os;
}
#endif
