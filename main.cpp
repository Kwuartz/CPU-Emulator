#include <iostream>

#include <stdexcept>
#include <thread>
#include <chrono>

#include <unordered_map>
#include <sstream>
#include <fstream>
#include <vector>

using namespace std;

const int MEMORY_SIZE = 1024;
const int REGISTER_COUNT = 20;

const int FRAME_WIDTH = 100;
const int FRAME_HEIGHT = 30;
const int FRAME_BUFFER_SIZE = FRAME_HEIGHT * FRAME_WIDTH;

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
    MUL,
    LSL,
    LSR,
    CMP,
    B,
    BEQ,
    BNE,
    BGT,
    BLT,
    FLUSH,
    WAIT,
    HALT
};

struct Instruction {
    Opcode op;
    vector<Argument> args;
};

unordered_map<int, char> symbolMap = {
    {0, ' '},
    {1, '|'},
    {2, 'O'},
    {3, '0'},
    {4, '1'},
    {5, '2'},
    {6, '3'},
    {7, '4'},
    {8, '5'},
    {9, '6'},
    {10, '7'},
    {11, '8'},
    {12, '9'}
};

unordered_map<string, Opcode> opcodeMap = {
    {"LDR", LDR},
    {"STR", STR},
    {"MOV", MOV},
    {"ADD", ADD},
    {"SUB", SUB},
    {"MUL", MUL},
    {"LSL", LSL},
    {"LSR", LSR},
    {"CMP", CMP},
    {"B", B},
    {"BEQ", BEQ},
    {"BNE", BNE},
    {"BGT", BGT},
    {"BLT", BLT},
    {"FLUSH", FLUSH},
    {"WAIT", WAIT},
    {"HALT", HALT}
};

struct CPU {
    vector<int> memory;
    vector<int> framebuffer;
    vector<int> registers;
    Flags flags;
    int pc;

    unordered_map<string, int> branchMap;
};

