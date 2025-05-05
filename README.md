# Supernova Instruction Set

A reduced instruction set designed to be the first compiler target,
opcodes and behaviors might change in future releases.

## Summary

- [Compiling](#compiling)
- [Command line arguments](#command-line-arguments)

## Compiling 

To compiling the project, you will need the zig compiler

Then, in the project folder

```bash
zig build run
```

which *should* compile everything, including tests, but they are small so it is fine

## Command Line Options

- diagnostic options
  - `-h` or `--help`
    - print help to the screen
  - `-v` or `--version`
    - output the version of the compiled project into the screen
  - `-p` or `--properties`
    - print the properties to the screen about the current vm in a human
      readable format
- configuration options
  - `--thread-count=[count]` (no effect)
    - set current amount of concurrent threads to spawn for a process, 
      defaults to one
  - `--start-thread=[id]` (no effect)
    - set on which thread the code should start
  - `--add-search-path [path]` (no effect)
    - add paths to search for modules
  - `--add-module [name]`
    - add modules to the vm
- sandbox flags
  - `--memory-limit=[size][prefix]`
    - define the biggest size the vm memory pointer can handle, prefix
    needed
      - `b` or `B` for bytes, `size` * 1
      - `k` for kilobytes, `size` * 1000
      - `K` for kibibytes, `size` * 1024
      - `m` for megabytes, `size` * 1000000
      - `M` for mebibytes, `size` * 1048576
      - `g` for gigabytes, `size` * 1000000000
      - `G` for gibibytes, `size` * 1073741824
      - `t` for terabytes, `size` * 1000000000000
      - `T` for tebibytes, `size` * 1099511627776
    - not setting this value before can cause errors if 
      `main_header.memory_size` is corrupted or set to be a value
       greater than needed
  - `--load-modules=(true|false)`
    - if `false`, all modules are not loaded and the entire running code
    is sandboxed and very little features are available (only modules
    with i/o interfaces)
    - default is true
  - `--enable-[instr]=(true|false)`
    - enable certain instruction groups, disabling can be used to
      emulate even more reduced instruction sets, usually defaults true
    - `--enable-div`: enable integer division instructions `udivr`, `udivi`, `sdivr`, `sdivi`
    - `--enable-int`: enable interrupt instructions
    - `--enable-float`: enable all floating point instructions 
    - `--enable-ioint`: enable interrupts triggered by i/o ports
    - `--enable-stack`: enable stack instructions (defaults to false)


## Instruction Layouts

This table defines how are the instruction types laid out, bit by bit,
with the most significant bit first.

R type instructions (R standing for register) are instructions that
use 3 registers and normally operate as `rd <- r1 ○ r2`, as `r1`
referring to the first argument (and not register r01) and r2
referring to the second argument (and not register r02). Register
`rd` is the destination register, nothing stops `rd` to be `r1` or
`r2`

instruction type | bits 63-20 | bits 19-16 | bits 15-12 | bits 11-8 | bits 7-0
:-:|:-:|:-:|:-:|:-:|:-:
R type | unused | rd | r2 | r1 | op

instruction type | bits 63-16 | bits 15-12 | bits 11-8 | bits 7-0
:-:|:-:|:-:|:-:|:-:
S type | immediate | rd | r1 | op

instruction type | bits 63-12 | bits 11-8 | bits 7-0
:-:|:-:|:-:|:-:
L type | immediate | r1 | op

## Register Layouts

register index | used as | requirements
:-: | :-:               | :-:
r00 | zero register     | none, read-"only" because writing is discarded
r01 - r12 | general use | none
r13 | `pcall` return    | save before `pcall`, general use otherwise
r14 | `pcall` return    | save before `pcall`, general use otherwise
r15 | `pcall` parameter | `pcall` switch, general use otherwise


## Instruction Set Resume

- group zero: bitwise instruction group [opcodes `0x00 - 0x0F`]
  - andr: [opcode `0x00`, R type]
    - executes a bitwise AND between `r1` and `r2`, result on `rd`
    - executes: `rd <- r1 & r2`

  - andi: [opcode `0x01`, S type]
    - executes a bitwise AND between `r1` and a mask immediate, result on `rd`
    - executes: `rd <- r1 & imm`

  - xorr: [opcode `0x02`, R type]
    - executes a bitwise XOR between `r1` and `r2`, result on `rd`
    - executes: `rd <- r1 ^ r2`

  - xori: [opcode `0x03`, S type]
    - executes a bitwise XOR between `r1` and a mask immediate, result on `rd`
    - executes: `rd <- r1 ^ imm`

  - orr: [opcode `0x04`, R type]
    - executes a bitwise OR between `r1` and `r2`, result on `rd`
    - executes: `rd <- r1 | r2`

  - ori: [opcode `0x05`, S type]
    - executes a bitwise OR between `r1` and a mask immediate, result on `rd`
    - executes: `rd <- r1 | imm`

  - not: [opcode `0x06`, R type]
    - executes a one's complement on `r1`, discard `r2`, result on `rd`
    - executes: `rd <- ~r1`

  - cnt [opcode `0x07`, S type]
    - executes a population count on register `r1`, excluding the highest `imm` bits, result on `rd`
    - executes: `rd <- popcnt(r1 & ((1 << imm) - 1))`
    - edge case:
      - if `imm >= 64`, clear `rd`

  - llsr [opcode `0x08`, R type]
    - executes a logical left shift on register `r1` `r2`, result on `rd`
    - executes: `rd <- r1 << r2`

  - llsi [opcode `0x09`, S type]
    - executes a logical left shift on register `r1` `imm` bits, result on `rd`
    - executes: `rd <- r1 << imm`
    - edge case:
      - if `imm >= 64`, clear `rd`

  - lrsr [opcode `0x0A`, R type]
    - executes a logical right shift on register `r1` for `r2` bits, result on `rd`
    - executes: `rd <- r1 >> r2`
    - edge case:
      - if `r2 >= 64`, clear `rd`

  - lrsi [opcode `0x0B`, S type]
    - executes a logical right shift on register `r1` for `imm` bits, result on `rd`
    - executes: `rd <- r1 >> imm`
    - edge case:
      - if `imm >= 64`, clear `rd`

  - reserved instructions block 0: opcodes [`0x0C` until `0x0F`]

- group one:
  - addr [opcode `0x10`, R type]
    - adds `r2` to `r1` and set `rd` as the result
    - executes: `rd <- r1 + r2`
    - edge case:
      - overflow is discarded

  - addi [opcode `0x11`, S type]
    - adds `imm` to `r1` and set `rd` as the result
    - executes: `rd <- r1 + imm`
    - edge case:
      - overflow is discarded

  - subr [opcode `0x12`, R type]
    - subtracts `r2` from `r1` and set `rd` as the result
    - executes: `rd <- r1 - r2`
    - edge case:
      - overflow is discarded

  - subi [opcode `0x13`, S type]
    - subtracts `imm` from `r1` and set `rd` as the result
    - executes: `rd <- r1 - imm`
    - edge case:
      - overflow is discarded

  - umulr [opcode `0x14`, R type]
    - set `rd` to `r2 (unsigned)` times `r1 (unsigned)`
    - executes: `rd <- u64(r1) * u64(r2)`
    - edge case:
      - overflow is discarded

  - umuli [opcode `0x15`, S type]
    - set `rd` to `imm (unsigned)` times `r1 (unsigned)` 
    - executes: `rd <- u64(r1) * u64(imm)`
    - edge case:
      - overflow is discarded

  - smulr [opcode `0x16`, R type]
    - set `rd` to `r2 (signed)` times `r1 (signed)` 
    - executes: `rd <- i64(r1) * i64(r2)`
    - edge case:
      - overflow is discarded

  - smuli [opcode `0x17`, S type]
    - set `rd` to `imm` times `r1` 
    - executes: `rd <- i64(r1) * i64(imm)`
    - edge case:
      - overflow is discarded

  - udivr [opcode `0x18`, R type]
    - set `rd` to `r1 (unsigned)` divided by `r2 (unsigned)`
    - executes: `rd <- u64(r1) / u64(r2)`
    - edge case:
      - overflow is discarded
      - `r2 = 0` triggers `pcall 1`

  - udivi [opcode `0x19`, S type]
    - set `rd` to `r1 (unsigned)` divided by `imm (unsigned)`
    - executes: `rd <- u64(r1) / u64(imm)`
    - edge case:
      - overflow is discarded
      - `imm = 0` triggers `pcall 1`

  - sdivr [opcode `0x1A`, R type]
    - set `rd` to `r1 (signed)` divided by `r2 (signed)`
    - executes: `rd <- i64(r1) / i64(r2)`
    - edge case:
      - overflow is discarded
      - `r2 = 0` triggers `pcall 1`

  - sdivi [opcode `0x1B`, S type]
    - set `rd` to `r1 (signed)` divided by `imm (signed)`
    - executes: `rd <- i64(r1) / i64(imm)`
    - edge case:
      - overflow is discarded
      - `imm = 0` triggers `pcall 1`

  - call [opcode `0x1C`, R type] (deprecated)
    - change execution context to another place
    - semantic renaming: `call rd, r1, r2` -> `call addr, sp, bp`
    - executes:
      - `u64[sp + 0] <- bp`
      - `u64[sp + 8] <- pc + 8`
      - `sp <- sp + 16`
      - `bp <- sp`
      - `pc <- addr`

  - push [opcode `0x1D`, S type] (deprecated)
    - push a value into given stack
    - semantic renaming `push rd, r1, imm` -> `push rv, sp, imv`
    - executes:
      - `u64[sp] <- rv + imv`
      - `sp <- sp + 8`

  - retn [opcode `0x1E`, R type] (deprecated)
    - return execution to previous context
    - semantic renaming `retn rd, r1, r2` -> `retn x0, sp, bp`
    - executes:
      - `sp <- sp - 16`
      - `bp <- u64[sp + 0]`
      - `pc <- u64[sp + 8]`
    - `x0` is ignored

  - pull [opcode `0x1F`, S type] (deprecated)
    - pull a value out of a given stack
    - semantic renaming `pull rd, r1, imm` -> `pull rv, sp, #0`
    - executes:
      - `sp <- sp - 8`
      - `rv <- u64[sp]`
    - `#0` is ignored

- group two:
  - ldb [opcode `0x20`, S type]
    - load byte from memory into a register
    - executes: `rd <- r0 | u8[r1 + imm]`
    - side effects:
      - if `r1 + imm` is bigger than memory size, `pcall 4` is triggered

  - ldh [opcode `0x21`, S type]
    - load half word from memory into a register
    - executes: `rd <- r0 | u16[r1 + imm]`
    - side effects:
      - if `r1 + imm` is bigger than memory size, `pcall 4` is triggered

  - ldw [opcode `0x22`, S type]
    - load word from memory into a register
    - executes: `rd <- r0 | u32[r1 + imm]`
    - side effects:
      - if `r1 + imm` is bigger than memory size, `pcall 4` is triggered

  - ldd [opcode `0x23`, S type]
    - load double word from memory into a register
    - executes: `rd <- u64[r1 + imm]`
    - side effects:
      - if `r1 + imm` is bigger than memory size, `pcall 4` is triggered

  - stb [opcode `0x24`, S type]
    - store byte from register into memory
    - executes: `u8[rd + imm] <- u8(r1)`
    - side effects:
      - if `rd + imm` is bigger than memory size, `pcall 4` is triggered

  - sth [opcode `0x25`, S type]
    - store half word from register into memory
    - executes: `u16[rd + imm] <- u16(r1)`
    - side effects:
      - if `rd + imm` is bigger than memory size, `pcall 4` is triggered

  - stw [opcode `0x26`, S type]
    - store word from register into memory
    - executes: `u32[rd + imm] <- u32(r1)`
    - side effects:
      - if `rd + imm` is bigger than memory size, `pcall 4` is triggered

  - std [opcode `0x27`, S type]
    - store half word from register into memory
    - executes: `u64[rd + imm] <- r2`
    - side effects:
      - if `rf + imm` is bigger than memory size, `pcall 4` is triggered

  - jal [opcode `0x28`, L type]
    - jump to a place in memory
    - executes:
      - `rd <- pc + 8`
      - `pc <- pc + i50[imm]`

  - jalr [opcode `0x29`, S type]
    - jump to a place in memory
    - executes:
      - `rd <- pc + 8`
      - `pc <- pc + r1 + imm`

  - je [opcode `0x2A`, S type]
    - jump to a place in memory when `rd == r1`
    - executes:
      - `if rd ^ r1 == 0`
        - `pc <- pc + imm * 8` 
      - `else`
        - `pc <- pc + 8`

  - jne [opcode `0x2B`, S type]
    - jump to a place in memory when `rd != r1`
    - executes:
      - `if rd ^ r1 != 0`
        - `pc <- pc + imm * 8` 
      - `else`
        - `pc <- pc + 8`

  - jgu [opcode `0x2C`, S type]
    - jump to a place in memory when `rd > r1`, both unsigned
    - executes:
      - `if (u64(rd) - u64(r1)) & (sign bit)`
        - `pc <- pc + imm * 8` 
      - `else`
        - `pc <- pc + 8`

  - jgs [opcode `0x2D`, S type]
    - jump to a place in memory when `rd > r1`, both signed
    - executes:
      - `if (i64(rd) - i64(r1)) & (sign bit)`
        - `pc <- pc + imm * 8` 
      - `else`
        - `pc <- pc + 8`
      
  - jleu [opcode `0x2E`, S type]
    - jump to a place in memory when `rd <= r1`, both unsigned
    - executes:
      - `if (i64(rd) - i64(r1)) & (sign bit) == 0`
        - `pc <- pc + imm * 8` 
      - `else`
        - `pc <- pc + 8`

  - jleu [opcode `0x2F`, S type]
    - jump to a place in memory when `rd <= r1`, both signed
    - executes:
      - `if (i64(rd) - i64(r1)) & (sign bit) == 0`
        - `pc <- pc + imm * 8` 
      - `else`
        - `pc <- pc + 8`

- group three:
  - setgur [opcode `0x30`, R type]
    - set `rd` to `1` in case `u64(r1) > u64(r2)`, else `0`
    - executes:
      - `rd <- u64(r1) > u64(r2) ? 1 : 0`

  - setgui [opcode `0x31`, S type]
    - set `rd` to `1` in case `u64(r1) > u64(imm)`, else `0`
    - executes:
      - `rd <- u64(r1) > u64(imm) ? 1 : 0`

  - setgsr [opcode `0x32`, R type]
    - set `rd` to `1` in case `i64(r1) > i64(r2)`, else `0`
    - executes:
      - `rd <- i64(r1) > i64(r2) ? 1 : 0`

  - setgsi [opcode `0x33`, S type]
    - set `rd` to `1` in case `i64(r1) > i64(imm)`, else `0`
    - executes:
      - `rd <- i64(r1) > u64(imm) ? 1 : 0`

  - setgur [opcode `0x34`, R type]
    - set `rd` to `1` in case `u64(r1) > u64(r2)`, else `0`
    - executes:
      - `rd <- u64(r1) > u64(r2) ? 1 : 0`

  - setgui [opcode `0x35`, S type]
    - set `rd` to `1` in case `u64(r1) > u64(imm)`, else `0`
    - executes:
      - `rd <- u64(r1) > u64(imm) ? 1 : 0`

  - setgsr [opcode `0x36`, R type]
    - set `rd` to `1` in case `i64(r1) > i64(r2)`, else `0`
    - executes:
      - `rd <- i64(r1) > i64(r2) ? 1 : 0`

  - setgsi [opcode `0x37`, S type]
    - set `rd` to `1` in case `i64(r1) > i64(imm)`, else `0`
    - executes:
      - `rd <- i64(r1) > u64(imm) ? 1 : 0`     
  
  - lui [opcode `0x38`, L type]
    - set the highest bits of `rd` to the value of `imm`, 
    - executes: 
      - `rd <- u64(imm) << 12`

  - auipc [opcode `0x39`, L type]
    - set `rd` to the sum of the address that the `aupic` instruction is
    located (pc) with an `imm` on the highest bits
    - executes:
      - `rd <- pc + (u64(imm) << 12)`
  
  - pcall [opcode `0x3A`, L type]
    - call the processor to execute certain subroutines, execution is
    lef for the implementation

  - pret [opcode `0x3B`, L type]
    - return from a `pcall` subroutine

## Interrupts

### Default interrupts/exceptions used by the virtual machine

- `pcall -1`: [Processor interface](#pcall--1)
- `pcall 0`: Division by zero
- `pcall 1`: General fault
- `pcall 2`: Double fault
- `pcall 3`: Triple fault
- `pcall 4`: Invalid instruction
- `pcall 5`: Page fault
- `pcall 6`: Invalid IO

everything after this is programmable (in theory), but it is reserved 
for any other virtual machines to implement until `pcall 0x1F`.

#### `pcall 0`: Division by zero

As the name suggests, this program call is triggered every time there is
a division by zero on the program. A compiler can simply put a divide by
zero instruction on a program and call it a breakpoint.

#### `pcall 1`: General Fault

General faults occur by any kind of unhandled exception the processor is
not able to detect or recognize.

#### `pcall 2`: Double Fault

A double fault occurs when any interrupt is called/triggered by
a general fault.

#### `pcall 3`: Triple Fault

A triple fault is one of the fatal faults inside the processor. There is
no way to handle a triple fault as it suggests a fault in the error
handling system itself. By not making software handle a triple fault, it
prevents crash loops. Whe, if ever, it is triggered, the implementation
can choose to go for a reset or a shutdown.

#### `pcall 4`: Invalid Instruction

The invalid instruction is thrown every time the instruction decoder
couldn't find a reasonable instruction to execute, and sets r15 to the
value of the instruction it tried to parse;

#### `pcall 5`: Page Fault

This interrupt is triggered when a there is any "wrong" access to
memory, being either mapped into an unmapped area, or not having enough
permissions into a memory region, sets r15 to the unusable address

## `pcall -1`

Only `pcall -1` is hardware/vm defined, all the other $2^{51}-1$
possible interrupts are programmable with a call to `pcall -1`.

The interface defined uses `r15` split in two 32 bit areas `space:switch` 
as interrupt space and functionality switches, while other registers are
used accordingly as each function needs.  

Normally, `switch = 0` will be a `space` implementation check, behaving
as a `orr r14 r0 r0` in case its features are not implemented.

**`pcall -1` functions**

- `intspace = 0`: [interrupt vector functions](#interrupt-vector-interrupt-space)
- - `fswitch = 0`: [interrupt vector check](#interrupt-vector-check)
- - `fswitch = 1`: [interrupt vector enable](#interrupt-vector-enable)
- `intspace = 1`: [paging functions](#paging-interrupt-space)
- - `fswitch = 0`: [paging check](#paging-check)
- - `fswitch = 1`: [paging enable](#paging-enable)
- `intspace = 2`: [model information](#model-information-interrupt-space)
- - `fswitch = 0`: [model check](#model-check)
- `intspace = 3`: [hyper functions](#hyper-function-interrupt-space)
- - `fswitch = 0` [is hosted](#hyper-hosted)
- - `fswitch = 1` [return to host](#hyper-return-host)

---

### Interrupt vector functions

#### Interrupt vector check

- Input: none

- Output:
  - `r14`: `0` if no interrupts are possible, making `pcall 0:0` shadow
    `orr r14, r0, r0`. `1` means interrupts are possible, but only in
    the address specified by `r12`, `2` means they are possible anywhere
    defined by the program, 

- `r13`: in case `r14 == 1`, sets bit flags to which hardware interrupts are supported
         in case `r14 == 2`, defines the amount of interrupts the processor is able to handle

trashed registers: none

#### interrupt vector enable

input registers:

- `r14` (possibly): if `pcall 0:0` returned `2`, set the interrupt vector register to the specified pointer, ignored if not

output registers: none

trashed registers: none

---

### Paging functions

#### Paging check

input registers: none

output registers:

- `r14`: set to the processor's amount of page level reach, 0 is unimplemented, 1 is linear paging or `≥ 2` for multiple levels
- `r13`: in case `r31` is not zero, returns the processor's page size

trashed registers: none

---

### Model Information functions

#### Information check

input registers: none

output registers:

- `r15`: set if the processor is able to give more information about itself

trashed registers: none

---

### Hypervisor functions

#### Hypervisor check

- Input: none

- Output: 
  - `r14`: the boolean value indicating if current processor is emulated

#### Hypervisor return

- Input: 
  `r1`: exit code

- Side Effects:
  - if this code was being ran from a virtual machine, it sends a 
  program end signal, to stop execution
  - if this code is in user mode, it sends control back to the kernel
  - ***this function has not yet defined behavior for kernel mode***