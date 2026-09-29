# Writing the Nova grammar: a guide

These notes cover what you need to write the grammar yourself. They don't give
you the grammar. The informal EBNF in section 8 of `ORIGINAL_SPECIFICATION.md`
is a starting point, but it has gaps. The sections below point them out.

---

## 1. What the grammar is for

The lexer turns characters into tokens. The grammar says which *sequences of
tokens* count as a valid program, and what shape (tree) they make.

The grammar's terminals are exactly your lexer's token types (`token.hpp`):
`IDENT`, `INT`, `STRING`, the keywords, and the operators and punctuation. If
something isn't a token, the grammar can't mention it.

**Check this first:** `i32`, `bool`, `str` and `Vec` are *not* keywords in your
lexer. They come out as `IDENT`. `_` is also an `IDENT`. That affects how you
write the `type` and `pattern` rules.

---

## 2. Notation (EBNF)

```
'fn'          terminal (a token)
name          nonterminal (another rule)
a b           a then b
a | b         a or b
[ a ]         a is optional
{ a }         a repeats zero or more times
( a )         grouping
```

A rule looks like `param = IDENT ':' type ;`

---

## 3. Recursive descent: the main constraint

The spec says the parser will be recursive descent. In that style **each rule
becomes one function**, and at every `|`, `[ ]` or `{ }` the function looks at
the *next token* to decide which way to go.

So the grammar has to answer: **"at every choice point, can I pick the right
branch from one token?"** (That's what LL(1) means.)

Two things break it:

1. **Left recursion.** `expr = expr '+' term` makes `parse_expr()` call itself
   forever. Write it as repetition instead: `expr = term { '+' term }`.
2. **Shared prefixes.** In `a = X Y | X Z`, both branches start with `X`.
   Factor it out: `a = X ( Y | Z )`.

When a spot really can't be decided from one token, you're allowed to peek two
tokens ahead, or to try one option and back up if it fails. Write those spots
down explicitly.

---

## 4. Suggested order

1. **Top level:** `program`, then `item` (fn / extern fn / struct / enum).
2. **Declarations:** function signatures, params, struct fields, enum variants.
3. **Types.**
4. **Blocks and statements.**
5. **Expressions,** one rule per precedence level (see section 5).
6. **Patterns** for `match`.
7. **Test it** by hand-parsing `examples/test_program.nova` line by line. Every
   line should fit a rule. When one doesn't, the grammar is missing something.

---

## 5. Encoding precedence

Precedence isn't a separate table. It's built into the **layering of rules**.
Each level only calls the level below it:

```
lowest  ->  or_level = and_level { OR_OP and_level }
            and_level = ...
            ...
highest ->  unary -> postfix -> primary
```

The rule that's called *last* binds *tightest*. `{ op x }` gives left
associativity. `[ op x ]` (only one allowed) makes an operator
**non-associative**, so `a < b < c` becomes a syntax error.

**You decide:** the order of `or`/`||`, `and`/`&&`, the comparisons, `|`, `&`,
`+ -` and `* / %`. Also where `not` sits: high like Rust's `!`, or low like
Python's `not`? Look at `a == b and not (a != b)` in the example and ask whether
the parentheses there would be needed.

---

## 6. Problems you'll run into in Nova

Each of these is a real ambiguity in your token set. You need a decision for
each one, written as a grammar rule or a note next to it.

**P1. No statement terminator.** The examples don't use `;`, and newlines
aren't tokens. So how does the parser know where a statement ends? Consider:
```
print(x)
(a + b)
```
Is that one call `print(x)(a + b)` or two statements? What about `return`
followed by a new line that starts with an identifier? (One option: `;` is an
optional separator and the parser is greedy. Another: require `;`. There are
others.)

**P2. `if x { ... }` vs struct literals.** `Point { x: 1, y: 2 }` is
`IDENT '{'`. So is the `x {` in `if x { ... }`. How does the parser tell them
apart? (Rust has a well-known answer to this. Look up "struct literal in
condition position".)

**P3. `r.alloc_array<i32>(10)` vs less-than.** After `.name`, a `<` could open
type arguments or mean less-than (`p.x < y`). One token isn't enough to
decide. Hint: think about what `a < b > (c)` means if comparisons are
non-associative.

**P4. `&` and `|` do double duty.** `&` is both "take a reference" and binary
and. Where in the expression the token shows up decides which one it is. Work
out which rule each meaning belongs to.

**P5. Keywords inside expressions and patterns.** `Some`, `None`, `Ok`, `Err`,
`free`, `true` and `false` are keywords, so a generic `IDENT` rule won't match
them. Each one needs its own mention in `primary` and/or `pattern`.

**P6. Things the example uses that section 8 doesn't cover:**
- `extern fn` with no body
- struct literals `Point { x: 1, y: 2 }`
- array literals `[1, 2, 3]`
- assignment `total = ...`, `a[i] = ...` (statement or expression?)
- enum payload labels `Circle(radius: i32)`, where section 8 only allows bare types
- qualified patterns `Shape.Circle(rad)`, where section 8 only has `ident(...)`
- trailing commas in enums and match arms
- section 8 has a typo: `"ident"` in the loop rule should be `ident`
  (a token, not a literal keyword)

**P7. Things nobody has decided yet:**
- negative numbers: `INT` has no sign, so is there a unary `-`?
- `else if` chains
- do `and`/`or` and `&&`/`||` both stay? Two spellings for one operator is
  extra grammar and extra tests for no benefit.
- can a match arm body be a `{ block }`?
- where does a block's value come from (the last expression?), and does `;`
  change that the way it does in Rust?

**P8. Block-like expressions at the start of a statement.**
```
if done { a } else { b }
-1
```
If `if` is an expression and `-` is binary, that parses as `(if ...) - 1`.
Rust's rule: at the start of a statement, `if`/`match`/`loop`/`{` end at their
closing `}`. Do you want that?

---

## 7. How to tell you're done

- Every token type in `token.hpp` shows up somewhere (or you know why it doesn't).
- No rule is left-recursive.
- At every `|`, `[ ]` and `{ }`, the branches start with different tokens, or
  there's a note explaining how the tie is broken.
- `examples/test_program.nova` and every code snippet in the spec parse by hand.
- A few invalid programs fail where you'd expect (`a < b < c`,
  `let = 5`, `fn (x) {}`).

When you have a draft, bring it back and I'll check it against these.
