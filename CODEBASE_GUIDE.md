# Codebase Guide — qtl (query template library)

*Written to enable rapid context recovery after a gap. Focuses on non-obvious
relationships, historical context, and active vs dead code. Line numbers are
approximate and may drift; use grep to confirm.*

---

## Build system

```
make <name>.test.out       # compile qtl/<name>.h as top-level TU via test.cpp
make all                   # builds all 8 test targets
```

`test.cpp` just does `#include TEST_H` where `TEST_H` is set on the command line.
Each header contains its own `main()` and test code inside:
```cpp
#if __INCLUDE_LEVEL__ + !defined __INCLUDE_LEVEL__ < 1+defined TEST_H
// test code here — compiled only when this header IS the top-level TU
#endif
```
This is **intentional** — headers are self-testing compilation units. Do not treat
the test code as dead or move it out.

Compiler: `clang++` from `/opt/homebrew/opt/llvm/bin/`, flags `-std=c++20`.
The `compiles/` directory is created by make and can be ignored.

---

## Test status (as of 2026-03-28)

| Target | Status | Notes |
|---|---|---|
| out.test.out | PASS | |
| string.test.out | EXIT 1 | Intentional stub — `exit(1)` at string.h:1054 before unreachable second test block. Assertions above it pass. |
| container.test.out | PASS | |
| number.test.out | PASS | |
| interval.test.out | PASS | |
| expr.test.out | PASS | |
| sql.test.out | PASS | |
| store.test.out | PASS | Fixed over several sessions; see below |

### Test run modes

Several test binaries read from `std::cin` and are interactive or fuzz-driven — they
are **not** self-terminating batch tests:

- **`expr.test.out`** — interactive expression evaluator. Type expressions at the
  prompt; it parses and evaluates them. Useful for ad-hoc testing of parser and eval
  changes (B1 regex op, etc.).

- **`store.test.out` (fuzz mode)** — full fuzz testing of `store.h` takes random
  byte input supplied by a fuzz tester (external driver), which transforms the random
  bytes into quasi-English distributed words. These words are then inserted, queried,
  and deleted against the store. The fuzz driver controls the input stream; running
  `store.test.out` directly from a terminal will block waiting for input.

When running `make all` (which pipes no stdin), the interactive tests complete quickly
because they hit EOF immediately. In fuzz mode the external driver provides the stream.

---

## File map

| File | Role | Active? |
|---|---|---|
| `qtl/store.h` | Core: row, path, lattice, store, lval, coroutine search | Yes — primary work area |
| `qtl/expr.h` | Expression parsing, building | Yes |
| `qtl/tree.h` | `tree<C>`, `operation<V>`, `lazyvec`, `enum op` | Yes — awkward partition (see below) |
| `qtl/operators.h` | `OP_TABLE` X-macro defining all op codes | Yes |
| `qtl/sql.h` | SQL parser and executor | Yes |
| `qtl/string.h` | lex::string, string parsing | Yes |
| `qtl/number.h` | lex::number | Yes |
| `qtl/interval.h` | interval<T>, boundary types | Yes |
| `qtl/out.h` | Output formatting, `operator<<` for all types — every type should be printable for tracing; some implementations may predate fold expressions/`std::format` and could be simplified | Yes |
| `qtl/container.h` | Container types | Yes |
| `qtl/radix_map.h` | Radix map (used by store internals) | Yes |
| `qtl/std20_recursive_generator.hpp` | C++20 coroutine generator — **active implementation** | Yes |
| `qtl/std20_recursive_generator.hpp.OLD` | Pre-rewrite generator — kept for reference | Dead (reference only) |
| `cppcoro.ARCHIVE/` | cppcoro generator — C++17/experimental path, archived 2026-03-30 | Archived (not in active source tree) |

---

## Key types and their roles

### `lex::scalar` (store.h ~line 273)
Union-like type: number, string, or interval. The atomic value type for all rows.
`lex::number::operator double()` exists — **this causes C++20 to synthesize `<=>` via
double comparison**, which breaks `std::set<row>` when NaN is involved (partial_ordering
returns unordered). Fixed by explicit `row::operator<` (see below).

**Internal encoding**: the binary representation is designed so that `memcmp` gives
the correct ordering for both string and numeric values — a fundamental constraint
since trie path comparison is byte-level.

