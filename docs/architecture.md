# Architecture

This document describes the design of the `gb-emulator` project. It captures
the *what* and *why* of the system's structure. For the rationale behind
specific significant decisions, see the [Architecture Decision Records](decisions/).

This is a living document. As the project evolves, decisions recorded here
may be revisited, but changes should be reflected in updates here (or
superseding ADRs).

---

## Goals

The project is a Game Boy emulator written in modern C++23. Its goals,
in priority order:

1. **Correctness on commercial DMG games.** Plays Tetris, Pokemon Red/Blue,
   Gold/Silver/Crystal, Super Mario Land, Zelda: Link's Awakening, Kirby,
   and other major titles.
2. **Verified by hardware test ROMs.** Passes Blargg's `cpu_instrs` and
   `instr_timing`, renders `dmg-acid2` correctly. Test ROMs run in CI on
   every commit.
3. **Cleanly architected.** Strict separation between emulation core and
   frontend. The core has no I/O dependencies; the frontend has no
   emulation logic. This is enforced by the build system, not just by
   convention.
4. **Modern C++ throughout.** C++23 idioms (`std::expected`, `std::span`,
   designated initializers, `[[nodiscard]]`, `constexpr` lookup tables)
   used naturally where they fit; not gratuitously.

What this project is **not** trying to be:

- A cycle-accurate reference. M-cycle granularity is sufficient for the
  goals above; T-cycle accuracy is explicitly out of scope.
- A complete Game Boy / Game Boy Color emulator. CGB hardware,
  Super Game Boy, link cable, IR, printer, and rumble are not supported.

---

## A note on timing accuracy

The Game Boy CPU runs at 4.194304 MHz. The fundamental clock unit is the
**T-cycle** (one tick of the master clock). Most CPU operations operate
in **M-cycles**, which are 4 T-cycles each — so the CPU effectively runs
at ~1.05 MHz at M-cycle granularity.

Emulator accuracy is typically described at one of three levels:

- **Instruction-level:** the emulator executes one whole opcode, then
  advances time by the opcode's total duration. Simplest. Fails timing-
  sensitive tests.
- **M-cycle:** the emulator advances time per memory access (each access
  is 1 M-cycle / 4 T-cycles). Passes Blargg's full test suite, runs every
  commercial game correctly, and catches the vast majority of timing
  bugs that matter to real software.
- **T-cycle:** the emulator advances 1 T-cycle at a time. Required for
  the most accuracy-sensitive Mooneye tests and some edge-case timing
  effects no commercial game depends on.

This project targets **M-cycle accuracy**. See ADR-003 for the rationale.

---

## Phases

Development is structured into three phases. Each is a coherent shippable
milestone.

### v1 — "the core works"

The emulator executes commercial DMG games end-to-end.

- CPU: full instruction set, M-cycle accurate, passes Blargg's
  `cpu_instrs` and `instr_timing`.
- PPU: complete, passes `dmg-acid2`. Mode 3 is fixed-length in v1.
- Cartridges: No-MBC, MBC1, MBC3 (with RTC), MBC5. Covers ~80% of the
  commercial DMG library.
- Bus, timer, joypad, interrupts: complete.
- Frontend: SDL3 window, keyboard input, framebuffer rendering with
  integer upscaling, VSync-driven frame pacing.
- APU: stubbed. Register writes are accepted and stored but produce no
  audio. The class exists with the full API so that v2 can fill it in
  without touching the rest of the system.
- Battery-backed cartridge RAM: auto-loaded from `<rom>.sav` on startup,
  auto-saved on exit, for cartridges with battery + RAM flags.
- Save states: full emulator state serializable to disk via a custom
  binary format with a version header.
- Debugger hooks: const accessors for CPU/PPU/memory state, `peek`/`poke`
  on the bus, `step_instruction()` on the CPU. No debugger UI yet — the
  hooks exist to power the headless test harness and v1.5 debugger.
- Headless integration test harness running Blargg / Mooneye / Acid ROMs
  in CI.

### v1.5 — "the debugger"

Adds a Dear ImGui debugger as a built-in development tool. Architecturally
this is a frontend feature — the core didn't need to change to make it
possible because the hooks were designed for it in v1.

Debugger features:

- CPU view: live register state, flag breakdown, current PC, cycle counter
  (`cpu().cycles()`, total T-cycles since reset)
- Disassembly view: rolling window centered on PC, current instruction
  highlighted
