# nsasm syntax, next generation

This document describes the source syntax nsasm will adopt as part of the
RFC-002 generalization to multiple 6502-family targets (NES, FDS) alongside
the SNES.  It is written from the user's point of view: what you type, what
it means, and what changes if you have existing nsasm sources.

This is forward-looking documentation.  The syntax described here lands in
stages; see "Migration and compatibility" at the end for what breaks and
when.

## The one idea behind all of it

On the SNES, a bus address like `$c08000` tells you everything: what the CPU
sees, and which ROM byte lives there.  LoROM and HiROM are fixed mappings, so
one address serves both purposes.

On the NES this stops being true.  A mapper cart (MMC1, MMC3) can put
different ROM banks behind the same bus address at different times, and the
same ROM byte can appear at more than one bus address.  So nsasm now
distinguishes:

- **Bus addresses** — what the CPU sees on its address pins.  Hardware
  registers, RAM variables, jump targets.  `$2000` on the NES is PPUCTRL,
  always.
- **Storage addresses** — a durable location in the cartridge: *this byte of
  this PRG bank*.  This is where your code and data actually live, and what
  labels on code refer to.

The connection between the two depends on machine state (which bank is
currently switched in), and nsasm tracks that state the same way it already
tracks the 65816 `m` and `x` flags.  When the tracking can't follow — a bank
number loaded from a table, say — you tell the assembler what to assume,
exactly as `.mode` does for flags today.

Everything below is the syntax-level surface of that idea.  If you only
target the SNES, almost all of it is optional: the mapping is fixed, so the
distinction rarely shows.

## The layout file

Each project gets a small layout file declaring the machine and its memory
regions.  This replaces header-sniffing the ROM for LoROM/HiROM:

```
machine nes
mapper  mmc1
region  prg0  storage=prg_rom[0x00000:0x04000]  window=$8000, $C000
region  prg7  storage=prg_rom[0x1C000:0x20000]  window=$C000  fixed
region  ram   bus=$0000:$0800
region  hw    bus=$2000:$4020  mmio
```

Reading this:

- `region prg0 ... window=$8000, $C000` — the first 16KB PRG bank, which the
  mapper can place at either bus window.
- `region prg7 ... fixed` — the last bank, hard-wired at `$C000`.
- `region ram` — console RAM: bus addresses, no cartridge storage.
- `region hw ... mmio` — hardware registers: bus addresses with no contents
  at all.

Region names (`prg0`, `prg7`, ...) are yours to choose, and become the
vocabulary used by `.segment`, `.assume`, and disassembler output.

A SNES layout file is trivial (`machine snes` / `mapping lorom`), and nsasm
provides the standard ones.

> The exact grammar of this file is still settling; the shape and semantics
> above are what's committed.

## Placing code: `.segment` and `.org`

Today, `.org $c08000` fully specifies where code goes, because a SNES bus
address is unambiguous.  On a mapper cart, `.org $8000` is not enough
information — *which bank?*  Placement is now two coordinates:

```
.segment "prg3"
.org $8000        ; this code assumes it is mapped in at $8000
```

`.segment` names the storage region (from the layout file) the following
code is stored in.  `.org` states the bus address the code expects to run
at.  Both matter: the first determines where bytes land in the ROM image,
the second determines how address operands and branches are computed.

**SNES sources are unaffected.**  `.org $c08000` alone remains valid: when
the mapping is fixed, the region is derivable from the bus address, and
nsasm derives it.

## Defining names: `=`, `:=`, and labels

`.equ` is retired, replaced by two operators (borrowed from ca65) that make
a distinction `.equ` couldn't express:

```
NUM_LIVES = 3         ; a plain number.  No address semantics.
PPUCTRL  := $2000     ; a bus address.  Hardware registers and RAM variables.
start:   LDA PPUSTATUS ; a label: the storage address of placed code/data.
```

- **`NAME = expr`** defines a constant.  It's a number; using it never
  involves address mapping.
- **`NAME := expr`** defines a *bus address*.  This is how you declare
  hardware registers and RAM locations — things that have an address but no
  bytes in the ROM image.  nsasm will ship per-machine include files
  (`PPUCTRL`, `OAMDMA`, ... for the NES) declared this way.
- **`label:`** on placed code or data binds, as always, to that code — now
  formally a *storage* address, so the label means "this byte of this bank"
  no matter where the bank is currently mapped.

If you've been using `.equ` for constants, switch to `=`.  If you've been
using it for hardware registers or RAM addresses, switch to `:=`.  The
assembler can now tell the difference, and uses it to catch real mistakes —
for instance, subtracting a label in one bank from a label in another is an
error rather than a meaningless number.

## Using addresses in operands: the rendering prefixes

