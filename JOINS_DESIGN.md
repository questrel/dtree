# Plan: Join Implementation for qtl store

## Summary

Implement SQL-style joins as a pipeline of composable process stages.
User-facing syntax is familiar SQL (`JOIN … ON`); the planner translates it into
row-transform stages plus a dumb merge-join process that always operates on col[0].

---

## Design decisions and rationale

### 1. Composable pipe stages over join-aware iterators

**Decision**: A join is assembled from independent processes connected by pipes:
`left-query | transform-left | sort | merge-join | transform-left | sort | right-query`

**Rationale**: The existing store already produces sorted `row` streams via coroutine
generators. A pipe stage has a well-defined interface (reads rows from stdin, writes
rows to stdout) with no knowledge of its neighbours. This means:
- Each stage can be tested and developed independently
- 3-way and N-way joins compose naturally (pipe pairs)
- Sort elimination is a planner optimisation, not an algorithmic special case
- The same infrastructure serves aggregation, dedup, projection, etc.

Alternative considered: join-aware iterator inside the store (cppcoro-style nested
generators). Rejected because it entangles join logic with the trie data structure and
makes parallelism harder to achieve.

---

### 2. Join key always in col[0]; planner emits transforms

**Decision**: The merge-join process compares col[0] of the left stream with col[0]
of the right stream. The planner rewrites `ON f(T1.x) = g(T2.y)` into:

```
T1-query  →  project [f(x), rest…]  →  (sort?)  ─┐
                                                    ├─ merge-join  →  unpack
T2-query  →  project [g(y), rest…]  →  (sort?)  ─┘
```

**Rationale**: Keeping the merge-join process dumb means it has one well-defined
contract and zero configuration surface. All the complexity of "which column is the
join key" lives in the planner, where it can be reasoned about statically.

Multi-column join keys (`ON T1.a=T2.a AND T1.b=T2.b`) are handled by packing them
into a single synthetic col[0] using the existing (planned) tuple/list expression.
This fits the existing architecture: packing expressions are independently useful for
projection, GROUP BY, and complex sort keys anyway.

Alternative considered: parameterise merge-join with key column indices.
Rejected because it leaks planner knowledge into the runtime stage and complicates
the pipe interface.

---

### 3. Sort elimination when trie order matches join key

**Decision**: The planner checks whether the trie traversal order for a query already
delivers rows sorted by the transform output. If so, the sort stage is omitted.

**Rationale**: The trie always delivers rows in lexicographic path order. For a simple
equi-join `ON T1.name = T2.name` where `name` is the leftmost trie dimension, both
sides already arrive sorted by the join key — no sort needed. This is the common case.

The planner detects this by checking: is `f` a simple column projection? Is that
column a trie dimension? Is the trie traversal left-to-right on that dimension?
If all yes: elide the sort stage.

For more complex `f` (computed expressions, packed tuples), a sort stage is always
inserted. Adding it is cheap in the pipe model.

---

### 4. Cross-row predicates (`h(T1.x, T2.y)`) fall back to cross-product + filter

**Decision**: If the `ON` clause cannot be split into `f(T1.cols) = g(T2.cols)`, emit
a cross-product stage followed by a filter stage.

**Rationale**: Genuinely cross-row predicates (e.g., range joins, regex matches across
tables) are uncommon and inherently O(N·M) in the worst case. Making cross-product a
first-class pipe stage (rather than a special case in merge-join) keeps the algorithm
uniform. The planner tries the decomposition first; if it fails, it emits cross-product.

Future optimisation: detect `f(T1.x) <= T2.y AND T2.y <= g(T1.x)` patterns (range
joins) and convert to a trie-range probe, but this is not needed for the initial
implementation.

---

### 5. SQL surface syntax and column scoping

**Decision**: Standard SQL join syntax:

```sql
SELECT T1.col1, T2.col2
FROM T1 JOIN T2 ON T1.name = T2.name
WHERE T1.col3 > 5
```

Column references in `ON` and `SELECT` are scoped by table alias prefix
(`T1.name` → column `name` in table `T1`). The parser resolves these using the
existing alias→`_col` mapping system, extended to support `table.alias` notation.

**Rationale**: Familiar syntax reduces learning curve. The scoping system already
distinguishes table names as row prefixes; extending it to qualify column references
in expressions is a natural continuation.

`LEFT JOIN`, `RIGHT JOIN`, `FULL OUTER JOIN` follow the same planner path, with the
merge-join process parameterised only on null-handling behaviour (emit unmatched left
rows / right rows). The transform and sort stages are unchanged.

