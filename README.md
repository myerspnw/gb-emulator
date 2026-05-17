# gb-emulator

[![CI](https://github.com/myerspnw/gb-emulator/actions/workflows/ci.yml/badge.svg)](https://github.com/myerspnw/gb-emulator/actions/workflows/ci.yml)
[![Lint](https://github.com/myerspnw/gb-emulator/actions/workflows/lint.yml/badge.svg)](https://github.com/myerspnw/gb-emulator/actions/workflows/lint.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-green.svg)](LICENSE)

An M-cycle-accurate Game Boy (DMG) emulator written in modern C++23.
The architecture is the point: a deterministic core with zero I/O
dependencies, a thin SDL3 frontend, and validation against the
standard hardware test ROM suite (Blargg, `dmg-acid2`, selected
Mooneye) running in CI on every commit.

## Status

🚧 **In active development.** The foundation is in place — build
system, CI matrix, architecture documentation, ADR record.
Implementation begins with the cartridge loader. A screenshot lands
here once the PPU renders its first frame.

> _Screenshot placeholder — coming with v1._

## Design highlights

- **M-cycle CPU accuracy.** The CPU advances time per memory access
  rather than per opcode, which is the granularity needed to pass
  Blargg's `instr_timing` and `mem_timing` suites. T-cycle accuracy is
  explicitly out of scope
  ([ADR-003](docs/decisions/0003-m-cycle-switch-dispatch.md)).
- **Strict core/frontend separation, enforced by the build.** The
  emulation core does not link SDL3 or any logging library. A CTest
  test (`cmake/CheckCoreBoundary.cmake`) scans `src/core/` on every
  run and fails if any `#include` reaches into the frontend, SDL3, or
  spdlog — the architecture invariant is a build-system property, not
  a convention.
- **Deterministic by construction.** Single-threaded, no system clocks
  in the core, no entropy used anywhere. Same ROM + same input
  sequence produces bit-identical framebuffers across runs and across
  machines, which is what makes the integration test suite possible
  ([ADR-002](docs/decisions/0002-single-threaded-core.md)).
- **Two-mechanism error model.** `std::expected<T, E>` for recoverable
  errors crossing the public API; `assert` / `std::unreachable` for
  invariant violations. No exceptions in the core
  ([ADR-004](docs/decisions/0004-expected-and-assert.md)).
- **C++23 used naturally.** `std::expected`, `std::span`,
  `std::move_only_function`, designated initializers, `constexpr`
  lookup tables, `[[nodiscard]]` where it earns its keep — not
  C++23-as-a-checklist.
- **Headless integration tests.** Tests link only the core, load
  hardware test ROMs through the same API the frontend uses, and
  assert on the same outputs real DMG hardware would produce
  (serial-port "Passed" / "Failed" for Blargg, framebuffer hash
  against golden image for `dmg-acid2`).
- **ADR-driven decisions.** Every significant architectural choice
  (DMG-only scope, single-threading, M-cycle accuracy, error model,
  cartridge polymorphism, boot ROM skip, debugger UI, framebuffer
  format) is recorded in [`docs/decisions/`](docs/decisions/) with the
  alternatives considered.

## Architecture

Three layers, dependency direction strictly downward:

```
┌────────────────────────────────────────────────────────────┐
│  Application (src/main.cpp)                                │
│  CLI parsing, instantiates App, runs                       │
└────────────────────────┬───────────────────────────────────┘
                         ▼
┌────────────────────────────────────────────────────────────┐
│  Frontend (gbe_frontend)                                   │
│  SDL3 window, audio, input, framebuffer upload, render     │
└────────────────────────┬───────────────────────────────────┘
                         ▼
┌────────────────────────────────────────────────────────────┐
│  Core (gbe_core)                                           │
│  CPU, Bus, PPU, APU, Timer, Joypad, Serial, Cartridge      │
│  No I/O. No SDL. No filesystem. No system clocks.          │
└────────────────────────────────────────────────────────────┘
```

The `Bus` is the spine of the core: every CPU memory access goes
through it, and peripherals communicate with each other only by
raising interrupts via the Bus. The full design is in
[`docs/architecture.md`](docs/architecture.md); the rationale behind
each significant decision is in [`docs/decisions/`](docs/decisions/).

## Roadmap

- **v1 — "the core works."** CPU (M-cycle accurate), PPU
  (`dmg-acid2`-correct, fixed mode-3 timing), MBC1/3/5 plus No-MBC
  cartridges, save states, headless test suite in CI. SDL3 frontend
  with keyboard input, VSync-driven frame pacing, framebuffer
  rendering. APU stubbed (registers accepted, no audio output yet).
- **v1.5 — "the debugger."** Dear ImGui overlay rendered inside the
  SDL window: register view, disassembly, memory hex view, VRAM tile
  viewer, tilemap viewer, sprite viewer, single-step / breakpoint
  controls. No core changes — the v1 const introspection accessors
  power the whole thing.
- **v2 — "completeness."** Open-ended. Likely candidates: real APU
  across all four channels, variable mode-3 PPU timing, memory and
  conditional breakpoints, configurable palettes, possibly CGB
  support if the appetite is there.

## What this is not

- A cycle-accurate (T-cycle) reference. M-cycle is sufficient for the
  goals and explicitly capped — running every commercial DMG title
  correctly is in scope; the most edge-case Mooneye timing tests are
  not.
- A complete Game Boy / CGB emulator. CGB hardware, Super Game Boy,
  link cable, IR, printer, and rumble are all out of scope.
- A drop-in replacement for a polished emulator like SameBoy or BGB.
  Quality-of-life features (cheats, fast-forward, configurable
  hotkeys, screenshot or video recording) are deliberately absent;
  the project is about the engineering.

## Building

Supported platforms: Linux and macOS. Windows isn't covered by CI and
has no documented build steps, though SDL3 itself runs there. Requires
a C++23 compiler (gcc 13+ or clang 16+), CMake 3.25+, and Ninja. See
[CONTRIBUTING.md](CONTRIBUTING.md#building) for full per-platform
dependency lists and optional tooling (ccache, lld, clang-tidy,
clang-format).

```bash
cmake --preset debug          # configure
cmake --build --preset debug  # compile
ctest --preset debug          # run tests
```

Available presets: `debug`, `release`, `relwithdebinfo`, `asan`,
`tsan`, `clang`. See
[CONTRIBUTING.md](CONTRIBUTING.md#configure-and-build) for what each
preset is for.

## License

MIT — see [LICENSE](LICENSE).
