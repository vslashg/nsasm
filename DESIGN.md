# nsasm Design Document

## Overview

nsasm is a 65816 assembler/disassembler targeting SNES ROM development. Its core
distinguishing feature is **state-aware instruction processing**: the 65816's
status register flags (M and X) affect instruction encoding sizes, so both
assembly and disassembly must track processor state to work correctly.

The project is a C++ library with utility binaries layered on top. It uses Bazel
as its build system.

---

## Module Map

```
nsasm/
  Instruction representation:
    mnemonic.h/cc          - 65816 mnemonics and pseudo-ops (ADD, SUB)
    addressing_mode.h/cc   - The 23 addressing modes
    instruction.h/cc       - Core Instruction struct; encode, decode, simulate
    statement.h/cc         - Statement = Instruction | Directive

  Processor state:
    execution_state.h/cc   - StatusFlags, RegisterValue, Stack, ExecutionState

  Encoding/decoding:
    opcode_map.h/cc        - 256-entry bidirectional opcode table
    decode.h/cc            - Raw bytes → Instruction

  Parsing:
    token.h/cc             - Lexer: source text → tokens
    parse.h/cc             - Tokens → Statement/Label
    expression.h/cc        - Lazy-evaluated expression trees
    numeric_type.h/cc      - 8/16/24-bit type system for expressions

  Assembly:
    directive.h/cc         - Assembler directives (.org, .equ, .db, ...)
    module.h/cc            - Single-file two-pass assembler
    assembler.h/cc         - Multi-module assembly with dependency resolution

  Disassembly:
    disassemble.h/cc       - Flow-sensitive disassembler

  ROM/memory:
    address.h/cc           - 24-bit SNES addresses and label values
    memory.h/cc            - InputSource / OutputSink interfaces
    rom.h/cc               - SNES ROM loading, mapping, patching
    ranges.h/cc            - Non-overlapping memory range tracking

  Supporting:
    calling_convention.h/cc - Subroutine return conventions
    identifiers.h/cc        - Qualified (module::label) names
    error.h/cc              - Error type and ErrorOr<T> result type
    file.h/cc               - Text file abstraction
```

---

## Key Data Types

### `Instruction`

The central representation of a single 65816 instruction:

- `mnemonic` — which operation (e.g., `M_lda`, `M_jmp`)
- `suffix` — optional size hint (`S_b`, `S_w`)
- `addressing_mode` — one of 23 modes (e.g., `A_dir_w`, `A_ind_by`)
- `arg1`, `arg2` — lazily-evaluated `Expression` trees
- `return_convention` — for JSR/JSL: what state callee returns with
- `location` — source position for error reporting

`Instruction` has methods for all four major operations:

| Method                      | Purpose                                               |
| --------------------------- | ----------------------------------------------------- |
| `CheckConsistency(flags)`   | Validate addressing mode is legal given current flags |
| `FixAddressingMode(flags)`  | Resolve sentinel modes to concrete ones               |
| `Execute(state)`            | Simulate instruction, update `ExecutionState`         |
| `Assemble(addr, ctx, sink)` | Encode to bytes                                       |

### `ExecutionState`

Tracks complete 65816 processor state for flow analysis. All fields use abstract
value domains rather than concrete values, to handle the case where state is
partially unknown.

**`StatusFlags`** — the M, X, E, and C bits, each stored in 2 bits using a
4-valued domain:

| Value        | Meaning                                |
| ------------ | -------------------------------------- |
| `B_off`      | Known 0                                |
| `B_on`       | Known 1                                |
| `B_original` | Unknown, but same as at function entry |
| `B_unknown`  | Fully unknown                          |

M and X are constrained by E: when E=1 (emulation mode), both are forced on.

**`RegisterValue`** — A, X, Y, and DBR registers, each one of:

- `T_unknown` — no information
- `T_original` — unchanged from function entry
- `T_value(v)` — known specific value

**`Stack`** — a simulated stack that tracks type information per entry (pushed
flags, pushed A high byte, etc.). This allows recovering register values after
push/pull sequences. When the stack becomes inconsistent (e.g., unknown number
of pushes), analysis degrades gracefully.

**Merging:** When control flow paths rejoin, states are merged using a join
operation. `B_on | B_off` → `B_unknown`; `B_original | B_original` →
`B_original`. This is used by the disassembler at branch targets.

### `Expression`

Instruction arguments are stored as expression trees, evaluated lazily via a
`LookupContext`. Subclasses:

- `Literal` — constant integer value
- `IdentifierExpression` — label or `.equ` name
- `BinaryExpression` — `+`, `-`, `*`, `/`
- `UnaryExpression` — `<` (low byte), `>` (high byte), `^` (bank byte), negation
- `Label` — wraps an expression with a display name (used in disassembler
  output)

Evaluation can fail (unresolved symbol, type mismatch), returning
`ErrorOr<int>`.

### `Statement`

A variant holding either an `Instruction` or a `Directive`. Directives include
`.org`, `.entry`, `.equ`, `.mode`, `.db`/`.dw`/`.dl`, `.begin`/`.end`. Like
instructions, directives can `Execute()` to update processor state and
`Assemble()` to emit bytes.

### `Address` and `LabelValue`

`Address` is a 24-bit SNES address expressed as `(bank, bank_address)`.
Arithmetic wraps within banks. `LabelValue` can be either a plain integer or an
`Address`, and drives how symbol values are formatted and used.

---

## Assembly Pipeline

