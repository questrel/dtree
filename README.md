# dtree

## Installation Instructions

library is header-only
```c++
#define TEST_H "qtl/xxx.h"
#include TEST_H
```
will generate a test exercizer for xxx library

```bash
make  # make all test.xxx.out
```

```bash
make CXX='clang++ -DDEBUG'  # with diagnostic trace prints
```

```bash
make xxx.fuzz.out  # afl-fuzz testing
AFL_NO_ARITH=1 AFL_EXIT_WHEN_DONE=1 /usr/local/bin/afl/afl-fuzz  -i- -x fuzz/xxx.dict -o fuzz/xxx.out -- ./xxx.fuzz.out
# may run for several weeks
```