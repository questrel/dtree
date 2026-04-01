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

## 7. The lattice — a DAG, not a trie

The name `lattice` is precise: the index is a directed acyclic graph (DAG), not a
tree. Multiple paths through the lattice can converge on the same `vertex` object.
This is intentional and is the mechanism by which the index optimises for query
patterns — common substructure is shared across paths, amortising traversal cost.
The `shared_ptr<vertex>` throughout the implementation is what enables this sharing.

Each **path** is a vector of interval coordinates, one per dimension. A path identifies
a position in the multi-dimensional space. For a 3-column table, a path might be
`[["A","B"), ["1","2"), ["10","20")]` — a box in 3D column-value space.

The lattice has two node types:
- **Leaf** (`rows`): a flat list of rows whose coordinates fall in this path's box
- **Internal node** (`splits`): a set of sub-interval divisions for one dimension,
  pointing to child vertices

Querying traverses the lattice top-down, pruning branches that cannot satisfy the
predicate (`path::requirements::admits(path)` does interval-based pruning before
fetching actual rows).

`path::requirements` holds two things:
1. A **row prefix** — the query must match rows that start with these values
2. An **optexpr** — an expression predicate evaluated per-row

A path is pruned if its coordinate box is inconsistent with either the prefix
or the expression (interval arithmetic determines this without touching rows).

**Why `dedup_search` exists**: because the same leaf vertex is reachable via multiple
paths, traversal can yield the same row more than once. Deduplication is a correctness
requirement, not a performance optimisation — without it, queries return duplicates.

**Planned**: dynamic allocation of shared nodes based on observed query patterns (see
section 14c). The current implementation has the DAG structure but does not yet
adaptively reshape it.

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

## 14a. String and tuple encoding — null-safe, self-terminating, hierarchical

The string encoding is designed to satisfy three constraints simultaneously:

1. **Null-safe**: encoded strings may contain null bytes, so a single `\0` cannot serve
   as a terminator.
2. **Self-terminating**: the end of a string can be detected without a separate length
   field — the encoding itself signals where the string ends.
3. **Hierarchically unambiguous**: composite tuples (and nested tuples) use different
   levels of terminators, so a decoder can unambiguously distinguish end-of-inner-string,
   end-of-tuple, end-of-outer-tuple, etc., without a schema.

The solution is multi-byte terminators with different escape levels. A terminator for a
string within a tuple is distinguishable from a terminator for the tuple itself, which
is distinguishable from a terminator for an outer tuple containing that tuple. This is
why the encoding is more complex than a simple null-terminated or length-prefixed
scheme: those approaches fail when the data can contain nulls and when keys are
composite or nested.

A reader who sees unusual multi-byte sequences at the boundaries of string values —
this is the hierarchical terminator scheme, not an artifact or bug.

---

## 14b. `memcmp`-ordered encoding with corpus-optimised numeric representation

The internal binary encoding of both `lex::string` and `lex::number` is designed so
that `memcmp` gives the correct total ordering for all value types. This is a
fundamental constraint imposed by the trie: trie path comparison is essentially a
byte-level comparison, so the encoding must be order-preserving across the full value
domain.

For numeric values, the encoding goes further: the mapping is chosen so that values
representable in small C++ numeric types (int, short, float, etc.) tend to have short
byte representations. This is a corpus-level optimisation — the goal is to minimise
average representation length across typical numeric data, reducing storage and
comparison overhead. It is not merely about type correctness; it reflects an assumption
about what values appear frequently in practice.

A reader who sees the numeric encoding and wonders "why this particular mapping?" — the
answer is: order-preserving for `memcmp` correctness, and compact for small common
values.

---

## 14c. Dynamic trie rebalancing based on query workload — envisioned, not implemented

The trie currently partitions on dimensions in a fixed order (defaulting to leftmost).
A harder optimisation would have nodes dynamically restructure themselves as information
about the query workload accumulates.

**The idea**: track, per node, which dimensions appear most frequently in query
predicates. If queries consistently filter on dimension 3 but the node currently splits
on dimension 1, the node could reorganize to split on dimension 3 instead — placing the
high-selectivity predicate earlier in the traversal and allowing more aggressive pruning.

**Why it is hard**:
- Requires per-node workload statistics (query counters per dimension) accumulating
  over time, not just a snapshot.
- The rebalancing operation must restructure the node without corrupting the data or
  invalidating concurrent queries.
- Policy decisions: what threshold triggers rebalancing? How to avoid thrashing if the
  query mix changes?
- The benefit depends on the query distribution being stable enough to predict — a
  random or adversarial workload could make rebalancing counterproductive.

