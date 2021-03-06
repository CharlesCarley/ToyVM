# Manual

1. [Manual](#manual)
    1. [Definitions](#definitions)
        1. [Registers](#registers)
        2. [Syntax](#syntax)
    2. [Sections](#sections)
        1. [Data types](#data-types)
    3. [Basic Operations](#basic-operations)
        1. [ret](#ret)
        2. [mov R0, R1](#mov-r0-r1)
        3. [mov R0, V](#mov-r0-v)
        4. [mov PC, V](#mov-pc-v)
        5. [mov PC, R0](#mov-pc-r0)
        6. [inc R0](#inc-r0)
        7. [dec R0](#dec-r0)
    4. [Branching](#branching)
        1. [bl SYM](#bl-sym)
        2. [bl ADDR](#bl-addr)
        3. [b ADDR](#b-addr)
    5. [Conditional branching](#conditional-branching)
        1. [cmp R0, R1](#cmp-r0-r1)
        2. [cmp R0, V](#cmp-r0-v)
        3. [cmp V, R0](#cmp-v-r0)
        4. [beq ADDR](#beq-addr)
        5. [bne ADDR](#bne-addr)
        6. [ble ADDR](#ble-addr)
        7. [bge ADDR](#bge-addr)
        8. [blt ADDR](#blt-addr)
        9. [bgt ADDR](#bgt-addr)
    6. [Math operations](#math-operations)
        1. [op R0, R1|V](#op-r0-r1v)
        2. [op R0, R1|V, R2|V](#op-r0-r1v-r2v)
    7. [Stack operations](#stack-operations)
        1. [stp  SP, V](#stp-sp-v)
        2. [ldp  SP, V](#ldp-sp-v)
        3. [str R0, [SP, V|R1]](#str-r0-sp-vr1)
        4. [ldr R0, [SP, V|R1]](#ldr-r0-sp-vr1)
    8. [Data access](#data-access)
        1. [adrp R0, ADDR](#adrp-r0-addr)
        2. [add R0, R1, ADDR](#add-r0-r1-addr)
        3. [strs R0, [R1:ADDR, R2|V]](#strs-r0-r1addr-r2v)
        4. [ldrs R0, [R1:ADDR, R2|V]](#ldrs-r0-r1addr-r2v)
    9. [Debugging](#debugging)
        1. [prg R0](#prg-r0)
        2. [prgi](#prgi)

## Definitions

| Name | Description                                                                    |
|:-----|:-------------------------------------------------------------------------------|
| R(n) | Is any value with the prefix b,w,l or x followed by a single digit [0-9]       |
| V    | Is any integer value in base 10, hexadecimal, binary, or a character constant. |
| PC   | Is the program counter.                                                        |
| SP   | The stack pointer.                                                             |
| SYM  | Refers to a symbol loaded from a dynamic library.                              |
| ADDR | Is a local address in the file referring to a label.                           |

### Registers

There are a total of 10 64 bit registers that can be used.

| Registers | Size   | Offset | C/C++    |
|:----------|:-------|-------:|:---------|
| b(n)      | 8-bit  |      8 | char[8]  |
| w(n)      | 16-bit |      4 | short[4] |
| l(n)      | 32-bit |      2 | int[2]   |
| x(n)      | 64-bit |      0 | int64_t  |

### Syntax

```asm
;----------------------------------------
             .data
;----------------------------------------
string:   .asciz  "Hello World"
var1:     .byte   'a'
var2:     .word   0xFFFF
var3:     .long   0xFFFFFFFF
var4:     .xword  0xFFFFFFFFFFFFFFFF
buffer:   .zero   128
;----------------------------------------
             .text
;----------------------------------------
                ; comment
main:           ; label
   mov x0, 0    ; op dest, src
top:
   cmp x0, 10
   bge done
   inc x0
   b   top
done:
   mov x0, 0    ; return value is in x0
   ret
```

## Sections

The code currently uses two section types for grouping declarations.

+ .data
+ .text

By default everything is in the text section and does not have to be supplied. The compiler will toggle between states every time a data section is found.

### Data types

The data section supports the following types:

| Type   | Description                         |
|:-------|:------------------------------------|
| .asciz | Represents an ASCII sequence.       |
| .zero  | Is a zeroed filled block of memory. |
| .byte  | 1-byte integer.                     |
| .word  | 2-byte integer.                     |
| .long  | 4-byte integer.                     |
| .xword | 8-byte integer.                     |
| .quad  | Same as .xword.                     |

  *At the moment all integers are written to the file as an 8-byte integer.*

## Basic Operations

### ret

+ Removes a branch from call stack and assigns an internal return code from register 0.

```asm
    mov x0, 0
    ret
```

### mov R0, R1

+ Sets the value in R0 to the value in R1.

```asm
    mov x0, x1
```

### mov R0, V

+ Sets the value in R0 to the constant V.

```asm
    mov x0, 123
    mov x0, 0xFF
    mov x0, 0b1010
    mov x0, 'A'
```

### mov PC, V

+ Explicitly sets the value of the program counter.

```asm
    mov pc, 1
    ret
```

### mov PC, R0

+ Sets the value of the program counter to the value in R0.

```asm
    mov x0, 2
    mov pc, x0
    ret
```

### inc R0

+ Increments the R0 register value by 1.

```asm
    mov x0, 'A'
    inc x0
    bl putchar
```

### dec R0

+ Decrements the R0 register value by 1.

```asm
    mov x0, 'B'
    dec x0
    bl putchar
```

## Branching

### bl SYM

+ Branches to a symbol defined in C/C++.

```asm
    mov x0, 'A'
    bl putchar
```

### bl ADDR

+ Branches to label ADDR and stores a link back to the calling instruction.

```asm
fn1:
    mov, 'A'
    bl   putchar
    ret

main:
    bl  fn1
    mov x0, 0
    ret
```

### b ADDR

+ Moves the current instruction to the location found in ADDR.

```asm

L2:
    ret
L1:
    mov x0, 0
    b   L2
```

## Conditional branching

### cmp R0, R1

+ Subtracts R1 from R0 then marks a flag based on the sign.

| Result | Flag |
|--------|------|
| R == 0 | Z    |
| R < 0  | L    |
| R > 0  | G    |

### cmp R0, V

+ Compares R0 with a constant.

### cmp V, R0

+ Compares a constant with R0.

### beq ADDR

+ Branch to the location found in ADDR if the Z flag is set.

```asm
    mov x0, 0
    cmp x0, 0
    beq L1:
    b   L2
L1:
    ret
L2:
    mov x0, -1
    ret
```

### bne ADDR

+ Branch to the location found in ADDR if the Z flag is not set.

```asm
    mov x0, 0
    cmp x0, 0
    bne L1
    b   L2
L1:
    mov x0, -1
    ret
L2:
    ret
```

### ble ADDR

+ Branch to the location found in ADDR if the Z flag is or the L flag is set.

### bge ADDR

+ Branch to the location found in ADDR if the Z flag is or the G flag is set.

### blt ADDR

+ Branch to the location found in ADDR if L flag is set.

### bgt ADDR

+ Branch to the location found in ADDR if G flag is set.

## Math operations

+ Where **op** is one of the following:

+ add
+ sub
+ mul
+ div
+ shr
+ shl

### op R0, R1|V

+ Preforms the operation on R0 and R1 and stores the result in R0.

```asm
    mov x0, 2
    mov x1, 2
    add x0, x1
    add x0, 2
    ret
```

### op R0, R1|V, R2|V

+ Preforms the operation on the R1 and R2 registers and stores the result in R0.

```asm
    mov x0, 2
    mov x1, 2
    add x3, x0, x1
    add x4, x0, 2
    ret
```

## Stack operations

### stp  SP, V

+ Stores V bytes of stack space.

```asm
    stp sp, 16
```

*V should be less than or equal to 256 bytes.
Stack objects are also stored in an 8 byte integer.*

### ldp  SP, V

+ Pops V bytes off the stack.

```asm
    ldp sp, 16
```

### str R0, [SP, V|R1]

+ Stores R0 on the stack at the supplied offset found in V or R1.

```asm
    stp sp, 8
    mov x0, 100
    str x0, [sp, 0]
    mov x0, 0
    ldp sp, 8
```

*If V is used, then the maximum value is truncated at 255.*

### ldr R0, [SP, V|R1]

+ Loads into R0 the value found on the stack at the offset found in V or R1.

```asm
    stp sp, 8
    mov x0, 100
    str x0, [sp, 0]
    mov x0, 0
    ldr x0, [sp, 0]
    ldp sp, 8
```

*If V is used, then the maximum value is truncated at 255.*

## Data access

### adrp R0, ADDR

+ Loads the memory address found at ADDR and puts it into R0.

```asm
    adrp x0, string
```

### add R0, R1, ADDR

+ Dereferences the memory address in R1 then places it in R0.

```asm
    adrp x1, string
    add  x0, x1, string
    bl   puts
```

### strs R0, [R1:ADDR, R2|V]

+ Stores the R0 register value in the R1 memory address at the supplied index found in R2 or V.

```asm
    adrp x0, buffer
    mov  x1, 'A'
    strs x1, [x0, 0]
```

### ldrs R0, [R1:ADDR, R2|V]

+ Loads the value in the R1 memory address at the supplied index found in R2 or V into the R0 register.

```asm
    adrp x0, string
    ldrs x1, [x0, 0]
```

## Debugging

### prg R0

+ Prints the value in R0 to stdout.
  
### prgi

+ Prints contents of all registers to stdout.
