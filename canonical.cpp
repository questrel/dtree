#include <algorithm> // for reverse<>()
#include <cctype>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <unistd.h>

using namespace std;

char usage[] =
" [-C(SV)]"
" [-S(QL)]"
" [-v(erbose)] (output column names before outputting column values)"
" [file]";

bool CSV = false;
bool SQL = false;
short verbose = 0;

struct element_t {
  char *word;
  char *work;
};

////////////////////////////////////////////////////////////////////////////////
//                                                                            //
//                            word transformations                            //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

bool vowel_table[256];
bool xvowel_table[256];

void init_tables() {
  vowel_table['a'] = vowel_table['e'] = vowel_table['i'] = vowel_table['o'] = vowel_table['u'] = true;
  vowel_table['A'] = vowel_table['E'] = vowel_table['I'] = vowel_table['O'] = vowel_table['U'] = true;
  xvowel_table['a'] = xvowel_table['e'] = xvowel_table['i'] = xvowel_table['o'] = xvowel_table['u'] = xvowel_table['y'] = true;
  xvowel_table['A'] = xvowel_table['E'] = xvowel_table['I'] = xvowel_table['O'] = xvowel_table['U'] = xvowel_table['Y'] = true;
}

// return true if letter is a vowel
bool isvowel(unsigned char c) {
  return vowel_table[c];
}

// return true if letter is an extended vowel
bool isxvowel(unsigned char c) {
  return xvowel_table[c];
}

// subroutine for comparing letters
static int compare_letters(const void *l1, const void *l2) {
  return *(char *) l1 - *(char *) l2;
}

// transform word to ASCII canonical form
// returns pointer to terminal NUL in a->work
char *make_ASCII_word(element_t *a) {
  char c, *p = a->word, *q = a->work;
  while (c = *p++)
    if (isascii(c))
      *q++ = c;
  *q = 0;
  return q;
}

// transform word to pure lowercase letters
// returns pointer to terminal NUL in a->work
char *make_dictionary_word(element_t *a) {
  char c, *p = a->word, *q = a->work;
  while (c = *p++)
    if (isascii(c) && isalpha(c))
      *q++ = tolower(c);
  *q = 0;
  return q;
}

// transform word to suffixial canonical form
// returns pointer to terminal NUL in a->work
char *make_suffixial(element_t *a) {
  char *q = make_dictionary_word(a);
  reverse<char *>(a->work, q);
  return q;
}

// transform word to transposal canonical form
// returns pointer to terminal NUL in a->work
char *make_transposal(element_t *a) {
  char *q = make_dictionary_word(a);
  qsort(a->work, q - a->work, 1, compare_letters);
  return q;
}

// transform word to consonant list canonical form
// returns pointer to terminal NUL in a->work
char *make_consonant_list(element_t *a) {
  char c, *p = a->word, *q = a->work;
  while (c = *p++)
    if (isascii(c) && isalpha(c) && !isxvowel(c))
      *q++ = tolower(c);
  *q = 0;
  qsort(a->work, q - a->work, 1, compare_letters);
  return q;
}

// transform word to vowel list canonical form
// returns pointer to terminal NUL in a->work
char *make_vowel_list(element_t *a) {
  char c, *p = a->word, *q = a->work;
  while (c = *p++)
    if (isascii(c) && isalpha(c) && isxvowel(c))
      *q++ = tolower(c);
  *q = 0;
  qsort(a->work, q - a->work, 1, compare_letters);
  return q;
}

// transform word to isomorph canonical form
// returns pointer to terminal NUL in a->work
char *make_isomorph(element_t *a) {
  char l = 'a';
  char t[128];
  memset(t, 0, sizeof(t));
  char c, *p = a->word, *q = a->work;
  while (c = *p++)
    if (isascii(c) && isalpha(c)) {
      c = tolower(c);
      if (!t[c])
        t[c] = l++;
      *q++ = t[c];
    }
  *q = 0;
  return q;
}

// transform word to letter bank canonical form
// returns pointer to terminal NUL in a->work
char *make_letter_bank(element_t *a) {
  char t[128];
  memset(t, 0, sizeof(t));
  char c, *p = a->word, *q = a->work;
  while (c = *p++)
    if (isascii(c) && isalpha(c)) {
      c = tolower(c);
      if (!t[c]) {
        t[c] = 1;
        *q++ = c;
      }
    }
  *q = 0;
  qsort(a->work, q - a->work, 1, compare_letters);
  return q;
}

// transform word to consonant pattern canonical form
// returns pointer to terminal NUL in a->work
char *make_consonancy(element_t *a) {
  char c, *p = a->word, *q = a->work;
  while (c = *p++)
    if (isascii(c) && isalpha(c) && !isxvowel(c))
      *q++ = tolower(c);
  *q = 0;
  return q;
}

