# A design challenge

This toolchain performs 65816 disassembly by maintaining a subset of possible runtime states (register size flags, etc.)  This model could be made more generic for other 6502-family environments, in particular the NES with mapper chip carts or Famicom disk system games.

This requires being much clearer in code about the differences between logical addresses (actual 8 and 16-bit runtime values) and "physical" addresses (which might even go beyond numbers -- we might differentiate between NES hardware register or individual FDS files.  Currently we just have a single address data type.

This also may cause revisiting nsasm syntax and variable / linking rules, and making backward-incompatible changes.  The distinction between physical and logical addresses isn't even captured in the syntax, really -- so real changes may be needed here.  We should try to move closer to cc65 ideas and syntax where we change things.

Come up with a high-level design for what changes are needed to nsasm syntax and code layout.
