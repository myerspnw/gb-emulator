# Integration tests

This directory will host the headless integration test suite that runs
Game Boy hardware test ROMs (Blargg, Mooneye, dmg-acid2) against
`gbe_core` and asserts on their outputs.

Currently empty. The `HeadlessRunner` harness and the first integration
tests land alongside the cartridge loader milestone (see
[`CLAUDE.md`](../../CLAUDE.md) "Current focus" and
[`docs/architecture.md`](../../docs/architecture.md#integration-tests)).

When this directory has tests, they will be wired into the build from
[`tests/CMakeLists.txt`](../CMakeLists.txt) alongside the unit tests.
