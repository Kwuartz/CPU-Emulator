#include <iostream>

#include <filesystem>
#include <stdexcept>
#include <algorithm>
#include <cstdlib>
#include <thread>
#include <chrono>

#include <unordered_map>
#include <sstream>
#include <fstream>
#include <vector>

using namespace std;

const int MEMORY_SIZE = 4096;
const int PAGE_SIZE = 128;
const int PAGE_COUNT = MEMORY_SIZE / PAGE_SIZE;
const int REGISTER_COUNT = 20;

const int FRAME_WIDTH = 100;
const int FRAME_HEIGHT = 30;
const int FRAME_BUFFER_SIZE = FRAME_HEIGHT * FRAME_WIDTH;

const int MAX_TEST_INSTRUCTIONS = 1000;

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

class Process {
    public:
        unordered_map<string, int> branchMap;
        vector<Instruction> instructions;

        vector<int> registers;
        Flags flags;
        string name;
        int id;
        int pc;

        Process(int registerCount, string processName, int processId) {
            registers = vector<int>(registerCount);
            name = processName;

            id = processId;
            pc = 0;
        }

        string getDescriptor() {
            return "Process ID " + to_string(id) + " (" + name + ")";
        }

        bool addPage(int pageNumber, int frameNumber) {
            auto it = pageMap.find(pageNumber);

            if (it != pageMap.end()) {
                return false;
            } else {
                pageMap[pageNumber] = frameNumber;
            }
            
            return true;
        }

        vector<int> getAllFrameNumbers() const {
            vector<int> frameNumbers;
            
            for (const auto& [page, frame]: pageMap) {
                frameNumbers.push_back(frame);
            }

            return frameNumbers;
        }

        int getFrameNumber(int pageNumber) const {
            auto it = pageMap.find(pageNumber);

            if (it != pageMap.end()) {
                return it->second;
            }

            return -1;
        }

    private:
        unordered_map<int, int> pageMap;
};

class Kernel {
    public:
        Kernel(int pageCount, int pageSize) {
            frameSize = pageSize;
            memorySize = pageCount * pageSize;
            memory = vector<int>(memorySize);
            
            int frameCount = memorySize / pageSize;
            for (int i = 0; i < frameCount; i++) {
                frameHeap.push_back(i);
            }
        }

        bool loadProgram(string directory, string name, int registerCount) {
            ifstream programFile(directory + name);

            if (programFile.fail() == true) {
                cout << "No program at the specified path: " << directory + name << "\n";
                return false;
            }
            
            int instructionNumber = 0;
            int lineNumber = 0;
            string programLine;
            
            int processId = processes.size();
            Process process(registerCount, name, processId);
            
            
            while (getline(programFile, programLine)) {
                if (programLine.size() == 0 || programLine.front() == ';') {
                    // Comment
                } else if (isInstruction(programLine)) {
                    Instruction inst;

                    if (parseInstruction(inst, programLine)) {
                        process.instructions.push_back(move(inst));
                        instructionNumber++;
                    } else {
                        cout << "Instruction on line " << lineNumber << " is invalid: " << programLine << "\n";
                        return false;
                    };
                    
                } else if (isBranch(programLine)) {
                    if (!parseBranch(process.branchMap, programLine, instructionNumber)) {
                        cout << "Branch on line " << lineNumber << " is invalid: " << programLine << "\n";
                        return false;
                    }
                } else {
                    cout << "Line " << lineNumber << " not recognised as instruction or branch: " << programLine << "\n";
                    return false;
                }

                lineNumber++;
            }

            processes.push_back(process);

            return true;
        }

        bool startExecution(int maxInstructions = -1) {
            while (processes.size() > 0) {
                for (auto it = processes.begin(); it != processes.end();) {
                    if (it->pc <= -1 || it->pc >= static_cast<int>(it->instructions.size())) {
                        bool ok = cleanupProcess(*it);
                        
                        if (ok) {
                            cout << it->getDescriptor() << "succesfully cleaned up" << "\n";
                        } else {
                            cout << it->getDescriptor() << " cleanup failed" << "\n";
                        }

                        it = processes.erase(it);
                    } else if (!executeInstruction(*it, it->instructions[it->pc])) {
                        cout << "Instruction number " << to_string(it->pc) << " failed to execute" << "\n";
                        cout << it->getDescriptor() << " exited with an error" << "\n";

                        bool ok = cleanupProcess(*it);
                        
                        if (ok) {
                            cout << it->getDescriptor() << " succesfully cleaned up" << "\n";
                        } else {
                            cout << it->getDescriptor() << " cleanup failed" << "\n";
                        }

                        it = processes.erase(it);
                    } else {
                        it++;

                        if (maxInstructions != -1) {
                            maxInstructions--;

                            if (maxInstructions == 0) {
                                break;
                            }
                        }
                    }
                }

                if (maxInstructions == 0) {
                    break;
                }
            }

            for (auto& process : processes) {
                cleanupProcess(process);
            }
            
            processes.clear();
            
            return true;
        }
    
