# Architectural Recommendations

*Note: The header-as-translation-unit test pattern (test code inside headers, compiled
only when the header is the top-level TU) is intentional and not discussed here.
Recommendations below are independent of that convention.*

---

## 1. Remove dead `lval` / `co_lval` code in `store.h`

**Priority: High — low risk, immediate payoff.**

**What exists:**
- `store::lval` (non-coroutine, ~line 2660): the original iterator-based query type.
  Uses `lattice::iterator` directly, no coroutines.
- `store::lval` (coroutine-based, ~line 2781): the active implementation. Uses
  `dedup_search` → `recursive_generator<row>`.
- `store::co_lval` (~line 3226): a second coroutine query type using member functions
  `co_search_vertex` / `co_search_leaf` that still contain `co_yield nested_gen`
  (now correct, but the whole class appears superseded).

Both the non-coroutine `lval` and `co_lval` appear to be dead — all active query paths
go through the coroutine `lval`. Their presence creates confusion about which path is
live and makes the file ~400 lines longer than necessary.

**Action:**
1. Confirm `co_lval` and the non-coroutine `lval` are unused (grep for construction
   sites and `using` aliases).
2. If confirmed dead, delete both. If either has tests that exercise it, migrate those
   tests to the coroutine `lval` first.
3. Also review `co_search_vertex` / `co_search_leaf` member functions inside `co_lval`:
   these duplicate the static `search_vertex` / `search_leaf` functions. Remove the
   member versions along with `co_lval`.

**Risk:** Low, provided the grep confirms no callers. The coroutine `lval` is the
implementation that passes all current tests.

---

## 2. Clean up unconditional `std::cerr` and `TRACE`/`NOTRACE` convention

**Priority: Medium — quality-of-life, catches real bugs (misdirected debug output).**

**Problems:**

(a) `cppcoro/recursive_generator.hpp` contains several always-on `std::cerr` calls
    that were left in from debugging sessions:
    - Line 142: `std::cerr << __PRETTY_FUNCTION__ << ":" << __LINE__ << "\n";` inside
      `is_complete()`
    - Line 155: same pattern inside `pull()`
    - Line 244: constructor trace in `iterator`
    - Lines 300, 307, 317: `begin()` and `end()` traces
    These fire on every query iteration, polluting stderr unconditionally.

(b) There are a few raw `std::cerr` statements in `store.h` outside any macro
    (some wrapped in `/*TRACE*/` comments, some not), left from debugging sessions.

Note: `NOTRACE(x)` is intentional and correct — it is a per-statement "off switch"
that permanently silences one noisy trace call without deleting it. `TRACE(x)` →
`NOTRACE(x)` suppresses that specific output; reversing it re-enables it. This is
by design and should not be renamed or removed.

**Action:**
1. In `cppcoro/recursive_generator.hpp`: wrap the unconditional `std::cerr` calls
   (lines ~142, 155, 244, 300, 307, 317) in `TRACE()` or delete them. Since `cppcoro/`
   is third-party-derived, prefer `#ifdef CPPCORO_TRACE ... #endif`.
2. Audit `store.h` for bare `std::cerr` outside any macro; wrap in `TRACE()` or
   delete.

**Risk:** Low. Purely cosmetic — no logic changes.

---

## 3. Add `qtl/generator.h` — a single include point for the generator type

**Priority: Medium — needed before any further coroutine work or C++ standard changes.**

**Current state:**
`store.h` includes `qtl/std20_recursive_generator.hpp` directly (C++20 path).
The cppcoro path (`#include <cppcoro/recursive_generator.hpp>`) is present in some
files and the makefile previously had a cppcoro build target. The selection between
them is implicit and scattered.

**Proposed `qtl/generator.h`:**

```cpp
// qtl/generator.h
// Single include point for recursive_generator<T>.
// Select implementation based on C++ standard or explicit override.
#pragma once

#if defined(USE_CPPCORO_GENERATOR)
#  include <cppcoro/recursive_generator.hpp>
   using cppcoro::recursive_generator;
#elif __cplusplus >= 202002L
#  include "qtl/std20_recursive_generator.hpp"
   // recursive_generator<T> is defined in std20_recursive_generator.hpp
#else
#  error "recursive_generator requires C++20 or cppcoro"
#endif
```

