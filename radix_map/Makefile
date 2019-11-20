#CC = g++
CC = clang++-7
SHELL = /bin/bash
CXXFLAGS = -I ../shared_memory_allocator $(CPPFLAGS)

NETNEWS = /usr/local/lib/wordlist/netnews/netnews.zip
CANON = /usr/local/lib/wordlist/canon
GOOCH = /usr/local/lib/wordlist/bigdatt77.txt
WORDS_DB = /usr/local/lib/wordlist/words.db

exer: exer.cpp
	$(CC) $(CXXFLAGS) -I ../common -MMD exer.cpp -o exer

exer_64: exer.cpp
	$(CC) $(CXXFLAGS) -D BIT_64 -I ../common -MMD exer.cpp -o exer_64

-include exer.d

dict.rad: exer dict
	exer -ldict -q
	
canon.rad.gz: canon.rad
	gzip -c canon.rad > canon.rad.gz

canon.rad: exer canon
	exer -icanon -q

canon.keys: $(CANON) ../canonical/canonical
	../canonical/canonical -v $(CANON) > canon.keys

canon.sources: $(CANON) ../canonical/canonical
	fgrep -iwf $(CANON) $(WORDS_DB) > canon.sources

gooch.rad.gz: gooch.rad
	gzip -c gooch.rad > gooch.rad.gz

gooch.rad: exer_64 gooch
	exer_64 -igooch -q

gooch: $(GOOCH) ../canonical/canonical
	sed 's/ .*//' $(GOOCH) | ../canonical/canonical > gooch

SRC = \
Makefile \
dict \
exer.cpp \
radix_map.h \
shared_memory_allocator.h

radix_map.zip: $(SRC)
	zip radix_map.zip $(SRC)
