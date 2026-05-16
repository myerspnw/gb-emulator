# ROMs

This directory holds ROM files used for development and testing.

---

## Legal

Game Boy ROMs are copyrighted by their respective publishers. **Do not
commit commercial ROMs to this repository.** The `.gitignore` at the
repo root ignores everything under `roms/` except `roms/test/` and
this file, so accidental commits of commercial ROMs are prevented by
default.

If you clone this repo and want to run commercial games, place the ROM
files anywhere under `roms/` (they will be gitignored) or pass the
path directly via CLI:

```bash
./build/release/src/gbe /path/to/your/game.gb
```

---

## `roms/test/`

Test ROMs used by the integration test suite. These are **not**
commercial ROMs — they are purpose-built hardware test programs
distributed freely for emulator developers.

### Blargg's test ROMs

Written by Shay Green (blargg). These are the primary correctness bar
for v1 — passing all of them is a goal of the project.

| ROM | Tests |
|-----|-------|
| `cpu_instrs.gb` | All CPU instructions, flags, and register behavior |
| `instr_timing.gb` | Cycle counts for every instruction |
| `mem_timing.gb` | Memory access timing |

Download from: https://gbdev.gg/pub/test-roms/

### dmg-acid2

PPU rendering correctness test. Produces a specific image; passing
means the PPU renders backgrounds, sprites, and the window layer
correctly.

Download from: https://github.com/mattcurrie/dmg-acid2/releases

### Mooneye Test Suite

Written by Gekkio. More thorough than Blargg, especially for timing
edge cases. We target passing the `acceptance/` suite; some tests
require T-cycle accuracy and are explicitly out of scope (see
ADR-003).

Download from: https://gekkio.fi/files/mooneye-test-suite/

---

## Adding test ROMs

Place downloaded test ROMs under `roms/test/`. They are tracked by
git (the `.gitignore` explicitly allows `roms/test/`). Commit them
with a message like:

```
test: add Blargg cpu_instrs and instr_timing test ROMs
```

Do not commit ROMs from commercial games. If you are unsure whether a
ROM is a test ROM or a commercial ROM, don't commit it.
