#include <iostream>

#include <unordered_map>
#include <sstream>
#include <fstream>
#include <vector>

using namespace std;

enum ArgumentType {
    ADDRESS,
    REGISTER,
    IMMEDIATE
};

struct Argument {
    ArgumentType type;
    int value;
};

enum Opcode {
    LOAD,
    STORE,
    ADD,
    SUB,
    BEQ,
    BGT,
    BLT,
    B
};

struct Instruction {
    Opcode op;
    vector<Argument> args;
};

struct Branch {
    string name;
    int position;
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
    {"B", B}
};

char mapChar(int value) {
    return symbolMap[value];
};

Opcode mapOpcode(const string& token) {
    return opcodeMap[token];
};

Argument parseArgument(const string& token) {
    Argument arg;
    
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
            type = ADDRESS;
            break;
    }

    int value = stoi(cleanToken);

    arg.type = type;
    arg.value = value;

    return arg;
};

Instruction parseLine(const string& line) {
    Instruction result;
    
    istringstream iss(line);
    string token;
    iss >> token;

    result.op = mapOpcode(token);

    while (iss >> token) {
        result.args.push_back(parseArgument(token));
    };
};

void main() {
    string fileName;

    cout << "Enter file name: ";
    cin >> fileName;

    vector<Instruction> instructions;

    ifstream programFile(fileName);

    string programLine;
    while (getline(programFile, programLine)) {
        instructions.push_back(parseLine(programLine));
    }
};