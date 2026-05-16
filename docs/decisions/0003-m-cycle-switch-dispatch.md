# ADR-003: M-cycle CPU accuracy with switch/case dispatch

**Status:** Accepted

## Context

Two related decisions about the CPU implementation: the timing accuracy
level, and the opcode dispatch strategy.

**Timing accuracy.** The Game Boy CPU runs at 4.194304 MHz. The accuracy
levels emulator authors choose between are described in the architecture
document under "A note on timing accuracy." Briefly: instruction-level
(execute whole opcode, advance time by total duration), M-cycle (advance
per memory access, 4 T-cycles each), or T-cycle (advance one cycle at a
time). M-cycle is the mainstream choice for emulators targeting
commercial-game compatibility; T-cycle is for accuracy-research emulators.

**Opcode dispatch.** The Game Boy has 256 base opcodes plus 256
CB-prefixed opcodes. Implementations choose between a giant switch
statement, a function-pointer lookup table, or a code-generation approach
(parsing an opcode metadata table and generating both the implementation
and tests).

These decisions are coupled. M-cycle accuracy pairs naturally with
switch dispatch (each `case` ticks the bus the right number of times,
in-line). T-cycle accuracy pulls toward a state-machine per instruction,
which fits a different code shape.

## Decision

**M-cycle accuracy.** The CPU advances time per memory access. Each
`Bus::read` and `Bus::write` ticks the bus by 4 T-cycles, advancing all
peripherals. Multi-cycle instructions (`PUSH`, `CALL`, conditional
branches) tick additional cycles inside the opcode handler as needed.

**Switch/case opcode dispatch.** A single `switch (opcode)` in
`Cpu::step_instruction()` covers all 256 base opcodes. A nested switch
inside the `0xCB` case handles CB-prefixed opcodes.

We do not plan to upgrade to T-cycle accuracy in v2 or any later version.
T-cycle is explicitly out of scope.

## Alternatives considered

### For accuracy

**Instruction-level accuracy.** Simplest. Fails Blargg's
`instr_timing.gb` and several `mem_timing` tests. Means commercial games
that rely on subtle timing (most timer-dependent code, some PPU register
poking) can desync. Not viable for the showpiece's "verified by Blargg"
bar.

**T-cycle accuracy.** Highest fidelity. Requires every multi-cycle
instruction to be decomposed into a state machine that progresses one
cycle at a time. Each instruction becomes ~3-5× more code. Interrupts
mid-instruction are modeled precisely (which matters for a handful of
Mooneye tests). The complexity is real and the showpiece bar doesn't
require it. ROI is low.

### For dispatch

**Function pointer table.** `void (*handlers[256])(Cpu&);` indexed by
opcode. Each case becomes a separate function. Slightly less code in any
one place but the dispatch logic is distributed across 256 functions.
Worse for debugger stepping (the call indirection hides the dispatch),
worse for cross-instruction tweaks (touching how the bus is ticked
requires editing many small functions). Negligible performance difference
under optimization.

**Computed goto (labels-as-values).** GCC/Clang extension that can be
slightly faster than switch. Non-standard, doesn't work with MSVC,
buys nothing visible. Not worth the portability hit.

**Code generation from an opcode table.** Parse a JSON or text file
describing every opcode and generate the implementation. Lower-friction
for adding instructions correctly. But: extra build step, generated
code is harder to read, and 512 opcodes is a one-time write — not a
maintenance burden. Several open-source emulators use code generation
and it works, but the cost isn't paid back at this project's scale.

### A note on benchmarking

We considered shipping multiple dispatch implementations with a benchmark
comparing them — a fun talking point given the project's performance
focus. Decided against: it's a v2 candidate if there's time, but adding
two implementations to maintain (when one is the production path and the
other exists for measurement) is overhead without compounding value. Done
once as a microbenchmark blog post or README section, it's a stronger
artifact than two code paths.

## Consequences

**Easier:**

- The whole CPU lives in one file (`cpu.cpp`) plus opcode helpers. A
  reviewer can scroll through and see every instruction's handling.
- Debugging is mechanical: set a breakpoint in `Cpu::step_instruction`,
  see exactly which case fires.
- The compiler generates a jump table from the switch automatically
  under `-O2`; performance is competitive with function pointer tables.
- M-cycle accuracy passes the showpiece-defining test suite (Blargg
  full + dmg-acid2 + curated Mooneye).
- Pairs naturally with the rest of the architecture — Bus::tick(n)
  works in M-cycle granularity.

**Harder / accepted:**

- Some Mooneye tests will fail. These probe T-cycle-precise behaviors
  (interrupt-timing edge cases, OAM bug at specific dots, instruction
  fetch quirks). Real games don't depend on them.
- The CPU class file will be long (~1500 lines including all instruction
  helpers). Splitting opcodes into multiple files is possible but loses
  the single-file readability that motivates the switch. The size is
  acceptable.
- A future contributor wanting to add T-cycle accuracy would face a
  substantial rewrite of the CPU. We accept this; it's outside the
  project's goals.

**Future:** T-cycle accuracy is permanently out of scope. If a serious
reason ever arose to revisit (which is unlikely), this ADR would be
superseded by a "T-cycle CPU rewrite" ADR.
