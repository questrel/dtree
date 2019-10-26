CXX=g++
CXX=clang++ 

CXXFLAGS= -v -Xlinker -L/usr/local/opt/llvm/lib -fno-omit-frame-pointer -fno-optimize-sibling-calls -I ~/boost_1_70_0 
#CXXFLAGS += -fsave-optimization-record -fprofile-instr-generate -fcoverage-mapping -ftest-coverage -fprofile-arcs
# -fsanitize-memory-track-origins 
# -fno-sanitize-address-use-after-scope
g++_includepath=/usr/local/include/c++/8.0.0:/usr/local/opt
g++-8_includepath=$(CPLUS_INCLUDE_PATH)
clang++_includepath:=$(CPLUS_INCLUDE_PATH)
clang++_flags= -glldb --std=c++2a -fdebug-macro -fstandalone-debug -fsave-optimization-record -fprofile-instr-generate -fcoverage-mapping -fdiagnostics-show-template-tree
g++_flags=--std=c++17
g++-8_flags=--std=c++17
CXXFLAGS+=$($(firstword $(CXX))_flags)
#CPLUS_INCLUDE_PATH=$($(firstword $(CXX))_includepath)
#ifeq ($(CXX),clang++)
#CXXFLAGS=--std=c++17 -include /usr/local/Cellar/llvm/6.0.0/lib/clang/6.0.0/include/emmintrin.h  -Xlinker -v -Xlinker -L/usr/local/opt/llvm/lib 

#endif
#ifeq ($(CXX),g++)
#CPLUS_INCLUDE_PATH=/usr/local/include/c++/8.0.0
#endif
#CXXFLAGS=--std=c++17 -include /usr/local/Cellar/llvm/6.0.0/lib/clang/6.0.0/include/emmintrin.h  -Xlinker -v -Xlinker -L/usr/local/opt/llvm/lib 
#CXXFLAGS += -lboost_regex
ERROR_LEVEL=-1
FXX=/usr/local/bin/afl/afl-clang++
FUZZ=/usr/local/bin/afl/afl-fuzz
#FFLAGS=-d
FENV=AFL_NO_ARITH=1 AFL_EXIT_WHEN_DONE=1 AFL_HANG_TMOUT=100
FDIR=fuzz
SPLIT=perl -pe 'BEGIN{$$a="aa"}open STDOUT,">$$ARGV[0]".$$a++;END{}' - 

#printenv:
#	printenv ; echo CXX=$(CXX); echo CXXFLAGS=$(CXXFLAGS); echo CPLUS_INCLUDE_PATH=$(CPLUS_INCLUDE_PATH) ; 
#	echo $(CXX)_includepath $($(CXX)_includepath)
#	echo $(CXX)_flags $($(CXX)_flags)

.SECONDARY:

.SUFFIXES:

%.test.log: %.test.out $(FDIR)/%.in/aa
	perl -MList::Util=shuffle -e 'print shuffle <>' $(FDIR)/$(*F).in/* | tee $(*F).test.in | ./$(*F).test.out 2>&1 | tee  $@

all: out.test.out string.test.out container.test.out number.test.out interval.test.out expr.test.out sql.test.out store.test.out
	
tests: out.test.log string.test.log container.test.log number.test.log interval.test.log expr.test.log sql.test.log store.test.log

interval.test.out: qtl/number.h qtl/container.h

store.test.out: qtl/number.h qtl/tree.h qtl/randstream.h qtl/interval.h qtl/container.h

sql.test.out: qtl/tree.h qtl/expr.h qtl/number.h qtl/interval.h qtl/store.h qtl/container.h

expr.test.out: qtl/tree.h qtl/interval.h qtl/number.h

canonical.out: canonical.cpp

%.out: %.cpp
	$(CXX) $(CXXFLAGS) -v -o $@ -g3 $<

%.test.out: qtl/%.h qtl/out.h qtl/string.h
	( $(CXX) $(CXXFLAGS) -v -o $@ -g3  -DTEST_H='"'$<'"' -fsanitize=address -fdiagnostics-color=always test.cpp 2>&1 && cp -av $? compiles  ) | tee ./$(*F).make.$(subst /,_,$(firstword ${CXX})).log

%.test.out: qtl/%.hpp
	$(CXX) -o $@ $(CXXFLAGS) -DTEST_H='"'$<'"'  -fsanitize=address test.cpp  

%.error.log: %.error.out
	 ./$(*F).error.out | tee  $@

%.error.out: qtl/%.h
	$(CXX) $(CXXFLAGS) -o $@  -DERROR_INJECT=$(ERROR_LEVEL) -fsanitize=address  -fdiagnostics-color=always -DTEST_H='"'$<'"' test.cpp  2>&1 | tee ./$(*F).error.make.${CXX}.log

%.fuzz: %.fuzz.out $(FDIR)/%.cmin $(FDIR)/%.dict
	$(FENV) $(FUZZ) $(FFLAGS) -i $(FDIR)/$(*F).cmin -x $(FDIR)/$(*F).dict -o $(FDIR)/$(*F).out -- ./$(*F).fuzz.out

%.fuzz.log: %.fuzz.out $(FDIR)/%.in 
	$(FUZZ) -i $(FDIR)/$(*F).in -o $(FDIR)/$(*F).out -- ./$(*F).fuzz.out 2>&1 | tee  $@

$(FDIR)/%.cmin: 
	mkdir -pv $(FDIR)/$(*F).cmin/ && /usr/local/bin/afl/afl-cmin -i $(FDIR)/$(*F).in/  -o  $(FDIR)/$(*F).cmin/ -- ./$(*F).fuzz.out

$(FDIR)/%.in/aa: %.test.out
	mkdir -pv $(FDIR)/$(*F).in/ && ./$(*F).test.out fuzz | $(SPLIT) $(FDIR)/$(*F).in/

$(FDIR)/%.dict: %.fuzz.out
	./$(*F).fuzz.out dict > $(FDIR)/$(*F).dict; 

$(FDIR)/%.in: %.fuzz.out
	mkdir -pv $(FDIR)/$(*F).in/ ;  ./$(*F).fuzz.out fuzz | $(SPLIT) $(FDIR)/$(*F).in/

%.fuzz.out: qtl/%.h qtl/out.h qtl/string.h
#	$(FXX) -DFUZZING -D'TRACE(x)=' -o $@ -g3  $(CXXFLAGS)  -DTEST_H='"'$<'"' -fsanitize=address  -fdiagnostics-color=always test.cpp 2>&1 | tee ./$(*F).fuzz.make.log
	AFL_HARDEN=1 $(FXX) -DFUZZING -D'TRACE(x)=' -o $@ -g3  $(CXXFLAGS)  -DTEST_H='"'$<'"' -fdiagnostics-color=always test.cpp 2>&1 | tee ./$(*F).fuzz.make.log


errors: out.error

test: cleanlog tests

%.debug: qtl/%.h
	 $(CXX) -glldb $(CXXFLAGS) -D__TEST='"'$<'"' test.cpp && gdb a.out

#all: qtl/*.h
#	bash -cvx 'for f in qtl/*.hpp ; do  g++ --std=c++17 -D__TEST='"'"'"'"'"'$$f'"'"'"'"'"' test.cpp && ./a.out ; done'

cleanlog:
	rm -f *.{test,error}.log

clean: cleanlog
	rm -f *.out