    private:
        vector<Process> processes;
        vector<int> frameHeap;
        vector<int> memory;

        int memorySize;
        int frameSize;

        int getAddress(Process& process, int virtualAddresss) {
            int pageNumber = virtualAddresss / frameSize;
            
            int frameNumber = process.getFrameNumber(pageNumber);

            // Page fault
            if (frameNumber == -1) { 
                frameNumber = getFreeFrame();

                if (frameNumber == -1) {
                    cout << "CRITICAL: " << process.getDescriptor() << " cannot be allocated a frame as memory is full" << "\n";
                    return -1;
                }

                process.addPage(pageNumber, frameNumber);   
            }
            
            int offset = virtualAddresss % frameSize;
            int address = frameNumber * frameSize + offset;

            if (address >= memorySize) {
                return -1;
            }

            return address;
        }

        bool cleanupProcess(const Process &process) {
            vector<int> frameNumbers = process.getAllFrameNumbers();

            bool ok;
            for (int frameNumber: frameNumbers) {
                ok = returnPage(frameNumber);
                
                if (!ok) {
                    return false;
                }
            }

            return true;
        }

        int getFreeFrame() {
            if (frameHeap.size() > 0) {
                int frame = frameHeap.back();
                frameHeap.pop_back();

                return frame;
            } else {
                return -1;
            }
        }

        bool returnPage(int frameNumber) {
            if (find(frameHeap.begin(), frameHeap.end(), frameNumber) == frameHeap.end()) {
                frameHeap.push_back(frameNumber);
                return true;
            } else {
                return false;
            }
        }

        bool getValue(Process& process, const Argument& arg, int& result) {
            if (arg.type == IMMEDIATE) {
                result = arg.value;
            } else if (arg.type == REGISTER) {
                result = process.registers[arg.value];
            } else if (arg.type == ADDRESS) {
                int address = getAddress(process, arg.value);

                if (address == -1) {
                    return false;
                }

                result = memory[address];
            } else {
                return false;
            }

            return true;
        }

        bool branch(Process& process, const Argument& arg) {
            if (!arg.label.empty()) {
                auto it = process.branchMap.find(arg.label);

                if (it != process.branchMap.end()) {
                    process.pc = it->second;
                } else {
                    return false;
                }
            } else {
                return false;
            }
            

            return true;
        }

        bool store(Process& process, const Argument& target, const Argument& source) {
            int value;

            if (!getValue(process, source, value)) {
                return false;
            }

            if (target.type == REGISTER) {
                if (target.value < REGISTER_COUNT) {
                    process.registers[target.value] = value;
                } else {
                    return false;
                }
            } else if (target.type == ADDRESS) {
                int address = getAddress(process, target.value);
                
                if (address == -1) {
                    return false;
                }

                memory[address] = value;
            } else {
                return false;
            }

            return true;
        }

        bool add(Process& process, const Argument& target, const Argument& operand1, const Argument& operand2, int sign) {
            int num1;
            int num2;

            if (!getValue(process, operand1, num1) || !getValue(process, operand2, num2)) {
                return false;
            }

            Argument result;
            int sum = num1 + (num2 * sign);
            
            result.type = IMMEDIATE;
            result.value = sum;

            return store(process, target, result);
        }

        bool multiply(Process& process, const Argument& target, const Argument& operand1, const Argument& operand2) {
            int num1;
            int num2;

            if (!getValue(process, operand1, num1) || !getValue(process, operand2, num2)) {
                return false;
            }

            Argument result;
            int product = num1 * num2;
            
            result.type = IMMEDIATE;
            result.value = product;

            return store(process, target, result);
        }

