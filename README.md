CPU struct:
Memory - Split into RAM and frame buffer past a certain index
Registers
Flags - For branching

Instruction format:
Uses assembly syntax similar to that used in AQA A-Level computer science
Only core instructions currently + flush for terminal output

Instruction parsing:
First it is determined whether an assembly line is a comment, branch or instruction
Then if it is an instruction then the args and opcode is extracted and it is added to an instruction vector as an instruction struct
If it is a branch the label is extracted and added to a branch map

Execution:
The CPU struct is passed to the execution handler alongside the current instruction (indexed using the pc)
Switch case identifies the instruction and performs neccesary argument validation
Instruction is executed

Debugging:
Each function returns whether it was executed/parsed succesfully through an ok bool
If not parsed succesfully the program continues ignoring the invalid line but if not executed succesfully then the full trace can be seen in terminal output and the program stops executing