String encoding satisfies three constraints simultaneously: (1) **null-safe** — encoded
strings may contain null bytes, so `\0` alone cannot terminate; (2) **self-terminating**
— end-of-string is detectable without a length field; (3) **hierarchically unambiguous**
— composite and nested tuples use different terminator levels so a decoder can
distinguish end-of-string, end-of-tuple, end-of-outer-tuple, etc. without a schema.
This requires multi-byte terminators with distinct escape levels. Unusual multi-byte
boundary sequences in string values are this scheme, not artifacts.

Numeric encoding is also chosen so that values representable in small C++ types (int,
float, etc.) tend to have short representations, optimising average overhead across a
typical corpus of numeric data.

### `row` (store.h ~line 274)
`std::vector<lex::scalar>`. Represents one tuple.
`row::operator<` at ~line 294 uses a lambda to force `lex::scalar::operator<` (not
spaceship-via-double). This was a hard-won fix — do NOT remove it or replace with
default `<=>`.
`row::operator==` at ~line 298 likewise explicit.
`row operator+` concatenates two rows (used for join output).

### `interval` / `basic_boundary` (interval.h)
Two design choices make interval arithmetic uniform with no special cases:

**Projective real line**: intervals live on RP¹ where ∞ is a single point, so they
can wrap around ∞. Division by an interval containing 0 always yields one wrapped
interval rather than two disjoint pieces — `1 / [-1,1]` gives `[1,-1]` (through ∞),
not `(-∞,-1] ∪ [1,+∞)`. Division is always defined, no split-result logic needed.

**±ε boundary displacements**: every boundary point is a value plus a signed
infinitesimal displacement. The `ma` field of `basic_boundary` is that sign:
`infi` (-1) means displaced toward −∞ (value − ε); `supre` (+1) means displaced
toward +∞ (value + ε). All comparisons reduce to a single strict `<` on these
displaced values — the open/closed distinction disappears entirely. Do NOT
describe `ma` in terms of open/closed: whether a displacement is "open" or
"closed" depends on lower-vs-upper context, and the design avoids needing to
know that context.

Two forms of `basic_boundary` exist:
- `lex::basic_boundary` — string-encoded, for persistence and lexicographic comparison
- `qtl::basic_boundary` — in-memory `pair<S,L>`, for computation

### `kleen` three-valued logic (store.h / interval.h)
Two distinct uses of kleen should not be conflated:

**`kleen` as a C++ class**: `kleen::T` (true), `kleen::F` (false), `kleen::U`
(unknown/maybe). A concrete C++ type returned by `admits()` and `satisfies()` during
lattice traversal — the pruning decision type.

**Kleen logic as the conceptual behaviour of `expr` evaluation**: when an expression is
evaluated against an interval of data values, the result behaves according to
Kleene's three-valued logic. The actual C++ return type of an expr evaluation may be
an interval (or other type) that is *interpreted* as T/F/U — this is distinct from
returning the `kleen` C++ class directly.

**Kleen values (conceptually) are degenerate intervals** — the same `&` (intersection)
and `|` (union) operators used for intervals work uniformly on kleen, because:
- `F` = empty interval ∅ — `F & x = F`, `F | x = x`
- `T` = universal interval — `T & x = x`, `T | x = T`
- `U` = any non-empty, non-universal interval

This means the query algebra (intersect intervals, compare, and/or the results) is
uniform across scalars, intervals, and kleen values. The type determines the
representation; the algebra is the same.

**Caution**: different intervals that both map to `U` can intersect to `F` (empty
intersection) or union to `T` (covering the whole domain). The query evaluation order
(intersect/union intervals → compare → and/or kleen results) prevents this from being
a problem in normal use. There is no type barrier enforcing this order; it is a
convention the programmer must respect.

**Lattice pruning**: evaluating a predicate against the interval of values at a trie
node yields a `kleen` C++ value — `T` (include entire subtree), `F` (prune), or `U`
(recurse). `path::requirements::admits()` is this evaluation.

