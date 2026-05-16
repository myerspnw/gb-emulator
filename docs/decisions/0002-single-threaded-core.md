# ADR-002: Single-threaded emulator core

**Status:** Accepted

## Context

The Game Boy emulator could be structured with the emulation core running
on its own thread, with the frontend (SDL window, audio output, input
events, rendering) on another. This is a common pattern in production
emulators because:

- The audio subsystem has hard real-time requirements — the audio
  callback must be fed samples regularly or audio underruns and you
  hear clicks. Threading the emulator so it can run ahead of audio
  consumption avoids underruns when the frontend has a slow frame.
- Input latency is reduced if the event-handling thread isn't gated by
  emulation work.
- On multi-core machines, parallelism is often "free."

The counter-argument is that single-threaded code is simpler,
deterministic, easier to test, and easier to debug. The emulated Game Boy
runs at ~4 MHz; emulating it on a 4+ GHz host is not performance-bound,
so threading is not a performance necessity.

This decision affects testing infrastructure, save state design, and
debugger architecture.

## Decision

The v1 emulator is single-threaded throughout. The core, the frontend,
the SDL event pump, the renderer — everything runs on the main thread.
`GameBoy::run_frame()` is synchronous; it returns when the PPU completes
a frame, and the frontend then renders and presents.

Revisit the decision for v2 if and only if audio quality demands it
(once the real APU is implemented). We will not pre-build threading
machinery in v1.

## Alternatives considered

**Threaded core, single audio buffer.** The emulator runs on its own
thread, writing framebuffer and audio samples into thread-safe buffers.
The frontend reads from them. Adds: a mutex or atomic flag per buffer,
careful handoff of framebuffer to avoid tearing, lockfree ringbuffer for
audio. None of this is hard, but it's complexity that buys nothing for
v1 (audio is stubbed, frames are completed inside one host-microsecond).

**Threaded core, separate audio thread.** Three threads: main (events,
render), emulator, audio. Most accurate audio timing. Highest complexity.
Overkill for the project.

**Single-threaded with audio-driven timing later.** When the APU lands
in v2, SDL's audio callback can pull samples on its own thread (which
happens regardless of how we structure our code — SDL drives the audio
callback). The emulator can produce samples into a buffer the audio
callback drains. This works even with a single-threaded emulator core,
as long as the buffer is sized to cover one frame's worth of audio.
That's the v2 plan if we keep the architecture.

## Consequences

**Easier:**

- Determinism is automatic. Same ROM + same inputs = bit-identical
  output. Integration tests can hash framebuffers and compare across
  runs and across machines.
- Save states are trivial. There are no threads to suspend before
  serializing.
- Debugging is straightforward. A breakpoint in CPU code stops the
  whole program; you can step through without thread synchronization
  surprises.
- No risk of data races, no need for `std::atomic` or mutexes, no
  TSan-only failure modes.

**Harder / accepted limitations:**

- If a frame takes longer than 16.7ms to emulate (extremely unlikely on
  modern hardware), the UI is unresponsive for that frame. Acceptable
  given the workload.
- When the real APU lands in v2, we'll need to think about how audio is
  fed without underruns. SDL's callback-on-its-own-thread model handles
  this naturally; the emulator just needs to fill the buffer regularly.
- Frame pacing is tied to the render loop (VSync), which means on a
  144 Hz monitor the emulator runs faster than real-time. v2 concern.

**Future:** If v2 audio quality genuinely requires the emulator to run
on its own thread, the boundary is already clean: `GameBoy` is a value
type that holds its own state, with no global state. Putting it on a
separate thread requires adding synchronization at its public API but
no internal restructuring.