// transform word to vowel pattern canonical form
// returns pointer to terminal NUL in a->work
char *make_vowel_pattern(element_t *a) {
  char c, *p = a->word, *q = a->work;
  while (c = *p++)
    if (isascii(c) && isalpha(c))
      *q++ = isxvowel(c) ? 'v' : 'c';
  *q = 0;
  return q;
}

// transform word to ending consonant string canonical form
// returns pointer to terminal NUL in a->work
char *make_ending_consonant_string(element_t *a) {
  char *last_vowel = a->word;
  for (char *p = last_vowel; *p; ++p)
    if (isascii(*p) && isvowel(*p))
      last_vowel = p;
  char *q = a->work;
  for (char *p = last_vowel + 1; *p; ++p)
    if (isascii(*p) && isalpha(*p))
      *q++ = tolower(*p);
  *q = 0;
  return q;
}

// transform word to letter position sum
// returns pointer a->work
char *make_letter_sum(element_t *a) {
  long sum = 0;

  for (char *p = a->word; *p; ++p)
    if (isascii(*p) && isalpha(*p))
      sum += tolower(*p) - 'a' + 1;
  sprintf(a->work, "%ld", sum);
  return a->work;
}

// transform word to letter position product
// returns pointer a->work
char *make_letter_product(element_t *a) {
  unsigned long long product = 1;

  for (char *p = a->word; *p; ++p)
    if (isascii(*p) && isalpha(*p))
      product *= tolower(*p) - 'a' + 1;
  sprintf(a->work, "%llu", product);
  return a->work;
}

const char *morse[] =
{
  ".-",				/* a */
  "-...",			/* b */
  "-.-.",			/* c */
  "-..",			/* d */
  ".",				/* e */
  "..-.",			/* f */
  "--.",			/* g */
  "....",			/* h */
  "..",				/* i */
  ".---",			/* j */
  "-.-",			/* k */
  ".-..",			/* l */
  "--",				/* m */
  "-.",				/* n */
  "---",			/* o */
  ".--.",			/* p */
  "--.-",			/* q */
  ".-.",			/* r */
  "...",			/* s */
  "-",				/* t */
  "..-",			/* u */
  "...-",			/* v */
  ".--",			/* w */
  "-..-",			/* x */
  "-.--",			/* y */
  "--..",			/* z */
/*  ".-.-",  a", ae ligature */
/*  ".--.-",  a', a`, a circumflex, a boll */
/*  "..-..",  e', e`, e circumflex */
/*  "---.",  o", o slash, oe ligature */
/*  "..--",  u" */
/*  "--.--",  n~ */
/*  "..--.", edh */
/*  ".--..", thorn */
/*  "----", ch */
};

// transform word to Morse code
// returns pointer to terminal NUL in a->work
char *make_morse_code(element_t *a) {
  char *q = a->work;
  for (char *p = a->word; *p; ++p)
    if (isascii(*p) && isalpha(*p)) {
      strcpy(q, morse[tolower(*p) - 'a']);
      q += strlen(q);
    }
  *q = 0; // in case there are no letters in word
  return q;
}

char phone[] = {
  '2',			/* a */
  '2',			/* b */
  '2',			/* c */
  '3',			/* d */
  '3',			/* e */
  '3',			/* f */
  '4',			/* g */
  '4',			/* h */
  '4',			/* i */
  '5',			/* j */
  '5',			/* k */
  '5',			/* l */
  '6',			/* m */
  '6',			/* n */
  '6',			/* o */
  '7',			/* p */
  '0',			/* q */
  '7',			/* r */
  '7',			/* s */
  '8',			/* t */
  '8',			/* u */
  '8',			/* v */
  '9',			/* w */
  '9',			/* x */
  '9',			/* y */
  '0',			/* z */
};

// transform word to telephone number
// NB. q and z are mapped to 0
// returns pointer to terminal NUL in a->work
char *make_phone(element_t *a) {
  char *q = a->work;
  for (char *p = a->word; *p; ++p)
    if (isascii(*p) && isalpha(*p))
      *q++ = phone[tolower(*p) - 'a'];
  *q = 0;
  return q;
}

// transform word to digital word
// each letter replaced by the digits of its position in the alphabet
// returns pointer to terminal NUL in a->work
char *make_digital_word(element_t *a) {
  char *q = a->work;
  for (char *p = a->word; *p; ++p)
    if (isascii(*p) && isalpha(*p)) {
      short d = tolower(*p) - 'a' + 1;
      if (d >= 20) { *q++ = '2'; d -= 20; }
      else if (d >= 10) { *q++ = '1'; d -= 10; }
      *q++ = d + '0';
    }
  *q = 0;
  return q;
}

// transform word to numbergram canonical form
// convert to digital word, then sort digits
// returns pointer to terminal NUL in a->work
char *make_numbergram(element_t *a) {
  char *retval = make_digital_word(a);
  qsort(a->work, retval - a->work, 1, compare_letters);
  return retval;
}