---

## Implementation stages

### Stage 0 — Prerequisites (may already be partially done)

- **Row slice/project expression**: `slice(row, col_list)` or `row[i:j]` expr that
  projects a subset of columns. Needed to build the transform stage output.
- **Tuple/pack expression**: packs multiple values into a single `lex::scalar` for
  multi-column join keys. Needed for compound `ON` clauses.
- These are planned independently; join implementation blocks on them.

### Stage 1 — Pipe row serialisation

Define a binary (or line-oriented text) format for transmitting `row` objects through
a pipe. Options:
- Reuse existing `operator<<` / `operator>>` on `row` (text, already works for SQL I/O)
- New compact binary format (faster but requires new codec)

Recommendation: start with text format (reuse existing), switch to binary if profiling
shows it matters.

Files: `qtl/out.h` (already has row output); add corresponding row parser if absent.

### Stage 2 — Sort process

A standalone process that reads rows from stdin, sorts by col[0], writes to stdout.
Uses `std::sort` on an in-memory vector (acceptable for result sets that fit in RAM).

For large result sets: external sort (write sorted runs to temp files, merge). Defer
until needed.

### Stage 3 — Transform process

A standalone process that reads rows and applies an `expr` to each, writing the
transformed row to stdout. The expr is supplied via command-line argument (serialised
expression AST) or a shared descriptor.

This is essentially `store` query output piped through an expression evaluator — the
existing expr evaluation machinery applies directly.

### Stage 4 — Merge-join process

Reads two sorted row streams (left on fd 3, right on fd 4, or via named pipes).
Compares col[0] of each; emits `left_row + right_row` for matching pairs.

```
while not (left_done or right_done):
    if left[0] < right[0]: advance left
    elif right[0] < left[0]: advance right
    else:  # equal — emit all pairs with this key
        collect all right rows with current key (may be multiple)
        for each left row with current key:
            for each collected right row: emit left + right
        advance both past current key
```

For LEFT JOIN: emit `left_row + null_row` when left has no match.
For cross-product fallback: nested loop with no key comparison, emit all pairs.

### Stage 5 — Planner (SQL → pipe graph)

Extend `sql.h` `select_clause` to:
1. Detect `FROM t1 JOIN t2 ON expr` syntax
2. Parse the `ON` expression and attempt to decompose into `f(T1) = g(T2)`
3. Emit the pipe graph: query processes + transforms + (optional sorts) + merge-join
4. Wire together via `pipe(2)` / `fork(2)` / `exec`

The planner output is a process tree, not a single coroutine. The top-level process
waits for the merge-join child and reads its output as the query result.

### Stage 6 — Column alias scoping for joined queries

Extend the alias resolution pass in the SQL parser to handle `T1.colname` and
`T2.colname` as scoped references, resolving to the correct `_col` index within each
table's row schema.

After a join, the output row is `T1_row + T2_row`. Output column references in
`SELECT` need to know the offset of `T2`'s columns in the concatenated row.

---

## Files to modify

| File | Change |
|---|---|
| `qtl/sql.h` | Parse `JOIN … ON`; emit pipe graph; extend alias scoping |
| `qtl/store.h` | Add `merge_join` / `cross_join` generator functions (for in-process use) |
| `qtl/expr.h` (or new file) | `slice`, `pack` expressions for row projection/packing |
| `qtl/out.h` | Row deserialisation (if not already present) |
| New: `qtl/join.h` | Merge-join and cross-product process entry points |
| New: `qtl/sort_rows.h` | Sort-rows process entry point |

---

## Open questions

1. **In-process vs out-of-process**: The pipe/fork approach gives parallelism but adds
   serialisation cost. For small result sets, an in-process coroutine join (no fork)
   may be faster. Should the planner choose based on estimated cardinality, or always
   use one model?

2. **Expression serialisation**: How is the transform expr passed to the child process?
   Command-line string (reparse), shared memory descriptor, or inherited fd? Shared
   memory is fastest but most complex.

3. **Null handling**: What is the `lex::scalar` representation of SQL NULL for outer
   joins? Currently `lex::scalar` may not have a null state.

4. **Sort stability**: The merge-join assumes total order on col[0]. If col[0] values
   are equal across multiple rows, are all combinations emitted? (Yes, per the
   algorithm above — this is the standard behaviour.)

5. **Trie traversal order control**: The note that "the path can be traversed from any
   column" suggests the trie supports non-leftmost traversal. If so, sort elimination
   applies more broadly — worth documenting which traversal orders are available.
