# Contributing

Thanks for your interest in this project. This document covers how to build,
test, and contribute changes.

This is a personal project I use as a portfolio piece, but it's MIT-licensed
and external contributions are welcome. Even if you're not contributing
upstream, the build and style information here applies to any fork.

---

## Building

### Requirements

- C++23 compiler:
  - GCC 13 or newer
  - Clang 16 or newer
  - Apple Clang 15 or newer (macOS)
- CMake 3.25 or newer
- Ninja
- Git
- (Optional) ccache, lld — speed up rebuilds and linking

### Linux (Ubuntu / Debian)

```bash
sudo apt install -y \
    build-essential cmake ninja-build git pkg-config \
    clang clang-tidy clang-format lld gdb ccache python3 \
    libx11-dev libxext-dev libxrandr-dev libxcursor-dev libxi-dev \
    libxinerama-dev libxss-dev libxkbcommon-dev libwayland-dev \
    libdecor-0-dev libegl1-mesa-dev libgles2-mesa-dev \
    libasound2-dev libpulse-dev libudev-dev libdbus-1-dev \
    libibus-1.0-dev
```

### macOS

```bash
xcode-select --install
brew install cmake ninja ccache
```

`clang-format` and `clang-tidy` aren't required locally — lint runs in
CI on Linux. If you want them on macOS for editor integration or
pre-push checks, `brew install llvm` provides newer versions than Apple
Clang ships with; prepend `$(brew --prefix llvm)/bin` to your `PATH`.

### Configure and build

