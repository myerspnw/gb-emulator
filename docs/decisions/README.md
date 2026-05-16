# Architecture Decision Records

This directory contains the project's Architecture Decision Records (ADRs).

An ADR captures a single architectural decision: what was decided, what
alternatives were considered, and what the consequences are. ADRs are
immutable once accepted — they document the project's reasoning at a
point in time. If a decision is later reversed, a new superseding ADR is
written, and the old one is marked superseded but not deleted.

The intent is that someone joining the project (or coming back to it
after time away) can read the ADRs in order and reconstruct the
architectural reasoning without having to interview anyone.

## Format

Each ADR is a Markdown file named `NNNN-short-title.md` where `NNNN` is
a zero-padded sequence number. The file follows this structure:

- **Status** — Accepted, Superseded by ADR-XXX, or Rejected
- **Context** — what's the situation that needs a decision
- **Decision** — what we chose
- **Alternatives considered** — what else we looked at, why we didn't pick them
- **Consequences** — what becomes easier, what becomes harder, what we're
  giving up

ADRs should be short. Two to four screens of text is typical. If an ADR
runs longer, it's probably trying to be more than one decision.

## Index

- [ADR-001: DMG-only scope](0001-dmg-only-scope.md)
- [ADR-002: Single-threaded emulator core](0002-single-threaded-core.md)
- [ADR-003: M-cycle CPU accuracy with switch/case dispatch](0003-m-cycle-switch-dispatch.md)
- [ADR-004: `std::expected` plus `assert` for error handling](0004-expected-and-assert.md)
- [ADR-005: Polymorphic cartridge model](0005-polymorphic-cartridge.md)
- [ADR-006: Skip boot ROM in v1](0006-skip-boot-rom.md)
- [ADR-007: Dear ImGui for the debugger UI](0007-dear-imgui.md)
- [ADR-008: Palette-index framebuffer](0008-palette-index-framebuffer.md)
