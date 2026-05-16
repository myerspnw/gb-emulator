# CLAUDE.md

Standing context for Claude Code when working in this repository.
Read this file before suggesting any code changes.

## What this project is

A Game Boy (DMG) emulator written in modern C++23. The repo is a
deliberate portfolio piece — code quality, documentation, and
architectural clarity are first-class concerns alongside correctness.

For the full picture, read in this order:

1. [`README.md`](README.md) — short overview
2. [`docs/architecture.md`](docs/architecture.md) — design document,
   the source of truth for "what should this code look like"
3. [`docs/decisions/`](docs/decisions/) — ADRs capturing the rationale
   behind every significant choice
4. [`CONTRIBUTING.md`](CONTRIBUTING.md) — build, style, and workflow

Do not propose architectural changes without checking the architecture
doc and the ADRs. Decisions there are intentional. Several "obvious"
alternatives have been considered and explicitly rejected — those are
documented in the ADRs' "Alternatives considered" sections.

## Current focus

> Update this section as the project moves through phases.

**Phase:** v1 — "the core works"

**Active work:** Initial project setup is complete. The build system,
CI, and documentation are in place. Implementation has not yet started.

**Next milestone:** Cartridge loader. The first real code will be the
ROM header parser, the no-MBC implementation, and the factory function
that produces a `std::unique_ptr<Cartridge>` from ROM bytes.

## Code conventions

Enforced by tooling where possible, but worth stating explicitly:

### Style

- C++23. Modern idioms (`std::expected`, `std::span`, designated
  initializers, `std::bit_cast`, `constexpr` lookup tables) where they
  fit naturally.
- Naming: see `.clang-tidy`. CamelCase types, lower_case functions and
  variables, trailing underscore on private members.
- Formatting: `.clang-format` is the source of truth. Run
  `clang-format -i` on any file before committing.
- Static analysis: `.clang-tidy` configured for `bugprone-*`,
  `clang-analyzer-*`, `cppcoreguidelines-*`, `modernize-*`,
  `performance-*`, `readability-*` (with a curated set of disabled
  checks).

### Strong preferences

- Always brace single-line conditionals and loops.
- `const auto&` in range-for loops unless mutation is intended.
- Prefer `unordered_*` containers when ordering isn't needed.
- No `using namespace std;` anywhere.
- No raw `new` / `delete`. Use `std::make_unique` / `std::make_shared`.
- Pre-increment (`++i`) over post-increment (`i++`).
- `#pragma once` over include guards.
- Strict warnings, treated as errors. Code must compile clean.
- `[[nodiscard]]` on functions whose return value carries information
  the caller must act on (errors, queried state).

### Error handling

Two categories, two mechanisms (see ADR-004):

- **External failures** (malformed ROM, unsupported MBC, corrupt save
  state, missing file) → `std::expected<T, ErrorEnum>`. Each subsystem
  defines its own error enum (`CartridgeError`, `SaveStateError`, etc.).
- **Invariant violations** (impossible dispatch case, internal
  precondition broken) → `assert` in debug, `std::unreachable()` in
  release. Crash on bugs; don't hide them.

No exceptions in the core. Frontend may use them at the SDL3 boundary
if natural.

### Architecture boundaries

These are enforced by the build system but stated here too because
violations are easy to introduce and hard to undo:

- `gbe_core` does not depend on SDL3, the filesystem, or any I/O.
  It operates on byte buffers handed in by the frontend.
- `gbe_frontend` depends on `gbe_core` but never the reverse.
- The `Bus` is the spine. Peripherals don't reference each other;
  they communicate via the Bus.
- The CPU is the only subsystem that calls `Bus::read` / `Bus::write`.
- Peripherals hold a `Bus&` only to call `request_interrupt`.

If a proposed change would require `gbe_core` to depend on SDL3, the
filesystem, or `std::chrono::system_clock::now()`, **stop and ask**.
That's either a real architectural change requiring discussion or a
sign the change should live in the frontend.

### Performance posture

- The emulator runs much faster than real-time on modern hardware. We
  are not performance-constrained for v1.
- The CPU dispatch (switch on opcode in `Cpu::step_instruction`) is
  the hot path. Treat changes there with extra care.
- Performance work belongs in benchmarks (`Google Benchmark`), not in
  speculative optimization. Measure before changing.
- Cache locality matters for the PPU's tile/sprite data; if you're
  refactoring there, run the framebuffer-hash snapshot tests to
  ensure correctness, and the benchmark suite to ensure no regression.

### Testing

- Unit tests in `tests/unit/`, using GoogleTest.
- Integration tests in `tests/integration/`, using `HeadlessRunner` to
  run Blargg / Mooneye / Acid ROMs against the emulator core.
- Anything visible from `gbe_core`'s public API gets a unit test before
  it's "done." Anything visible from running a game gets an integration
  test if a suitable test ROM exists.
- Tests link `gbe_core` only — never `gbe_frontend`. The headless
  testability of the core is an architectural invariant; tests
  enforce it.

## Working with the build

Common commands (also in `CONTRIBUTING.md`):

```bash
cmake --preset debug          # configure debug build
cmake --build --preset debug  # compile
ctest --preset debug          # run tests

cmake --preset asan           # configure with ASan + UBSan
cmake --build --preset asan
ctest --preset asan

cmake --preset clang          # release build with clang (CI matrix coverage)
```

ccache and lld are auto-detected and used if available.

When making changes, prefer to verify with `asan` for anything touching
memory access patterns (the CPU and the PPU especially). The asan
preset catches use-after-free, buffer overflows, and undefined behavior
that release builds happily run through.

## When in doubt

- Match the existing style and patterns in the file you're editing.
  Consistency beats local optimization.
- If a change would touch more than one ADR's territory, write the
  change as a draft and ask before applying. Cross-cutting changes
  are sometimes right but always worth a second look.
- Small, focused PRs over large refactors. Each PR should be a single
  Conventional Commit type (`feat`, `fix`, `refactor`, etc.) with a
  clear scope.
- Tests come with code, not after. A new feature lands with its tests
  in the same PR.
- The architecture doc and ADRs are living documents. If a change
  invalidates something in them, update them in the same PR.

## What not to do

- Don't add dependencies without discussion. The current dependency
  set (SDL3, fmt, spdlog, GoogleTest; Dear ImGui in v1.5) is
  deliberate. New dependencies need a clear justification.
- Don't propose threading the emulator core. ADR-002 explains why
  it's single-threaded; revisit only if v2 audio quality demands it.
- Don't propose T-cycle accuracy. ADR-003 explains why M-cycle is
  the permanent target.
- Don't add a CGB code path. ADR-001 explains the DMG-only scope.
- Don't introduce exceptions into the core. ADR-004 explains the
  error-handling model.
- Don't suggest `using namespace std;`, raw `new`/`delete`, or other
  patterns the style guide rules out.
- Don't generate code with TODO/FIXME markers without explaining what
  they're waiting on. Either leave a clear context comment or open
  an issue and reference it.

## Reporting

When you've made a non-trivial change, end your message with a short
summary of:

- What changed
- What tests cover it
- What you intentionally left out and why
- Anything that needs human review or follow-up

This makes self-review (the next-day PR check) faster and more
reliable.