- Memory hex view: all regions browsable, editable in-place
- VRAM tile viewer: all 384 tiles rendered as an 8×48 grid
- Tilemap viewer: the two BG tilemaps rendered as they'd appear on LCD
- Sprite (OAM) viewer: all 40 sprite entries with their data
- Palette viewer: BG and OBJ palettes
- Execution controls: pause, single-step, step-over (CALL/RET), step-out,
  run-to-cursor
- PC breakpoints with on/off toggle
- PPU state inspection: current mode, scanline, dot

v1.5 is gated by the same correctness bar as v1 — adding the debugger
must not regress any test or change emulator behavior with the debugger
disabled.

### v2 — "completeness"

Anything beyond v1.5 is open-ended. Likely candidates if the project
continues:

- Real APU implementation, all 4 channels
- More accurate PPU (variable-length mode 3, fetcher state machine,
  OAM bug, mid-line palette changes)
- Memory and conditional breakpoints in the debugger
- Configuration file support
- Threading model if audio quality demands it
- CGB hardware support
- TAS-style frame-perfect input recording

None of v2 is planned in detail. The architecture's job in v1/v1.5 is to
leave the doors open for these without pre-building scaffolding.

---

## High-level structure

The codebase is split into three layers:

```
┌──────────────────────────────────────────────────────────┐
│  Application (src/main.cpp)                              │
│  - CLI parsing, instantiates App, runs                   │
└──────────────────────────────────────────────────────────┘
            │ depends on
            ▼
┌──────────────────────────────────────────────────────────┐
│  Frontend (src/frontend, library: gbe_frontend)          │
│  - SDL3 window, audio device, input handling             │
│  - Framebuffer texture upload, render                    │
│  - Frame pacing                                          │
│  - In v1.5: Dear ImGui debugger UI                       │
└──────────────────────────────────────────────────────────┘
            │ depends on
            ▼
┌──────────────────────────────────────────────────────────┐
│  Core (src/core, library: gbe_core)                      │
│  - Game Boy emulation logic                              │
│  - No SDL, no filesystem operations, no system clocks    │
│  - Operates on byte buffers passed in by the frontend    │
│  - Pure deterministic state machine                      │
└──────────────────────────────────────────────────────────┘
```

The dependency direction is enforced by the build system: `gbe_core` does
not link against SDL3 and cannot include SDL headers. Any attempt to
introduce I/O dependencies into the core will fail to compile.

This separation enables several properties:

- **Tests are fast and headless.** Integration tests link `gbe_core`
  only, run on CI without a display, and produce deterministic output.
- **The frontend is replaceable.** A different frontend (terminal, web,
  test harness) can be written without touching the core.
- **Save states are simple.** The core's full state is serializable
  because the core owns no resources that aren't part of its serialized
  representation.

---

## The Core

### Top-level type: `GameBoy`

```cpp
class GameBoy {
public:
    GameBoy();
    ~GameBoy();

    // Move-only. Holding internal references means copying is unsafe.
    GameBoy(const GameBoy&) = delete;
    GameBoy& operator=(const GameBoy&) = delete;
    GameBoy(GameBoy&&) = default;
    GameBoy& operator=(GameBoy&&) = default;

    // ── Lifecycle ─────────────────────────────────────────
    // Takes ownership of the ROM bytes. The caller (typically the frontend
    // reading a file) gives up its buffer; the core moves it into the
    // cartridge object for the cartridge's lifetime.
    [[nodiscard]] std::expected<void, CartridgeError>
        load_cartridge(std::vector<std::byte> rom);

    // Installs a wall-clock provider for the MBC3 RTC (the only place
    // in the core where wall-clock time matters). The core never calls
    // std::chrono::system_clock::now() itself — see "MBC3 RTC and
    // wall-clock time" below for the design.
    //
    // Default provider returns 0s (RTC frozen at the Unix epoch), which
    // is deterministic and right for tests. May be called before or
    // after load_cartridge; takes effect on the next RTC latch.
    using WallClockFn = std::function<std::chrono::seconds()>;
    void set_wall_clock(WallClockFn fn);

    // ── Execution ─────────────────────────────────────────
    // Runs until the next VBlank, OR until a debug condition fires
    // (breakpoint hit, single-step in progress). Caller can distinguish
    // via stopped_at_breakpoint() below.
    void run_frame();

    // Executes exactly one CPU instruction. Used by the debugger.
    void step_instruction();

    // True if the last run_frame() exited because of a debug condition
    // rather than reaching VBlank.
    [[nodiscard]] bool stopped_at_breakpoint() const noexcept;

    // ── Input ─────────────────────────────────────────────
    void set_button(Button b, bool pressed);

    // ── Output ────────────────────────────────────────────
    // Framebuffer is 160×144 bytes of palette indices (0-3). The frontend
    // converts indices to ARGB via its palette. See "Rendering" below.
    [[nodiscard]] std::span<const std::uint8_t> framebuffer() const noexcept;

    // Drains the APU's sample buffer into the caller's span. Returns the
    // number of samples written. Caller-provided storage avoids exposing
    // internal buffers whose lifetime is unclear.
    [[nodiscard]] std::size_t drain_audio(std::span<float> out);

    // ── Persistence ───────────────────────────────────────
    // Save state serialization uses a size-then-fill pattern. Caller
    // queries the required buffer size, allocates, then asks the core
    // to fill it. Avoids hidden allocations and matches the subsystem
    // serialization interface (see Cartridge below).
    [[nodiscard]] std::size_t save_state_size() const noexcept;
    void serialize(std::span<std::byte> out) const;

    [[nodiscard]] std::expected<void, SaveStateError>
        deserialize(std::span<const std::byte> data);

    [[nodiscard]] std::span<const std::byte> cartridge_ram() const noexcept;
    void load_cartridge_ram(std::span<const std::byte> data);

    // ── Introspection (for debugger and tests) ────────────
    [[nodiscard]] const Cpu&   cpu()   const noexcept;
    [[nodiscard]] const Bus&   bus()   const noexcept;
    [[nodiscard]] const Ppu&   ppu()   const noexcept;
    [[nodiscard]] const Timer& timer() const noexcept;
};
```