**Related work**: conceptually similar to splay trees (self-adjusting to access
patterns) and adaptive radix trees (ART), but applied to a multi-dimensional trie
rather than a single-key index.

Not implemented. The aggregate caching optimisation (below) is a prerequisite or at
least a natural companion — nodes that cache aggregates also need to track which queries
cover them, which overlaps with the workload-tracking needed for rebalancing.

---

## 14d. Aggregate caching in trie nodes — envisioned, not implemented

SQL aggregate functions (SUM, COUNT, AVG, VARIANCE) are not yet implemented. When
they are, a trie-level caching optimisation is possible:

**The insight**: when a query's range fully covers a trie node (everything below the
node satisfies the predicate), aggregate values for the entire subtree can be computed
once and stored in the node rather than traversing every leaf on each query. This turns
an O(n) subtree scan into an O(1) cache read for fully-covered nodes.

**Incremental maintenance**: once a node is known to participate in aggregate queries,
its cached values need to be updated (or invalidated) on each insert or delete within
that subtree. The challenge is doing this without numerical drift.

**Numerical stability**: naive incremental updates to mean and variance accumulate
floating-point error. The numerically stable approaches are:
- **Welford's online algorithm** (1962): computes running mean and variance in one
  pass with good numerical properties. Suitable for incremental inserts.
- **Chan et al. parallel algorithm** (1979): combines aggregate statistics from two
  groups — needed when merging subtree caches after a trie split, or when computing
  node aggregates from child node caches.
- **Decremental updates** (removing a value from a running aggregate): harder than
  incremental, and less well-studied for numerical stability. May require recomputation
  from children rather than direct subtraction.

The specific references were researched but not recorded. Welford and Chan are the
most likely starting points.

---

## 14d. Prefix-compressed string storage — envisioned, not implemented

Within a trie node, every value stored there shares the node's path prefix by
construction — the trie structure guarantees it. Storing that prefix redundantly in
each value wastes space and comparison time.

A `prefixed_string` class was envisioned to make this transparent: it would store only
the suffix (the part beyond the shared prefix) plus a lightweight reference to the
prefix, reconstructing the full value on demand. The calling code would see a normal
string interface; the compression would be invisible.

The same mechanism extends naturally to intervals: when an interval's lower and upper
bounds share a common prefix (which they often do within a node, since they bound a
narrow range), the prefix would be stored once and only the diverging suffixes kept for
each endpoint.

This optimisation has not been implemented. The encoding design (`memcmp`-ordered,
self-terminating, hierarchically terminated) was chosen partly with this in mind —
prefix sharing is well-defined because the encoding is order-preserving and
byte-comparable.

---

## 15. Intervals on the projective real line with ±ε boundaries

Two design choices combine to make interval arithmetic uniform, with no special cases:

**1. Projective real line — ∞ is a single point, intervals wrap**

Intervals live on the projective real line RP¹, where +∞ and -∞ are the same point.
This means intervals can wrap around ∞. The key consequence: division by an interval
containing 0 always produces a single valid interval rather than two disjoint pieces.

On the extended reals, `1 / [-1, 1]` would give `(-∞, -1] ∪ [1, +∞)` — a split
result that requires special handling. On RP¹, this is a single wrapped interval
`[1, -1]` (reading "from 1, going through ∞, to -1"). Division is always defined,
always returns one interval, no exceptions, no split-result logic.

**2. ±ε boundary displacements — no open/closed flags**

Rather than tracking open vs closed endpoints with boolean flags and writing case
logic for every combination (`[a,b]`, `(a,b)`, `[a,b)`, `(a,b]`), boundaries encode
the open/closed distinction as an infinitesimal ±ε displacement from the value.
A closed lower bound `a` (meaning `x >= a`) is represented as `a - ε`; an open lower
bound (meaning `x > a`) is `a + ε`. Similarly for upper bounds.

The result: all interval comparisons reduce to a single strict `<` ordering on a
linearly ordered field. `[a,b]` is `a-ε < x < b+ε`, `(a,b)` is `a+ε < x < b-ε`,
and mixed cases fall out automatically.

`lex::basic_boundary` (interval.h ~line 120) and `qtl::basic_boundary` (~line 148)
represent this same concept in two forms:

- `lex::basic_boundary` inherits from `lex::string`: the boundary (value + ε
  displacement) is encoded as a string, suitable for persistence and lexicographic
  comparison.
- `qtl::basic_boundary` inherits from `std::pair<S,L>`: an in-memory pair for
  computation.

There is a conversion constructor in one direction. The design decouples serialisation
(`lex::`) from computation (`qtl::`): deserialise via `lex::`, compute via `qtl::`.
A `#if 0` block (~line 134) shows a disabled conversion from a tighter coupling.

