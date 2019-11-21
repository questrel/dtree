#include <algorithm>
#include <cassert>
#include <cstring>
#include <fstream>
#include "ioerror.h" // for Throw
#include <iostream>
#include <map>
#include <memory> // for std::shared_ptr
#include "radix_map.h" // for questrel::radix_map
#include <time.h>
#include <unistd.h> // for unlink()
#include <vector>

using namespace std;
using namespace questrel;

typedef size_t pos_type;
typedef radix_map < pos_type > Radix_map;
typedef Radix_map::iterator Xiter;
typedef pair < Xiter, Xiter > Xpair;

class key_less {
public:
  bool operator()(string const &x, string const &y) const { return character_set_t(dictorder).key_less(x.c_str(), y.c_str()); }
  bool operator()(shared_ptr<string> const &x, shared_ptr<string> const &y) const { return character_set_t(dictorder).key_less(x->c_str(), y->c_str()); }
};

typedef multimap < shared_ptr<string>, pos_type, key_less,
	shared_memory_allocator<multimap<shared_ptr<string>, pos_type>::value_type> > Map;
typedef Map::iterator Miter;

typedef vector<Map::key_type> Vector;
typedef Vector::iterator Viter;

template <typename T>
struct deleter_t { void operator()(T *p) { delete p; } };

char usage[] =
    " [-c(lear) file]"
    " [-d(ump)]"
    " [-k(ey dump)]"
    " [-i(nput) file]"
    " [-l(oad) file]"
    " [-o(pen) file]"
    " [-p(rint)]"
    " [-q(uit)]"
    " [-s(earch) string]"
    " [-t(est) file]"
    " [-v(erbose)]";

short verbose = 0;

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
      string line;
      getline(fin, line);
      os << line;
    }
    os << endl;
  }
};

inline double milliseconds(clock_t t) {
  return double (t) * 1000 / CLOCKS_PER_SEC;
}

string get_file_name(const string & file_name, const char *suffix) {
  string::size_type i = file_name.find_last_of('/');
  i = (i == string::npos) ? 0 : (i + 1);
  string::size_type j = file_name.find_last_of('.');
  j = (j == string::npos) ? (file_name.length() - i) : (j - i);
  return file_name.substr(i, j) + suffix;
}

Radix_map x;

string file_name = "dict";

