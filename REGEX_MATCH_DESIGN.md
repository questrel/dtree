# Regex Match Operator — Design Notes

*Status: design only, no code changes made*

---

## Goal

Add a `matches` operator to the expression tree so queries can filter string-valued
columns against a user-supplied regular expression. Two evaluation contexts:

| Context | Input | What we need |
|---|---|---|
| Full row eval (`satisfies`) | One concrete string value | `std::regex_match(value, pattern)` |
| Trie pruning (`admits`) | An interval `[lo, hi]` of strings | Can *any* string in the range match? |

The full-row case is straightforward. The admits() case requires evaluating a regex
against an interval — the interesting part.

---

## Evaluating a regex against an interval

### Why it matters

`admits()` is the trie pruning predicate. When the store descends into a subtree, it
asks: could any row in this subtree satisfy the query? If the predicate is a regex and
the answer is definitively "no", the entire subtree can be skipped.

### Theoretical basis

Strings in a lexicographic interval `[lo, hi]` form a **regular language** — so
intersecting them with a regex is the intersection of two regular languages, which is
decidable. This means in principle we can determine exactly whether the intersection is
empty (prune) or non-empty (keep).

### Approximation using `std::regex match_partial` (proposed first implementation)

**Key observation**: every string in `[lo, hi]` begins with the common prefix of `lo`
and `hi`. If the regex cannot match *any* string beginning with that prefix, the entire
interval is disjoint from the regex language — prune.

`std::regex_constants::match_partial` (standard C++) returns true if the input string
is a prefix of *some* string that matches the pattern.

```
common_prefix = longest prefix shared by lo and hi
if match_partial(common_prefix, pattern) == false:
    → kleen::F  (prune subtree)
else:
    → kleen::U  (unknown, don't prune)
```

**Properties:**
- **Sound**: never prunes a node that contains a matching row
- **Incomplete**: may fail to prune when `lo` and `hi` diverge early (short common prefix)
- **Cheap**: one regex call per node, no custom automaton code

**Example where it works well**: `col MATCHES '^apple.*'` with a node covering
`['apple_a', 'apple_z']`. Common prefix `'apple'`, match_partial true → don't prune
(correct). Node covering `['banana_a', 'banana_z']`, match_partial on `'banana'` →
false → prune (correct).

**Example where it misses a pruning opportunity**: node covering `['aardvark', 'zebra']`.
Common prefix is `''` — the approximation can't prune even if no string in the range
matches. The exact approach (below) handles this.

### Exact approach: product automaton emptiness test (deferred)

Build a product NFA of:
1. **Regex NFA** (Thompson construction from the pattern)
2. **Range automaton** for `[lo, hi]` (5-state DFA: BOTH, LO_ONLY, HI_ONLY, FREE, REJECT)

Run BFS on the product state space from `(regex_start, BOTH)`. If any accepting pair
`(regex_accepting, {BOTH|LO_ONLY|HI_ONLY|FREE})` is reachable: don't prune. Otherwise:
prune.

This is complete (never misses a pruning opportunity) but requires implementing a
small NFA library (~300 lines). Deferred until the match_partial approximation proves
inadequate in practice.

The range automaton has this transition structure (at byte position k):

| Current state | Byte b | Next state |
|---|---|---|
| BOTH | b < lo[k] | REJECT |
| BOTH | b == lo[k] == hi[k] | BOTH |
| BOTH | b == lo[k] < hi[k] | LO_ONLY |
| BOTH | lo[k] < b < hi[k] | FREE |
| BOTH | b == hi[k] > lo[k] | HI_ONLY |
| BOTH | b > hi[k] | REJECT |
| LO_ONLY | b < lo[k] | REJECT |
| LO_ONLY | b == lo[k] | LO_ONLY |
| LO_ONLY | b > lo[k] | FREE |
| HI_ONLY | b > hi[k] | REJECT |
| HI_ONLY | b == hi[k] | HI_ONLY |
| HI_ONLY | b < hi[k] | FREE |
| FREE | any | FREE |

The product automaton operates on the decoded (logical) strings, since the lex::scalar
encoding is memcmp-order-preserving — the logical ordering and the encoded ordering
match.

---

## What needs to be added

### 1. `qtl/operators.h` — new op

```cpp
X(matches,2,~=,in2,less_equal)
```

One line in OP_TABLE. This auto-generates the `op::matches` enum value and the entry
in the optable dispatch array.

### 2. `qtl/store.h` — eval function

New eval function for `op::matches`, registered in the optable alongside `equal_to`,
`less`, etc. The function:
- Extracts the regex pattern from the right operand (a string literal — point interval)
- If left is a point interval (exact value): `std::regex_match(value, re)` → kleen::T/F
- If left is a range interval: match_partial on common prefix → kleen::F or kleen::U

The exact API for decoding `lex::scalar` to `std::string` and accessing interval
lo/hi bounds needs to be confirmed by reading `interval.h` and `string.h` before
writing the eval function.

### 3. `qtl/expr.h` — parser

Add `~=` as an infix comparison operator in the expression parser, at the same
precedence level as `==`, `<`, etc.

### 4. `qtl/sql.h` — SQL keyword

Add `MATCHES` keyword:
```sql
col MATCHES 'pattern'
```
Equivalent to `col ~= 'pattern'` in expression syntax. Follow the existing pattern for
`==` and `<=` in the Spirit X3 grammar.

---

## What needs to be confirmed before implementing

1. **`lex::scalar` → `std::string` decoding**: is there a direct conversion, or must
   we go through `operator<<` and capture output? Check `string.h` and `out.h`.

2. **Interval lo/hi accessor names**: `.lo()`, `.hi()`, `.lower()`, `.upper()`, or
   something else? Check `interval.h`.

3. **kleen→operand_t**: how do existing comparison eval functions (e.g. `equal_to`)
   return kleen::T / kleen::F? The `operand_t` is `interval<lex::scalar>` — there must
   be a convention (e.g. `interval::from_bool` or constructing a `[1,1]` vs `[0,0]`
   interval). Mirror that pattern exactly.

4. **optable registration syntax**: confirm the exact initialiser syntax used for
   other ops in the static optable array/map in store.h.

---

## Non-goals / deferred

- **Full product-NFA admits()**: implement if match_partial proves insufficient
- **Regex substitution** (`sub`, `gsub`): separate feature, separate design
- **Regex anchoring semantics**: whether `MATCHES` is anchored (`^...$`) or
  substring (`.*...*`) — needs a decision. `std::regex_match` is anchored;
  `std::regex_search` is substring. Recommend `regex_match` (anchored) as the
  default, matching SQL `LIKE` semantics, with a separate `CONTAINS` or `SEARCH`
  op for substring.