        bool shift(Process& process, const Argument& target, const Argument& operand1, const Argument& operand2, int direction) {
            int num1;
            int num2;

            if (!getValue(process, operand1, num1) || !getValue(process, operand2, num2)) {
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

            return store(process, target, result);
        }

        bool compare(Process& process, const Argument& operand1, const Argument& operand2) {
            int num1;
            int num2;

            if (!getValue(process, operand1, num1) || !getValue(process, operand2, num2)) {
                return false;
            }

            process.flags.equal = num1 == num2;
            process.flags.greater = num1 > num2;
            process.flags.less = num1 < num2;
            
            return true;
        }

        bool flushFrame(const Process& process) {
            string frame;
            
            cout << "\033[H\033[2J";

            for (int i = 0; i < 1; i++) {
                // Temp
                int code = memory[i];

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

        bool executeInstruction(Process& process, const Instruction& inst) {
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
                        getValue(process, inst.args[1], sourceAddress.value);
                    }

                    ok = store(process, inst.args[0], sourceAddress);

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
                        getValue(process, inst.args[1], targetAddress.value);
                    }

                    ok = store(process, targetAddress, inst.args[0]);

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

                    ok = store(process, inst.args[0], inst.args[1]);

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

                    ok = add(process, inst.args[0], inst.args[1], inst.args[2], 1);

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

                    ok = add(process, inst.args[0], inst.args[1], inst.args[2], -1);

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

                    ok = multiply(process, inst.args[0], inst.args[1], inst.args[2]);

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

                    ok = shift(process, inst.args[0], inst.args[1], inst.args[2], 1);

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

                    ok = shift(process, inst.args[0], inst.args[1], inst.args[2], -1);

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

                    ok = compare(process, inst.args[0], inst.args[1]);

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

                    ok = branch(process, inst.args[0]);

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

                    if (!process.flags.equal) {
                        ok = true;
                        break;
                    }

                    ok = branch(process, inst.args[0]);

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

                    if (process.flags.equal) {
                        ok = true;
                        break;
                    }

                    ok = branch(process, inst.args[0]);

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

                    if (!process.flags.greater) {
                        ok = true;
                        break;
                    }

                    ok = branch(process, inst.args[0]);

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

                    if (!process.flags.less) {
                        ok = true;
                        break;
                    }

                    ok = branch(process, inst.args[0]);

                    if (ok) {
                        incrementPC = false;
                    } else {
                        cout << "Branch failed" << "\n";
                    }

                    break;
                
                case FLUSH:
                    cout << "Flush" << endl;
                    ok = flushFrame(process);

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

                    if (!getValue(process, inst.args[0], delay)) {
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
                    process.pc = -1;
                    break;
            }

            if (incrementPC) {
                process.pc++;
            }

            return ok;
        };
};

bool checkGitLabCI() {
    const char* ci = getenv("GITLAB_CI");
    return ci && string(ci) == "true";
}

const string programsDirectory = "programs/";
namespace fs = filesystem;

int main() {
    Kernel kernel(PAGE_COUNT, PAGE_SIZE);

    bool ok;

    if (!fs::exists(programsDirectory)) {
        cout << "Programs directory does not exist\n";
        return 0;
    }

    for (const auto& entry : fs::directory_iterator(programsDirectory)) {
        fs::path filePath = entry.path();
        string fileName = filePath.filename().string();
        string extension = filePath.extension().string();
        
        if (extension != ".asm") {
            cout << " File not loaded as it is not an .asm file: " << fileName << "\n";
            continue;
        }

        ok = kernel.loadProgram(programsDirectory, fileName, REGISTER_COUNT);
        
        if (!ok) {
            cout << "Error while loading .asm file: " << fileName <<"\n";
        } else {
            cout << "Succesfully loaded .asm file: " << fileName <<"\n";
        }

        std::cout << entry.path() << "\n";
    }

    cout << "Programs loaded succesfully" << "\n";
    
    bool isGitlabCI = checkGitLabCI();

    if (isGitlabCI) {
        ok = kernel.startExecution(MAX_TEST_INSTRUCTIONS);
    } else {
        ok = kernel.startExecution();
    }

    if (ok) {
        cout << "All programs executed succesfully" << "\n";
    } else {
        cout << "Error during exectuion" << "\n";
    }

    return 0;
};