void run(char cmd, const char *arg, istream &in = cin) {
  switch (cmd) {
  case 'c':
    {
      if (arg && *arg)
        file_name = arg;
      unlink(get_file_name(file_name, ".map").c_str());
      unlink(get_file_name(file_name, ".rad").c_str());
      break;
    }
  case 'd':
    cout << x;
    break;
  case 'k':
    for_each(x.begin(), x.end(), print_key(cout));
    break;
  case 'i': // insert lines as records with data followed by TAB separated list of keys
    {
      if (arg && *arg)
        file_name = arg;
      ifstream fin(file_name.c_str());
      if (!fin)
        cerr << "unable to read from file: " << file_name << Error << Exit;
      x.open(get_file_name(file_name, ".rad").c_str(), create);
      unsigned long count_records = 0, count_keys = 0;
      clock_t start = clock();
      string line;
      while (getline(fin, line)) {
        vector<Radix_map::key_type> keys;
        for (char *state, *key = strtok_r(const_cast<char *>(line.c_str()), "\t", &state); (key = strtok_r(NULL, "\t", &state)); )
          keys.push_back(key);
        if (keys.size()) {
          offset record_offset = x.add_record(line.c_str(), line.length(), 1, timestamp()); // fixed record id (one level of indirection)
          ++count_records;
          x.update_keys(record_offset, keys);
          count_keys += keys.size();
        }
      }
      double d = milliseconds(clock() - start);
      //assert(count_keys == x.size());
      cout << "x.size()=" << x.size() << endl;
      cout << "added " << count_records << " records and " << count_keys << " keys in " << d << " milliseconds" << endl;
    }
    break;
  case 'l':
    {
      if (arg && *arg)
        file_name = arg;
      ifstream fin(file_name.c_str());
      if (!fin)
        cerr << "unable to read from file: " << file_name << Error << Exit;
      x.open(get_file_name(file_name, ".rad").c_str(), create, dictorder);
      unsigned long c = 0;
      clock_t start = clock();
      Xiter hintpos;
      string line;
      for (pos_type p; (p = fin.tellg()), getline(fin, line);)
        //x[line.c_str()] = p; // fast and dumb
        //if (x.insert(make_pair(line.c_str(), p)).second) ++c; // slow
        //hintpos = x.insert(hintpos, make_pair(line.c_str(), p)), ++c; // medium
        if (x.insert(hintpos, line.c_str(), p)) // fast
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
    x.open(get_file_name(file_name, ".rad").c_str(), read_write);
    cout << "size = " << x.size() << endl;
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
        if (x.use_records()) {
	  try {
	    cout << x.get_record(p.first->second).c_str() << endl;
	  } catch (exception &ex) {
	    cout << "get_record failed on record id " << p.first->second << ": " << ex.what() << endl;
	  }
	} else {
	  if (!fin.seekg(p.first->second))
	    cout << " [error seeking to " << p.first->second << "]" << endl;
	  else {
	    string line;
	    getline(fin, line);
	    cout << " " << line;
	  }
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
      clock_t start;
      double d;
      start = clock();
      Map *m;
      Miter im;
      string map_file_name(get_file_name(file_name, ".map"));
      const char *map_file_name_c_str = map_file_name.c_str();
      if (access(map_file_name_c_str, F_OK) == 0) { // file already exists
	shared_memory_allocator<Map> a(map_file_name_c_str);
	m = a.get_root();
      } else { // file does not exist
	shared_memory_allocator<Map> a(map_file_name_c_str);
	deleter_t<string> d;
	Map *root = reinterpret_cast<Map*>(a.allocate(sizeof(Map)));
	a.set_root(root);
	m = new (root) Map(a);
	ifstream fin(file_name.c_str());
	string line;
	im = m->begin();
	for (pos_type pos; (pos = fin.tellg()), getline(fin, line); ) {
	  string *p = new (a.allocate(sizeof(string))) string(a);
	  *p = line;
	  im = m->insert(im, make_pair(shared_ptr<string>(p, d, a), pos));
	}
      }
      d = milliseconds(clock() - start);
      cout << "std::map: loaded " << m->size() << " in " << d << " milliseconds" << endl;
      start = clock();
      for (im = m->begin(); im != m->end(); ++im);
      d = milliseconds(clock() - start);
      cout << "std::map: traverse " << m->size() << " in " << d << " milliseconds" << endl;
      /*
      Radix_map x(get_file_name(file_name, ".rad").c_str(), read_only);
      Xiter ix;
      start = clock();
      for (ix = x.begin(); ix != x.end(); ++ix);
      d = milliseconds(clock() - start);
      cout << "radix_map: traverse " << x.size() << " in " << d << " milliseconds" << endl;
      */
      srand(0);
      Vector v;
      for (im = m->begin()/*, ix = x.begin()*/; im != m->end(); ++im/*, ++ix*/) {
	/*
        if (!character_set_t(dictorder).key_equal(im->first->c_str(), ix->first)
            || m->count(im->first) != x.count(ix->first)) {
          cout <<
              "key or count mismatch at" <<
              " std::map = " << im->first->c_str() << ',' << im->second <<
              " count = " << m->count(im->first) <<
              " radix_map = " << ix->first << ',' << ix->second <<
              " count = " << x.count(ix->first) <<
              endl;
          exit(-1);
        }
	*/
        if ((rand() & 63) == 0)
          v.push_back(im->first);
      }
      int n = 0;
      start = clock();
      for (Viter i = v.begin(); i != v.end(); ++i) {
        int c = m->count(*i);
        if (c)
          n += c;
        else {
          cout << "std::map: cannot count " << *i << endl;
          exit(-1);
        }
      }
      d = milliseconds(clock() - start);
      cout << "std::map: counted " << n << " in " << d << " milliseconds" << endl;
      /*
      n = 0;
      start = clock();
      for (Viter i = v.begin(); i != v.end(); ++i) {
        int c = x.count((*i)->c_str());
        if (c)
          n += c;
        else {
          cout << "radix_map: cannot count " << *i << endl;
          exit(-1);
        }
      }
      d = milliseconds(clock() - start);
      cout << "radix_map: counted " << n << " in " << d << " milliseconds" << endl;
      */
      start = clock();
      for (Viter i = v.begin(); i != v.end(); ++i)
        if (m->find(*i) == m->end()) {
          cout << "std::map: cannot find " << *i << endl;
          exit(-1);
        }
      d = milliseconds(clock() - start);
      cout << "std::map: found " << v.size() << " in " << d << " milliseconds" << endl;
      /*
      start = clock();
      for (Viter i = v.begin(); i != v.end(); ++i)
        if (x.find((*i)->c_str()) == x.end()) {
          cout << "radix_map: cannot find " << *i << endl;
          exit(-1);
        }
      d = milliseconds(clock() - start);
      cout << "radix_map: found " << v.size() << " in " << d << " milliseconds" << endl;
      */
      n = 0;
      start = clock();
      for (Viter i = v.begin(); i != v.end(); ++i) {
        int c = m->count(*i);
        if (c)
          n += c;
        else {
          cout << "std::map: cannot count " << *i << endl;
          exit(-1);
        }
      }
      d = milliseconds(clock() - start);
      cout << "std::map: counted " << n << " in " << d << " milliseconds" << endl;
      /*
      n = 0;
      start = clock();
      for (Viter i = v.begin(); i != v.end(); ++i) {
        int c = x.count((*i)->c_str());
        if (c)
          n += c;
        else {
          cout << "radix_map: cannot count " << *i << endl;
          exit(-1);
        }
      }
      d = milliseconds(clock() - start);
      cout << "radix_map: counted " << n << " in " << d << " milliseconds" << endl;
      */
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
  for (char c; (c = getopt(argc, argv, ":c:dki:l:o:pqs:t:v")) != -1;)
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
