# ADR-008: Palette-index framebuffer

**Status:** Accepted

## Context

The PPU produces a 160×144 framebuffer that the frontend uploads to a
texture each frame. There are two natural representations for that
framebuffer:

- **Palette-index.** Each pixel is a single byte (0-3) selecting one of
  the four DMG palette entries. The frontend maps indices to ARGB at
  upload time via a palette table.
- **Final ARGB.** Each pixel is a 32-bit ARGB color. The PPU has already
  applied the palette; the frontend uploads pixels directly to the
  texture.

This is a small decision but it has consequences for memory layout,
palette customization, and the boundary between core and frontend.

## Decision

Use palette-index. The framebuffer is `std::array<std::uint8_t,
160 * 144>`. The PPU writes indices (0-3) per pixel. The frontend
converts indices to ARGB via a palette lookup at texture upload time.

The palette lives in the frontend, not the core. The DMG palette table
in the frontend is:

```cpp
constexpr std::array<uint32_t, 4> dmg_palette = {
    0xFFE0F8D0,  // lightest
    0xFF88C070,
    0xFF346856,
    0xFF081820   // darkest
};
```

## Alternatives considered

**Final ARGB framebuffer.** Each pixel is a 32-bit ARGB color. Pros:
no conversion step in the frontend; the texture upload is a direct
memcpy. Cons: the framebuffer is 4× larger (90 KB vs 23 KB; both fit in
L2 cache, so not actually meaningful here). The palette gets baked into
the core, which means swapping palettes (a v2 feature) would require
either re-rendering the framebuffer from scratch or holding both
palette-index and ARGB representations. The core takes on a
"presentation" responsibility that's really a frontend concern.

**Sub-byte packed palette indices (2 bits per pixel).** Theoretically
possible — DMG only needs 2 bits per pixel. Framebuffer would be
~5.6 KB. Cons: the access pattern (read 2 bits at an arbitrary pixel)
requires shifts and masks, making the PPU's pixel-write loop more
complex for no real gain. Memory is not the constraint at 23 KB; this
is premature optimization.

**Pre-converted ARGB with a `palette()` setter.** Frontend tells the
core which palette to use; core stores the ARGB framebuffer. Cons:
mixing concerns. The core shouldn't have an opinion about how its
output is displayed.

## Consequences

**Easier:**

- The framebuffer is 4× smaller (23 KB vs 90 KB). Trivially fits in
  L1 cache, which matters slightly for the PPU's per-pixel write loop
  and the frontend's per-frame upload pass.
- The palette is purely a frontend concern. Custom palettes (sepia,
  Game Boy Pocket grayscale, etc.) become a frontend feature with no
  core changes.
- The core has no opinion about display colors. This keeps the
  separation between core (deterministic state machine) and frontend
  (presentation) clean.
- The PPU writes one byte per pixel — natural for the per-scanline
  rendering pipeline.

**Harder / accepted:**

- The frontend does a conversion pass each frame. At 160×144 = 23,040
  pixels, this is trivial work; on any host of the last 20 years it
  runs in microseconds. Acceptable cost.
- A test that wants to compare the framebuffer to a golden image needs
  to either compare palette indices (matches what the core produces) or
  do the same conversion (matches what the user sees). Snapshot tests
  will compare palette indices for simplicity.

**Future:** v2 may add support for user-selectable palettes. Because
the palette is a frontend concern, this is a frontend-only feature — no
core changes. Adds maybe a few lines to the rendering path.
