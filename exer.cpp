#include <algorithm>
#include <cassert>
#include <cstring>
#include <fstream>
#include "ioerror.h"
#include <iostream>
#include <map>
#include "radix_map.h"
#include <time.h>
#include <vector>

using namespace std;
using namespace questrel;

typedef size_t pos_type;
typedef radix_map < pos_type > Radix_map;
typedef Radix_map::iterator Xiter;
typedef pair < Xiter, Xiter > Xpair;

class key_less {
public:
  bool operator()(char *const &x, char *const &y) const { return character_set_t(dictorder).key_less(x, y); }
};

typedef multimap < char *, pos_type, key_less > Map;
typedef Map::iterator Miter;
typedef pair < Miter, Miter > Mpair;

typedef vector < char *>Vector;
typedef Vector::iterator Viter;

char
 usage[] =
    " [-d(ump)]"
    " [-k(ey dump)]"
    " [-i(nput) file]"
    " [-l(oad) file]"
    " [-o(pen) file]"
    " [-p(rint)]"
    " [-q(uit)]"
    " [-s(earch) string]"
    " [-t(est)]"
    " [-v(erbose)]"
    " [file]";

short
 verbose = 0;

class print_key {
  ostream & os;
public:
  print_key(ostream & os_):os(os_) {}
  void operator () (const Radix_map::value_type & v) {
    os << v.first << endl;
  }
};

class print_key_value {
  ostream & os;
  ifstream & fin;
public:
  print_key_value(ostream & os_, ifstream & fin_) : os(os_), fin(fin_) {}
  void operator () (const Radix_map::value_type & v) {
    os << v.first << '\t';
    if (!fin.seekg(v.second))
      os << "[error seeking to " << v.second << "]";
    else {
      char buf[256];
      fin.getline(buf, sizeof(buf));
      os << buf;
    }
    os << endl;
  }
};

inline double milliseconds(clock_t t) {
  return double (t) * 1000 / CLOCKS_PER_SEC;
}

string get_radix_map_name(const string & file_name) {
  string::size_type i = file_name.find_last_of('/');
  i = (i == string::npos) ? 0 : (i + 1);
  string::size_type j = file_name.find_last_of('.');
  j = (j == string::npos) ? (file_name.length() - i) : (j - i);
  return file_name.substr(i, j) + ".rad";
}

Radix_map x;

string file_name = "dict";

