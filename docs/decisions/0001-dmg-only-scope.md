# ADR-001: DMG-only scope

**Status:** Accepted

## Context

The Game Boy hardware family has several variants we could target:

- **DMG** — the original 1989 Game Boy. 4-color grayscale LCD, 4 MHz CPU,
  smaller CPU register state, simpler PPU.
- **CGB** — Game Boy Color (1998). Adds color palettes (32K colors),
  double-speed CPU mode, extra VRAM bank, HDMA, expanded sound mixing.
  Backward-compatible with DMG cartridges.
- **SGB** — Super Game Boy. DMG hardware embedded in an SNES cartridge.
- **MGB / GBP / GBL** — pocket / pocket light, hardware variants of DMG.

Targeting CGB compatibility (with backward DMG support) is the most
ambitious option and would give us the largest playable game library.
DMG-only is the simplest and gets to a shippable emulator fastest.

This is fundamentally a scope decision — the question is "how much
hardware are we emulating?"

## Decision

Target the DMG only. Do not implement CGB hardware, SGB, or any other
variant. Cartridges that detect CGB hardware and refuse to run on DMG
are accepted as out of scope (they'll show "GAME BOY COLOR ONLY" screens
and that's the correct behavior on real DMG hardware too).

## Alternatives considered

**CGB with DMG compatibility.** The maximalist option. Supports the
larger library including all major Pokemon games in color, Zelda DX,
Wario Land 3, Donkey Kong Country, etc. Significantly more PPU work
(palette tables, attribute bytes per tile, DMA quirks), more I/O register
plumbing, and a doubled set of test ROMs to pass. Realistic effort
estimate: 1.5-2× the DMG-only work for the same level of polish.

**Both DMG and CGB simultaneously, mode-switched at boot.** Same as
above in scope; the runtime cost of detecting mode and dispatching is
small but the testing matrix doubles.

**SGB.** Niche. SGB-specific features (border graphics, multiplayer
adapters) are rarely the reason someone plays a Game Boy game.

## Consequences

**Easier:**

- The PPU is materially simpler — single 4-color palette, no per-tile
  attribute byte, no second VRAM bank.
- The CPU has no double-speed mode, simplifying timing analysis.
- Test ROM coverage is feasible at the showpiece quality bar — Blargg's
  full DMG suite plus dmg-acid2 plus selected Mooneye DMG tests is
  achievable. The CGB test ROM landscape is broader and many of those
  tests probe behaviors we wouldn't implement anyway.
- The cartridge library covered is still large: the entire pre-1998
  Game Boy library, plus the DMG-compatible portions of newer titles.

**Harder / lost:**

- No Pokemon Gold/Silver/Crystal in color (they're DMG-compatible, so
  they run, but in grayscale rather than 4-color).
- No CGB-exclusive titles (Donkey Kong Country, Wario Land 3, etc.).
- A reviewer noticing the scope may ask why CGB wasn't included. The
  honest answer is "CGB roughly doubles the work and DMG is sufficient
  to demonstrate the engineering."

**Future:** Adding CGB later is possible but not free. The PPU would
need substantial extension; cartridge handling has color-mode flags;
CPU gets a double-speed mode. It's a v2 candidate (listed as such in
the architecture doc) but not pre-architected for. If we ever add it,
this ADR would be superseded by a "CGB support" ADR.
