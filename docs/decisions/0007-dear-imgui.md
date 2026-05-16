# ADR-007: Dear ImGui for the debugger UI

**Status:** Accepted

## Context

The v1.5 milestone adds a built-in debugger UI that lets a user inspect
and control the running emulator: CPU registers, memory, VRAM contents,
sprite data, breakpoints, single-step controls. This is a non-trivial
UI: many panels, real-time data updates, interactive controls.

The debugger needs to run alongside the emulator inside the same
process. It needs to render at 60 fps while the emulator is running.
It needs to be reasonably nice-looking out of the box, since this is a
showpiece artifact and screenshots will appear in the README.

C++ has many GUI library options. The major contenders for a project
like this:

- **Dear ImGui** — immediate-mode GUI library. Renders inside another
  rendering context (SDL3 in our case). Industry standard for in-engine
  tooling.
- **Qt** — retained-mode toolkit. Massive ecosystem, polished native
  look. Substantial dependency (~100 MB build), separate windowing
  model.
- **wxWidgets** — retained-mode toolkit using native widgets per
  platform. Lighter than Qt but still substantial.
- **Web-based** — emulator runs a local HTTP server; browser-based UI
  connects to it. Decouples UI tech from C++.

Different debuggers in the emulator landscape have made different
choices: BGB uses native Win32 (proprietary), SameBoy uses Cocoa
(Mac-only), Higan/ares use Qt, mGBA uses Qt. Dolphin, RPCS3, and most
modern emulator-adjacent tools use ImGui.

## Decision

Use Dear ImGui for the v1.5 debugger UI, rendered inside the SDL window
via the official `imgui_impl_sdl3` and `imgui_impl_sdlrenderer3` backends.
The emulator's framebuffer and the debugger panels share the same SDL
window with ImGui's docking layout.

ImGui will be added as a FetchContent dependency in v1.5, pinned to a
specific release tag like our other dependencies.

## Alternatives considered

**Qt.** Strong, mature, the choice many serious emulators have made.
Cons: large dependency that significantly slows the build and inflates
the binary; separate event loop and windowing model that doesn't
integrate with SDL3 naturally; learning curve for QWidget / QML; pulls
in dozens of system dependencies. Overkill for an in-process debugging
overlay.

**wxWidgets.** Slightly lighter than Qt. Same fundamental drawbacks:
external windowing, separate event loop, build complexity.

**Web-based UI.** The emulator hosts a small HTTP/WebSocket server;
the user opens a browser to view the debugger. Pros: total decoupling
of UI tech from C++; UI can be styled freely; could even be a project
in its own right. Cons: adds an HTTP server dependency to the core
binary (or the frontend); cross-process communication has latency that
might be visible at 60 fps for a memory viewer; adds the second
discipline of web frontend development. Interesting and modern but
substantially more work than ImGui for the same showpiece value.

**Native Win32 / Cocoa / GTK.** Per-platform native code. Cross-platform
maintenance burden. Rejected.

**No GUI debugger; just a CLI debugger via stdin/stdout.** Tempting for
its minimalism. Cons: a graphical debugger is much more impressive in a
README screenshot than a text REPL. The visual tile/sprite/VRAM viewers
in particular are core showpiece elements and don't have a sensible CLI
form.

## Consequences

**Easier:**

- ImGui renders inside our existing SDL3 window. No second windowing
  system to learn or integrate. The frame loop becomes "run emulator
  frame, draw framebuffer, draw ImGui overlay, present" — one SDL
  present call covers both.
- ImGui's documentation and example code are extensive. The widget
  set covers everything we need: hex editors, treeviews, image
  displays for the VRAM viewer, plots if we want them later.
- ImGui is what serious emulator-tooling work in C++ looks like in
  2025. The signal to a reviewer is "this person uses the same tools
  the industry uses."
- ImGui's MIT license is compatible with our project's MIT license.

**Harder / accepted:**

- ImGui is another dependency to FetchContent. The build gets longer.
  Not a meaningful cost.
- ImGui's aesthetic is recognizable — its default styling is widely
  used in game-engine tooling. We accept that the debugger will *look
  like* an ImGui debugger. Custom styling is possible but adds work
  we won't do.
- Immediate-mode GUI has a learning curve if a contributor hasn't
  used one before. The mental model ("rebuild the UI every frame from
  current state") is different from retained-mode toolkits. The code
  is straightforward to read; it's writing-from-scratch that has the
  curve.

**Future:** ImGui supports add-on packages (ImPlot for charts,
ImGuiColorTextEdit for code views, etc.) if we want to expand the
debugger later. None of those are planned for v1.5.