---

## 16. `cmp_rule` in `expr.h` — interval notation that enables query optimisation

The comparison rule (~line 619) does more than build a comparison AST node. When it
detects the pattern `lower_bound <= x::x <= upper_bound` (where `x::x` is the special
"self" symbol representing the value being tested), it **immediately constructs an
interval literal** rather than building a subtree of two comparisons.

The `lower <= x <= upper` syntax is the natural notation for expressing an interval
value — it reads as a range constraint. The resulting `expr(op::interval, {a,b})`
representation is also what enables trie pruning: the `admits()` check can use an
interval node directly without per-row expression tree evaluation. The optimisation
is a consequence of using the right representation, not the motivation for it.
The pattern check looks for the `x::x` identifier in the fusion attribute and constructs
the interval if found; otherwise falls through to standard comparison node construction.

A reader will see what looks like a parser semantic action that is also the natural
representation for range values, which happens to be exactly what the query engine's
interval-arithmetic path needs.

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

The TODO list at the top of `radix_map.h` (~lines 13–40) documents features that were
considered and deliberately deferred in favour of single-user speed — a priority when
the code was written. `dtree`/`store` shares this priority, to a greater degree.
Notably absent:

- No `erase()` / node shrinking / coalescing allocate
- No regular expression support
- No multithreading (the `multiuser` flag and "Make multithreaded using hazard pointers"
  TODO show it was anticipated, not ruled out — per the author: "there's no reason to
  gratuitously throw away concurrent access")
- No byte-order conversion

Do not assume any of these features exist or will be added without a deliberate design
decision, but concurrent access in particular is not architecturally excluded.

---

## 23. `class model` in `store.h` — model-based validation

`class model` is a deliberately simple, unindexed database used for testing. It stores
rows in a plain container and answers queries via full linear search — no trie, no
interval pruning, no index structures. Every query visits every row.

Its purpose is model-based testing: `store` (the complex, indexed implementation) is
validated by running the same queries against both `store` and `model` and checking
that the results agree. If the trie is built or searched incorrectly, `store`'s output
will diverge from `model`'s, which is always correct by construction (it's too simple
to have index bugs).

The name `model` is the conventional term for this pattern — a reference implementation
that defines correct external behaviour, used to validate a more complex one. Without
this context, a reader might mistake it for a data-model or schema class.

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

---

## 24. `operator<<` convention — everything is printable; cleanup opportunities

**Design intent**: every type in the codebase is supposed to have an `ostream <<`
output operator, primarily to make tracing and debugging easier. `out.h` is the
dedicated home for these operators on complex types.

**Historical complexity**: implementing `operator<<` for composite and recursive
types was awkward in early C++ (pre-C++17). Some implementations may reflect that
era — verbose, manual recursion, or workarounds that are no longer necessary.

**Cleanup opportunities with modern C++**:
- **Fold expressions** (C++17, already available): can simplify `operator<<` over
  parameter packs and tuple-like types — replacing manual recursion with a single
  `(os << ... << args)` fold.
- **`std::format`** (C++20, already available): type-safe, locale-independent
  formatting; cleaner than `<<` chains with `setw`/`setprecision` manipulators.
- **`std::print`** (C++23, not yet available): wraps `std::format` with direct output;
  would replace `std::cout << std::format(...)` patterns if the standard is bumped.

Some updating may already have been done. Where `operator<<` implementations look
more complex than the type warrants, fold expressions or `std::format` may be the
right simplification. `out.h` is the first place to look.

---

## 30. OP_TABLE / parser coupling — maintenance trap and options

`OP_TABLE` in `operators.h` and the expression parser in `expr.h` are **loosely
coupled**. Adding an entry to `OP_TABLE` does NOT automatically make the parser
recognise the operator. `pow` (`^`) is the existing proof: it is in `OP_TABLE` but
has no parse rule and cannot be entered by a user.

The reasons are structural:

- **OP_TABLE** drives: `enum class op` ordinals, the `optable` dispatch array
  (eval/stringify function pointers), and the pretty-printer's parenthesization logic
  (`par()` in `tree.h`, which uses the `precedence_after` integer).
- **`expr.h` parser** is a hand-written Boost Spirit grammar. Precedence is encoded
  entirely in the **nesting structure** of named rules:
  `or_rule → and_rule → not_rule → cmp_rule → sum_rule → product_rule → pre_rule → atom_rule`.
  Each rule calls only the next-lower level as its operand, enforcing precedence by
  grammar structure. The `precedence_after` field plays no role here.

### Options for tighter coupling

**Option A — Table-driven Pratt parser (replaces Spirit)**

