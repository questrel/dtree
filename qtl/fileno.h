//https://www.ginac.de/~kreckel/fileno/
#include <iosfwd>
template <typename charT, typename traits>
int fileno(const std::basic_ios<charT, traits>& stream);
#include <cstdio>  // declaration of ::fileno
#include <fstream>  // for basic_filebuf template
#include <cerrno>

#if defined(__GLIBCXX__) || (defined(__GLIBCPP__) && __GLIBCPP__>=20020514)  // GCC >= 3.1.0
# include <ext/stdio_filebuf.h>
#endif
#if defined(__GLIBCXX__) // GCC >= 3.4.0
# include <ext/stdio_sync_filebuf.h>
#endif

//! Similar to fileno(3), but taking a C++ stream as argument instead of a
//! FILE*.  Note that there is no way for the library to track what you do with
//! the descriptor, so be careful.
//! \return  The integer file descriptor associated with the stream, or -1 if
//!   that stream is invalid.  In the latter case, for the sake of keeping the
//!   code as similar to fileno(3), errno is set to EBADF.
//! \see  The <A HREF="https://www.ginac.de/~kreckel/fileno/">upstream page at
//!   https://www.ginac.de/~kreckel/fileno/</A> of this code provides more
//!   detailed information.
template <typename charT, typename traits>
inline int
fileno_hack(const std::basic_ios<charT, traits>& stream)
{
    // Some C++ runtime libraries shipped with ancient GCC, Sun Pro,
    // Sun WS/Forte 5/6, Compaq C++ supported non-standard file descriptor
    // access basic_filebuf<>::fd().  Alas, starting from GCC 3.1, the GNU C++
    // runtime removes all non-standard std::filebuf methods and provides an
    // extension template class __gnu_cxx::stdio_filebuf on all systems where
    // that appears to make sense (i.e. at least all Unix systems).  Starting
    // from GCC 3.4, there is an __gnu_cxx::stdio_sync_filebuf, in addition.
    // Sorry, darling, I must get brutal to fetch the darn file descriptor!
    // Please complain to your compiler/libstdc++ vendor...
#if defined(__GLIBCXX__) || defined(__GLIBCPP__)
    // OK, stop reading here, because it's getting obscene.  Cross fingers!
# if defined(__GLIBCXX__)  // >= GCC 3.4.0
    // This applies to cin, cout and cerr when not synced with stdio:
    typedef __gnu_cxx::stdio_filebuf<charT, traits> unix_filebuf_t;
    unix_filebuf_t* fbuf = dynamic_cast<unix_filebuf_t*>(stream.rdbuf());
    if (fbuf != NULL) {
        return fbuf->fd();
    }

    // This applies to filestreams:
    typedef std::basic_filebuf<charT, traits> filebuf_t;
    filebuf_t* bbuf = dynamic_cast<filebuf_t*>(stream.rdbuf());
    if (bbuf != NULL) {
        // This subclass is only there for accessing the FILE*.  Ouuwww, sucks!
        struct my_filebuf : public std::basic_filebuf<charT, traits> {
            int fd() { return this->_M_file.fd(); }
        };
        return static_cast<my_filebuf*>(bbuf)->fd();
    }

    // This applies to cin, cout and cerr when synced with stdio:
    typedef __gnu_cxx::stdio_sync_filebuf<charT, traits> sync_filebuf_t;
    sync_filebuf_t* sbuf = dynamic_cast<sync_filebuf_t*>(stream.rdbuf());
    if (sbuf != NULL) {
#  if (__GLIBCXX__<20040906) // GCC < 3.4.2
        // This subclass is only there for accessing the FILE*.
        // See GCC PR#14600 and PR#16411.
        struct my_filebuf : public sync_filebuf_t {
            my_filebuf();  // Dummy ctor keeps the compiler happy.
            // Note: stdio_sync_filebuf has a FILE* as its first (but private)
            // member variable.  However, it is derived from basic_streambuf<>
            // and the FILE* is the first non-inherited member variable.
            FILE* c_file() {
                return *(FILE**)((char*)this + sizeof(std::basic_streambuf<charT, traits>));
            }
        };
        return ::fileno(static_cast<my_filebuf*>(sbuf)->c_file());
#  else
        return ::fileno(sbuf->file());
#  endif
    }
# else  // GCC < 3.4.0 used __GLIBCPP__
#  if (__GLIBCPP__>=20020514)  // GCC >= 3.1.0
    // This applies to cin, cout and cerr:
    typedef __gnu_cxx::stdio_filebuf<charT, traits> unix_filebuf_t;
    unix_filebuf_t* buf = dynamic_cast<unix_filebuf_t*>(stream.rdbuf());
    if (buf != NULL) {
        return buf->fd();
    }

    // This applies to filestreams:
    typedef std::basic_filebuf<charT, traits> filebuf_t;
    filebuf_t* bbuf = dynamic_cast<filebuf_t*>(stream.rdbuf());
    if (bbuf != NULL) {
        // This subclass is only there for accessing the FILE*.  Ouuwww, sucks!
        struct my_filebuf : public std::basic_filebuf<charT, traits> {
            // Note: _M_file is of type __basic_file<char> which has a
            // FILE* as its first (but private) member variable.
            FILE* c_file() { return *(FILE**)(&this->_M_file); }
        };
        FILE* c_file = static_cast<my_filebuf*>(bbuf)->c_file();
        if (c_file != NULL) {  // Could be NULL for failed ifstreams.
            return ::fileno(c_file);
        }
    }
#  else  // GCC 3.0.x
    typedef std::basic_filebuf<charT, traits> filebuf_t;
    filebuf_t* fbuf = dynamic_cast<filebuf_t*>(stream.rdbuf());
    if (fbuf != NULL) {
        struct my_filebuf : public filebuf_t {
            // Note: basic_filebuf<charT, traits> has a __basic_file<charT>* as
            // its first (but private) member variable.  Since it is derived
            // from basic_streambuf<charT, traits> we can guess its offset.
            // __basic_file<charT> in turn has a FILE* as its first (but
            // private) member variable.  Get it by brute force.  Oh, geez!
            FILE* c_file() {
                std::__basic_file<charT>* ptr_M_file = *(std::__basic_file<charT>**)((char*)this + sizeof(std::basic_streambuf<charT, traits>));
#  if _GLIBCPP_BASIC_FILE_INHERITANCE
                // __basic_file<charT> inherits from __basic_file_base<charT>
                return *(FILE**)((char*)ptr_M_file + sizeof(std::__basic_file_base<charT>));
#  else
                // __basic_file<charT> is base class, but with vptr.
                return *(FILE**)((char*)ptr_M_file + sizeof(void*));
#  endif
            }
        };
        return ::fileno(static_cast<my_filebuf*>(fbuf)->c_file());
    }
#  endif
# endif
#else
//#  error "Does anybody know how to fetch the bloody file descriptor?"
//#pragma message "Does anybody know how to fetch the bloody file descriptor?"
#ifndef __clang__
    return stream.rdbuf()->fd();  // Maybe a good start?
#endif
#endif
    errno = EBADF;
    return -1;
}

