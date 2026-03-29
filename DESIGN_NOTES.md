# Design Notes — Non-obvious things in qtl

*Tutorial-style explanations of design patterns and decisions that are not
immediately apparent from reading the code. Aimed at someone who knows C++ but
is new to this codebase.*

---

## 1. Headers as self-testing compilation units

Each `qtl/*.h` file contains both library code and its own test suite. The test code
is guarded by:

```cpp
#if __INCLUDE_LEVEL__ + !defined __INCLUDE_LEVEL__ < 1 + defined TEST_H
int main(...) { /* tests */ }
#endif
```

`__INCLUDE_LEVEL__` is 0 when the file is the top-level translation unit (compiled
directly), and >0 when it is `#include`d. `TEST_H` is defined on the command line
when compiling via `test.cpp`.

The effect: a header's `main()` is compiled only when that header is the top-level TU.
This means you can test a header two ways:

```bash
clang++ -std=c++20 qtl/interval.h        # compile directly
make interval.test.out                   # compile via test.cpp with -DTEST_H=...
```

The second form is what `make all` uses. The first form is useful for quick iteration.

This is intentional. It keeps tests physically co-located with the code they test —
there is no separate `test/` directory to get out of sync.

---

## 2. `tree<C>` inherits from `C`, not wraps it

```cpp
template<class C>
class tree : public C {
public:
    std::vector<tree> branches;
    ...
};
```

`tree<C>` IS a `C` — it inherits all of `C`'s interface. The only thing `tree` adds
is the `branches` vector and the `recurse` method.

The consequence: you can call methods of `C` directly on a `tree<C>` object.
For `tree<operation<V>>` (the expression tree), `t.eval(...)` calls
`operation::eval` directly — no `.value()` or `.payload()` indirection required.

Why inheritance instead of composition? The expression tree needs to be passed to
functions that expect `operation<V>` (e.g., the output formatting code). Inheritance
means `tree<operation<V>>` is implicitly convertible to `operation<V>` and can be
passed anywhere an `operation<V>` is expected without explicit casting.

The recursion: `tree<operation<V>>` has `branches: vector<tree<operation<V>>>`.
`operation<V>` does not contain `tree` — the tree structure lives entirely in `tree`.
This avoids a circular dependency between the two types.

---

## 3. `recurse` — a tree fold

`tree<C>::recurse` is a bottom-up tree fold (a catamorphism in functional programming
terms). It evaluates the entire subtree and reduces it to a single value:

```cpp
template<class A, class... Args>
A recurse(A (C::*f)(const std::vector<A>&, Args...) const, Args... args) const;
```

You pass a pointer to a `C` member function, and `recurse`:
1. Recursively calls itself on each branch, collecting results in `vector<A>`
2. Calls `(this->*f)(results, args...)` to compute this node's value from its children

Example: evaluating an expression tree over a row.
```cpp
// f is operation::eval — takes evaluated children, returns interval result
auto result = expr_tree.recurse(&operation<V>::eval, symbol_table, row_values);
```

The `lazyvec` overload of `recurse` is lazy — child branches are not evaluated until
the parent function asks for them. This enables short-circuit evaluation (e.g.,
`false && expr` avoids evaluating `expr`).

---

## 4. The X-macro `OP_TABLE` pattern

`operators.h` defines operations via an X-macro:
```cpp
#define OP_TABLE        \
X(lit,    0, ,   lit,  lit)        \
X(column, 0, col,column,lit)       \
X(plus,   2, +,  left_,multiplies) \
...
```
Columns: `X(name, arity, symbol, fixity, precedence_after)`.

This single definition is used in multiple places by re-defining `X`:

```cpp
// Generate enum values:
enum class op {
#define X(n,a,o,f,p) n,
    OP_TABLE
#undef X
};

// Generate dispatch table entries (in operation's optable):
#define X(n,a,o,f,p) { eval_##n, stringify_##n, #n, a, PREC_##p },
    static const methods optable[] = { OP_TABLE };
#undef X
```