A Pratt parser uses a runtime table of operators with *binding powers* (precedences)
and null/left denotation functions. The `precedence_after` chain already encodes the
right information: an operator's position in the chain determines its binding power.
The `fixity` column (`left_`, `right2`, `preop`, `in2`, etc.) maps directly to Pratt's
null denotation (prefix) vs left denotation (infix/postfix) distinction.

With this approach, adding an `X(...)` line to `OP_TABLE` and a denotation function
would be sufficient to add a fully working operator — no separate grammar change.

*Pratt vs Spirit*: Spirit is declarative and composable but the grammar structure is
encoded in C++ types (template metaprogramming), making it resistant to table-driven
generation. Pratt parsers are procedural and data-driven — the table IS the grammar
for operator expressions — and are well-suited to domains where the operator set is
defined by a table. The trade-off is that Spirit grammars are more readable for
complex non-operator syntax (function calls, interval syntax, SQL fragments), while
Pratt handles operator precedence more cleanly. A hybrid — Pratt for the operator
tower, Spirit for atoms and special forms — would be natural here.

**Option B — Use OP_TABLE to populate Spirit symbol tables (partial coupling)**

Spirit already uses a `cmp_sym` symbols table for comparison operators. Extend this:
use the OP_TABLE X-macro to populate `symbols<>` tables for all operator levels, so
the mapping from operator strings to `op` enum values is always table-driven. The
grammar nesting would still be hand-written, but adding an operator to OP_TABLE would
at least make it tokenizable without a separate edit. Manual grammar rule changes would
still be needed to wire the new operator into the precedence ladder.

**Option C — Assertion / test (cheapest)**

Add a compile-time or test-time check that the set of non-zero-arity, parseable
operators in OP_TABLE matches a known set in the parser. Fails loudly when they drift.
Not true coupling, but catches the `pow` class of bug early.

Option B is the best incremental improvement given the existing Spirit codebase.
Option A is the right long-term design if the parser needs significant extension.

---

## 31. Dedup, canonical-path traversal, and parallel search

### Why dedup exists

`dedup_search` is a direct consequence of the lattice being a DAG: shared `vertex`
nodes (via `shared_ptr`) are reachable via multiple paths, so traversal can yield the
same leaf rows more than once. The current fix is a per-traversal visited set.

### Making dedup unnecessary: canonical-path traversal

Define for each shared node a *canonical parent* (e.g., the path that created it, or
the lexicographically smallest parent). During traversal:

```
visit(node, via_parent):
    if node.canonical_parent ≠ via_parent: return   // non-canonical, skip
    result = predicate.admits(node)
    if result == F: return                           // prune
    if result == T: emit all rows; return            // include all
    for child in children sorted by efficiency_criterion:
        visit(child, node)
```

Non-duplication becomes structural (one immutable field per node, one comparison per
visit) rather than algorithmic (a visited set allocated per traversal).

Maintenance cost: when a split restructures the lattice (database cracking), canonical
parents of reparented nodes must be updated. This is exactly the bookkeeping `confluence`
was intended to support.

### Choosing traversal order for efficiency

With canonical-path dedup, the order children are visited is a free policy choice,
independent of correctness. Candidates:

- **Dense subtrees first** — for K-result queries, find results sooner
- **High-selectivity first** — subtrees where the predicate is tightest, more likely
  to resolve to T or F, less likely to yield U and require recursion
- **Estimated match density** — if row counts are stored at interior nodes, use them
  to rank children

### Parallel traversal

Canonical-path traversal is naturally parallel because the "should I process this
node?" check reads an immutable field — no synchronization needed. Multiple threads
can traverse different subtrees simultaneously:

- When a U node has multiple children, dispatch them to a thread pool in
  efficiency-priority order
- Each thread checks its canonical-parent independently, skips non-canonical arrivals
  without coordination
- Only output collection requires synchronization

**Advantages**: broad queries that generate many U nodes benefit significantly on
multi-core hardware. Dispatching in efficiency order means results and early-termination
conditions are reached sooner.

**Complications to resolve before implementing**:
- The coroutine-based `recursive_generator` is inherently sequential; parallel traversal
  needs a different output mechanism (thread-safe queue, callback, or futures)
- K-result queries need a shared cancellation signal so threads stop when enough
  results have been found
- Result ordering becomes non-deterministic; post-sort or priority-queue collection
  required if order matters
- The lattice was designed single-user; none of this infrastructure currently exists
  (though concurrent access is not architecturally excluded — see note 31 on radix_map)

Tight queries that resolve to T or F at high levels see little benefit from
parallelism; the gain is real for broad queries over large datasets.