short sumword[] = {
    80,	// a
    17,	// b
    34,	// c
    34,	// d
    118,// e
    20,	// f
    20,	// g
    49,	// h
    72,	// i
    3, 	// j
    9, 	// k
    42,	// l
    28,	// m
    68,	// n
    78,	// o
    24,	// p
    1,	// q
    60,	// r
    66,	// s
    91,	// t
    31,	// u
    11,	// v
    19,	// w
    4,	// x
    20,	// y
    1	// z
};

// transform word to sumword
// equal to sum of letter frequencies
// returns a->work
char *make_sumword(element_t *a) {
  long sum = 0;

  for (char *p = a->word; *p; ++p)
    if (isascii(*p) && isalpha(*p))
      sum += sumword[tolower(*p) - 'a'];
  sprintf(a->work, "%ld", sum);
  return a->work;
}

// transform word to shiftword canonical form
// all letters shifted so that first letter is 'a'
// returns pointer to terminal NUL in a->work
char *make_shiftword(element_t *a) {
  char *q = a->work;
  short shift = 1; // will not be positive
  for (char c, *p = a->word; c = *p++; )
    if (isascii(c) && isalpha(c)) {
      c = tolower(c);
      if (shift == 1) // first letter
        shift = 'a' - c;
      c += shift;
      if (c < 'a') // letters less than first lettrer are shifted to end of alphabet
        c += 26; 
      *q++ = c;
    }
  *q = 0;
  return q;
}

struct lookup_t {
  const char *type;
  char *(*function)(element_t *);
  bool is_numeric = false;
} lookup[] = {
  {"ASCII_word", make_ASCII_word},
  {"consonancy", make_consonancy},
  {"consonant_list", make_consonant_list},
  {"dictionary_word", make_dictionary_word},
  {"digital_word", make_digital_word},
  {"ending_consonant_string", make_ending_consonant_string},
  {"isomorph", make_isomorph},
  {"letter_bank", make_letter_bank},
  {"letter_product", make_letter_product, true},
  {"letter_sum", make_letter_sum, true},
  {"morse_code", make_morse_code},
  {"numbergram", make_numbergram},
  {"phone", make_phone},
  {"shiftword", make_shiftword},
  {"suffixial", make_suffixial},
  {"sumword", make_sumword, true},
  {"transposal", make_transposal},
  {"vowel_list", make_vowel_list},
  {"vowel_pattern", make_vowel_pattern},
};

string escape(const char *s, const char *escape_chars) {
  string retval;
  for (const char *p = s; *p; ) {
    if (strchr(escape_chars, *p))
      retval += '\\';
    retval += *p++;
  }
  return retval;
}

int main(int argc, char *argv[]) {

  for (short c; (c = getopt(argc, argv, ":CSv")) != -1; )
    switch (c) {
    case 'C':
      CSV = true;
      break;
    case 'S':
      SQL = true;
      break;
    case 'v':
      ++verbose;
      break;
    case ':':
      cerr << "option -" << (char)optopt << " requires an operand" << endl;
      // fall through
    case '?':
      cerr << "usage: " << argv[0] << usage << endl;
      exit(2);
    }
  istream &in((optind < argc) ? *(new ifstream(argv[optind++])) : cin);
  if (!in) {
    cerr << "unable to read from file: " << argv[--optind] << endl;
    exit(1);
  }
  if (optind != argc) {
    cerr << "usage: " << argv[0] << usage << endl;
    exit(2);
  }
  init_tables();
  if (verbose) {
    const char *sep = "";
    if (SQL)
      cout << "create table word (";
    for (auto l : lookup) {
      if (CSV)
	cout << sep << "\"" << l.type << "\"";
      else if (SQL)
	cout << sep << l.type << (l.is_numeric ? " double" : " varchar(255)");
      else 
	cout << sep << l.type;
      sep = (CSV | SQL) ? "," : "\t";
    }
    if (SQL)
      cout << ")";
    cout << endl;
  }
  for (string line; getline(in, line); ) {

    const char *sep = "";
    if (SQL)
      cout << "insert into word (";
    for (auto l : lookup) {
      element_t a;
      a.word = const_cast<char *>(line.c_str());
      a.work = reinterpret_cast<char *>(malloc(line.length() * 10)); 
      l.function(&a);
      if (CSV) {
	if (l.is_numeric)
	  cout << sep << a.work;
	else
	  cout << sep << "\"" << escape(a.work, "\\,\"") << "\"";
      } else if (SQL) {
	if (l.is_numeric)
	  cout << sep << a.work;
	else
	  cout << sep << "'" << escape(a.work, "\\,'") << "'";
      } else
        cout << sep << a.work;
      free(a.work);
      sep = (CSV | SQL) ? "," : "\t";
    }
    if (SQL)
      cout << ")";
    cout << endl;
  }
}