The `GameBoy` class is the only type the frontend talks to. Everything else
is exposed for debugging and testing, but day-to-day usage is through this
interface.

### Subsystems

The core consists of these components:

- **`Cpu`** — Sharp LR35902 emulation. 256 base opcodes + 256 CB-prefixed
  opcodes. Executes one instruction per `step_instruction()` call, ticking
  the Bus by the appropriate number of M-cycles per memory access. Owns
  a monotonically increasing T-cycle counter (`cycles()`) used by the
  debugger and tests to track elapsed emulated time.
- **`Bus`** — The memory router. Owns WRAM (8 KB), HRAM (127 B), the
  IE/IF interrupt registers. Holds references to all peripherals.
  Dispatches reads/writes by address region. Advances all peripherals on
  `tick()`.
- **`Ppu`** — Picture Processing Unit. Owns VRAM (8 KB) and OAM (160 B).
  Produces the 160×144 framebuffer. Internally dot-clocked.
- **`Apu`** — Audio Processing Unit. In v1: a stub that accepts register
  writes and returns silence. In v2: 4 channels (2 pulse, 1 wave, 1 noise).
- **`Timer`** — DIV and TIMA registers. Generates timer interrupts.
- **`Joypad`** — P1 register. Maps `Button` state into the hardware register.
- **`Cartridge`** — Polymorphic. Concrete subclasses for each MBC type.
  Owns ROM data and optional cart RAM.

### The Bus as spine

The Bus is the architecture's central element. Every CPU memory access
goes through it. Every M-cycle tick from the CPU ripples through the Bus
to peripherals. Peripherals do not reference each other directly: if one
peripheral needs to signal another (the most common case is raising an
interrupt), it does so through the Bus.

Peripherals do hold a reference back to the Bus for this reason — to
call `bus_.request_interrupt(...)`. The CPU also holds a Bus reference
for memory access. The dependency graph between core types is therefore:
CPU → Bus → peripherals, with peripherals also able to call back into
Bus for interrupt signaling. The Bus is the only intermediary; peripherals
never reference each other.

```
                  ┌────────────────┐
                  │      CPU       │
                  └────────┬───────┘
                  read/    │   tick(n)
                  write    │
                           ▼
              ┌─────────────────────────┐
              │           Bus           │◄──── owns WRAM, HRAM, IE/IF
              └─┬───┬───┬───┬───┬───┬───┘
                │   │   │   │   │   │
                ▼   ▼   ▼   ▼   ▼   ▼   (dispatch into peripherals)
              PPU APU Timer Joy Cart
                │   │   │   │
                └───┴───┴───┴────► request_interrupt() back to Bus
```

The Bus exposes:

