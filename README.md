# Nerd Snipe Assembler (nsasm)

nsasm is a new 65816 assembler/disassembler, with an initial focus on SNES development.  A lot of work remains to be done, but enough exists at this point that I can semi-credibly claim that this is the beginnings of an actual toolchain.

I know the world wasn’t asking for yet another 6502-family toolchain.  Certainly the SNES hacking community wasn’t.  So why does nsasm exist?  It’s a fair question.

It’s called the “Nerd Snipe Assembler” because I was nerd-sniped into writing it.  A friend working on SNES romhacks challenged me to apply a trivial modification to a SNES game using existing popular tools.  His assumption was that I would be so bothered by the process, he could trick me into writing new tools.

My friends know me well.  I'm amazed I've gotten this far.

Software library development is my career, and nsasm reflects my thoughts about how a proper toolchain ought to behave.  It cares about the things I care about, which might make me happier than it makes you.  But hey, it's free!  For the sake of clarity, I’ll share some of these guiding principles below.

## The assembler knows about SNES memory mappings and ROM file formats.

Attempting to use tools that didn't get this was how I got nerd-sniped into this project to begin with.

During both disassembly and hacking, you’re going to have to work with address literals.  These addresses are what the hardware abstraction works with, and it’s what human beings should use, too.  The notion of hackers reaching for tools to convert machine addresses to ROM file offsets and back is rather absurd.  It’s an unnecessarily low-level extra step, and it introduces pointless cognitive load.

nsasm tools always use addresses in the SNES address space to refer to locations in a ROM file.  In principle I’m certainly happy to support the binary formats and memory mappings of other 6502-family platforms.  I just have neither the knowledge or need to do this work at the moment.

## The semantics of the assembly code should be easily determined from the local context.

This is important: you should never have to look somewhere else to understand a line of code syntactically.  This guideline keeps the tools simple and the .asm files understandable.  This choice has some notable consequences.

### No macro system

Textual macros make things easier for the writer, but harder for the reader.  Because readability is _far_ more important than writability, this is a bad tradeoff.  Users who really need a macro system can use m4 or the like -- but they shouldn’t.

### Labels don’t carry type information.

Symbol references are always assumed to be 16-bit.  `ADC something` always assembles as 16-bit direct addressing.  This is true even if `something` happens to live in the zero page.  You shouldn’t have to look elsewhere in the file, or in other files, to know which addressing mode is being assembled.

When a label’s 24-bit value is needed, a `@` prefix is used.  `ADC @something` always assembles as long (24-bit) direct addressing (again, even if `something` is in bank zero).  There’s no 8-bit type prefix, exactly, but the `<` prefix operator (which extracts the low byte of a value) has the same effect.

## No textual inclusions

This also follows from the above.  When you want to refer to a label or `.equ` value from another file, just refer to it by name.  The linker will handle resolving the references in a consistent order.

## The assembler tracks the processor state via syntactic analysis

An unfortunate 65816 feature is that instruction decoding is configured by runtime flag.  The size of an opcode can differ based on status register bits.  There are two possible approaches to this problem during assembly: either the programmer must tell the assembler what the flag state is (by directive or instruction suffixes like `ADC.b $1234`), or the assembler can attempt to track the state in an analysis step.

I chose the latter.  This is a case where either approach feels fine.  My decision stems from the fact that nsasm is both a disassembler and an assembler.  Since a disassembler has to do this tracking in any event, I could use the same library code for doing the tracking in the assembler.

## Limit novel syntax.

65816 assembly syntax is what it is, for better and worse.  I’m not looking to overhaul it.  Obviously nsasm will have its own directives, syntax, and features where that makes sense, but this is not a free-for-all.  We’ll save the novelties for our ROM hacks.  The tools need to be unsurprising.