Adding a new operation: add one `X(...)` line to `OP_TABLE`. The enum, dispatch
table, and any other generated structures update automatically.

There is a dead alternative definition in `operators.h` inside `#else` (short names:
`eq`, `ne`, `lt`). This is preserved history and can be ignored.

---

## 5. `operation<V>` — one struct, three roles

`operation<V>` serves as the payload for every node in an expression tree. It
carries fields for three distinct roles, only some of which apply to any given node:

**Role 1 — Operation identity**: `enum op o` (which operation this node represents),
looked up in the static `optable` for arity, eval function, and stringify function.

**Role 2 — Literal value storage**: `optional<V> cachevalue`. For `lit` nodes, this
holds the literal's value directly. It is NOT lazily computed — for literals it is
set at construction and never changes. The name "cache" is misleading for this use.
For computed operations, the cache update is disabled (commented out) to avoid
stale values when the same expression tree is evaluated over multiple rows.

**Role 3 — Name/scope binding**: `optional<string> identifier` (for `name` and
`column` nodes — which named variable or column number), and `optional<symbol_table>
sym` (for nodes that introduce a local binding scope).

This multi-role design means you can always check `cachevalue` to get a literal's
value, `identifier` to get a column reference's name, and `o` to know the operation —
without needing a variant or virtual dispatch. The downside is that every node carries
fields it doesn't use.

---

## 6. `TRACE` / `NOTRACE` — per-statement debug control

```cpp
TRACE(x)    // expands x only in debug/trace mode (globally controlled)
NOTRACE(x)  // never expands — always a no-op
```

`NOTRACE` is a way to selectively disable individual `TRACE` calls without deleting
them. If a particular debug output is too noisy, change `TRACE(x)` to `NOTRACE(x)`.
The output stays in the source, is easy to find by grep, and can be re-enabled by
changing it back. It gives fine-grained control on top of the global trace flag.
The suppression is typically temporary (for a debugging session) rather than permanent,
but there is no runtime distinction — `NOTRACE(x)` is always a no-op regardless.

The naming reads as: "TRACE = active trace statement; NOTRACE = trace statement that
is currently turned off." Both are findable with the same grep pattern.

Note: `cppcoro/recursive_generator.hpp` contains some bare `std::cerr` calls (not
wrapped in any macro) left from debugging sessions. These are unconditional and should
be wrapped in `TRACE()` or removed.

---

## 7. The lattice / trie structure

The store indexes data in a multi-dimensional trie called a `lattice`.

Each **path** is a vector of interval coordinates, one per dimension. A path identifies
a position in the multi-dimensional space. For a 3-column table, a path might be
`[["A","B"), ["1","2"), ["10","20")]` — a box in 3D column-value space.

The trie has two node types:
- **Leaf** (`rows`): a flat list of rows whose coordinates fall in this path's box
- **Internal node** (`splits`): a set of sub-interval divisions for one dimension,
  pointing to child vertices

Querying traverses the trie top-down, pruning branches that cannot satisfy the
predicate (`path::requirements::admits(path)` does interval-based pruning before
fetching actual rows).

`path::requirements` holds two things:
1. A **row prefix** — the query must match rows that start with these values
2. An **optexpr** — an expression predicate evaluated per-row

A path is pruned if its coordinate box is inconsistent with either the prefix
or the expression (interval arithmetic determines this without touching rows).

---

## 8. Table naming via row prefix

There is no separate table registry. Tables are a convention:

```sql
INSERT INTO customers VALUES ("Alice", 30)
```
stores the row `{"customers", "Alice", 30}` in the lattice.

```sql
SELECT name, age FROM customers WHERE age > 25
```
queries `store[{"customers"}][age_predicate]` — the `{"customers"}` prefix prunes
the trie to the sub-space where the first column is "customers".