Then `store.h` (and any future file using generators) includes `qtl/generator.h`
instead of either backend directly.

**Benefits:**
- The standard/backend selection is in one place.
- Adding a new generator backend (e.g. `std::generator` from C++23) requires one file
  change, not a grep-and-replace.
- Makes it explicit that `recursive_generator` is a library choice, not a built-in.

**Action:**
1. Create `qtl/generator.h` as above.
2. Replace `#include "qtl/std20_recursive_generator.hpp"` in `store.h` with
   `#include "qtl/generator.h"`.
3. Check other files that include either generator header and redirect them.

**Risk:** Very low — purely a forwarding header.

---

## 4. Split `store.h` into logical units

**Priority: Low-to-medium — high payoff but significant effort; best done when join
work motivates new files anyway.**

**Current state:**
`store.h` is ~3300 lines containing at least five separable concerns:

| Lines (approx) | Content |
|---|---|
| 274–392 | `row`, `rows`, row operators, `lex::scalar` helpers |
| 393–563 | `store::splits`, interval/boundary types for trie structure |
| 564–770 | `path`, `path::pathspec`, `path::segment` |
| 771–969 | `path::requirements`, predicate evaluation, `satisfies()` |
| 1480–2360 | `lattice`, `lattice::vertex`, `lattice::iterator` |
| 2660–3300 | `store`, `lval`, `search_vertex`, `dedup_search`, test code |

**Proposed split:**

```
qtl/row.h        — row, rows, row operators
qtl/path.h       — path, pathspec, segment, requirements, satisfies()
qtl/lattice.h    — lattice, vertex, kv, iterator
qtl/store.h      — store class, lval, search_vertex, dedup_search, test (thinner)
```

Each new header includes the ones it depends on. `store.h` becomes mostly the store
class and query layer.

**Benefits:**
- Easier to navigate and reason about each component.
- `path.h` and `lattice.h` can be included by join-related files without pulling in
  the entire store.
- Shorter files = faster mental model when debugging.

**Complication:**
The header-as-TU test pattern means each split header needs its own test block
(`#if __INCLUDE_LEVEL__ + ... < 1`) or the tests stay consolidated in `store.h`'s
test block (which includes the sub-headers). The latter is simpler: keep the test code
in `store.h` even after splitting the library code out.

**Recommended approach:**
Do the split incrementally, starting with `row.h` (most self-contained), then `path.h`,
then `lattice.h`. At each step, `store.h` simply gains `#include "qtl/row.h"` etc.
Tests remain in `store.h` until there's a reason to split them too.

**Risk:** Medium. Moving declarations across headers can expose missing includes and
circular dependencies. Do one header at a time, run `make all` after each.

---

## 5. Clarify `std20_recursive_generator.hpp` vs cppcoro status

**Priority: Low — documentation and hygiene.**

Now that `std20_recursive_generator.hpp` is complete and correct, the relationship
between it and `cppcoro/recursive_generator.hpp` should be documented explicitly.

**Action:**
1. Add a comment at the top of `std20_recursive_generator.hpp` stating it is the
   active implementation, replacing `cppcoro/recursive_generator.hpp` for C++20
   builds, and that the old file is retained for reference / C++17 fallback.
2. Add a comment at the top of `cppcoro/recursive_generator.hpp` noting it is the
   C++17/experimental-coroutines path, superseded by `std20_recursive_generator.hpp`
   in C++20 builds, and that the `std::cerr` traces in it are debug artifacts.
3. Keep `qtl/std20_recursive_generator.hpp.OLD` (the pre-rewrite version) as reference
   for now; can be deleted once the rewrite has been in use for a while.

---

## Summary table

| # | What | Priority | Risk | Effort |
|---|---|---|---|---|
| 1 | Remove dead lval/co_lval | High | Low | Small |
| 2 | Fix TRACE/NOTRACE, cppcoro cerr | Medium | Low | Small |
| 3 | Add qtl/generator.h | Medium | Low | Tiny |
| 4 | Split store.h | Low-Medium | Medium | Large |
| 5 | Document std20 vs cppcoro status | Low | Low | Tiny |

Items 1–3 and 5 can be done opportunistically in any order.
Item 4 is best deferred until a natural seam appears (e.g. join work introduces new
files that need to include lattice types independently of store).
