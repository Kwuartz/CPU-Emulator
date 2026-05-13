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

enum Opcode {
    LOAD,
    STORE,
    ADD,
    SUB,
    BEQ,
    BGT,
    BLT,
    B,
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
    {"LOAD", LOAD},
    {"STORE", STORE},
    {"ADD", ADD},
    {"SUB", SUB},
    {"BEQ", BEQ},
    {"BGT", BGT},
    {"BLT", BLT},
    {"B", B},
    {"HALT", HALT}
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

void executeInstruction(const Instruction& inst, int& pc) {
    switch (inst.op) {
        default:
            pc++;
    }
};

int main() {
    string fileName;

    cout << "Enter file name: ";
    cin >> fileName;

    vector<Instruction> instructions;
    unordered_map<string, int> branchMap;

    ifstream programFile(fileName);

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
            if (!parseBranch(branchMap, programLine, instructionNumber)) {
                cout << "Branch on line " << lineNumber << " is invalid: " << programLine << "\n";
            }
        } else {
            cout << "Line" << lineNumber << "not recognised as instruction or branch: " << programLine << "\n";
        }

        lineNumber++;
    }

    int pc = 0;
    vector<int> memory(256);
    vector<int> registers(10);

    while (pc != -1 && pc != instructions.size()) {
        executeInstruction(instructions[pc], pc);
    }

    return 0;
};