This is elegant: the trie's prefix pruning naturally scopes queries to a table
without any special-casing. It also means tables can overlap in column-value space
if their name prefixes differ, and a "join" is just a query that spans two prefixes.

---

## 9. `lval` — the query result type

`lval` (logical value) is the result of `store[predicate]`. It is lazy — it holds
a reference to the lattice and the accumulated predicate, but does not execute the
query until iterated.

Predicates are chained with `operator[]`:
```cpp
store["customers"_s]["age"_col > 25_n]  // prefix then expression
```
Each `operator[]` returns a new `lval` with the predicate updated, leaving the
original unchanged.

`lval::begin()` materialises the query by constructing an iterator that drives
`dedup_search`. The iterator holds the coroutine generator internally.

**Important lifetime rule**: the expression passed to `lval` must outlive the iterator.
`path::requirements::expr()` has two overloads — const returns by value (temporary),
non-const returns by reference. If you construct an iterator via a temporary `lval`,
the expression stored in the temporary dies and the coroutine holds a dangling
reference, producing silent wrong results (predicate evaluates garbage). This is an
easy mistake to introduce when refactoring. The active code avoids it by constructing
the iterator directly from the `lval` whose lifetime is controlled by the caller.

---

## 10. `recursive_generator` — the C++20 coroutine design

`qtl/std20_recursive_generator.hpp` implements a generator that supports both
`co_yield value` and `co_yield nested_generator`. The latter is what makes it
"recursive" — a generator can delegate to a sub-generator and its values appear
seamlessly in the parent's output stream.

The key design is a **linked chain** in the promise objects:

```
root_promise
  m_root = &root_promise      (always points to root)
  m_parentOrLeaf = &child_promise   (root tracks the current deepest active leaf)

child_promise
  m_root = &root_promise      (all nodes know the root)
  m_parentOrLeaf = &root_promise    (non-root nodes point to their parent)
```