**Design note — `<=>` on intervals (not yet implemented)**: neither a C++ `operator<=>`
nor an OP_TABLE `<=>` entry exists in this codebase. A `partial_ordering` class is
defined in `interval.h` only under `#ifdef TEST` — the design was developed and
prototyped but not pushed to working code. When completed, the natural return type
would be `interval<sign>` where `sign ∈ {-1, 0, 1}`, not a kleen triple or
`std::partial_ordering`. `sgn(a - b)` would map a numeric difference interval to an
interval on the 3-point ordered set `{-1, 0, 1}`, and because intervals live on RP¹,
wrapping is possible:
- Non-wrapping difference → conventional interval on `{-1, 0, 1}`, e.g. `[-1, 0]`
- Wrapping difference (large positive OR large negative, never zero) → wrapping interval
  `[1, -1]` on `{-1, 0, 1}`, the only case that gives "not equal but sign unknown"

All 7 non-empty subsets of `{-1, 0, 1}` would be reachable (6 conventional + 1 wrapping);
the empty subset is the only impossible result. The `&` and `|` operators on `interval<sign>`
would be intersection and union of possible-ordering sets — the same algebra as everywhere
else. Individual relational operators (`<`, `=`, `>`) would extract whether a specific
sign value lies in the result interval, returning kleen.

**Kleen is `interval<data_value>`, not `interval<bool>`**: it is tempting to model kleen
as an interval on the two-point truth domain `{0=F, 1=T}`, but this breaks the algebra.
If `T = {1}` and `U = {0,1}`, then `T ∩ U = {1} = T`, but kleen requires `T & U = U`.
The correct model is that kleen values arise from evaluating a predicate against an
interval of *data values*: `T` = all data values satisfy, `F` = none satisfy, `U` = some
do and some don't. The `&` and `|` for kleen are inherited from AND/OR of predicates over
the data domain, not from interval operations on a boolean domain.

**The interval size tension**: a larger interval simultaneously means two opposite things
depending on context:
- *Value interval* (trie node): wider = more uncertainty about the actual data value.
  Universal interval = completely unknown value.
- *Satisfaction interval* (kleen/predicate): larger set of satisfying values = closer to
  `T`. Universal = predicate is always true.

These run in opposite directions. A narrow value interval (more known about the data)
lets predicates resolve to `T` or `F`; a wide value interval drives results toward `U`.
The sharpest illustration: `x == x` has a universal solution set (`T` — always true,
useless as a filter) while `x` itself is completely unconstrained. Maximum truth,
maximum uncertainty about the actual value. Conversely, a useful filter (`x == 5`) has
a small solution set — "mostly false" for arbitrary `x` — but when it fires, it tells
you exactly what `x` is.

### `path` (store.h ~line 564)
`vector<interval<lex::scalar>>` — position in the lattice. Each element is a
coordinate range in one dimension.

### `path::requirements` (store.h ~line 771)
`tuple<row prefix, optexpr expr>`. The active filter on a query.
`path::requirements::admits(path)` — checks if a path can satisfy the requirements
(interval-based pruning before full row evaluation).
`(path, row).satisfies(requirements)` — full predicate check.

### `lattice` (store.h ~line 1480)
A DAG (directed acyclic graph), not just a trie. Maps `path → shared_ptr<vertex>`.
`vertex` is either a `rows` (leaf) or a structure of `splits` (internal node).
Rows are stored at leaves. Internal nodes partition the space by dimension.

**Multiple paths to the same node are intentional**: different query paths through the
lattice can converge on the same `vertex` object (sharing is enabled by `shared_ptr`).
This is the mechanism for query-pattern optimisation — common substructure is shared
across paths, amortising traversal cost. The name `lattice` reflects this: it is
mathematically a lattice/DAG, not a tree. The dynamic rebalancing optimisation
(see planned features) would allocate and merge shared nodes based on observed queries.

**`dedup_search` is a direct consequence**: since the same leaf is reachable via
multiple paths, traversal can yield the same row more than once. Deduplication is
required for correctness, not just performance.

**`confluence` map** (store.h ~line 1280): tracks paths where multiple traversal routes
converge on the same splits structure. `^=` is a consensus operator — keep a splits
entry only if every traversal agrees on it, clear it otherwise. Intended for the
adaptive-splitting scheme (database cracking): a path is a good split candidate only
if all traversals consistently see the same structure there. Currently only appears in
TRACE/debug output; never influences control flow. The adaptive splitting that would
use it is not yet complete (see `split()` / `choosesplit()`).

