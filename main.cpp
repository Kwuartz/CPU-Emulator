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
};

Opcode mapOpcode(const string& token) {
    auto it = opcodeMap.find(token);

    if (it != opcodeMap.end()) {
        return it->second;
    } else {
        throw runtime_error("Invalid opcode: " + token);
    }
};

bool isInstruction(const string& line) {
    istringstream iss(line);
    string token;

    if (iss >> token) {
        return opcodeMap.find(token) != opcodeMap.end();
    } else {
        return false;
    }  
};

bool isBranch(const string& line) {
    istringstream iss(line);
    string token;

    if (iss >> token) {
        return token.back() == ':';
    } else {
        return false;
    } 
};

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
};

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
};

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
};

bool executeInstruction(CPU& cpu, const Instruction& inst) {
    bool ok = false;
    bool incrementPC = true;

    switch (inst.op) {
        case LDR:
            break;
        case STR:
            break;
        case MOV:
            break;
        case ADD:
            break;
        case SUB:
            break;
        case LSL:
            break;
        case LSR:
            break;
        case B:
            ok = branch(cpu, inst.args[0]);
            if (ok) {
                incrementPC = false;
            } else {
                cout << "Branch failed" << "\n";
            }

            break;
        case BEQ:
            break;
        case BGT:
            break;
        case BLT:
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