When `co_yield nested_gen` happens:
1. The parent's `yield_value(recursive_generator&)` updates the chain
2. **Immediately resumes the child** (starts it while the parent's frame is live)
3. If the child isn't empty, suspends the parent (awaitable returns false)

When the iterator calls `++`:
1. `root.pull()` resumes the current leaf (`m_parentOrLeaf`)
2. If the leaf is done, walks up: `m_parentOrLeaf = m_parentOrLeaf->m_parentOrLeaf`
3. Resumes the parent, which continues past its `co_yield nested_gen` point
4. Repeats until a live value is found or the root itself is done

Why immediately resume the child in step 2? Because the child generator is a local
variable in the parent coroutine. If we don't start it immediately, the parent
suspends and the local variable (the child generator object) is still on the parent's
coroutine frame. Starting the child while the parent is still at the `co_yield` point
guarantees the child's handle is valid and the parent's frame is still alive.

`begin()` calls `pull()` before returning — so the iterator is always positioned at
the first value (never at a `co_yield nested_gen` suspension point).

---

## 11. `dedup_search` — why it exists

The lattice can yield the same row from multiple paths. This happens when the trie's
interval partitioning places a row's coordinates in an overlap region between two
sibling nodes (possible during incremental insertion before the trie rebalances).

`dedup_search` wraps `search_vertex` with a `std::set<row> seen` filter:
```cpp
for (auto r : search_vertex(l, p, e))
    if (seen.insert(r).second) co_yield r;
```

This requires `row::operator<` to be defined correctly. The default C++20 spaceship
operator synthesized for `row` (a `vector<lex::scalar>`) would use `lex::number`'s
`operator double()` conversion, producing comparisons via double — which gives
`partial_ordering::unordered` for NaN, breaking `std::set`. The explicit `operator<`
using a lambda (`lexicographical_compare` with direct `lex::scalar::operator<`)
bypasses this. Do not remove it.

---

## 12. The `search_vertex` flattening — history

`search_vertex` currently uses:
```cpp
co_yield search_leaf(v->log(), e);       // nested generator
co_yield search_vertex(l, path2, e);     // recursive nested generator
```

This is the natural, idiomatic form. Before the `std20_recursive_generator` was
rewritten (2026-03-28), the generator was broken for nested yields (only the first
child of a multi-child node was visited; the parent handle was lost). As a workaround,
`search_vertex` was temporarily flattened to:
```cpp
for (auto r : search_leaf(...)) co_yield r;
for (auto r : search_vertex(...)) co_yield r;
```

After the generator rewrite, the idiomatic form was restored. The flattened version
is preserved in a `#if 0` block with a comment explaining the history. It is NOT dead
code to delete — it is a documented fallback if the generator ever needs to be
replaced with a simpler implementation.

---

## 13. `lex::string` — dual-field lazy view

`lex::string` (string.h ~line 264) maintains two mutable members:
- `_opt`: an `optional<std::string>` — the owned storage
- `_this`: a `std::string_view` — a view into either `_opt` or external memory

The pattern is lazy materialisation combined with copy-on-write. `_this` is the fast
read path (no allocation); `_opt` is populated on-demand by `std_()` when owned
storage is needed. After any mutation, `view_()` reassigns `_this` to point into the
updated `_opt`, keeping the two in sync. The invariant is: `_this` always reflects the
current value, but `_opt` may not exist yet.

A reader seeing `mutable` on both fields and a `const operator=` may think this is
thread-unsafe COW. The mutability is purely for lazy initialisation; the
sync discipline is implicit and distributed across operator overloads (defined via a
`#define inherit` macro block).

---

## 14. `lex::number` — three lazy caches for one value

`lex::number` (number.h ~line 700) holds three separate `optional` fields:
- `S`: semantic form (mantissa/exponent decomposition)
- `D`: `double`
- `DF`: `cpp_dec_float_34` (arbitrary precision)

Only one is populated at a time, on demand. `operator double()` (~line 825) selects
which to fill/read based on which is already available. Negation (~line 746) preserves
whichever is present.

The field names are intentionally short (`S`, `D`, `DF`) — they are private
implementation details. The hierarchy exists because some callers only need a `double`
(fast, lossy), while others need full precision (`DF`), and others need the symbolic
decomposition (`S`) for exact arithmetic. Lazy conversion avoids paying for precision
you don't need.

**Critical gotcha**: `operator double()` returns `std::numeric_limits<double>::quiet_NaN()`
for null numbers (~line 838). Since `NaN != NaN` in IEEE 754, any code that compares
numbers with `==` will silently fail if either is null. This is why `row::operator<`
in store.h uses a direct `lex::scalar::operator<` lambda rather than the default
spaceship operator, which would synthesize comparisons via `double`.

---

## 15. Two parallel `basic_boundary` types

`lex::basic_boundary` (interval.h ~line 120) and `qtl::basic_boundary` (interval.h
~line 148) represent the same mathematical concept — a boundary with a value and an
open/closed margin — but in different forms:

- `lex::basic_boundary` inherits from `lex::string`: the boundary is encoded as a
  string, suitable for persistence and lexicographic comparison.
- `qtl::basic_boundary` inherits from `std::pair<S,L>`: an in-memory pair for
  computation.

There is a conversion constructor in one direction. A `#if 0` block (~line 134) shows
a disabled conversion that was once more tightly coupled. The design evolved to
decouple serialisation (`lex::`) from computation (`qtl::`): you deserialise via
`lex::`, compute via `qtl::`.

---

## 16. `cmp_rule` in `expr.h` — query optimisation disguised as parsing

The comparison rule (~line 619) does more than build a comparison AST node. When it
detects the pattern `lower_bound <= x::x <= upper_bound` (where `x::x` is the special
"self" symbol representing the value being tested), it **immediately constructs an
interval literal** rather than building a subtree of two comparisons.

This is a parse-time query optimisation: the resulting `expr(op::interval, {a,b})` can
be used directly for trie pruning without any expression tree evaluation per row.
The pattern check looks for the `x::x` identifier in the fusion attribute and constructs
the interval if found; otherwise falls through to standard comparison node construction.

A reader will see what looks like a parser semantic action but is actually optimising
the representation for the query engine's interval-arithmetic path.

---

## 17. `fold()` in `expr.h` — constant folding at construction time

`fold()` (~line 274) constructs an expression node but immediately evaluates it if all
children have `cachevalue` set (i.e., are literals). If so, it returns a `lit` node
containing the precomputed result instead of the operation node.

This is compile-time constant folding: `2 + 3` in a SQL expression becomes `lit(5)`
before query execution begins, not during per-row evaluation. Almost all operator rule
actions use `fold()` rather than `expr()` directly — the difference is invisible at
the call site but significant for query performance.

---

## 18. `symtab`/`valtab` — nested and flattened namespace storage

`sql.h` maintains symbol tables keyed by a namespace prefix. Qualified column
references are stored in *both* a nested form (`symtab[table][col]`) and a flattened
form (`symtab[""][table + "." + col]`). The default (unqualified) namespace uses the
empty string `""` as the key.

This is intentional flexibility: lookups work whether or not the caller provides a
table prefix. The flattened form allows `customers.age` to resolve in the global
namespace without knowing which table it belongs to; the nested form allows iterating
all columns for a given table. Both are populated during `CREATE TABLE` parsing.

Note: if you see specific table/column names like `"customers"` or `"age"` in the
relevant code, those are test strings, not part of the generic mechanism.

---

## 19. `set_clause` in `sql.h` — uncertain status

The `set_clause` handler (~line 103) has a `#if 0` block where the active code stores
the parsed result without updating `valtab`, while the disabled code *would* have
updated it. Whether this is:
(a) a pre-existing incomplete implementation,
(b) a C++20 / Spirit x3 compatibility workaround from a recent port, or
(c) intentional deferral

is uncertain. Do not assume `SET colname = expr` followed by `SELECT colname` works
end-to-end without testing. If fixing this, check whether the Spirit x3 attribute
passing semantics changed between the C++17 and C++20 builds.

---

## 20. `plength()` `#if 0` — reference-preserved old implementation

`string.h`'s `plength()` (~line 269) has a `#if 0` block enclosing an older
implementation that manually walked the string parsing NUL escape sequences, replaced
by a Boost.Spirit x3 parser-based approach. The old code is probably preserved for
reference (as with `search_vertex`'s flattened form), but unlike that case there is no
comment explaining why it was kept. It may be safe to delete; verify first that the
two implementations were equivalent before discarding.

---

## 21. `radix_map.h` TODO list — explicit design boundary

The TODO list at the top of `radix_map.h` (~lines 13–40) is not aspirational. It
documents an explicit boundary: features that were considered and deliberately
deferred. Notably:

- No `erase()` / node shrinking / coalescing allocate
- No regular expression support
- Single-threaded only
- No byte-order conversion

This radix map is designed for read-heavy persistent workloads, not as a general
concurrent data structure. Do not assume any of these features exist or will be added
without a deliberate design decision.

---

## 22. Boost.Spirit fusion attribute indexing in `expr.h`

Spirit x3 semantic actions receive parsed attributes as fusion sequences. `expr.h`
accesses them by position: `at_c<0>(_attr(ctx))`, `at_c<1>(_attr(ctx))`, etc. For
compound rules like `product_rule`, `at_c<0>` is the first operand and `at_c<1>` is a
vector of `(op, operand)` pairs (~lines 488, 524).

These positional indices are tied to the grammar rule structure. If a rule's attribute
type changes (e.g., by reordering alternatives or adding an optional), all downstream
`at_c<N>` accesses break silently — the wrong value is read with no compiler error,
because the types often remain compatible. When modifying parser rules, check all
`at_c` usages in the corresponding action.

