# Nova — Language Specification & Project Report

## 1. Overview

Nova is a small, statically-typed, compiled systems language inspired by Rust, Go, and C++. Its central thesis:

> Nova replaces two of Rust's hardest compiler mechanisms with simpler equivalents. Lifetime parameters are replaced by **regions** — a reference's validity is checked against the lexical scope/arena it borrows from, not an abstract lifetime variable. Automatic destructors are replaced by treating **`free` as just another move** — use-after-free and double-free are caught by the same linear-use (move) checker that already governs ordinary ownership transfer.

The result: real memory-safety guarantees (no use-after-free, no double-free, no use-after-scope, no null, no data races within a single borrow) built from three small, testable compiler passes instead of one large one — at the cost of permitting memory leaks and requiring an explicit `free` call for heap-backed data.

Target output: x86-64 assembly, assembled/linked via the system toolchain (`as`/`ld`).

---

## 2. Keywords and reserved words

```
fn let mut return
if else match
loop break continue
region arr free extern
struct enum
true false
and or not
Option Some None
Result Ok Err
```

No `while`, `for`, `class`, `interface`, `async`, `await`, `go`, `new` (allocation is via `region.alloc_array` / `Vec.new`), `null`/`nil`.

---

## 3. Types

### 3.1 Primitives
`i8 i16 i32 i64 u8 u16 u32 u64 bool` — fixed width only, no arbitrary-precision or implicit widening. `bool` requires an explicit `true`/`false`; no truthy/falsy coercion anywhere (`if` requires a literal `bool` expression).