### `radix_map` (radix_map.h)
Persistent radix tree used by store internals. File-mapped; supports `open`/`create`
with a `multiuser` flag (`bit_mask(6)`) and `have_lock` flag (`bit_mask(7)`) for
future thread safety.

**Design intent (confirmed by author)**: designed for single-user speed — a priority
when the code was written. `dtree`/`store` shares this priority, to a greater degree.
Concurrent access was not architecturally excluded: the `multiuser` flag and a TODO
"Make multithreaded using hazard pointers" show it was anticipated as a future project.
There is no reason to gratuitously discard concurrent access support if it is added.

### `store` (store.h ~line 3243 area)
Owns the `lattice`. The user-facing database object.
SQL operations (`INSERT`, `SELECT`, etc.) go through `sql.h` which calls `store` methods.
Tables are row-prefix namespaced: a row in table "T1" is stored as
`{"T1", col1, col2, ...}` — the table name is literally the first column value.

### `model` (store.h — near `store`)
A deliberately simple unindexed database used for **model-based testing**. Stores rows
in a plain container; answers queries via full linear search with no trie or index
pruning. Correct by construction (too simple to have index bugs). Used to validate
`store` by running the same queries against both and checking agreement. The name is the
conventional term: a reference implementation that defines correct external behaviour.

### `lval` — two implementations, one active

**Non-coroutine `lval`** (~line 2660): Original iterator-based implementation.
Uses `lattice::iterator` directly. **Appears dead** — all active query paths use the
coroutine version. Confirm before deleting.

**Coroutine `lval`** (~line 2781): **This is the active one.**
Holds `shared_ptr<lattice>` and `path::requirements predicate`.
`operator[](const expr&)` returns a new `lval` with the predicate applied.
`operator[](const row&)` returns a new `lval` with a row prefix applied.
`begin()` returns an iterator that drives `dedup_search`.

**`co_lval`** (~line 3226): A second coroutine implementation with member functions
`co_search_vertex` / `co_search_leaf`. Also **appears dead**. These member functions
still use the `co_yield nested_gen` idiom (now correct but the class is superseded).

### `search_vertex` / `search_leaf` / `dedup_search` (store.h ~line 3176)

`search_leaf(rows, expr)` → `recursive_generator<row>`: yields rows from a leaf that
satisfy the predicate.

`search_vertex(lattice, path, expr)` → `recursive_generator<row>`: recursively
traverses the trie, yielding all matching rows.
**Currently uses `co_yield nested_gen` (idiomatic form)**. A `#if 0` block at ~line
3198 preserves the flattened `for(auto r : ...) co_yield r` form that was used as a
workaround when the std20 generator was broken. The comment explains the history.

`dedup_search(lattice, path, expr)` → `recursive_generator<row>`: wraps
`search_vertex`, filters duplicates via `std::set<row>`. Needed because the lattice
can yield the same row from multiple paths (lattice overlaps).

---

## The `store::lval::begin()` lifetime trap

**Critical**: `store::lval::iterator::begin()` at ~line 2999 creates a new
`iterator(B, *(B->l), {}, B->predicate.expr())`. The `B` pointer is `const lval*`.
`path::requirements::expr() const` returns **by value** (a temporary `optexpr`).
The non-const overload returns by reference.

If this is ever changed back to `iterator(...).begin()` (double-construction),
the coroutine will hold a dangling reference to the temporary `optexpr`. This causes
silent wrong results (predicate evaluates garbage) rather than a crash.
**The fix is the single-construction form at line 3002: `auto i = iterator(B, ...); return i;`**

---

## `recursive_generator` — history and current state

### cppcoro version (`cppcoro/recursive_generator.hpp`)
Original implementation using `std::experimental::coroutine`. C++17/Clang path.
Still present. Has unconditional `std::cerr` debug traces left in (lines 142, 155, 244,
300, 307, 317) — these fire on every query if this version is used. The C++20 build
does NOT use this file.

### std20 version (`qtl/std20_recursive_generator.hpp`)
**Active for C++20 builds.** Full rewrite (2026-03-28) using cppcoro's
`m_root/m_parentOrLeaf` chain design ported to `<coroutine>`.

Key design: `promise_type` has two pointers:
- `m_root`: always points to the root promise of the generator chain
- `m_parentOrLeaf`: for root = current leaf; for non-root = parent