```cpp
class Bus {
public:
    // read/write tick the bus by 4 T-cycles. They are non-const because
    // they advance time as a side effect — this is fundamental to M-cycle
    // accuracy. A "const read" would be lying about its effects.
    [[nodiscard]] uint8_t read(uint16_t addr);
    void                 write(uint16_t addr, uint8_t value);
    void                 tick(int t_cycles);

    // Debugger hooks: read/write without ticking. peek is const because
    // it truly has no side effects; poke is non-const (it mutates memory)
    // but distinct from write in that it doesn't advance time.
    [[nodiscard]] uint8_t peek(uint16_t addr) const noexcept;
    void                  poke(uint16_t addr, uint8_t value);

    void request_interrupt(InterruptType which);

    // ... accessors for introspection
};
```

Memory map (DMG):

| Address range   | Region          | Owner                  |
|-----------------|-----------------|------------------------|
| `0x0000-0x3FFF` | ROM Bank 00     | Cartridge              |
| `0x4000-0x7FFF` | ROM Bank N      | Cartridge (banked)     |
| `0x8000-0x9FFF` | VRAM            | PPU                    |
| `0xA000-0xBFFF` | External RAM    | Cartridge (banked)     |
| `0xC000-0xDFFF` | WRAM            | Bus                    |
| `0xE000-0xFDFF` | Echo RAM        | Bus (mirrors WRAM `0xC000-0xDDFF`) |
| `0xFE00-0xFE9F` | OAM             | PPU                    |
| `0xFEA0-0xFEFF` | Unusable        | Bus (returns 0xFF)     |
| `0xFF00-0xFF7F` | I/O registers   | Joypad/Timer/APU/PPU   |
| `0xFF80-0xFFFE` | HRAM            | Bus                    |
| `0xFFFF`        | IE register     | Bus                    |

Note that ROM and external RAM addresses on the CPU bus (16-bit) get
translated by the cartridge into larger addresses (up to 23-bit for an
8 MB MBC5 ROM). The cartridge holds its own banking state; the Bus just
forwards `read_rom(addr)` / `write_rom(addr, value)` calls and lets the
cartridge do the translation.

The dispatch in `Bus::read`/`write` is a switch on the address region.
The switch is one source of truth for the memory map — it stays
debuggable in gdb and is the natural place to find "where does this
address get handled?"

### CPU

The CPU is implemented as a single class with a giant switch-on-opcode
in `Cpu::step_instruction()`:

```cpp
void Cpu::step_instruction() {
    uint8_t op = fetch_byte();    // ticks bus 4 T-cycles
    switch (op) {
    case 0x00: nop(); break;
    case 0x01: ld_bc_d16(); break;
    case 0x02: ld_bc_addr_a(); break;
    // ... 250+ more cases ...
    case 0xCB: execute_cb_prefixed(); break;
    // ...
    }
}
```

Rationale: the switch keeps every instruction's logic visible in one
place, generates a jump table under optimization, steps cleanly in a
debugger, and pairs naturally with M-cycle accuracy (each opcode handler
ticks the bus the appropriate number of times). See ADR-003.

CPU registers are modeled with a union to support both the paired view
(`AF`, `BC`, `DE`, `HL`) and the individual view (`A`, `F`, `B`, `C`, …):

```cpp
struct Registers {
    union { struct { uint8_t f, a; }; uint16_t af; };
    union { struct { uint8_t c, b; }; uint16_t bc; };
    union { struct { uint8_t e, d; }; uint16_t de; };
    union { struct { uint8_t l, h; }; uint16_t hl; };
    uint16_t sp;
    uint16_t pc;
};
```

Byte order matches little-endian hardware (low byte at lower address).

This use of overlapping union members for type-punning is technically
undefined behavior in standard C++ (only one union member is "active"
at a time per the standard). In practice, every major compiler treats
this idiom as well-defined on platforms where it works (which includes
all our targets — x86-64 and ARM64 on Linux and macOS). All Game Boy
emulators do this; the alternative is either bit-shift accessors that
add visual noise to every instruction, or `std::bit_cast` calls per
access. We accept the pragmatic UB here.

### PPU

The PPU is internally dot-clocked. The Bus's `tick(n)` advances the PPU
by `n` dots. The PPU is a state machine with four modes:

- **Mode 2 (OAM Scan):** 80 dots per scanline
- **Mode 3 (Drawing):** 172 dots minimum, variable up to ~289 (v1: fixed at 172)
- **Mode 0 (HBlank):** remainder of the 456-dot scanline
- **Mode 1 (VBlank):** lines 144-153 (4560 dots)

Total frame: 154 lines × 456 dots = 70224 dots ≈ 59.7 Hz at the 4.194 MHz
clock.

