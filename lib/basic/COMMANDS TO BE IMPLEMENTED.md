# BASTOS Commands To Be Implemented

This document lists BASTOS keywords that are defined in `create-keywords.sh` but
not yet documented in the manual, likely because they are not yet implemented or
fully functional.

## List of Commands

### BIN
Binary conversion function (similar to CHR$, STR$)

### EDIT
Line editor command for modifying program lines

### ORIGIN
Set graphics origin point (for PLOT/UNPLOT coordinate system)

### PEEK
Read byte from memory address
```basic
value = PEEK address
```

### POKE
Write byte to memory address
```basic
POKE address, value
```

### USR
Call machine code routine
```basic
result = USR address
```

### RAND
Random function (relationship with RND to be clarified)

### TAB
Tabulation/cursor positioning function

### DEBUG
Debug mode activation/control

---

**Note**: These commands are reserved keywords in the BASTOS tokenizer but their
implementation status should be verified before documenting them in the main
manual.