`yield_value(recursive_generator&)` immediately resumes the child generator
(sets up chain, calls `child.resume()`). This is the key correctness property —
the child starts while the parent's stack frame (and temporaries) are still alive.

`pull()` on root: resumes the leaf, then walks up the chain while leaves are done.
`begin()` calls `pull()` first — so the iterator always starts at the first value.

**Old broken design** preserved as `.OLD` file. Three bugs it had:
1. `begin()` returned `{}` as first value when first yield was a nested generator
2. Parent handle lost when child exhausted (only first child of multi-child node visited)
3. `advance()` crashed when called on a done coroutine handle

---

## `tree.h` / `expr.h` architecture — the awkward partition

`tree.h` contains THREE things that shouldn't all be together:
1. `lazyvec<A,V,P>` (~line 22): lazy iterator adapter; used by `tree::recurse`.
   Predates `std::ranges`. Has two function slots: `p` (per-element transform, analogous
   to `views::transform`) and `f` (whole-vector reduce, no direct `std::ranges`
   equivalent). If ever modernised, preserve the `f` slot — it is not covered by
   `views::transform` alone.
2. `tree<C>` (~line 106): generic tree node that **inherits from C** (not wraps).
   `tree<C>` IS a `C` plus `std::vector<tree<C>> branches`.
   `recurse(member_fn_ptr, args...)` is a bottom-up tree fold (catamorphism) —
   evaluates all branches, passes results to the node's function.
3. `operation<V, V1>` (~line 195): domain-specific expression payload.
   Contains `enum op o`, `optional<V> cachevalue`, `optional<string> identifier`,
   `optional<symbol_table> sym`, `struct methods` (function pointers for eval/stringify),
   and presumably a static `optable` array indexed by `op`.

`enum class op` is defined in `tree.h` by expanding `OP_TABLE` from `operators.h`.
This makes `tree.h` domain-specific despite `tree<C>` being generic.

The `expr` type alias is defined in **`store.h`**, not `expr.h`:
```cpp
using expr = tree<operation<interval<lex::scalar,void>, intvec<lex::scalar>>>;
using optexpr = optional<expr>;
```
Anything that needs `expr` must include `store.h`, dragging in the entire store.

`cachevalue` in `operation`: for `lit` nodes this IS the value (not a cache).
For computed operations the cache update is commented out. The field name is misleading.

There is a dead alternate `OP_TABLE` definition in `operators.h` inside `#else` (uses
short names: `eq`, `ne`, `lt` etc.). Preserved history — ignore it.

---

## `operators.h` — X-macro pattern

```cpp
#define OP_TABLE \
X(lit,0,,lit,lit) \
X(name,0,,name,lit) \
...
```
Column format: `X(name, arity, operator_symbol, fixity, precedence_after)`.
Used to generate both `enum class op { lit, name, column, ... }` and the `optable`
dispatch array entries. Adding a new operation requires adding one `X(...)` line here.

**OP_TABLE and the parser are loosely coupled — a maintenance trap**: adding an entry
to OP_TABLE does NOT automatically make the parser recognise the operator. The parser
in `expr.h` is a hand-written Boost Spirit grammar where precedence is encoded entirely
in the nesting structure of named rules (`or_rule` → `and_rule` → `not_rule` →
`cmp_rule` → `sum_rule` → `product_rule` → `pre_rule` → `atom_rule`). Each rule calls
only the next-lower level as its operand, enforcing precedence by grammar structure.

The `precedence_after` field (5th column) is **not used by the parser** — it is used
only by the pretty-printer (`par()` in `tree.h`) to decide where to emit parentheses
when stringifying an expression tree.

`pow` (`^`) is the existing example of this gap: it is in OP_TABLE but has no
corresponding parse rule in `expr.h` and cannot be entered by the user.

**`x::x` interval syntax**: the `interval` op uses operator symbol `<= x::x <`, producing
the expression syntax `lo <= x::x < hi`. The `x::x` token is borrowed from mathematical
set-builder notation where `:` means "such that": `{x : lo ≤ x < hi}`. C++'s `::` was
chosen for visual resemblance to that colon — it is not scope resolution in this context.
The syntax was chosen because `[lo,hi)` and `(lo,hi]` do not parse in C++ at all, and
`(lo,hi)` has different meaning depending on context.

