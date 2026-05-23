# Assembly Emulator

A simple C++ emulator for a custom assembly language, similar to the AQA A-Level Computer Science assembly language set.

## Building & Running

```bash
g++ -std=c++17 -o emulator main.cpp
./emulator
```

All `.asm` files in the `programs/` directory are loaded and executed automatically.

## Instruction Set

| Instruction | Example | Description |
|---|---|---|
| `MOV` | `MOV R0, #5` | Move a value into a register |
| `LDR` | `LDR R0, 100` | Load a value from memory into a register |
| `STR` | `STR R0, 100` | Store a register value into memory |
| `ADD` | `ADD R0, R1, #2` | Add two values, store in register |
| `SUB` | `SUB R0, R1, R2` | Subtract, store in register |
| `MUL` | `MUL R0, R1, R2` | Multiply, store in register |
| `LSL` | `LSL R0, R1, #2` | Logical shift left |
| `LSR` | `LSR R0, R1, #2` | Logical shift right |
| `CMP` | `CMP R0, #10` | Compare two values (sets flags) |
| `B` | `B loop` | Unconditional branch to label |
| `BEQ` | `BEQ end` | Branch if equal |
| `BNE` | `BNE loop` | Branch if not equal |
| `BGT` | `BGT loop` | Branch if greater than |
| `BLT` | `BLT loop` | Branch if less than |
| `WAIT` | `WAIT #500` | Pause execution for N milliseconds |
| `FLUSH` | `FLUSH` | Render memory contents to the terminal |
| `HALT` | `HALT` | End the program |

## Program Format

- Lines starting with `;` are comments
- Labels end with `:`
- Registers are prefixed with `R` (e.g. `R0`, `R1`)
- Immediate values are prefixed with `#` (e.g. `#42`)

```asm
; Count down from 10
    MOV R0, #10
loop:
    SUB R0, R0, #1
    CMP R0, #0
    BNE loop
    HALT
```

## Scheduling

- All programs run as separate processes in a round-robin loop
- Each process executes one instruction per turn, then the next process runs
- Continues until all processes have halted or errored out
- `WAIT #ms` sleeps the thread for that many milliseconds before the next process runs

## Memory & Paging

- 4096 bytes of physical memory, split into 32 frames of 128 bytes each
- Each process has its own virtual address space mapped to physical frames via a page table
- If a process accesses a page that hasn't been allocated yet, the kernel assigns a free frame (page fault handling)
- Frames are freed when a process halts
- If no free frames are left, the process is terminated
- Each process has 20 registers (R0-R19) independent from all other processes