In v1, Mode 3 is fixed at 172 dots regardless of sprite count or scrolling.
This passes `dmg-acid2` and all Blargg tests but fails some Mooneye
mode-3-timing tests. Variable Mode 3 is a v2 concern.

The framebuffer is a `std::array<std::uint8_t, 160 * 144>` of palette
indices (each pixel is 0-3, selecting one of the four DMG shades).
The PPU writes indices as each scanline finishes drawing.
`GameBoy::framebuffer()` returns a `std::span` over this array. The
frontend converts indices to ARGB pixels via the palette table at
upload time.

**Framebuffer is read-consistent at VBlank.** Because `run_frame()`
returns at VBlank — after every scanline of the current frame has been
drawn — the frontend can safely call `framebuffer()` immediately after
`run_frame()` and read a complete, consistent frame. Reading the
framebuffer mid-frame is not possible in the single-threaded model.

The framebuffer is not cleared between frames. The PPU overwrites every
pixel during normal operation. When the LCD is disabled by the game
(by clearing bit 7 of LCDC), the framebuffer retains its last drawn
state until the LCD is re-enabled and the PPU resumes — matching real
hardware behavior, where the LCD displays its last contents (or solid
white) when disabled.

Storing palette indices (not final colors) in the core has two benefits:
the framebuffer is 4× smaller than ARGB8888, and the palette becomes a
frontend concern — supporting custom palettes in v2 won't require core
changes.

### Cartridges

`Cartridge` is an abstract base class:

```cpp
class Cartridge {
public:
    virtual ~Cartridge() = default;

    [[nodiscard]] virtual uint8_t read_rom(uint16_t addr) const = 0;
    virtual void                  write_rom(uint16_t addr, uint8_t value) = 0;
    [[nodiscard]] virtual uint8_t read_ram(uint16_t addr) const = 0;
    virtual void                  write_ram(uint16_t addr, uint8_t value) = 0;

    [[nodiscard]] virtual bool has_battery() const noexcept = 0;
    [[nodiscard]] virtual std::span<const std::byte> ram() const noexcept = 0;
    virtual void load_ram(std::span<const std::byte> data) = 0;

    // Save-state hooks. Each MBC type has its own State struct holding
    // bank registers, RAM enable flags, RTC state (MBC3), etc. The
    // serialized form is variable-length per MBC type; the size is
    // discoverable via state_size().
    [[nodiscard]] virtual std::size_t state_size() const noexcept = 0;
    virtual void serialize(std::span<std::byte> out) const = 0;
    [[nodiscard]] virtual std::expected<void, SaveStateError>
        deserialize(std::span<const std::byte> data) = 0;
};
```

Concrete implementations in v1:

- `NoMbc` — direct ROM mapping, optional non-banked RAM
- `Mbc1` — ROM and RAM banking, mode register
- `Mbc3` — ROM banking, RAM banking, RTC chip
- `Mbc5` — extended ROM banking up to 8 MB

A factory function reads the cartridge header and returns the appropriate
type:

```cpp
[[nodiscard]] std::expected<std::unique_ptr<Cartridge>, CartridgeError>
make_cartridge(std::vector<std::byte> rom);
```

The header parsing validates: Nintendo logo bytes, header checksum,
cartridge type byte, ROM size, RAM size. Mismatches return specific
error variants.

The `Bus` holds `std::unique_ptr<Cartridge>`. The polymorphic dispatch
adds a vtable indirection per cartridge access; at emulated CPU speed
(~4 MHz), this is well within budget. See ADR-005.

### Boot ROM

v1 does **not** support the Game Boy boot ROM. The CPU starts in the
post-boot state:

```
PC = 0x0100      AF = 0x01B0      BC = 0x0013
DE = 0x00D8      HL = 0x014D      SP = 0xFFFE
```

I/O registers are initialized to their post-boot values per Pan Docs.

The Bus's read dispatch reserves the `0x0000-0x00FF` range as a separate
switch case (rather than folding it into the cartridge case), preserving
the option to add boot ROM support later by switching that case based on
a `boot_rom_mapped_` flag.

### Interrupts

Five interrupts:

| Bit | Interrupt   | Source      | Vector  |
|-----|-------------|-------------|---------|
| 0   | VBlank      | PPU         | 0x0040  |
| 1   | LCD STAT    | PPU         | 0x0048  |
| 2   | Timer       | Timer       | 0x0050  |
| 3   | Serial      | (Unused v1) | 0x0058  |
| 4   | Joypad      | Joypad      | 0x0060  |

