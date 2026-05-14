#include <iostream>

#include <stdexcept>

#include <unordered_map>
#include <sstream>
#include <fstream>
#include <vector>

using namespace std;

enum ArgumentType {
    ADDRESS,
    REGISTER,
    IMMEDIATE,
    LABEL
};

struct Argument {
    ArgumentType type;
    int value;
    string label;
};

struct Flags {
    bool equal;
    bool greater;
    bool less;
};

enum Opcode {
    LDR,
    STR,
    MOV,
    ADD,
    SUB,
    LSL,
    LSR,
    CMP,
    B,
    BEQ,
    BGT,
    BLT,
    HALT
};

struct Instruction {
    Opcode op;
    vector<Argument> args;
};

unordered_map<int, char> symbolMap = {
    {0, ' '},
    {1, '|'},
    {2, 'O'}
};

unordered_map<string, Opcode> opcodeMap = {
    {"LDR", LDR},
    {"STR", STR},
    {"MOV", MOV},
    {"ADD", ADD},
    {"SUB", SUB},
    {"LSL", LSL},
    {"LSR", LSR},
    {"CMP", CMP},
    {"B", B},
    {"BEQ", BEQ},
    {"BGT", BGT},
    {"BLT", BLT},
    {"HALT", HALT}
};

struct CPU {
    vector<int> memory;
    vector<int> registers;
    Flags flags;
    int pc;

    unordered_map<string, int> branchMap;
};

char mapSymbol(int code) {
    auto it = symbolMap.find(code);

    if (it != symbolMap.end()) {
        return it->second;
    } else {
        throw runtime_error("Invalid symbol code: " + to_string(code));
    }
}

Opcode mapOpcode(const string& token) {
    auto it = opcodeMap.find(token);

    if (it != opcodeMap.end()) {
        return it->second;
    } else {
        throw runtime_error("Invalid opcode: " + token);
    }
}

bool isInstruction(const string& line) {
    istringstream iss(line);
    string token;

    if (iss >> token) {
        return opcodeMap.find(token) != opcodeMap.end();
    } else {
        return false;
    }  
}

bool isBranch(const string& line) {
    istringstream iss(line);
    string token;

    if (iss >> token) {
        return token.back() == ':';
    } else {
        return false;
    } 
}

bool parseBranch(unordered_map<string, int>& branchMap, const string& token, int instructionNumber) {
    string branchName = token.substr(0, token.size() - 1);

    if (branchName.size() > 0) {
        branchMap[branchName] = instructionNumber;
    } else {
        return false;
    }

    return true;
}

bool parseArgument(Argument& arg, const string& token) {
    ArgumentType type;
    string cleanToken = token;

    switch (token[0]) {
        case 'R':
            type = REGISTER;
            cleanToken = token.substr(1);
            break;

        case '#':
            type = IMMEDIATE;
            cleanToken = token.substr(1);
            break;

        default:
            if (isalpha(token[0])) {
                type = LABEL;
                arg.type = type;
                arg.label = token;
                return true;
            }

            type = ADDRESS;
            break;
    }

    if (cleanToken.size() == 0) {
        cout << "Empty argument" << "\n";
        return false;
    }

    int value;
    try {
        value = stoi(cleanToken);
    } catch (...) {
        cout << "Non integer argument" << "\n";
        return false;
    }

    arg.type = type;
    arg.value = value;

    return true;
}

bool parseInstruction(Instruction& inst, const string& line) {
    istringstream iss(line);
    string token;
    iss >> token;

    inst.op = mapOpcode(token);

    while (iss >> token) {
        Argument arg;

        if (!token.empty() && token.back() == ',') {
            token.pop_back();
        }

        if (parseArgument(arg, token)) {
            inst.args.push_back(arg);
        } else {
            cout << "Invalid argument: " << token << "\n";
            return false;
        }
    };

    return true;
}

bool branch(CPU& cpu, const Argument& arg) {
    if (!arg.label.empty()) {
        auto it = cpu.branchMap.find(arg.label);

        if (it != cpu.branchMap.end()) {
            cpu.pc = it->second;
        } else {
            return false;
        }
    } else {
        return false;
    }
    

    return true;
}

bool getValue(CPU& cpu, const Argument& arg, int& result) {
    if (arg.type == IMMEDIATE) {
        result = arg.value;
    } else if (arg.type == REGISTER) {
        result = cpu.registers[arg.value];
    } else if (arg.type == ADDRESS) {
        result = cpu.memory[arg.value];
    } else {
        return false;
    }

    return true;
}

bool store(CPU& cpu, const Argument& target, const Argument& source) {
    int value;

    if (!getValue(cpu, source, value)) {
        return false;
    }

    if (target.type == REGISTER) {
        cpu.registers[target.value] = value;
    } else if (target.type == ADDRESS) {
        cpu.memory[target.value] = value;
    } else {
        return false;
    }

    return true;
}

