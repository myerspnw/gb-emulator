# gb-emulator

[![CI](https://github.com/myerspnw/gb-emulator/actions/workflows/ci.yml/badge.svg)](https://github.com/myerspnw/gb-emulator/actions/workflows/ci.yml)
[![Lint](https://github.com/myerspnw/gb-emulator/actions/workflows/lint.yml/badge.svg)](https://github.com/myerspnw/gb-emulator/actions/workflows/lint.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-green.svg)](LICENSE)

An M-cycle-accurate Game Boy (DMG) emulator written in modern C++23.

## Status

🚧 In active development.

## Building

Supported platforms: Linux and macOS. Windows is not currently supported
— there's no CI lane and no documented build steps. SDL3 itself runs on
Windows, so a port is plausible but unproven.

Requires a C++23 compiler (gcc 13+ or clang 16+), CMake 3.25+, and Ninja.
See [CONTRIBUTING.md](CONTRIBUTING.md#building) for the full per-platform
dependency list (Linux apt packages, macOS brew formulae) and optional
tooling (ccache, lld, clang-tidy, clang-format).

Build and test (development):

```bash
cmake --preset debug
cmake --build --preset debug
ctest --preset debug
```

Build for shipping:

```bash
cmake --preset release
cmake --build --preset release
```

Available presets: `debug`, `release`, `relwithdebinfo`, `asan`, `tsan`, `clang`.
See [CONTRIBUTING.md](CONTRIBUTING.md#configure-and-build) for what each preset is for.

## Architecture

See [docs/architecture.md](docs/architecture.md).

## License

This project is licensed under the MIT License — see [LICENSE](LICENSE) for details.