The project uses [CMake presets](https://cmake.org/cmake/help/latest/manual/cmake-presets.7.html).
Common presets:

| Preset            | Build type      | Notes                                   |
|-------------------|-----------------|-----------------------------------------|
| `debug`           | Debug           | Used during development                 |
| `release`         | Release         | Optimized; default for shipping         |
| `relwithdebinfo`  | RelWithDebInfo  | Optimized + debug info; for profiling   |
| `asan`            | Debug + ASan/UBSan | Runtime memory & UB checking         |
| `tsan`            | Debug + TSan    | Thread safety checking                  |
| `clang`           | Release (clang) | Force clang compiler                    |

```bash
# Debug build with tests
cmake --preset debug
cmake --build --preset debug
ctest --preset debug

# Release build
cmake --preset release
cmake --build --preset release

# Run the binary
./build/release/src/gbe
```

The first configure of each preset downloads SDL3, fmt, spdlog, and GoogleTest
via FetchContent. Subsequent configures are cached.

To share a FetchContent cache across presets (avoid re-downloading per
preset), set this in your shell profile:

```bash
export FETCHCONTENT_BASE_DIR=$HOME/.cache/cmake-fetchcontent
```

---

## Code style

Code style is enforced by `.clang-format` and `.clang-tidy`. CI rejects PRs
that don't pass both. Run them locally before opening a PR.

### Format

```bash
# Format a single file in place
clang-format -i src/core/gameboy.cpp

# Format everything tracked by git
git ls-files '*.cpp' '*.hpp' | xargs clang-format -i

# Check without modifying (CI behavior)
git ls-files '*.cpp' '*.hpp' | xargs clang-format --dry-run --Werror
```

### Lint

```bash
# Configure a build first (clang-tidy needs compile_commands.json)
cmake --preset debug

# Lint a single file
clang-tidy -p build/debug src/core/gameboy.cpp

# Lint everything
git ls-files '*.cpp' | xargs clang-tidy -p build/debug
```

### Naming conventions

Enforced by `readability-identifier-naming` in `.clang-tidy`:

- Types (`class`, `struct`, `enum`): `CamelCase` (`GameBoy`, `Mbc1`)
- Functions, methods, variables: `lower_case` (`read_byte`, `wram_size`)
- Private members: trailing underscore (`cycles_`, `bus_`)
- Namespaces: `lower_case` (`gbe`, `gbe::cpu`)
- Enum constants: `CamelCase` (`Mode::OamScan`, `Mode::HBlank`)
- `constexpr` variables: `lower_case`

### General preferences

- Always brace single-line conditionals and loops.
- `const auto&` in range-for loops unless mutation is intended.
- Prefer `unordered_*` containers when ordering isn't needed.
- Use `std::expected<T, Error>` for recoverable errors; `assert` for invariants.
- No `using namespace std;` anywhere.
- No raw `new` / `delete`. Use `std::unique_ptr` / `std::make_unique`.
- Pre-increment (`++i`) over post-increment (`i++`).
- `#pragma once` over include guards.

---

## Workflow

### Branching

`main` is always green. All non-trivial changes go through a branch and PR.

Branch naming: `<type>/<short-description>`

- `feat/cpu-load-instructions`
- `fix/mbc1-rom-bank-overflow`
- `refactor/bus-as-spine`
- `docs/architecture-overview`
- `ci/clang-tidy-job`

Trivial changes (typo fixes, dependency tag bumps) may be committed directly
to `main`.

### Commit messages

This project uses [Conventional Commits](https://www.conventionalcommits.org/).

Format: `<type>(<scope>): <subject>`

Types:

- `feat` — new functionality
- `fix` — bug fix
- `refactor` — code restructuring with no behavior change
- `perf` — performance improvement
- `docs` — documentation only
- `test` — test code only
- `build` — build system, dependencies
- `ci` — CI configuration
- `chore` — anything else (formatting, comments)

Scope is usually a subsystem: `cpu`, `ppu`, `apu`, `mmu`, `cart`, `bus`,
`build`, `ci`, `docs`.

Examples:

```
feat(cpu): implement 8-bit load instructions
fix(ppu): correct mode 3 timing for sprite-heavy lines
refactor(bus): extract MMU read/write into separate class
docs(architecture): add ADR for M-cycle accuracy
test(cpu): cover flag behavior for ADC instructions
ci: add macOS to build matrix
```

Subjects use the imperative mood ("add", not "added"). Keep them under
72 characters. Add a body if the why isn't obvious from the what.

### Pull requests

For non-trivial changes:

1. Branch from `main`.
2. Commit your work.
3. Push and open a PR on GitHub.
4. Wait at least until the next session, then **self-review** before merging
   (see checklist below).
5. CI must be green.
6. Squash-merge to `main`.

The PR is squashed on merge, so feel free to have messy WIP commits on the
branch. The squashed commit's message follows Conventional Commits.

### Self-review checklist

Before merging your own PR:

- [ ] Diff does what the PR title claims, nothing extra.
- [ ] No commented-out code, no leftover `printf` / `spdlog::trace` debugging.
- [ ] All new TODOs / FIXMEs have an issue link or clear context.
- [ ] New public functions, classes, and non-trivial types have a brief
      comment explaining their purpose.
- [ ] New behavior has tests (unit or integration).
- [ ] CI is green: build (gcc + clang, Linux + macOS), tests, ASan/UBSan,
      clang-format, clang-tidy.
- [ ] Commit message accurately summarizes the change.

---

## Running tests

```bash
cmake --preset debug
cmake --build --preset debug
ctest --preset debug
```

Verbose output on failure is enabled by default. To filter to a specific test:

```bash
./build/debug/tests/gbe_unit_tests --gtest_filter='Sanity.*'
```

To run with sanitizers:

```bash
cmake --preset asan
cmake --build --preset asan
ctest --preset asan
```

---

## Architecture

See [docs/architecture.md](docs/architecture.md) for the high-level design.
Significant architecture decisions are recorded in `docs/decisions/` as ADRs.

---

## Reporting issues

Open an issue on GitHub. Include:

- What you did
- What you expected
- What actually happened
- Build details (preset, compiler, OS, ROM if relevant)
- A minimal repro if possible

---

## License

By contributing, you agree your contributions are licensed under the
project's [MIT License](LICENSE).
