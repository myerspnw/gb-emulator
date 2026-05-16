# ADR-005: Polymorphic cartridge model

**Status:** Accepted

## Context

The Game Boy had several memory bank controller (MBC) chip types that
appear inside cartridges. The CPU's address space is 16-bit (64 KB
addressable), but ROMs grew to 8 MB by the end of the platform's life.
MBCs sit between the CPU and the ROM/RAM chips and let the CPU swap
which slice of ROM is currently visible by writing to "ROM addresses"
that are actually MBC control registers.

We support four cartridge types in v1: No-MBC (ROM-only, like Tetris),
MBC1, MBC3 (with RTC), and MBC5. Each has its own banking scheme,
register layout, and quirks. From the rest of the emulator's perspective,
they all do the same thing: respond to `read_rom(addr)`,
`write_rom(addr, value)`, `read_ram(addr)`, `write_ram(addr, value)`.

The question is how to model this in C++. The three reasonable choices
are:

- **Inheritance / virtual dispatch.** `class Cartridge` interface, one
  concrete class per MBC type.
- **`std::variant`.** A tagged union of the concrete types; dispatch
  via `std::visit`.
- **Templates / CRTP.** Each consumer of `Cartridge` is templated on
  the concrete type. Requires the Bus to be templated, which infects
  everything.

## Decision

Use virtual dispatch. `Cartridge` is an abstract base class with pure
virtual methods. Concrete subclasses (`NoMbc`, `Mbc1`, `Mbc3`, `Mbc5`)
implement the interface. The `Bus` holds a `std::unique_ptr<Cartridge>`.
A factory function reads the cartridge header and constructs the right
concrete type.

```cpp
class Cartridge {
public:
    virtual ~Cartridge() = default;
    virtual uint8_t read_rom(uint16_t addr) const = 0;
    virtual void    write_rom(uint16_t addr, uint8_t value) = 0;
    virtual uint8_t read_ram(uint16_t addr) const = 0;
    virtual void    write_ram(uint16_t addr, uint8_t value) = 0;
    // ... save-state hooks, RAM accessors ...
};

std::expected<std::unique_ptr<Cartridge>, CartridgeError>
make_cartridge(std::vector<std::byte> rom);
```

## Alternatives considered

**`std::variant<NoMbc, Mbc1, Mbc3, Mbc5>`.** The Bus holds the variant
by value; reads dispatch via `std::visit`. Pros: no heap allocation, no
vtable, modern idiom. Cons: every cartridge access becomes a `std::visit`
call, which is a switch on the variant's index plus inlined work. The
Bus does this on every CPU memory access — millions of times per second.
The cost is real but small. The bigger downside: the variant declaration
hardcodes the set of supported MBCs. Adding HuC1 or MBC2 later requires
changing every site that has the variant in its type. Polymorphism
absorbs this with a new subclass.

**Templates / CRTP.** Bus is templated on the Cartridge type. Zero
virtual call overhead, full inlining. But: Bus is templated, so its
header is in everyone's include path, the CPU has to know which
cartridge type is in play at compile time, and the whole thing breaks
the moment you want a runtime cartridge selection — which we do (we
load arbitrary ROMs at runtime). Templates aren't a workable shape for
this problem unless paired with type erasure, which lands us back where
polymorphism started.

**Function pointer table per cartridge.** Custom v-table by hand. No
benefit over polymorphism; just less clear.

## Consequences

**Easier:**

- Adding a new MBC type is a new subclass and a factory entry. Zero
  changes elsewhere.
- The interface is small and clear (4 read/write methods plus save-state
  hooks). Easy to test each MBC independently.
- Polymorphism is the textbook pattern for this exact shape of problem;
  it reads instantly to any C++ reviewer.

**Harder / accepted:**

- Each cartridge access incurs a vtable indirection (~1-2 ns). At
  emulated CPU speed (~4 MHz, 250 ns per emulated cycle), this is
  imperceptible. Even at host-clock speeds, it's well within budget.
- The cartridge lives on the heap (`std::unique_ptr`). This is one
  allocation per emulator instance, not per call; not a concern.
- Compile-time information about the cartridge type is lost — for
  instance, the compiler can't inline `Mbc1::read_rom` into the Bus's
  dispatch. We accept this.

**Future:** Adding MBC2, MBC6, MBC7, HuC1, HuC3, or any other cartridge
type is a new subclass. The interface may grow over time (e.g., adding
sensor support for MBC7's accelerometer) but the existing types just
default-implement the new methods.