//! 8-Bit character instantiation: fileno(ios).
template <>
int
fileno<char>(const std::ios& stream)
{
    return fileno_hack(stream);
}

#if !(defined(__GLIBCXX__) || defined(__GLIBCPP__)) || (defined(_GLIBCPP_USE_WCHAR_T) || defined(_GLIBCXX_USE_WCHAR_T))
//! Wide character instantiation: fileno(wios).
template <>
int
fileno<wchar_t>(const std::wios& stream)
{
    return fileno_hack(stream);
}
#endif
#include <unistd.h>
bool isatty(std::ios &s){
#ifdef __clang__ 
  return false;
#else
  return isatty(fileno(s)); 
#endif
}
#include <sys/ioctl.h>
#include <stdio.h>
#include <unistd.h>
std::pair<int,int> size_w_h(std::ios &s){
  int cols = 132;//80
  int lines = 66;//24
  if( 
    isatty(s) 
  ){
#ifdef TIOCGSIZE
  struct ttysize ts;
  ioctl(STDIN_FILENO, TIOCGSIZE, &ts);
  cols = ts.ts_cols;
  lines = ts.ts_lines;
#elif defined(TIOCGWINSZ)
  struct winsize ts;
  ioctl(STDIN_FILENO, TIOCGWINSZ, &ts);
  cols = ts.ws_col;
  lines = ts.ws_row;
#endif /* TIOCGSIZE */
  }else{
  }
  return {cols,lines};
}

