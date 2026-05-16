# ADR-004: `std::expected` plus `assert` for error handling

**Status:** Accepted

## Context

C++ offers several mechanisms for signaling that something went wrong:
exceptions, error-code return values, output parameters, `std::optional`,
`std::expected` (C++23), `abort()` / `assert` / `std::unreachable`, and
custom Result-like types. Choosing among them is partly stylistic but
also has real consequences for performance, debuggability, and the way
error flow appears in calling code.

In an emulator, errors fall into two clearly separated categories:

1. **External failures.** Things the caller did, or things outside the
   emulator's control. Loading a ROM file that doesn't exist or is
   malformed. Encountering an MBC type we don't support. Loading a save
   state with a wrong version. These are *expected to happen sometimes*,
   and the program needs to handle them and continue running.

2. **Invariant violations.** Things that should be impossible if the
   code is correct. A memory dispatch falling off the end of its switch.
   An enum value outside its declared range. A precondition violated by
   internal calls. These represent *bugs*. The right response is to
   crash with a clear stack trace, not to "recover."

A coherent error-handling strategy distinguishes these and uses
different mechanisms for each.

## Decision

**`std::expected<T, E>` for category 1: external failures.** Functions
that can fail in expected ways return `std::expected` with a specific
error enum. The caller is expected to handle the error case explicitly
(via `if (!result)`, `value_or`, etc.). Examples:

```cpp
std::expected<std::unique_ptr<Cartridge>, CartridgeError>
make_cartridge(std::vector<std::byte> rom);

std::expected<void, SaveStateError>
GameBoy::deserialize(std::span<const std::byte> data);
```

**`assert` and `std::unreachable()` for category 2: invariant
violations.** Internal preconditions, exhaustive switches, and dispatch
fall-throughs use `assert` (debug) and `std::unreachable` (release) to
crash on bug paths. Examples:

```cpp
uint8_t Bus::read(uint16_t addr) {
    switch (address_region(addr)) {
    case Region::Rom:  return cart_->read_rom(addr);
    case Region::Wram: return wram_[addr - 0xC000];
    // ... all regions ...
    }
    std::unreachable();   // address_region() returned an unknown value
}
```

We do not use exceptions in the core. The frontend may use exceptions
for SDL3 wrapping if natural, but the core's API is exception-free.

## Alternatives considered

**Exceptions everywhere.** The classic C++ approach. `try`/`catch` at
the frontend boundary; the core throws for any error. Cons: exception
overhead in the hot path matters for the CPU dispatch (millions of
operations per second). The recovery story for "the emulator hit a bad
opcode" is "tell the user, stop the emulator" — unwinding 30 stack
frames is overkill. Modern C++ projects with low-level performance
requirements increasingly avoid exceptions in their hot paths.

**Error enum + out-parameter, classic style.** `CartridgeError
make_cartridge(std::vector<std::byte> rom, std::unique_ptr<Cartridge>&
out);` Pre-C++23 idiom. Works, but `std::expected` does the same job
with cleaner syntax and no risk of forgetting to check the return value
(since `expected` is `[[nodiscard]]` by convention).

**Single mechanism (expected everywhere, including invariants).** All
errors flow through `std::expected`. Cons: every internal call needs to
unwrap the expected and propagate; the noise obscures intent. More
importantly, it *hides bugs*. A function that "can fail" because of a
bug is indistinguishable from a function that "can fail" because of
input. Crashing on bugs is the right design.

**Single mechanism (asserts everywhere, including external errors).**
External failures crash the program. Loading a bad ROM crashes the
emulator. Obviously wrong — external failures should be graceful.

**`std::optional` instead of `std::expected`.** Works for cases where
the error has no payload, but loses the *kind* of error. Bad ROMs can
fail in multiple distinguishable ways (file not found, malformed header,
unsupported MBC), and the caller (the frontend) should be able to show
specific error messages. `std::expected<T, ErrorEnum>` preserves this.

## Consequences

**Easier:**

- Calling code that handles external errors is explicit. The compiler
  flags an unhandled `std::expected` if marked `[[nodiscard]]`.
- Bugs surface immediately and clearly. A failed assert in CI tells you
  exactly which invariant was violated and what the stack was.
- The core has no exception specifications to maintain or worry about;
  `noexcept` annotations are easy.
- Performance characteristics are predictable. No exception unwinding
  in the hot path; no implicit error-flow.

**Harder / accepted:**

- C++23's `std::expected` is recent. Compilers we care about (GCC 13+,
  Clang 16+) implement it; older compilers don't. Not a constraint for
  this project since we target C++23.
- Each subsystem with category-1 errors defines its own error enum
  (`CartridgeError`, `SaveStateError`, etc.). This is a small amount of
  housekeeping. A single shared enum would couple unrelated error
  spaces; per-subsystem enums are cleaner.
- The asymmetry between mechanisms requires judgment at each call site:
  is this a category-1 or category-2 situation? In practice this is
  obvious — anything reading external bytes is category 1, anything
  inside a single algorithm with controlled inputs is category 2.

**Future:** If the project grows to where exception-using libraries are
brought in for frontend work, exception handling at the FFI boundary
is straightforward. The core stays exception-free.