bool mapSymbol(int code, char& symbol) {
    auto it = symbolMap.find(code);

    if (it != symbolMap.end()) {
        symbol = it->second;
    } else {
        return false;
    }

    return true;
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

class Kernel {
    public:
        CPU cpu;
        vector<Instruction> instructions;

        Kernel() {
            cpu.memory = vector<int>(MEMORY_SIZE);
            cpu.framebuffer = vector<int>(FRAME_BUFFER_SIZE);
            cpu.registers = vector<int>(REGISTER_COUNT);
            cpu.pc = 0;
        }

        bool loadProgram(string path) {
            ifstream programFile(path);

            string programLine;
            int lineNumber = 0;
            int instructionNumber = 0;

            while (getline(programFile, programLine)) {
                if (programLine.size() == 0 || programLine.front() == ';') {
                    // Comment
                } else if (isInstruction(programLine)) {
                    Instruction inst;

                    if (parseInstruction(inst, programLine)) {
                        instructions.push_back(move(inst));
                        instructionNumber++;
                    } else {
                        cout << "Instruction on line " << lineNumber << " is invalid: " << programLine << "\n";
                        return false;
                    };
                    
                } else if (isBranch(programLine)) {
                    if (!parseBranch(cpu.branchMap, programLine, instructionNumber)) {
                        cout << "Branch on line " << lineNumber << " is invalid: " << programLine << "\n";
                        return false;
                    }
                } else {
                    cout << "Line " << lineNumber << " not recognised as instruction or branch: " << programLine << "\n";
                    return false;
                }

                lineNumber++;
            }

            return true;
        }

        bool runProgram() {
            while (cpu.pc != -1 && cpu.pc != instructions.size()) {
                if (!executeInstruction(cpu, instructions[cpu.pc])) {
                    cout << "Instruction number " << to_string(cpu.pc) << " failed to execute" << "\n";
                    return false;
                }
            }
            
            return true;
        }
    
    private:
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
                if (target.value < REGISTER_COUNT) {
                    cpu.registers[target.value] = value;
                } else {
                    return false;
                }
            } else if (target.type == ADDRESS) {
                if (target.value < MEMORY_SIZE) {
                    cpu.memory[target.value] = value;
                } else if (target.value < MEMORY_SIZE + FRAME_BUFFER_SIZE) {
                    cpu.framebuffer[target.value - MEMORY_SIZE] = value;
                } else {
                    return false;
                }
                
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

        bool multiply(CPU& cpu, const Argument& target, const Argument& operand1, const Argument& operand2) {
            int num1;
            int num2;

            if (!getValue(cpu, operand1, num1) || !getValue(cpu, operand2, num2)) {
                return false;
            }

            Argument result;
            int product = num1 * num2;
            
            result.type = IMMEDIATE;
            result.value = product;

            return store(cpu, target, result);
        }

        bool shift(CPU& cpu, const Argument& target, const Argument& operand1, const Argument& operand2, int direction) {
            int num1;
            int num2;

            if (!getValue(cpu, operand1, num1) || !getValue(cpu, operand2, num2)) {
                return false;
            }

            Argument result;
            int product;

            if (direction == 1) {
                product = num1 << num2;
            } else {
                product = num1 >> num2;
            }
            
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

        bool flushFrame(const CPU& cpu) {
            string frame;
            
            cout << "\033[H\033[2J";

            for (int i = 0; i < cpu.framebuffer.size(); i++) {
                int code = cpu.framebuffer[i];

                if (i % FRAME_WIDTH == 0) {
                    frame += "\n";
                }

                char symbol;

                if (mapSymbol(code, symbol)) {
                    frame += symbol;
                } else {
                    return false;
                }
            }

            cout << frame << endl;

            return true;
        }

        bool executeInstruction(CPU& cpu, const Instruction& inst) {
            bool ok = false;
            bool incrementPC = true;

            switch (inst.op) {
                case LDR: {
                    if (inst.args.size() < 2) {
                        ok = false;
                        cout << "Not enough arguments for load register operation" << "\n";
                        break;
                    }

                    Argument sourceAddress = inst.args[1];
                    if (inst.args[1].type == REGISTER) {
                        sourceAddress.type = ADDRESS;
                        getValue(cpu, inst.args[1], sourceAddress.value);
                    }

                    ok = store(cpu, inst.args[0], sourceAddress);

                    if (!ok) {
                        cout << "Load register operation failed" << "\n";
                    }

                    break;
                }

                case STR: {
                    if (inst.args.size() < 2) {
                        ok = false;
                        cout << "Not enough arguments for store register operation" << "\n";
                        break;
                    }

                    Argument targetAddress = inst.args[1];
                    if (inst.args[1].type == REGISTER) {
                        targetAddress.type = ADDRESS;
                        getValue(cpu, inst.args[1], targetAddress.value);
                    }

                    ok = store(cpu, targetAddress, inst.args[0]);

                    if (!ok)  {
                        cout << "Store register operation failed" << "\n";
                    }

                    break;
                }

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

                    if (inst.args[1].type == ADDRESS) {
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

                case MUL:
                    if (inst.args.size() < 3) {
                        ok = false;
                        cout << "Not enough arguments for multiply operation" << "\n";
                        break;
                    }

                    if (inst.args[0].type != REGISTER || inst.args[1].type != REGISTER) {
                        ok = false;
                        cout << "Only register addresses can be provided to the first 2 arguments of a multiply operation" << "\n";
                        break;
                    }

                    if (inst.args[2].type == ADDRESS) {
                        ok = false;
                        cout << "Memory addresses can not be provided as an operand to a multiply operation" << "\n";
                        break;
                    }

                    ok = multiply(cpu, inst.args[0], inst.args[1], inst.args[2]);

                    if (!ok)  {
                        cout << "Multiply operation failed" << "\n";
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

                case BNE:
                    if (inst.args.size() < 1) {
                        ok = false;
                        cout << "Not enough arguments for branch operation" << "\n";
                        break;
                    }

                    if (cpu.flags.equal) {
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
                
                case FLUSH:
                    cout << "Flush" << endl;
                    ok = flushFrame(cpu);

                    if (!ok) {
                        cout << "Flush frame failed" << "\n";
                    }

                    break;

                case WAIT:
                    if (inst.args.size() < 1) {
                        ok = false;
                        cout << "Not enough arguments for wait operation" << "\n";
                        break;
                    }

                    int delay;

                    if (!getValue(cpu, inst.args[0], delay)) {
                        ok = false;
                        cout << "Wait delay argument parsing failed" << "\n";
                        break;
                    }

                    this_thread::sleep_for(chrono::milliseconds(delay));
                    ok = true;

                    if (!ok) {
                        cout << "Wait failed" << "\n";
                    }

                    break;

                case HALT:
                    ok = true;
                    cpu.pc = -1;
                    break;
            }

            if (incrementPC) {
                cpu.pc++;
            }

            return ok;
        };
};



const string directory = "programs";

int main() {
    Kernel kernel;

    string fileName;

    cout << "Enter file name: ";
    cin >> fileName;

    bool ok;

    ok = kernel.loadProgram(directory + "/" + fileName);
    
    if (ok) {
        cout << "Program loaded succesfully" << "\n";
    } else {
        cout << "Error while loading program" << "\n";
        return 0;
    }
    
    ok = kernel.runProgram();

    if (ok) {
        cout << "Program executed succesfully" << "\n";
    } else {
        cout << "Error during program exectuion" << "\n";
    }

    return 0;
};