#if __INCLUDE_LEVEL__ + !defined __INCLUDE_LEVEL__ < 1+defined TEST_H
#include <iostream>
#include <fstream>
#include <sstream>
void check_default_streams()
{
    int fd;
    fd = fileno(std::cin);
    if (fd != 0)
        std::cerr << "fileno(std::cin)==" << fd << std::endl;
    fd = fileno(std::cout);
    if (fd != 1)
        std::cerr << "fileno(std::cout)==" << fd << std::endl;
    fd = fileno(std::cerr);
    if (fd != 2)
        std::cerr << "fileno(std::cerr)==" << fd << std::endl;
    fd = fileno(std::clog);
    if (fd != 2)
        std::cerr << "fileno(std::clog)==" << fd << std::endl;
}

void check_default_wstreams()
{
    int fd;
    fd = fileno(std::wcin);
    if (fd != 0)
        std::cerr << "fileno(std::wcin)==" << fd << std::endl;
    fd = fileno(std::wcout);
    if (fd != 1)
        std::cerr << "fileno(std::wcout)==" << fd << std::endl;
    fd = fileno(std::wcerr);
    if (fd != 2)
        std::cerr << "fileno(std::wcerr)==" << fd << std::endl;
    fd = fileno(std::wclog);
    if (fd != 2)
        std::cerr << "fileno(std::wclog)==" << fd << std::endl;
}

int main()
{
    std::ios_base::sync_with_stdio(true);
    check_default_streams();
    check_default_wstreams();
    std::ios_base::sync_with_stdio(false);
    check_default_streams();
    check_default_wstreams();

    int fd;
    std::ifstream ist("/dev/zero");
    fd = fileno(ist);
    if (fd != 3)
        std::cerr << "fileno(\"/dev/zero\")==" << fd << std::endl;
    std::ofstream ost("/dev/null");
    fd = fileno(ost);
    if (fd != 4)
        std::cerr << "fileno(\"/dev/null\")==" << fd << std::endl;
    std::stringstream sst;
    fd = fileno(sst);
    if (fd != -1)
        std::cerr << "fileno(stringstream)==" << fd << std::endl;

    std::wifstream wist("/dev/zero");
    fd = fileno(wist);
    if (fd != 5)
        std::cerr << "fileno(L\"/dev/zero\")==" << fd << std::endl;
    std::wifstream wost("/dev/null");
    fd = fileno(wost);
    if (fd != 6)
        std::cerr << "fileno(L\"/dev/null\")==" << fd << std::endl;
    std::wstringstream wsst;
    fd = fileno(wsst);
    if (fd != -1)
        std::cerr << "fileno(wstringstream)==" << fd << std::endl;
    std::cout << "cout is" << (isatty( std::cout )?"":" not") << " a tty" << std::endl;
    std::cerr << "cerr is" << (isatty( std::cerr )?"":" not") << " a tty" << std::endl;
    std::cout << "cin is" << (isatty( std::cin )?"":" not") << " a tty" << std::endl;
   {  auto[c,l]=size_w_h( std::cout );     std::cout << "cout is " << c << "x" << l << std::endl; }
    { auto[c,l]=size_w_h( std::cerr );     std::cout << "cout is " << c << "x" << l << std::endl; }
    { auto[c,l]=size_w_h( std::cin );    std::cout<< "cin is " << c << "x" << l << std::endl; }
}
#endif