The Bus owns the IE (`0xFFFF`) and IF (`0xFF0F`) registers. Peripherals
raise interrupts via `bus_.request_interrupt(...)`, which sets the
appropriate bit in IF. The CPU checks `IF & IE` each instruction and,
if interrupts are enabled (IME flag), jumps to the appropriate vector.

### Input

The frontend translates keyboard events to `Button` enum values and
calls `gameboy.set_button(b, pressed)`. The keyboard mapping is
hardcoded in v1:

| Key       | Button |
|-----------|--------|
| ↑ / W     | Up     |
| ↓ / S     | Down   |
| ← / A     | Left   |
| → / D     | Right  |
| Z         | A      |
| X         | B      |
| Enter     | Start  |
| RShift    | Select |

The Joypad subsystem owns the current button state and exposes it through
the P1 register at `0xFF00`. The P1 register has a quirk: the CPU first
**writes** to P1 to select which button row to read (direction keys vs
action keys, bits 4 and 5 active-low), then **reads** P1 to get the four
selected buttons (bits 0-3, active-low). The Joypad implementation tracks
the most recently written row-select value and returns the appropriate
buttons on read.

### Error handling

The core uses two distinct error-handling mechanisms, applied for
different error categories:

**`std::expected<T, E>` for category 1: external failures.**
Used wherever the caller is reasonably expected to handle the error.
Examples: loading a malformed ROM, an unsupported MBC type, a corrupt
save state, a file that doesn't exist.

**`assert` and `std::unreachable()` for category 2: invariant
violations.** Used where the failure represents a bug in our own code.
Examples: an address dispatch falling off the end, an enum value outside
its declared range, a precondition violated by internal calls.

A failed assert is a bug, not a state to recover from. A `std::expected`
return is communication: the operation can't complete and the caller
needs to decide what to do.

See ADR-004 for the full rationale.

### Save states

A save state is a serialized snapshot of the entire emulator state at a
point in time. Format:

```
struct SaveStateHeader {
    char     magic[4];      // "GBE\0"
    uint32_t version;       // semantic version of the format
    uint32_t cart_hash;     // CRC32 of the loaded ROM, for compatibility check
    uint32_t reserved[3];   // pad to 24 bytes; reserved for future use
};
```

The header deliberately carries no timestamp. The save file's mtime is
the canonical record of when it was written, and the core has no access
to the system clock — threading a timestamp parameter through
`serialize()` purely for metadata isn't worth the API surface.

Following the header, each subsystem writes its `State` POD struct in
a defined order. `GameBoy::serialize()` returns the concatenated bytes;
`GameBoy::deserialize()` parses them back.

The format is **versioned**. Loading a state with a different version
returns `SaveStateError::IncompatibleVersion`. Forward compatibility is
not promised — bumping the version invalidates all previous saves. The
showpiece bar is "works"; saving across emulator versions is an explicit
non-goal.

The `cart_hash` field guards against loading a state into a different
cartridge. Mismatch returns `SaveStateError::CartridgeMismatch`.

**Cartridge state during deserialize.** The MBC type isn't stored in the
save state — it's implicit. A save state is always loaded into a
GameBoy that already has a cartridge inserted; the existing cartridge's
type determines the format of the cartridge-state bytes. Because
`cart_hash` requires the loaded ROM to match, the MBC type is guaranteed
to be the same as when the save state was written. If for any reason
the cartridge-state bytes are malformed for the current cartridge, the
cartridge's `deserialize` returns `SaveStateError::CorruptCartridgeState`.

### Battery-backed cartridge RAM

Distinct from save states. Real Game Boy cartridges with batteries
(MBC1/3/5 with RAM, where the header type byte indicates battery support)
preserve their RAM across power cycles. We emulate this with two
frontend-driven steps:

- On startup: after `gameboy.load_cartridge(rom)` succeeds, the frontend
  looks for `<rom-path>.sav` next to the ROM. If found, it reads the
  bytes and calls `gameboy.load_cartridge_ram(data)`. If not, the cart's
  RAM keeps its default-init values.
- On shutdown (or whenever the frontend chooses to checkpoint): if the
  cart has battery + RAM, the frontend reads `gameboy.cartridge_ram()`
  and writes the bytes to `<rom-path>.sav`.

The frontend is responsible for resolving the `.sav` path and for all
file I/O. The core exposes `cartridge_ram()` and `load_cartridge_ram(data)`
but has no notion of files and never writes to disk on its own.

### MBC3 RTC and wall-clock time