**Overflow policy**: debug builds trap on signed/unsigned overflow with a source-line message; release builds wrap silently (two's-complement). This mirrors the same "trap in debug, defined in release" policy used for array bounds checks, for consistency.

### 3.2 Compound types
- **Fixed array** `[T; N]` — stack-allocated, size known at compile time, bounds-checked at runtime (trap on out-of-bounds in both debug and release — a safety check, not a debug-only diagnostic).
- **Region array** `arr<T>` — allocated from an arena (`region.alloc_array<T>(n)`), size known at allocation time but not compile time. Bounds-checked identically to fixed arrays.
- **Slice** `&[T]` / `&mut [T]` — a `(pointer, length)` view over a fixed array, region array, or another slice. Zero-copy: no data is duplicated, only the view is constructed. Bounds-checked against the *slice's* length, not the parent's.
- **`Vec<T>`** — heap-backed growable array (`malloc`/`realloc`/`free` via `extern` FFI). Freed manually; see §6.
- **`str`** — an immutable `&[u8]` byte slice. No separate string type; string literals are `str`.
- **`struct`** — named product type, fields accessed with `.`.
- **`enum`** — tagged union (sum type), pattern-matched with `match`; all `match` expressions over an enum must be exhaustive (§7.3).
- **`Option<T>`** = `enum Option<T> { Some(T), None }` — replaces null.
- **`Result<T, E>`** = `enum Result<T, E> { Ok(T), Err(E) }` — replaces exceptions.

### 3.3 Reference types
`&T` (shared/read-only) and `&mut T` (exclusive/read-write) — see §5.

---

## 4. Control flow

Nova is **expression-oriented**: `if`, `match`, and blocks all evaluate to a value.

### 4.1 `if` / `else`
```nova
let x = if a > b { a } else { b }
```
Condition must be a `bool` expression — no implicit conversion from any other type.

### 4.2 The single loop construct: `loop`
There is exactly one looping keyword. All iteration patterns desugar to it.

```nova
loop { ... }                  // unconditional, exits only via break
loop i in 0..10 { ... }        // range iteration, sugar over the same construct
```

- `break` exits the loop; `break value` exits and yields `value` as the loop expression's result (loops are expressions too).
- `continue` skips to the next iteration.
- Rationale: cutting `while`/`for` shrinks the grammar (fewer statement forms to parse, type-check, and borrow-check) and removes an entire class of "which loop did you mean" stylistic inconsistency — a deliberate, opinionated design choice, not an omission.

### 4.3 `match`
Pattern matching over enums (and, trivially, over `bool`/integer literals). Must be exhaustive — see §7.3.

```nova
match shape {
    Shape.Circle(r)  => 3 * r * r,
    Shape.Rect(w, h) => w * h,
    Shape.Point      => 0,
}
```

---

## 5. Pointers, references, and borrowing

### 5.1 No raw pointers in user code
Nova exposes **no raw pointer type** to user code. All indirection goes through `&T` / `&mut T`, which compile directly to machine pointers in codegen — this is how zero-copy parameter passing and slicing are implemented, without exposing pointer arithmetic or unchecked dereference to the programmer.

### 5.2 Reference rules (the borrow checker)
1. `&x` — shared reference. Any number may coexist.
2. `&mut x` — exclusive reference. At most one may exist, and no `&x` may coexist with it, for the same root binding, within the same scope.
3. A reference cannot outlive the region or stack scope of the value it points to (the **region-escape check**, §6).
4. References are not stored inside a struct/array that escapes its creation scope — this avoids the need for lifetime parameters on struct definitions entirely (a deliberate scope-reduction versus Rust's full lifetime-parameter system).

The borrow rule is checked **per root binding**, not per field (no path-sensitive/field-sensitive aliasing analysis) — a conservative but tractable simplification: borrowing one field of a struct is treated as borrowing the whole struct for the purposes of the one-writer-xor-many-readers rule.

**Known, stated simplification**: a borrow must begin and end within a single straight-line block; borrows are not tracked as "live" across `if`/`match` branches with differing borrow states. This is the single riskiest part of the checker and is explicitly scoped down to keep it implementable in one semester.

### 5.3 Zero-copy in practice
```nova
fn sum_all(a: &[i32; 1000]) -> i32 {
    let mut total = 0
    loop i in 0..1000 { total = total + a[i] }
    total
}

fn scale(a: &mut [i32; 1000], factor: i32) {
    loop i in 0..1000 { a[i] = a[i] * factor }
}
```
Neither function copies the array; both operate through a pointer generated by the reference.

---

## 6. Memory model: regions replace lifetimes

### 6.1 Regions (arenas)
A `region` block introduces a bump-allocated arena. All region-array/allocation calls inside it return memory tied to that region; the entire region is freed in **one bulk operation** when the block closes — no per-allocation free, no fragmentation, no free-list bookkeeping.

```nova
fn main() {
    region r {
        let arr = r.alloc_array<i32>(1000)
        loop i in 0..1000 { arr[i] = i * 2 }
        process(arr)          // fine — still inside region r
    }                          // <- entire arena freed here, in bulk
    // process(arr)            // COMPILE ERROR: arr does not escape region r
}
```

### 6.2 Why regions instead of lifetime parameters
Rust's lifetime parameters (`'a`) are a general system for expressing "how long is this reference valid" independent of any specific scope structure — powerful, but the source of most of a real borrow checker's implementation complexity (variance, lifetime inference, higher-ranked trait bounds). Nova sidesteps this: **a reference's lifetime is always exactly the region or stack scope it points into.** There is no separate lifetime syntax. The same escape-check pass that validates region arrays also validates references — one mechanism, two use sites.

### 6.3 Move semantics
Non-`Copy` values (region arrays, `Vec<T>`, structs containing either) move on assignment or by-value function call. After a move, the source binding is marked consumed; any further use is a compile error.

```nova
let data = r.alloc_array<i32>(5)
let other = data      // move
data[0] = 1           // COMPILE ERROR: `data` was moved into `other`
```

### 6.4 `Vec<T>` and manual `free`
`Vec<T>` is the one heap-backed (non-arena) type, implemented via `extern` FFI over `malloc`/`realloc`/`free`. It is **freed manually**, and `free(v)` is treated by the type checker as an ownership-consuming operation — i.e., exactly like a move:

```nova
let v = Vec.new<i32>()
v.push(1)
free(v)
v.push(2)      // COMPILE ERROR: use of moved value `v`
free(v)        // COMPILE ERROR: use of moved value `v` — double-free caught at compile time
```

This is the report's second thesis point: **no destructor-insertion pass is needed.** An automatic-drop system (RAII) would require analyzing every control-flow exit edge (`return`, `break`, panic) of every scope that owns heap data — a materially harder, more error-prone pass. By making `free` an ordinary consuming function call, Nova gets full use-after-free and double-free safety by reusing the move checker unchanged. The tradeoff, stated plainly: **leaks are possible** (a `Vec` never freed simply wastes memory) but never cause memory corruption. An optional, cheap compile-time warning (not a hard error) flags `Vec` bindings still "owned" at function-scope end.

---

## 7. Compile-time checking — full pass list

The compiler performs the following passes over the AST, each independently testable:

1. **Lexing/parsing** — syntax errors.
2. **Name resolution** — undefined identifiers, scope construction.
3. **Type checking** — type mismatches; `Option`/`Result` require pattern matching to unwrap (no implicit unwrap, no null).
4. **Region/escape checking** — a reference or region-allocated value used outside the region/scope that produced it is a compile error (§6.1).
5. **Borrow checking** — the one-writer-xor-many-readers rule (§5.2).
6. **Move checking** — linear-use tracking; also governs `free` (§6.4).
7. **Exhaustiveness checking** — every `match` over an `enum` must cover all variants or include a wildcard; missing a case is a compile error.
8. **Overflow/bounds policy** — inserted as runtime-trap instructions during codegen (debug) or defined-wrap instructions (release); this is a codegen concern, not a separate static pass.

Passes 4-6 together are what the report should describe as "Nova's borrow checker" — three small, separable passes rather than one monolithic system.

---

## 8. Grammar (informal EBNF)

```
program     := item*
item        := fn_decl | struct_decl | enum_decl

fn_decl     := "fn" ident "(" params? ")" ("->" type)? block
params      := param ("," param)*
param       := ident ":" type

struct_decl := "struct" ident "{" field ("," field)* "}"
field       := ident ":" type

enum_decl   := "enum" ident "{" variant ("," variant)* "}"
variant     := ident ("(" type ("," type)* ")")?

type        := "i8" | "i16" | "i32" | "i64"
             | "u8" | "u16" | "u32" | "u64" | "bool" | "str"
             | "[" type ";" int_lit "]"          // fixed array
             | "arr" "<" type ">"                 // region array
             | "Vec" "<" type ">"                 // heap-backed growable array
             | "&" type | "&" "mut" type           // reference
             | "&" "[" type "]"                    // slice
             | "Option" "<" type ">"
             | "Result" "<" type "," type ">"
             | ident ("<" type ">")?                // struct/enum, optionally generic

stmt        := "let" "mut"? ident (":" type)? "=" expr
             | expr
             | "return" expr?
             | "region" ident block

block       := "{" stmt* "}"

expr        := literal | ident | binop | call | field_access | index
             | "if" expr block ("else" block)?
             | "match" expr "{" arm+ "}"
             | "loop" ("ident" "in" expr ".." expr)? block
             | "break" expr? | "continue"
             | "&" expr | "&" "mut" expr

arm         := pattern "=>" expr ","
pattern     := ident | ident "(" ident ("," ident)* ")" | "_"
```

---

## 9. Compiler architecture

```
source.nova
   |  Lexer                        -> tokens
   v  Parser (recursive descent)   -> AST
   v  Name resolution              -> scoped symbol table
   v  Type checker                 -> typed AST
   v  Region/escape checker        -> region-tagged AST
   v  Borrow checker                -> borrow-annotated AST
   v  Move checker (incl. free)     -> move-state-annotated AST
   v  Exhaustiveness checker        -> validated AST
   v  IR lowering                   -> 3-address SSA-lite IR
   v  Codegen                       -> x86-64 assembly
   v  as / ld (system toolchain)    -> linked binary
```

Fallback: a tree-walking interpreter over the typed AST is kept alive from the type-checking stage onward, so the project has a working end-to-end demo even if codegen is incomplete near the deadline.

---

## 10. Runtime / intrinsics (not a "standard library")

- `print(x)` — resolved per static type at compile time; no runtime dispatch.
- `panic(msg)` — prints message + source line, exits; used for overflow traps and bounds-check failures.
- `read_line(buf: &mut [u8; N]) -> i32` — wraps a libc read call; returns bytes read; truncates on overflow (documented limitation).
- `extern fn` — FFI declarations resolved at link time against libc (`malloc`, `realloc`, `free`), used only to implement `Vec<T>`.

No collections beyond `Vec<T>`, no file I/O beyond `read_line`, no networking, no formatting beyond primitive `print`.

---

## 11. Explicitly out of scope (and why)

| Feature | Why excluded |
|---|---|
| Inheritance | Rust/Go (Nova's own inspirations) omit it in favor of composition; would require vtables/dynamic dispatch touching both type checker and codegen for no safety-story payoff. |
| async/await | Requires a scheduler/runtime and function-to-state-machine lowering — roughly as much work as the rest of the compiler combined; no synergy with the region/borrow/move safety thesis. |
| Goroutines / concurrency | Same runtime-cost problem as async, plus would require `Send`/`Sync`-style trait bounds to remain "safe," which is a materially separate type-system feature. |
| General generics (beyond `Vec`/`Box`) | Full generic structs/functions add real complexity to type checking and monomorphization for limited demo value; only the two generic types actually needed are supported. |
| Self-hosting | Needs `Box<T>` (recursive types), `Map<K,V>`, and file I/O — named as the concrete, well-understood gap rather than attempted. |
| Modules (`import`/`export`) | Requires a module-resolution pass ahead of name resolution, cross-file compilation units, and visibility modifiers; no interaction with the region/borrow/move safety model that is this project's core contribution. Single-file programs only, same as CS143's COOL. |

---

## 12. Project timeline (13 weeks)

| Weeks | Milestone |
|---|---|
| 1-2 | Lexer + grammar finalized |
| 3-4 | Parser -> AST |
| 5 | Name resolution + symbol tables |
| 6-7 | Type checker (primitives, structs, enums, Option/Result) |
| 8 | Region + escape checker |
| 9 | Borrow checker + move checker (reused for `free`) |
| 10 | Exhaustiveness checker for `match` |
| 10.5 | `Vec<T>` + `extern` FFI (`malloc`/`free`) |
| 11 | IR lowering (AST -> 3-address IR); fallback interpreter kept working |
| 12 | Codegen -> x86-64 assembly |
| 13 | Integration, test suite, report/demo prep |

### Risk register
- **Borrow checker across branches** — mitigated by requiring borrows to stay within a single straight-line block (§5.2), stated explicitly as a known simplification.
- **Register allocation** — no real allocator; spill everything to stack slots, note as future work.
- **Scope creep** — inheritance/async/goroutines/full generics are cut in writing (§11), not left as ambiguous stretch goals.

---

## 13. Worked example programs

See accompanying example files for: basic loop/arithmetic, Option/exhaustive match, region-array escape error, move/use-after-move error, reference borrow-conflict error, `Vec` push + manual free + double-free error, struct-with-region-array move.