void run(char cmd, const char *arg, istream &in = cin) {
  switch (cmd) {
  case 'd':
    cout << x;
    break;
  case 'k':
    for_each(x.begin(), x.end(), print_key(cout));
    break;
  case 'i': // insert lines as records with single key, which is line up to TAB character
    {
      if (arg && *arg)
        file_name = arg;
      x.open(get_radix_map_name(file_name).c_str(), create, dictorder);
      unsigned long c = 0;
      clock_t start = clock();
      Xiter hintpos;
      char buf[256];
      while (in.getline(buf, sizeof(buf))) {
	char *separator = strchr(buf, '\t');
	if (separator)  {
	  offset record_offset = x.add_record(buf, strlen(buf), 1, timestamp()); // fixed record id (one level of indirection)
	  *separator = 0;
	  x.update_keys(record_offset, vector<Radix_map::key_type>(1, Radix_map::key_type(buf)));
	  ++c;
	}
      }
      double d = milliseconds(clock() - start);
      if (c)
        assert(c == x.size());
      cout << "added " << x.size() << " records in " << d << " milliseconds" << endl;
    }
    break;
  case 'l':
    {
      if (arg && *arg)
        file_name = arg;
      ifstream fin(file_name.c_str());
      if (!fin)
        cerr << "unable to read from file: " << file_name << Error << Exit;

      x.open(get_radix_map_name(file_name).c_str(), create, dictorder);
      unsigned long c = 0;
      clock_t start = clock();
      Xiter hintpos;
      char buf[256];
      for (pos_type p; (p = fin.tellg()), fin.getline(buf, sizeof(buf));)
        //x[buf] = p; // fast and dumb
        //if (x.insert(make_pair(buf, p)).second) ++c; // slow
        //hintpos = x.insert(hintpos, make_pair(buf, p)), ++c; // medium
        if (x.insert(hintpos, buf, p)) // fast
          ++c;
      double d = milliseconds(clock() - start);
      if (c)
        assert(c == x.size());
      cout << "added " << x.size() << " in " << d << " milliseconds" << endl;
    }
    break;
  case 'o':
    if (arg && *arg)
      file_name = arg;
    x.open(get_radix_map_name(file_name).c_str(), read_write);
    cout << "size = " << x.size() << endl;
    cout << "free space = " << x.free_space() << endl;
    break;
  case 'p':
    {
      if (arg && *arg)
        file_name = arg;
      ifstream fin(file_name.c_str());
      for_each(x.begin(), x.end(), print_key_value(cout, fin));
    }
    break;
  case 'q':
    exit(0);
  case 's':
    {
      string query = arg;
      Xpair p;
      clock_t t = 0;
      unsigned long c = 0;
      while (t < 1000 && ++c < 1000000) {
        if (query == "l") {
          clock_t start = clock();
          p = x.longest();
          t += clock() - start;
        } else {
          clock_t start = clock();
          p = x.equal_range(query.c_str());
          t += clock() - start;
        }
      }
      double d = milliseconds(t) / c;
      if (p.first == p.second)
        cout << "not found: " << query << " in " << d << " milliseconds" << endl;
      cout << "found: " << p.first->first;
      if (p.first.count() > 1)
        cout << " (count = " << p.first.count() << ")";
      cout << " in " << d << " milliseconds:";
      ifstream fin(file_name.c_str());
      if (!fin)
        cerr << "unable to read from file: " << file_name << Error << Exit;
      while (p.first != p.second) {
        if (!fin.seekg(p.first->second))
          cout << " [error seeking to " << p.first->second << "]";
        else {
          char buf[256];
          fin.getline(buf, sizeof(buf));
          cout << " " << buf;
        }
        ++p.first;
      }
      cout << endl;
    }
    break;
  case 't':
    {
      if (arg && *arg)
        file_name = arg;
      Map m;
      clock_t start;
      double d;
      start = clock();
      Miter im = m.begin();
      char buf[256];
      ifstream fin(file_name.c_str());
      for (pos_type p; (p = fin.tellg()), fin.getline(buf, sizeof(buf)); )
        im = m.insert(im, make_pair(strdup(buf), p));
      d = milliseconds(clock() - start);
      cout << "map: loaded " << m.size() << " in " << d << " milliseconds" << endl;
      start = clock();
      for (im = m.begin(); im != m.end(); ++im);
      d = milliseconds(clock() - start);
      cout << "map: traverse " << m.size() << " in " << d << " milliseconds" << endl;
      Radix_map x(get_radix_map_name(file_name).c_str(), read_only);
      Xiter ix;
      start = clock();
      for (ix = x.begin(); ix != x.end(); ++ix);
      d = milliseconds(clock() - start);
      cout << "radix_map: traverse " << x.size() << " in " << d << " milliseconds" << endl;
      srand(0);
      Vector v;
      for (im = m.begin(), ix = x.begin(); im != m.end(); ++im, ++ix) {
        if (!character_set_t(dictorder).key_equal(im->first, ix->first)
            || m.count(im->first) != x.count(ix->first)) {
          cout <<
              "key or count mismatch at" <<
              " map = " << im->first << ',' << im->second <<
              " count = " << m.count(im->first) <<
              " radix_map = " << ix->first << ',' << ix->second <<
              " count = " << x.count(ix->first) <<
              endl;
          exit(-1);
        }
        if ((rand() & 63) == 0)
          v.push_back(im->first);
      }
      int n = 0;
      start = clock();
      for (Viter i = v.begin(); i != v.end(); ++i) {
        int c = m.count(*i);
        if (c)
          n += c;
        else {
          cout << "map: cannot count " << *i << endl;
          exit(-1);
        }
      }
      d = milliseconds(clock() - start);
      cout << "map: counted " << n << " in " << d << " milliseconds" << endl;
      n = 0;
      start = clock();
      for (Viter i = v.begin(); i != v.end(); ++i) {
        int c = x.count(*i);
        if (c)
          n += c;
        else {
          cout << "radix_map: cannot count " << *i << endl;
          exit(-1);
        }
      }
      d = milliseconds(clock() - start);
      cout << "radix_map: counted " << n << " in " << d << " milliseconds" << endl;
      start = clock();
      for (Viter i = v.begin(); i != v.end(); ++i)
        if (m.find(*i) == m.end()) {
          cout << "map: cannot find " << *i << endl;
          exit(-1);
        }
      d = milliseconds(clock() - start);
      cout << "map: found " << v.size() << " in " << d << " milliseconds" << endl;
      start = clock();
      for (Viter i = v.begin(); i != v.end(); ++i)
        if (x.find(*i) == x.end()) {
          cout << "radix_map: cannot find " << *i << endl;
          exit(-1);
        }
      d = milliseconds(clock() - start);
      cout << "radix_map: found " << v.size() << " in " << d << " milliseconds" << endl;
      n = 0;
      start = clock();
      for (Viter i = v.begin(); i != v.end(); ++i) {
        int c = m.count(*i);
        if (c)
          n += c;
        else {
          cout << "map: cannot count " << *i << endl;
          exit(-1);
        }
      }
      d = milliseconds(clock() - start);
      cout << "map: counted " << n << " in " << d << " milliseconds" << endl;
      n = 0;
      start = clock();
      for (Viter i = v.begin(); i != v.end(); ++i) {
        int c = x.count(*i);
        if (c)
          n += c;
        else {
          cout << "radix_map: cannot count " << *i << endl;
          exit(-1);
        }
      }
      d = milliseconds(clock() - start);
      cout << "radix_map: counted " << n << " in " << d << " milliseconds" << endl;
      cout << "test passed" << endl;
    }
    break;
  case 'v':
    ++verbose;
    break;
  }
}

void prompt() {
  cout << usage << ": ";
}

int main(int argc, char *argv[])
{

  for (char c; (c = getopt(argc, argv, ":dkl:o:pqs:tv")) != -1;)
    switch (c) {
    case ':':
      cerr << "option -" << (char) optopt << " requires an operand" << endl;
      // fall through
    case '?':
      cerr << "usage: " << argv[0] << usage << endl;
      exit(2);
    default:
      run(c, optarg);
      break;
    }
  istream & in((optind < argc) ? *(new fstream(argv[optind++])) : cin);
  if (!in) {
    cerr << "unable to read from file: " << argv[--optind] << endl;
    exit(1);
  }
  if (optind != argc) {
    cerr << "usage: " << argv[0] << usage << endl;
    exit(2);
  }
  for (string line; prompt(), getline(in, line);)
    run(line[0] == '-' ? line[1] : line[0], line.substr(line[0] == '-' ? 2 : 1).c_str(), in);
}