The MBC3 cartridge type includes a real-time clock chip. Its registers
expose seconds, minutes, hours, and a 9-bit day counter, all advancing
at 1 Hz from the cartridge's reference time. This is the only point in
the emulation core where wall-clock time matters.

The core must not call `std::chrono::system_clock::now()` itself — that
would couple it to the host system, break determinism, and violate the
core/frontend split (see ADR-002 and CLAUDE.md). Instead, the frontend
installs a callable:

```cpp
using WallClockFn = std::function<std::chrono::seconds()>;
void GameBoy::set_wall_clock(WallClockFn fn);
```

The provider returns the current wall-clock time as seconds since the
Unix epoch. The MBC3 implementation invokes it only when needed —
specifically on an RTC latch (a write of `0x01` to the latch register at
`0x6000-0x7FFF` after a preceding `0x00`). It does *not* call the
provider on every RTC register read. The latched seconds-value is stored
alongside the RTC's register state in the cartridge's save-state struct,
so that across save/restore the RTC behaves as if it had been ticking
the whole time.

**The default provider returns `std::chrono::seconds{0}`** — RTC frozen
at the Unix epoch. This is deliberate: it keeps every test deterministic
without per-test setup. Production frontends opt in:

```cpp
gb.set_wall_clock([] {
    return std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch());
});
```

For headless integration tests that need to exercise day-night cycles
(Pokemon Gold/Silver/Crystal), the test harness can install a clock
that advances by a fixed amount per frame, keeping the RTC's behaviour
reproducible across CI runs and across machines.

This is the *only* injection point. No other subsystem reads wall-clock
time; if a future feature looks like it needs to, that's a sign it
belongs in the frontend instead.

---

## The Frontend

`gbe_frontend` is the SDL3-based application shell. Its responsibilities:

- Create and manage the SDL window
- Pump SDL events; translate keyboard events to `Button` state
- Each frame: call `gameboy.run_frame()`, upload framebuffer to SDL
  texture, render, present
- Frame pacing via SDL3 VSync
- File I/O: read ROM file, read/write `.sav` file, read/write save state
- Handle CLI arguments

The frontend is intentionally thin in v1. The `App` class is a state
holder; most of the work is in the main loop.

### Main loop

The main loop is straightforward because the emulator core is synchronous
and single-threaded:

```cpp
while (running) {
    handle_events();           // SDL events → button presses, window resize, quit
    gameboy.run_frame();       // blocks until the PPU completes a frame
    upload_framebuffer();      // convert palette indices to ARGB, copy to SDL texture
    drain_audio();             // pull samples from APU (silent in v1)
    render();                  // SDL present (waits for VSync)
}
```

`gameboy.run_frame()` returning is the signal that a frame is ready.
This is the heartbeat of the system: one iteration of the loop produces
one frame.

### Frame pacing

v1 uses SDL3's VSync. On a 60 Hz monitor this gives 60 fps; on a 144 Hz
monitor, the emulator runs faster than real-time (a known limitation).
Precise 59.7 Hz pacing requires either (a) audio-driven timing (impossible
without a real APU) or (b) `std::this_thread::sleep_until` with careful
catch-up logic. Both are v2 concerns.

### Rendering

The framebuffer is 160×144 pixels at the Game Boy's 4 DMG colors:

```cpp
constexpr std::array<uint32_t, 4> dmg_palette = {
    0xFFE0F8D0,  // lightest
    0xFF88C070,
    0xFF346856,
    0xFF081820   // darkest
};
```

(Colors approximate the green-tinted DMG LCD. Configurable in v2.)

The Game Boy framebuffer is uploaded to an SDL texture as a 160×144
ARGB8888 texture once per frame. SDL3 scales it to the window with
nearest-neighbor filtering. Default window size: 4× scale (640×576),
resizable.

### CLI

```
gbe <rom-path> [options]
  --scale=<int>             Initial window scale factor (default: 4)
  --load-state=<path>       Load a save state on startup
  --no-vsync                Disable VSync
```

Argument parsing is done manually in `main.cpp`; no third-party arg
parser. The flag set is small enough that hand-rolling is simpler than
adding a dependency.

### Debugger UI (v1.5)

