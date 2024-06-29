# Supernova Instruction Set

A reduced instruction set designed to be the first compiler target,
opcodes and behaviors might change in future releases.

## Compiling 

To compiling the project, you will need
 - [CMAKE](https://cmake.org/) (version 3.5 or higher) to build

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

`r00` is hardwired to zero, `r01` and `r02` are required for the interrupt stack

register index | used as | requirements
:-: | :-:               | :-:
r00 | zero register     | none, writing to is a no-op
r01 | stack pointer     | needs to be valid for `pcall`
r02 | frame pointer     | needs to be vaild for `pcall`
r03 - r12 | general use | none
r13 | `pcall` return    | save before pcall 
r14 | `pcall` return    | save before pcall 
r15 | `pcall` parameter | function switch  


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
    - executes a logical left shift on register `r1` for `r2` bits, result on `rd`
    - executes: `rd <- r1 << r2`
    - edge case:
      - if `r2 >= 64`, clear `rd`

  - llsi [opcode `0x09`, S type]
    - executes a logical left shift on register `r1` for `imm` bits, result on `rd`
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
      - overflow is discarted

  - addi [opcode `0x11`, S type]
    - adds `imm` to `r1` and set `rd` as the result
    - executes: `rd <- r1 + imm`
    - edge case:
      - overflow is discarted

  - subr [opcode `0x12`, R type]
    - subtracts `r2` from `r1` and set `rd` as the result
    - executes: `rd <- r1 - r2`
    - edge case:
      - overflow is discarted

  - subi [opcode `0x13`, S type]
    - subtracts `imm` from `r1` and set `rd` as the result
    - executes: `rd <- r1 - imm`
    - edge case:
      - overflow is discarted

  - umulr [opcode `0x14`, R type]
    - multiplies `r2 (unsigned)` with `r1 (unsigned)` and set `rd` as the result
    - executes: `rd <- (uint64_t)r1 * (uint64_t)r2`
    - edge case:
      - overflow is discarted

  - umuli [opcode `0x15`, S type]
    - multiplies `imm (unsigned)` with `r1 (unsigned)` and set `rd` as the result
    - executes: `rd <- (uint64_t)r1 * (uint64_t)imm`
    - edge case:
      - overflow is discarted

  - smulr [opcode `0x16`, R type]
    - multiplies `r2 (signed)` with `r1 (signed)` and set `rd` as
    - executes: `rd <- (int64_t)r1 * (int64_t)r2`
    - edge case:
      - overflow is discarted

  - smuli [opcode `0x17`, S type]
    - multiplies `imm` with `r1` and set `rd` as the result
    - executes: `rd <- (int64_t)r1 * (int64_t)imm`
    - edge case:
      - overflow is discarted

  - udivr [opcode `0x18`, R type]
    - divides `r1 (unsigned)` by `r2 (unsigned)` and set `rd` as the result
    - executes: `rd <- (uint64_t)r1 / (uint64_t)r2`
    - edge case:
      - overflow is discarted
      - `r2 = 0` triggers `pcall 1`

  - udivi [opcode `0x19`, S type]
    - divides `r1 (unsigned)` by `imm (unsigned)` and set `rd` as the result
    - executes: `rd <- (uint64_t)r1 / (uint64_t)imm`
    - edge case:
      - overflow is discarted
      - `imm = 0` triggers `pcall 1`

  - sdivr [opcode `0x1A`, R type]
    - divides `r1 (signed)` by `r2 (signed)` and set `rd` as
    - executes: `rd <- (int64_t)r1 / (int64_t)r2`
    - edge case:
      - overflow is discarted
      - `r2 = 0` triggers `pcall 1`

  - sdivi [opcode `0x1B`, S type]
    - divides `r1 (signed)` by `imm (signed)` and set `rd` as the result
    - executes: `rd <- (int64_t)r1 / (int64_t)imm`
    - edge case:
      - overflow is discarted
      - `imm = 0` triggers `pcall 1`

  - call [opcode `0x1C`, R type]
    - change execution context to another place
    - semantic renaming: `call rd, r1, r2` -> `call addr, sp, bp`
    - executes:
      - `u64[sp + 0] <- bp`
      - `u64[sp + 8] <- pc + 8`
      - `sp <- sp + 16`
      - `bp <- sp`
      - `pc <- addr`

  - push [opcode `0x1D`, S type]
    - push a value into given stack
    - semantic renaming `push rd, r1, imm` -> `push rv, sp, imv`
    - executes:
      - `u64[sp] <- rv + imv`
      - `sp <- sp + 8`

  - retn [opcode `0x1E`, R type]
    - return execution to previous context
    - semantic renaming `retn rd, r1, r2` -> `retn x0, sp, bp`
    - executes:
      - `sp <- sp - 16`
      - `bp <- u64[sp + 0]`
      - `pc <- u64[sp + 8]`
    - `x0` is ignored

  - pull [opcode `0x1F`, S type]
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
    - executes: `u8[rd + imm] <- r1 & 0xff`
    - side effects:
      - if `rd + imm` is bigger than memory size, `pcall 4` is triggered

  - sth [opcode `0x25`, S type]
    - store half word from register into memory
    - executes: `u16[rd + imm] <- r1 & 0xffff`
    - side effects:
      - if `rd + imm` is bigger than memory size, `pcall 4` is triggered

  - stw [opcode `0x26`, S type]
    - store word from register into memory
    - executes: `u32[rd + imm] <- r1 & 0xffffffff`
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
## interrupts

### default interrupts/exceptions used by the virtual machine

- `pcall -1`: [Processor interface](#pcall--1)
- `pcall 0`: Divison by zero
- `pcall 1`: General fault
- `pcall 2`: Double fault
- `pcall 3`: Triple fault
- `pcall 4`: Invalid instruction
- `pcall 5`: Page fault
- `pcall 6`: Invalid IO

everything after this is programmable (in theory), but some different
implementations might use other values

#### `pcall 0`: Division by zero

As the name suggests, this program call is triggered every time there is
a division by zero on the program. A compiler can simply put a divide by zero
instruction on a program and call it a breakpoint

#### `pcall 1`: General Fault

General faults occur by any kind of unhandled exception the processor is not
able to detect or recognize

#### `pcall 2`: Double Fault

A double fault occurs when any interrupt is called/triggered by a general fault

#### `pcall 3`: Triple Fault

A triple fault is one of the fatal faults inside the processor. There is no way 
to handle a triple fault as it probably suggests a fault in the error handling
system, and not making software handle prevents crash loops. If it is ever triggered
the processor is able to choose to go for a reset or a shutdown 

## `pcall -1`

Only `pcall -1` is hardware/vm defined, all the other $2^{51}-1$ possible interrupts are programmable with a call to `pcall -1`:

The interface defined uses r15 split in two 32bit areas `intspace:fswitch` as interrupt space and functionality switches, while other registers are used accordingly as each function needs.
Normally, `fswitch = 0` will be a `instspace` implementation check, behaving as a `orr r14 r0 r0` in case `intspace`'s feature is not implmemented.

**`pcall -1` functions**

- `intspace = 0`: [interrupt vector functions](#interrupt-vector-interrupt-space)
- - `fswitch = 0`: [interrupt vector check](#interrupt-vector-check)
- - `fswitch = 1`: [interrupt vector enable](#interrupt-vector-enable)
- `intspace = 1`: [paging functions](#paging-interrupt-space)
- - `fswitch = 0`: [paging check](#paging-check)
- - `fswitch = 1`: [paging enable](#paging-enable)
- `intspace = 2`: [model information](#model-information-interrupt-space)
- - `fswitch = 0`: [model check](#model-check)
- - `fswitch = 1`: []
- `intspace = 3`: [hyper functions](#hyper-function-interrupt-space)
- - `fswitch = 0` [is hosted](#hyper-hosted)

---

### interrupt vector interrupt space

#### interrupt vector check

input registers: none

output registers:

- `r14`: `0` if no interrupts are possible, `pcall 0:0` is just a shadow to `orr r14, r0, r0`,  
         `1` if interrupts are possible, but only in the address specified by `r12`,  
         `2` if interrupts are possible anywhere defined by the program,  

- `r13`: in case `r14 == 1`, sets bit flags to which hardware interrupts are supported
         in case `r14 == 2`, defines the amount of interrupts the processor is able to handle

trashed registers: none

#### interrupt vector enable

input registers:

- `r14` (possibly): if `pcall 0:0` returned `2`, set the interrupt vector register to the specified pointer, ignored if not

output registers: none

trashed registers: none

---

### paging interrupt space

#### paging check

input registers: none

output registers:

- `r14`: set to the processor's amount of page level reach, 0 is unimplemented, 1 is linear paging or `≥ 2` for multiple levels
- `r13`: in case `r31` is not zero, returns the processor's page size

trashed registers: none

---

### model information interrupt space

#### information check

input registers: none

output registers:

- `r15`: set if the processor is able to give more information about itself

trashed registers: none