bool add(CPU& cpu, const Argument& target, const Argument& operand1, const Argument& operand2, int sign) {
    int num1;
    int num2;

    if (!getValue(cpu, operand1, num1) || !getValue(cpu, operand2, num2)) {
        return false;
    }

    Argument result;
    int sum = num1 + (num2 * sign);
    
    result.type = IMMEDIATE;
    result.value = sum;

    return store(cpu, target, result);
}

bool shift(CPU& cpu, const Argument& target, const Argument& operand1, const Argument& operand2, int direction) {
    int num1;
    int num2;

    if (!getValue(cpu, operand1, num1) || !getValue(cpu, operand2, num2)) {
        return false;
    }

    Argument result;
    int product = num1 * 2^(direction * num2);
    
    result.type = IMMEDIATE;
    result.value = product;

    return store(cpu, target, result);
}

bool compare(CPU& cpu, const Argument& operand1, const Argument& operand2) {
    int num1;
    int num2;

    if (!getValue(cpu, operand1, num1) || !getValue(cpu, operand2, num2)) {
        return false;
    }

    cpu.flags.equal = num1 == num2;
    cpu.flags.greater = num1 > num2;
    cpu.flags.less = num1 < num2;
    
    return true;
}

bool executeInstruction(CPU& cpu, const Instruction& inst) {
    bool ok = false;
    bool incrementPC = true;

    switch (inst.op) {
        case LDR:
            if (inst.args.size() < 2) {
                ok = false;
                cout << "Not enough arguments for load register operation" << "\n";
                break;
            }

            if (inst.args[1].type != ADDRESS) {
                ok = false;
                cout << "Memory address not provided to load register operation" << "\n";
                break;
            }

            ok = store(cpu, inst.args[0], inst.args[1]);

            if (!ok) {
                cout << "Load register operation failed" << "\n";
            }

            break;

        case STR:
            if (inst.args.size() < 2) {
                ok = false;
                cout << "Not enough arguments for store register operation" << "\n";
                break;
            }

            if (inst.args[1].type != ADDRESS) {
                ok = false;
                cout << "Memory address not provided to store register operation" << "\n";
                break;
            }

            ok = store(cpu, inst.args[1], inst.args[0]);

            if (!ok)  {
                cout << "Store register operation failed" << "\n";
            }

            break;

        case MOV:
            if (inst.args.size() < 2) {
                cout << "Not enough arguments for move operation" << "\n";
                ok = false;
                break;
            }

            if (inst.args[0].type != REGISTER) {
                ok = false;
                cout << "Only a register address can be provided as the first argument of a move operation" << "\n";
                break;
            }

            if (inst.args[2].type == ADDRESS) {
                ok = false;
                cout << "Memory addresses can not be provided to a move operation" << "\n";
                break;
            }

            ok = store(cpu, inst.args[0], inst.args[1]);

            if (!ok)  {
                cout << "Move operation failed" << "\n";
            }

            break;

        case ADD:
            if (inst.args.size() < 3) {
                ok = false;
                cout << "Not enough arguments for add operation" << "\n";
                break;
            }

            if (inst.args[0].type != REGISTER || inst.args[1].type != REGISTER) {
                ok = false;
                cout << "Only register addresses can be provided to the first 2 arguments of an add operation" << "\n";
                break;
            }

            if (inst.args[2].type == ADDRESS) {
                ok = false;
                cout << "Memory addresses can not be provided as an operand to an add operation" << "\n";
                break;
            }

            ok = add(cpu, inst.args[0], inst.args[1], inst.args[2], 1);

            if (!ok)  {
                cout << "Add operation failed" << "\n";
            }

            break;

        case SUB:
            if (inst.args.size() < 3) {
                ok = false;
                cout << "Not enough arguments for subtract operation" << "\n";
                break;
            }

            if (inst.args[0].type != REGISTER || inst.args[1].type != REGISTER) {
                ok = false;
                cout << "Only register addresses can be provided to the first 2 arguments of a subtract operation" << "\n";
                break;
            }

            if (inst.args[2].type == ADDRESS) {
                ok = false;
                cout << "Memory addresses can not be provided as an operand to a subtract operation" << "\n";
                break;
            }

            ok = add(cpu, inst.args[0], inst.args[1], inst.args[2], -1);

            if (!ok)  {
                cout << "Add operation failed" << "\n";
            }

            break;
        case LSL:
            if (inst.args.size() < 3) {
                ok = false;
                cout << "Not enough arguments for logical shift left operation" << "\n";
                break;
            }

            if (inst.args[0].type != REGISTER || inst.args[1].type != REGISTER) {
                ok = false;
                cout << "Only register addresses can be provided to the first 2 arguments of a logical shift left operation" << "\n";
                break;
            }

            if (inst.args[2].type == ADDRESS) {
                ok = false;
                cout << "Memory addresses can not be provided as an operand to a logical shift left operation" << "\n";
                break;
            }

            ok = shift(cpu, inst.args[0], inst.args[1], inst.args[2], 1);

            if (!ok)  {
                cout << "Logical shift left operation failed" << "\n";
            }

            break;

        case LSR:
            if (inst.args.size() < 3) {
                ok = false;
                cout << "Not enough arguments for logical shift right operation" << "\n";
                break;
            }

            if (inst.args[0].type != REGISTER || inst.args[1].type != REGISTER) {
                ok = false;
                cout << "Only register addresses can be provided to the first 2 arguments of a logical shift right operation" << "\n";
                break;
            }

            if (inst.args[2].type == ADDRESS) {
                ok = false;
                cout << "Memory addresses can not be provided as an operand to a logical shift right operation" << "\n";
                break;
            }

            ok = shift(cpu, inst.args[0], inst.args[1], inst.args[2], -1);

            if (!ok)  {
                cout << "Logical shift right operation failed" << "\n";
            }

            break;

        case CMP:
            if (inst.args.size() < 2) {
                ok = false;
                cout << "Not enough arguments for compare operation" << "\n";
                break;
            }

            if (inst.args[0].type != REGISTER) {
                ok = false;
                cout << "Only a register address can be provided as the first argument of a compare operation" << "\n";
                break;
            }

            if (inst.args[1].type == ADDRESS) {
                ok = false;
                cout << "A memory address can not be provided as the operarond of a compare operation" << "\n";
                break;
            }

            ok = compare(cpu, inst.args[0], inst.args[1]);

            if (!ok) {
                cout << "Compare failed" << "\n";
            }
            
            break;

            break;

        case B:
            if (inst.args.size() < 1) {
                ok = false;
                cout << "Not enough arguments for branch operation" << "\n";
                break;
            }

            ok = branch(cpu, inst.args[0]);

            if (ok) {
                incrementPC = false;
            } else {
                cout << "Branch failed" << "\n";
            }
            
            break;

        case BEQ:
            if (inst.args.size() < 1) {
                ok = false;
                cout << "Not enough arguments for branch operation" << "\n";
                break;
            }

            if (!cpu.flags.equal) {
                ok = true;
                break;
            }

            ok = branch(cpu, inst.args[0]);

            if (ok) {
                incrementPC = false;
            } else {
                cout << "Branch failed" << "\n";
            }

            break;

        case BGT:
            if (inst.args.size() < 1) {
                ok = false;
                cout << "Not enough arguments for branch operation" << "\n";
                break;
            }

            if (!cpu.flags.greater) {
                ok = true;
                break;
            }

            ok = branch(cpu, inst.args[0]);

            if (ok) {
                incrementPC = false;
            } else {
                cout << "Branch failed" << "\n";
            }

            break;

        case BLT:
            if (inst.args.size() < 1) {
                ok = false;
                cout << "Not enough arguments for branch operation" << "\n";
                break;
            }

            if (!cpu.flags.less) {
                ok = true;
                break;
            }

            ok = branch(cpu, inst.args[0]);

            if (ok) {
                incrementPC = false;
            } else {
                cout << "Branch failed" << "\n";
            }

            break;

        case HALT:
            cpu.pc = -1;
            break;
    }

    if (incrementPC) {
        cpu.pc++;
    }

    return ok;
};