---

## SQL layer (`sql.h`)

Tables are row-prefixed: `INSERT INTO T1 VALUES (a, b)` stores row `{"T1", a, b}`.
`SELECT ... FROM T1 WHERE ...` queries `store[{"T1"}][predicate]`.

`SET colname = expr` / `ALIAS colname = expr` define computed column aliases that map
SQL column names to `_col` literal expressions.

No JOIN support yet. Design planned in `JOINS_DESIGN.md`.

---

## `TRACE` / `NOTRACE` macro semantics

```cpp
TRACE(x)    // expands x only when TRACE/debug mode is on
NOTRACE(x)  // never expands — always a no-op
```
`NOTRACE` is a per-statement "off switch" for individual trace calls. Change
`TRACE(x)` to `NOTRACE(x)` to silence one noisy output without deleting it;
change it back to re-enable. Both are grep-findable, giving fine-grained control
on top of the global trace flag.

Separately: `cppcoro/recursive_generator.hpp` has bare `std::cerr` calls (lines 142,
155, 244, 300, 307, 317) that are not wrapped in any macro — these are unconditional
and fire whenever that code path runs.

---

## Design documents (in project root)

| File | Contents |
|---|---|
| `JOINS_DESIGN.md` | Full join implementation plan: pipe-stage architecture, planner design, open questions |
| `ARCH_RECOMMENDATIONS.md` | Prioritised refactoring recommendations with rationale |
| `CONCEPTS.md` | Higher-level architectural overview (maintained by user) |

---

## Planned but unimplemented features (relevant to near-term work)

- **Row slice/project expressions**: `slice(row, col_list)` for projecting subsets of
  columns. Needed for join transforms. Partially planned.
- **Tuple/pack expression**: packing multiple scalars into one synthetic `lex::scalar`.
  Needed for multi-column join keys and GROUP BY.
- **Regex match/substitute predicates**: pattern matching in `expr` tree.
- **Non-leftmost trie traversal**: trie currently defaults to leftmost-first; traversal
  from arbitrary column is noted as possible but details not confirmed in code.
- **Joins**: see `JOINS_DESIGN.md`. No implementation started.
  **Key design insight**: join *logic* requires no new machinery. An interval expression
  `lo <= x::x < hi` with non-constant `lo`/`hi` (e.g., column references from a second
  store) evaluates to `U` at interior trie nodes (bounds unresolved) and to `T`/`F` at
  leaves where the column values are concrete. The existing kleen evaluation and lattice
  traversal already handle this correctly — the traversal descends into all `U` nodes and
  prunes `F` nodes, which is exactly the join-filter behaviour. The implementation work
  is therefore the co-traversal mechanics (how two stores' tries are stepped together)
  and the slice/pack expressions for projecting join keys — not new predicate logic.
- **Dynamic trie rebalancing based on query workload**: as queries accumulate, the trie
  could track which dimensions appear most frequently in predicates and reorganize nodes
  to put high-selectivity dimensions closer to the root, making common queries prune
  more aggressively. Harder than aggregate caching — requires workload statistics per
  node, a restructuring algorithm that preserves data integrity, and a policy for when
  rebalancing is worth the cost. Conceptually related to splay trees and adaptive radix
  trees. Not implemented.
- **Aggregate queries + node-level caching**: SUM, COUNT, AVG, VARIANCE are not yet
  implemented. When implemented, a further optimisation is possible: once a trie node
  is known to be fully included in a query's range, aggregate quantities for its entire
  subtree can be computed once and cached in the node. Caches would be updated (or
  invalidated) incrementally on insert/delete. The numerical stability of incremental
  mean/variance updates was researched but references not recorded — likely Welford's
  online algorithm (1962) for running variance, and Chan et al. (1979) for combining
  aggregates across subtrees (needed for merge after split). Decremental updates (on
  delete) are harder than incremental ones.
- **Prefix-compressed string storage**: within a trie node, all values share the
  node's path prefix by construction — storing that prefix in each value is redundant.
  A `prefixed_string` class was envisioned to transparently store just the suffix plus
  a reference to the shared prefix, reconstructing the full value on demand. The same
  mechanism would apply to interval endpoints that share a common prefix, storing the
  prefix once and only the diverging suffixes for each bound. Not implemented.