```
Source .asm files
      │
      ▼  File::OpenFile()
  File (lines of text)
      │
      ▼  Module::LoadAsmFile()
  Module (parsed Statements + Labels)
      │
      │  Assembler::AddAsmFile() × N files
      ▼  Assembler::Assemble()
      │
      ├─ FindAssemblyOrder()        topological sort of .equ dependencies
      ├─ Module::RunFirstPass() × N  simulate execution to fix instruction sizes
      ├─ Module::RunSecondPass() × N evaluate .equ expressions
      └─ Module::Assemble() × N     encode instructions → OutputSink
```

### Two-Pass Assembly

The 65816's status-flag-dependent instruction sizes require knowing sizes before
addresses, and knowing addresses before evaluating labels. nsasm resolves this
with two passes per module:

**First pass** — simulate execution from `.entry` directives, tracking
`ExecutionState` line by line. This resolves sentinel addressing modes
(`A_imm_fm`, `A_imm_fx`) to concrete ones, establishing the byte size of each
instruction and therefore the address of each label.

**Second pass** — evaluate `.equ` expressions now that all labels have
addresses. `.equ` expressions may reference symbols defined in other modules;
`Assembler` runs a topological sort over the module dependency graph first.

### Cross-Module References

Labels in module `foo` can be referenced from module `bar` as `foo::label`. The
`Assembler` provides an `AssemblerLookupContext` that resolves these across
module boundaries.

---

## Disassembly Pipeline

```
ROM binary
      │
      ▼  Rom::LoadRomFile()
   Rom (implements InputSource)
      │
      ▼  Disassembler::Disassemble(entry_address, initial_flags)
      │
      │  Worklist: (address, ExecutionState)
      │
      ├─ Read bytes at address
      ├─ Decode(bytes, state.flags) → Instruction
      ├─ instruction.Execute(&state)     update state
      ├─ Store DisassembledInstruction
      ├─ Follow branch targets           add to worklist
      └─ Continue to next instruction (unless exit or unconditional branch)
      │
      ▼  DisassemblyMap: address → DisassembledInstruction
```

Disassembly is flow-sensitive: each address is processed with the
`ExecutionState` that would be in effect when execution reaches it. Branch
targets may be visited multiple times (from different source paths); states are
merged at each target, and the target is re-processed if its merged state
differs from what was previously used.

The disassembler generates labels automatically for all jump targets, and uses
`ReturnConvention` annotations on JSR/JSL calls to determine what state is in
effect after the call returns.

---

## Opcode Table

`opcode_map.h` contains a 256-entry table mapping each opcode byte to a
`(mnemonic, addressing_mode)` pair. Two sentinel addressing modes handle
status-flag-dependent immediate sizes:

- `A_imm_fm` — immediate size depends on M flag (used by LDA, ADC, etc.)
- `A_imm_fx` — immediate size depends on X flag (used by LDX, LDY, CPX, CPY)

At decode time, these sentinels are resolved to `A_imm_b` or `A_imm_w` based on
the current `ExecutionState`. If the relevant flag is `B_unknown`, decoding
fails with an error — this is by design, as ambiguous instruction sizes cannot
be safely decoded.

---

## ROM Handling

The 65816 uses 24-bit addresses (0x000000–0xFFFFFF). SNES ROMs are stored as
flat files with one of three bank mapping schemes:

| Mode    | Name    | Description                             |
| ------- | ------- | --------------------------------------- |
| $20/$30 | LoRom   | 32KB banks; gaps for hardware registers |
| $21/$31 | HiRom   | 64KB banks; continuous mapping          |
| $25/$35 | ExHiRom | Extended layout for large ROMs          |

`Rom::SnesToRomAddress()` handles conversion. All nsasm APIs work exclusively in
SNES address space; file offsets are never exposed to the user.

`RomOverwriter` holds a modified in-memory copy of the ROM and writes it back to
disk. `RomIdentityTest` is an `OutputSink` that validates a reassembly produces
identical bytes to the original — useful for round-trip testing.

---

## Error Handling

nsasm uses `ErrorOr<T>` pervasively instead of exceptions. `Error` carries a
human-readable message and an optional source location. Most operations return
`ErrorOr<T>` and callers propagate errors explicitly.

A `NSASM_RETURN_IF_ERROR(expr)` macro is provided to reduce boilerplate for
error propagation.

---

## Addressing Mode Naming

Addressing mode names follow the pattern `A_<type>_<size>`:

- Types: `imp` (implied), `acc` (accumulator), `imm` (immediate), `dir`
  (direct/absolute), `ind` (indirect), `stk` (stack-relative), `mov` (block
  move), `rel` (relative branch)
- Sizes: `b` (byte), `w` (word), `l` (long/24-bit)
- Modifiers: `x`/`y` (indexed), `i` (indirect), `il` (indirect long), `iy`/`ily`
  (indirect indexed)

---

## Notable Design Choices

**No macros.** Readability is prioritized over writability. The syntax aims to
be unsurprising: any line is self-contained.

**Explicit addressing sizes at reference sites.** `ADC something` is always
16-bit direct. `ADC @something` is always 24-bit. The `<` prefix extracts the
low byte. No implicit disambiguation based on where a symbol is defined.

**Shared state-tracking code.** The same `ExecutionState` machinery — and the
same `Execute()` methods on `Instruction` — is used by both the assembler (first
pass) and the disassembler. This avoids duplicating the simulation logic.

**Addresses over file offsets.** Users, tools, and diagnostics always use SNES
addresses. The ROM mapping math is hidden in `rom.cc`.

**No concern for performance.** The target platform is tiny; everything fits in
RAM and completes instantly. The code prefers clarity over economy.