const string directory = "programs";

int main() {
    string fileName;

    cout << "Enter file name: ";
    cin >> fileName;

    CPU cpu;
    cpu.memory = vector<int>(256),
    cpu.registers = vector<int>(10),
    cpu.pc = 0;

    vector<Instruction> instructions;

    ifstream programFile(directory + "/" + fileName);

    string programLine;
    int lineNumber = 0;
    int instructionNumber = 0;
    while (getline(programFile, programLine)) {
        if (programLine.size() == 0) {
            cout << "Line" << lineNumber << " is empty" << "\n";
        } else if (isInstruction(programLine)) {
            Instruction inst;

            if (parseInstruction(inst, programLine)) {
                instructions.push_back(move(inst));
                instructionNumber++;
            } else {
                cout << "Instruction on line " << lineNumber << " is invalid: " << programLine << "\n";
            };
            
        } else if (isBranch(programLine)) {
            if (!parseBranch(cpu.branchMap, programLine, instructionNumber)) {
                cout << "Branch on line " << lineNumber << " is invalid: " << programLine << "\n";
            }
        } else {
            cout << "Line " << lineNumber << " not recognised as instruction or branch: " << programLine << "\n";
        }

        lineNumber++;
    }

    while (cpu.pc != -1 && cpu.pc != instructions.size()) {
        if (!executeInstruction(cpu, instructions[cpu.pc])) {
            cout << "Instruction number " << to_string(cpu.pc) << " failed to execute" << "\n";
        }
    }

    return 0;
};