Unchanged in spirit from today, and worth restating precisely.  A prefix on
a label operand is a **rendering contract, not a value transformation**.
All three forms refer to the same underlying address; the prefix selects the
operand encoding, and the assembler *verifies* the address is actually
reachable that way:

- `ADC label` — 16-bit operand.  Asserts the label is reachable at a 16-bit
  bus address given everything the assembler knows about current mapper /
  bank state.  (On the SNES this is the familiar "same bank as PC" rule.)
- `ADC <label` — 8-bit zero-page / direct-page operand, with the matching
  reachability assertion.
- `ADC @label` — 24-bit long operand.  65816 only.

If the assertion can't be proven, that's an error at assembly time — not a
silently wrong byte.  You fix it either by using a different prefix or by
telling the assembler more about machine state (`.assume`, below).

As today, symbol references default to 16-bit with no prefix, so a line of
code reads locally without hunting for the label's definition.

### Byte extraction is now spelled out

The `>` (high byte) and `^` (bank byte) prefix operators are **removed**.
Extracting a numeric byte from an address — for building pointer tables, or
loading half an address into a register — is a genuinely different operation
from rendering an address as an operand, and it now looks different:

```
LDA #low_byte(handler)    ; was: LDA #<handler (ambiguous with the prefix)
LDX #high_byte(handler)   ; was: LDX #>handler
LDY #bank_byte(handler)   ; was: LDY #^handler
```

These are ordinary functions producing ordinary numbers.  `<` survives only
as the zero-page rendering prefix described above.

## Telling the assembler about machine state

### `.mode` (unchanged)

`.mode` still asserts 65816 flag state where the assembler's own tracking
loses the thread.  Nothing changes here; it's the model for what follows.

### `.assume` (new)

The mapper-state analogue of `.mode`.  Where flag tracking can lose the `m`
bit, bank tracking can lose a bank register — most commonly because the code
loads the bank number from a table.  `.assume` states the fact the assembler
can't derive:

```
.assume prg_lo = prg3     ; the switchable low window holds bank 3 here
```

From that point in the analysis, operands and jump targets through that
window resolve against bank 3, and the usual rendering assertions are
checked against it.

Expect to use this in real NES work — not as a rare escape hatch, but the
way `.mode` gets used in real 65816 code today.  Bank-switching code is
exactly the code the static analysis cannot see through, and `.assume` is
how you carry it across.

### `.entry` (extended)

`.entry` already declares the flag state at an entry point.  It now carries
mapper/system state too, so an entry point can say both "m is 8-bit here"
and "bank 3 is mapped low here" in one place.

## A complete NES example

```
; layout file: mmc1.layout (see above)

.segment "prg7"           ; the fixed bank
.org $C000

reset:
    SEI
    LDA #%00010000
    STA PPUCTRL           ; PPUCTRL := $2000, from nes.inc — a bus address

    LDA #3
    JSR switch_bank       ; maps prg3 into the low window
    .assume prg_lo = prg3
    JSR draw_title        ; draw_title lives in prg3; nsasm checks it is
                          ; reachable at a 16-bit bus address right now
    ...

.segment "prg3"
.org $8000

draw_title:               ; a storage address: prg3, offset 0
    ...
```

## Migration and compatibility

There is exactly **one backward-incompatible release**, staged while SNES is
still the only target so that NES support arrives later as a purely additive
change.  What it means for existing sources:

| You write today            | You will write                                |
| -------------------------- | --------------------------------------------- |
| `NAME .equ expr` (number)  | `NAME = expr`                                 |
| `NAME .equ addr` (hw/RAM)  | `NAME := addr`                                |
| `#>label`                  | `#high_byte(label)`                           |
| `#^label`                  | `#bank_byte(label)`                           |
| `#<label` (byte extract)   | `#low_byte(label)`                            |
| `.org $c08000`             | unchanged                                     |
| `label` / `<label` / `@label` operands | unchanged                         |
| `.mode`, `.entry`          | unchanged (`.entry` gains optional state)     |

The conversions are mechanical, and line-oriented enough that a sed script
covers most of a codebase; a fixer may accompany the release.

## Known rough edges

Being honest about what is not yet settled:

- **Ambiguous windows.**  When a bank is visible at two bus windows at once,
  a plain `label` operand could render as either.  Rule: nsasm uses the
  window it can *prove* from tracked state, and errors asking for an
  `.assume` otherwise.
- **RAM variables with sizes.**  `NAME := addr` declares a location but not
  an extent.  A way to declare "N bytes reserved at this bus address" that
  participates in the region system is still being designed.
- **`.assume` and disassembly.**  How assumptions attach to a disassembly in
  progress (as opposed to a source file you're assembling) is still an open
  UX question.
