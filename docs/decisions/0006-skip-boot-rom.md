# ADR-006: Skip boot ROM in v1

**Status:** Accepted

## Context

Real Game Boy hardware executes a 256-byte boot ROM at power-on. The
boot ROM scrolls the Nintendo logo, plays a startup chime, validates the
cartridge header's Nintendo logo bytes, and then jumps to the cartridge's
entry point at `0x0100`. The boot ROM is Nintendo's copyrighted code; it
cannot be legally distributed.

Emulators have three options:

- Skip the boot ROM entirely; initialize CPU registers and I/O state to
  their post-boot values; start execution at `0x0100`.
- Support an optional boot ROM file. The user supplies their own (dumped
  from real hardware they own); the emulator executes it if provided.
- Bundle a clean-room boot ROM (some open-source projects do this,
  notably SameBoy).

The boot ROM execution is also a subtle accuracy test: the logo scroll
animation exercises the PPU and the timer at exact dot-precise timings,
and a slightly-off PPU can render the logo with visible distortion.

## Decision

v1 skips the boot ROM entirely. CPU registers and I/O state are
initialized to their canonical post-boot values per Pan Docs. Execution
starts at `0x0100` immediately on `load_cartridge`.

The Bus's read dispatch reserves the `0x0000-0x00FF` range as a separate
switch case (rather than folding it into the cartridge case), preserving
the option to add boot ROM support later by switching that case on a
`boot_rom_mapped_` flag and a held `boot_rom_` byte array.

## Alternatives considered

**Optional boot ROM with command-line flag.** The emulator runs without
one by default; if `--boot-rom=<path>` is given, it loads and executes
the boot ROM. Doesn't ship anything copyrighted. Implementation cost is
modest in pure code terms (a CLI flag, a Bus shadow region, the
`0xFF50` register write to unmap, and initial CPU state at PC=0 instead
of PC=0x100). However: the boot ROM scrolls the Nintendo logo at
dot-precise PPU timings. Several emulator authors have found that
chasing the bugs the boot ROM surfaces in the PPU costs days to weeks
of work. We don't have appetite for that cost.

**Bundle a clean-room boot ROM.** Some open-source emulators (SameBoy)
implement a boot ROM from scratch that's legally distributable. Not
hard *technically*, but requires writing Game Boy assembly that mimics
the boot ROM's behavior closely enough that games can rely on the
post-boot register state. For our project, this is the same work as
"skip boot ROM and hardcode register state" plus extra assembly that
nobody will see.

**Distribute a dumped boot ROM.** Illegal. Not considered.

## Consequences

**Easier:**

- v1 skips a category of PPU-timing edge cases (the boot ROM's logo
  scroll specifically). The PPU still has to be accurate enough to
  pass dmg-acid2 and Blargg, but it doesn't have to handle the boot
  ROM's specific timing patterns.
- The "what state does the CPU start in" question is answered by a
  constant initialization, not by emulating 256 bytes of Z80 code.
- No CLI complexity for the boot ROM flag.

**Harder / accepted limitations:**

- No Nintendo logo scroll on startup. A subset of users will miss this;
  it's an iconic part of the Game Boy experience. The cost is purely
  aesthetic.
- A small number of games perform unusual checks at startup that
  succeed because of specific post-boot register values. The standard
  post-boot state covers all of these for DMG; no commercial DMG game
  is known to misbehave from missing the boot ROM.
- Reviewers familiar with the emulator landscape may notice the
  omission. Honest answer: scope discipline. The boot ROM is
  cosmetic for our goals; debugging the PPU edge cases it exercises
  is not.

**Future:** Adding optional boot ROM support is a v2-or-later concern.
The Bus already has the architectural hook (separate switch case for
`0x0000-0x00FF`). Adding it would be a one-week project, not a rewrite.
If we ever do add it, this ADR is superseded by an "Optional boot ROM
support" ADR documenting the choice.
