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
| `qtl/out.h` | Output formatting, `operator<<` for all types | Yes |
| `qtl/container.h` | Container types | Yes |
| `qtl/radix_map.h` | Radix map (used by store internals) | Yes |
| `qtl/std20_recursive_generator.hpp` | C++20 coroutine generator — **active implementation** | Yes |
| `qtl/std20_recursive_generator.hpp.OLD` | Pre-rewrite generator — kept for reference | Dead (reference only) |
| `cppcoro/recursive_generator.hpp` | cppcoro generator — C++17/experimental path | Superseded for C++20 |

---

## Key types and their roles

### `lex::scalar` (store.h ~line 273)
Union-like type: number, string, or interval. The atomic value type for all rows.
`lex::number::operator double()` exists — **this causes C++20 to synthesize `<=>` via
double comparison**, which breaks `std::set<row>` when NaN is involved (partial_ordering
returns unordered). Fixed by explicit `row::operator<` (see below).

### `row` (store.h ~line 274)
`std::vector<lex::scalar>`. Represents one tuple.
`row::operator<` at ~line 294 uses a lambda to force `lex::scalar::operator<` (not
spaceship-via-double). This was a hard-won fix — do NOT remove it or replace with
default `<=>`.
`row::operator==` at ~line 298 likewise explicit.
`row operator+` concatenates two rows (used for join output).

### `path` (store.h ~line 564)
`vector<interval<lex::scalar>>` — position in the lattice. Each element is a
coordinate range in one dimension.

### `path::requirements` (store.h ~line 771)
`tuple<row prefix, optexpr expr>`. The active filter on a query.
`path::requirements::admits(path)` — checks if a path can satisfy the requirements
(interval-based pruning before full row evaluation).
`(path, row).satisfies(requirements)` — full predicate check.

### `lattice` (store.h ~line 1480)
The trie index. Maps `path → shared_ptr<vertex>`.
`vertex` is either a `rows` (leaf) or a structure of `splits` (internal node).
Rows are stored at leaves. Internal nodes partition the space by dimension.

### `store` (store.h ~line 3243 area)
Owns the `lattice`. The user-facing database object.
SQL operations (`INSERT`, `SELECT`, etc.) go through `sql.h` which calls `store` methods.
Tables are row-prefix namespaced: a row in table "T1" is stored as
`{"T1", col1, col2, ...}` — the table name is literally the first column value.

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
1. `lazyvec<A,V,P>` (~line 22): lazy iterator adapter; used by `tree::recurse`
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
