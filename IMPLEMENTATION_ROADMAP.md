# Implementation Roadmap

*Written 2026-03-30. Covers near-term work, design decisions made, and sequencing rationale.*

---

## Prioritised work groups

### Group A — Quick wins (independent, do first)

Small changes that reduce confusion and make subsequent work cleaner. Safe to do in
any order.

**A1. Remove dead `lval` / `co_lval` code**
Remove the non-coroutine `lval` (~line 2660 in store.h) and `co_lval` (~line 3226).
All active query paths go through the coroutine `lval`. These add ~400 lines of
confusion about which path is live. Confirm with grep before deleting.
See ARCH_RECOMMENDATIONS.md §1.

**A2. Delete `cppcoro/`**
The std20 C++20 implementation (`qtl/std20_recursive_generator.hpp`) is stable — all
8 tests pass. The cppcoro path has unconditional `std::cerr` output on every query
iteration, making it actively harmful if accidentally compiled in. Since we are not
targeting C++17, archive the `cppcoro/` directory (move it out of the active source
tree, e.g. to `cppcoro.ARCHIVE/`).
Precaution: grep for `cppcoro` in all headers and the makefile; confirm nothing active
references it before archiving.

**A3. Add `qtl/generator.h`**
A thin forwarding header, now simplified since there is no cppcoro fallback:
```cpp
#pragma once
#include "qtl/std20_recursive_generator.hpp"
// recursive_generator<T> is defined in std20_recursive_generator.hpp
```
Replace the direct `#include "qtl/std20_recursive_generator.hpp"` in store.h (and
anywhere else it appears) with `#include "qtl/generator.h"`. This makes any future
generator backend change a one-file edit.

**A4. Document std20 as sole implementation**
Add a comment at the top of `qtl/std20_recursive_generator.hpp` noting it is the
sole active generator implementation as of C++20; the cppcoro directory has been
removed.
See ARCH_RECOMMENDATIONS.md §5.

---

### Group B — Medium scope, fairly independent

**B1. Regex match operator**

New `matches` op for filtering string columns by regular expression.
Full design: see `REGEX_MATCH_DESIGN.md`.

Design decisions settled:
- **Expression syntax**: `col ~= 'pattern'`
- **SQL syntax**: `col MATCHES 'pattern'`
- **Semantics**: anchored — `std::regex_match` (full string must match pattern).
  For substring matching, user writes `.*pattern.*`.
  Rationale: anchored maps cleanly onto admits() pruning and is the less surprising
  default for a database predicate.
- **admits() pruning**: `std::regex_constants::match_partial` on the common prefix of
  the interval's lo/hi bounds. Sound (never prunes incorrectly); may miss pruning
  opportunities when the common prefix is short. Full product-NFA approach deferred.

Files to modify: `qtl/operators.h`, `qtl/store.h`, `qtl/expr.h`, `qtl/sql.h`.

Before coding: confirm the lex::scalar→std::string decoding path, interval lo/hi
accessor names, and the kleen→operand_t conversion convention used by existing
comparison ops. See REGEX_MATCH_DESIGN.md §"What needs to be confirmed".

**B2. Opportunistic audits**
- `set_clause` in sql.h: uncertain whether missing valtab update is a C++20/Spirit x3
  incompatibility, a pre-existing bug, or intentional deferral. Read and determine.
- `plength()` `#if 0` block in string.h: determine if the old implementation is
  deletable.

---

### Group C — Prerequisites for joins

These must exist before join work can begin. They are also independently useful
(projection, GROUP BY, complex sort keys).

**C1. Row slice / project expression**

`slice(row, i, j, ...)` — extracts specified columns from a row.

Design decisions:
- Overloaded to return either a `row` (vector of scalars) or a packed `lex::scalar`
  depending on context / call form.
- Operates on logical (decoded) values — encode/decode is transparent to the user.
- Column indices are static (fixed at parse/build time), not runtime expressions,
  for the initial implementation.

Syntax (proposed, not finalised):
- Expression: `slice(col_expr, 0, 2, 3)` or similar
- New op in OP_TABLE; eval function in store.h

**C2. Tuple / pack expression**

`pack(col1, col2, ...)` — combines multiple scalars into a single `lex::scalar`.

Design decisions:
- **Reuses the existing hierarchical `lex::scalar` tuple encoding.** Ordering semantics
  fall out for free: a packed composite key compares correctly under memcmp, which is
  exactly what trie traversal and join key ordering require. No new type needed.
- `pack(row)` and `pack(scalar1, scalar2, ...)` as overloaded calling conventions.
- Interconvertible with `row`: `unpack(scalar) → row`, `pack(row) → scalar`.
- Transparent to user: operations work at the logical level; the internal encoding
  is not user-visible.

Primarily needed for multi-column join keys and GROUP BY. Also independently useful
for constructing composite trie keys.

---

### Group D — Larger work (depends on C)

**D1. Joins**
Full design in `JOINS_DESIGN.md`. Blocked on C1 (slice) and C2 (pack).
Do not start until both are complete and tested.

**D2. Split `store.h`**
Proposed split into `row.h`, `path.h`, `lattice.h`, `store.h` (thinner).
See ARCH_RECOMMENDATIONS.md §4.
**Defer until join work motivates new files** (e.g. `join.h` needs to include
`lattice.h` independently of `store.h`). No benefit to doing it purely for its own
sake beforehand.

---

### Group E — Envisioned, low priority

These are design-only for now; no implementation planned in the near term.

- **Aggregate queries + node caching**: SUM, COUNT, AVG, VARIANCE with incremental
  cache in trie nodes. Numerically stable updates via Welford (1962) / Chan et al.
  (1979). See DESIGN_NOTES.md §14d.
- **Dynamic trie rebalancing**: restructure the DAG based on observed query workload.
  Relates to splay trees / adaptive radix trees. See DESIGN_NOTES.md §14c.
- **Prefix-compressed string storage**: `prefixed_string` stores suffix + shared
  prefix reference, reducing redundant storage within trie nodes. See DESIGN_NOTES.md
  §14e.

---

## Sequencing summary

```
A1–A4  (any order, do first)
  │
  ├─ B1  (regex match)
  ├─ B2  (audits, opportunistic)
  │
  ├─ C1  (slice)
  ├─ C2  (pack)
  │       │
  │       └─ D1  (joins)
  │
  └─ D2  (store.h split — wait for D1 to motivate it)
```

E items are independent of all of the above and can be picked up at any time if
motivation arises.