The v1.5 debugger uses [Dear ImGui](https://github.com/ocornut/imgui)
rendered inside the SDL window via the SDL3 backend. The emulator runs
in the same window as the debugger panels, docked alongside.

The debugger reads from `GameBoy`'s const accessors. It does not call
the emulator's mutating methods directly except for the execution
controls (pause, step). When paused, the emulator stops in
`run_frame()`. Single-step calls `step_instruction()`.

The debugger uses `Bus::peek` / `Bus::poke` for memory reads and writes,
which bypass timing. This means looking at memory in the debugger never
disturbs the emulator state — important for debugging timing-sensitive
games.

---

## Testing

### Unit tests

Live in `tests/unit/`. Use GoogleTest. Cover individual subsystems:

- CPU instruction-by-instruction: load known state, execute one opcode,
  assert resulting state. Covers register effects, flag effects, cycle
  counts.
- Cartridge header parsing: synthetic ROMs with various header values.
- MBC banking: directed writes that change bank registers, verify
  subsequent reads return the right bytes.
- Save state round-trip: serialize, deserialize, compare state.

### Integration tests

Live in `tests/integration/`. Use the `HeadlessRunner` test harness.

A `HeadlessRunner` constructs a `GameBoy`, loads a test ROM, and runs
frames until a configurable condition is met or a timeout elapses.
For Blargg ROMs, the condition is the serial port (`0xFF01`) writing
the string "Passed" or "Failed". For Mooneye ROMs, it's the CPU registers
containing the magic Fibonacci sequence (the Mooneye protocol).

Test ROMs covered in CI:

- `cpu_instrs.gb` (Blargg) — every CPU instruction
- `instr_timing.gb` (Blargg) — instruction cycle counts
- `mem_timing.gb` (Blargg) — memory access timing
- `dmg-acid2.gb` — PPU rendering correctness (compares output to golden
  PNG)
- Selected Mooneye tests for timing and edge cases

### Determinism

The emulator core is deterministic: same ROM + same input sequence
produces bit-identical output. This is a property of the architecture
(single-threaded, no real-time clocks except where emulated, no system
entropy used) and is leveraged by:

- Snapshot tests that hash the framebuffer after N frames
- CI runs that produce identical results across machines
- Future-you reproducing a bug exactly

**One exception:** the MBC3 RTC chip (Real-Time Clock). When the
production wall-clock provider is installed, games that use the RTC
(Pokemon Gold/Silver/Crystal day-night cycles, primarily) are not fully
deterministic across runs. The default provider returns `0s`, which
freezes the RTC and restores determinism; CI relies on this default,
and tests that need controlled RTC advancement install a fake provider.
See ["MBC3 RTC and wall-clock time"](#mbc3-rtc-and-wall-clock-time) for
the full API.

---

## Build

The build system is documented in [CONTRIBUTING.md](../CONTRIBUTING.md).
Key properties relevant here:

- Self-contained: all dependencies (SDL3, fmt, spdlog, GoogleTest; plus
  Dear ImGui added in v1.5) fetched via CMake's FetchContent
- Reproducible: dependency versions pinned to specific tags
- Multi-config: build presets for Debug, Release, RelWithDebInfo, ASan,
  TSan, clang
- Strict: warnings-as-errors, clang-tidy, clang-format checked in CI
- Multi-platform: builds on Linux and macOS in CI

The dependency direction between libraries (`gbe_frontend` depends on
`gbe_core`, but not vice versa) is enforced by the build system through
the `target_link_libraries` graph. `gbe_core` does not list SDL3 as a
dependency; any attempt to `#include <SDL3/...>` from core code fails to
compile.

---

## Decisions

Significant architectural decisions are recorded as ADRs in
[`docs/decisions/`](decisions/):

- ADR-001: DMG-only scope
- ADR-002: Single-threaded emulator core
- ADR-003: M-cycle CPU accuracy with switch/case dispatch
- ADR-004: `std::expected` plus `assert` for error handling
- ADR-005: Polymorphic cartridge model
- ADR-006: Skip boot ROM in v1
- ADR-007: Dear ImGui for the debugger UI
- ADR-008: Palette-index framebuffer instead of ARGB

Each ADR captures the context, the decision, the alternatives considered,
and the consequences. Decisions can be superseded; the ADR record
shows how the project's thinking evolved.

---

## Open questions and known limitations

Things that aren't fully decided or aren't going to work in v1:

- Some Mooneye tests will fail due to fixed Mode 3 timing
- Audio is silent in v1
- VSync-driven frame pacing means the emulator runs faster than real-time
  on high-refresh-rate monitors
- No CGB support; some games that detect CGB hardware may behave oddly
- No link cable; multiplayer-only games won't run their multiplayer modes
- Serial port output is captured for tests but not surfaced to the user
- No screenshot or video recording (could be added trivially in v2)

These are not bugs. They're acknowledged scope limits.
