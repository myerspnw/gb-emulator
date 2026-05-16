# gb-emulator

A cycle-accurate Game Boy (DMG) emulator written in modern C++23.

## Status

🚧 In active development.

## Building

Requires a C++23 compiler (gcc 13+ or clang 16+), CMake 3.25+, and Ninja.

On Ubuntu / Debian:

```bash
sudo apt install build-essential cmake ninja-build git pkg-config \
    libx11-dev libxext-dev libxrandr-dev libxcursor-dev libxi-dev \
    libxinerama-dev libxss-dev libxkbcommon-dev libwayland-dev \
    libdecor-0-dev libegl1-mesa-dev libgles2-mesa-dev \
    libasound2-dev libpulse-dev libudev-dev libdbus-1-dev
```

Build:

```bash
cmake --preset release
cmake --build --preset release
ctest --preset debug   # run tests (uses debug preset)
```

Available presets: `debug`, `release`, `relwithdebinfo`, `asan`, `tsan`, `clang`.

## Architecture

See [docs/architecture.md](docs/architecture.md).

## License

TBD.
