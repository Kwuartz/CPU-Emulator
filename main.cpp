#include <filesystem>
#include <ncurses.h>
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

const int FRAME_WIDTH = 64;
const int FRAME_HEIGHT = 32;
const int FRAME_SPACING_X = 1;
const int FRAME_SPACING_Y = 0;

const int LOG_WINDOW_HEIGHT = 10;

const int FRAME_BUFFER_SIZE = FRAME_HEIGHT * FRAME_WIDTH;
const int FRAME_BUFFER_START = 4096;
const int FRAME_BUFFER_END = FRAME_BUFFER_START + FRAME_BUFFER_SIZE;

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
        return false;
    }

    int value;

    try {
        value = stoi(cleanToken);
    } catch (...) {
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
            return false;
        }
    };

    return true;
}

class Process {
    public:
        WINDOW* window;

        unordered_map<string, int> branchMap;
        vector<Instruction> instructions;

        vector<int> registers;
        Flags flags;
        string name;
        int id;
        int pc;

        Process(int registerCount, string processName, int processId) {
            int windowWidth = FRAME_WIDTH + 2;
            int windowHeight = FRAME_HEIGHT + 2;

            window = newwin(windowHeight, windowWidth, 0, 0);
            registers = vector<int>(registerCount);
            pc = 0;

            name = processName;
            id = processId;

            updateWindowPosition();
        }

        string getDescriptor() {
            return "Process ID " + to_string(id) + " (" + name + ")";
        }

        void updateWindowPosition() {
            int programWidth = FRAME_WIDTH + 2;
            int programHeight = FRAME_HEIGHT + 2;
            int programDistanceX = programWidth + FRAME_SPACING_X;
            int programDistanceY = programHeight + FRAME_SPACING_Y;
            int windowsPerRow = max(1, COLS / programDistanceX);
            int windowRow = id / windowsPerRow;
            int windowCol = id % windowsPerRow;

            mvwin(window, windowRow * programDistanceY, windowCol * programDistanceX);
            touchwin(window);
            wrefresh(window);
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
    WINDOW* logWindow;
    int logWindowHeight;
    int logWindowWidth;
    int logCursorY;

    public:
        Kernel(int pageCount, int pageSize) {
            frameSize = pageSize;
            memorySize = pageCount * pageSize;
            memory = vector<int>(memorySize);
            
            int frameCount = memorySize / pageSize;
            for (int i = 0; i < frameCount; i++) {
                frameHeap.push_back(i);
            }

            logWindowHeight = LOG_WINDOW_HEIGHT;
            logWindowWidth = COLS;
            logWindow = newwin(logWindowHeight, logWindowWidth, 0, 0);
            logCursorY = 1;
            updateLogWindow();
        }

        bool loadProgram(string directory, string name, int registerCount) {
            ifstream programFile(directory + name);

            if (programFile.fail() == true) {
                log("No program at the specified path: " + directory + name);
                return false;
            }
            
            int instructionNumber = 0;
            int lineNumber = 1;
            string programLine;
            
            int processId = processes.size();
            processes.emplace_back(registerCount, name, processId);
            updateLogWindow();
            Process& process = processes.back();
            
            
            while (getline(programFile, programLine)) {
                programLine.erase(
                    remove(programLine.begin(), programLine.end(), '\r'),
                    programLine.end()
                );

                if (programLine.size() == 0 || programLine.front() == ';') {
                    // Empty line or comment
                } else if (isInstruction(programLine)) {
                    Instruction inst;

                    if (parseInstruction(inst, programLine)) {
                        process.instructions.push_back(move(inst));
                        instructionNumber++;
                    } else {
                        log("Instruction on line " + to_string(lineNumber) + " is invalid: " + programLine);
                        cleanupProcess(process);
                        processes.pop_back();
                        return false;
                    };
                    
                } else if (isBranch(programLine)) {
                    if (!parseBranch(process.branchMap, programLine, instructionNumber)) {
                        log("Branch on line " + to_string(lineNumber) + " is invalid: " + programLine);
                        cleanupProcess(process);
                        processes.pop_back();
                        return false;
                    }
                } else {
                    log("Line " + to_string(lineNumber) + " not recognised as instruction or branch: " + programLine);
                    cleanupProcess(process);
                    processes.pop_back();
                    return false;
                }

                lineNumber++;
            }

            return true;
        }

        bool startExecution(int maxInstructions = -1) {
            while (processes.size() > 0) {
                for (auto it = processes.begin(); it != processes.end();) {
                    if (it->pc <= -1 || it->pc >= static_cast<int>(it->instructions.size())) {
                        bool ok = cleanupProcess(*it);
                        
                        if (ok) {
                            log(it->getDescriptor() + "succesfully cleaned up");
                        } else {
                            log(it->getDescriptor() + " cleanup failed");
                        }

                        it = processes.erase(it);
                    } else if (!executeInstruction(*it, it->instructions[it->pc])) {
                        log("Instruction number " + to_string(it->pc) + " failed to execute");
                        log(it->getDescriptor() + " exited with an error");

                        bool ok = cleanupProcess(*it);
                        
                        if (ok) {
                            log(it->getDescriptor() + " succesfully cleaned up");
                        } else {
                            log(it->getDescriptor() + " cleanup failed");
                        }

                        it = processes.erase(it);
                    } else {
                        it++;
                    }
                }

                if (maxInstructions != -1) {
                    maxInstructions--;

                    if (maxInstructions == 0) {
                        log("Max instructions reached");
                        break;
                    }
                }
            }

            for (auto& process : processes) {
                cleanupProcess(process);
            }

            processes.clear();

            return true;
        }

        void log(const string& message) {
            int contentWidth = max(0, logWindowWidth - 2);

            if (logCursorY > logWindowHeight - 2) {
                wscrl(logWindow, 1);
                logCursorY = logWindowHeight - 2;
            }

            mvwhline(logWindow, logCursorY, 1, ' ', contentWidth);
            mvwprintw(logWindow, logCursorY, 1, "%.*s", contentWidth, message.c_str());
            logCursorY++;

            box(logWindow, 0, 0);
            wrefresh(logWindow);
        }
    
    private:
        vector<Process> processes;
        vector<int> frameHeap;
        vector<int> memory;

        int memorySize;
        int frameSize;

        void updateLogWindow() {
            int programWidth = FRAME_WIDTH + 2;
            int programHeight = FRAME_HEIGHT + 2;
            int programDistanceX = programWidth + FRAME_SPACING_X;
            int programDistanceY = programHeight + FRAME_SPACING_Y;
            int windowsPerRow = max(1, COLS / programDistanceX);
            int processCount = processes.size();
            int processRows = max(1, (processCount + windowsPerRow - 1) / windowsPerRow);
            int logY = processRows * programDistanceY;

            werase(logWindow);
            wrefresh(logWindow);

            logWindowHeight = LOG_WINDOW_HEIGHT;
            logWindowWidth = COLS;

            wresize(logWindow, logWindowHeight, logWindowWidth);
            mvwin(logWindow, logY, 0);
            scrollok(logWindow, true);
            wsetscrreg(logWindow, 1, logWindowHeight - 2);
            logCursorY = 1;
            box(logWindow, 0, 0);
            wrefresh(logWindow);
        }

        int getAddress(Process& process, int virtualAddresss) {
            int pageNumber = virtualAddresss / frameSize;
            
            int frameNumber = process.getFrameNumber(pageNumber);

            // Page fault
            if (frameNumber == -1) { 
                frameNumber = getFreeFrame();

                if (frameNumber == -1) {
                    log("CRITICAL: " + process.getDescriptor() + " cannot be allocated a frame as memory is full");
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
            delwin(process.window);

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

        bool flushFrame(Process& process) {
            for (int row = 0; row < FRAME_HEIGHT; row++) {
                for (int col = 0; col < FRAME_WIDTH; col++) {
                    int virtualAddress = FRAME_BUFFER_START + row * FRAME_WIDTH + col;
                    int address = getAddress(process, virtualAddress);

                    if (address == -1) {
                        return false;
                    }

                    int code = memory[address];

                    char symbol;
                    if (mapSymbol(code, symbol)) {
                        mvwaddch(process.window, row + 1, col + 1, symbol);
                    } else {
                        return false;
                    }
                }
            }

            box(process.window, 0, 0);
            wrefresh(process.window);
            return true;
        }

        bool executeInstruction(Process& process, const Instruction& inst) {
            bool ok = false;
            bool incrementPC = true;

            switch (inst.op) {
                case LDR: {
                    if (inst.args.size() < 2) {
                        ok = false;
                        log("Not enough arguments for load register operation");
                        break;
                    }

                    Argument sourceAddress = inst.args[1];
                    if (inst.args[1].type == REGISTER) {
                        sourceAddress.type = ADDRESS;
                        getValue(process, inst.args[1], sourceAddress.value);
                    }

                    ok = store(process, inst.args[0], sourceAddress);

                    if (!ok) {
                        log("Load register operation failed");
                    }

                    break;
                }

                case STR: {
                    if (inst.args.size() < 2) {
                        ok = false;
                        log("Not enough arguments for store register operation");
                        break;
                    }

                    Argument targetAddress = inst.args[1];
                    if (inst.args[1].type == REGISTER) {
                        targetAddress.type = ADDRESS;
                        getValue(process, inst.args[1], targetAddress.value);
                    }

                    ok = store(process, targetAddress, inst.args[0]);

                    if (!ok)  {
                        log("Store register operation failed");
                    }

                    break;
                }

                case MOV:
                    if (inst.args.size() < 2) {
                        log("Not enough arguments for move operation");
                        ok = false;
                        break;
                    }

                    if (inst.args[0].type != REGISTER) {
                        ok = false;
                        log("Only a register address can be provided as the first argument of a move operation");
                        break;
                    }

                    if (inst.args[1].type == ADDRESS) {
                        ok = false;
                        log("Memory addresses can not be provided to a move operation");
                        break;
                    }

                    ok = store(process, inst.args[0], inst.args[1]);

                    if (!ok)  {
                        log("Move operation failed");
                    }

                    break;

                case ADD:
                    if (inst.args.size() < 3) {
                        ok = false;
                        log("Not enough arguments for add operation");
                        break;
                    }

                    if (inst.args[0].type != REGISTER || inst.args[1].type != REGISTER) {
                        ok = false;
                        log("Only register addresses can be provided to the first 2 arguments of an add operation");
                        break;
                    }

                    if (inst.args[2].type == ADDRESS) {
                        ok = false;
                        log("Memory addresses can not be provided as an operand to an add operation");
                        break;
                    }

                    ok = add(process, inst.args[0], inst.args[1], inst.args[2], 1);

                    if (!ok)  {
                        log("Add operation failed");
                    }

                    break;

                case SUB:
                    if (inst.args.size() < 3) {
                        ok = false;
                        log("Not enough arguments for subtract operation");
                        break;
                    }

                    if (inst.args[0].type != REGISTER || inst.args[1].type != REGISTER) {
                        ok = false;
                        log("Only register addresses can be provided to the first 2 arguments of a subtract operation");
                        break;
                    }

                    if (inst.args[2].type == ADDRESS) {
                        ok = false;
                        log("Memory addresses can not be provided as an operand to a subtract operation");
                        break;
                    }

                    ok = add(process, inst.args[0], inst.args[1], inst.args[2], -1);

                    if (!ok)  {
                        log("Add operation failed");
                    }

                    break;

                case MUL:
                    if (inst.args.size() < 3) {
                        ok = false;
                        log("Not enough arguments for multiply operation");
                        break;
                    }

                    if (inst.args[0].type != REGISTER || inst.args[1].type != REGISTER) {
                        ok = false;
                        log("Only register addresses can be provided to the first 2 arguments of a multiply operation");
                        break;
                    }

                    if (inst.args[2].type == ADDRESS) {
                        ok = false;
                        log("Memory addresses can not be provided as an operand to a multiply operation");
                        break;
                    }

                    ok = multiply(process, inst.args[0], inst.args[1], inst.args[2]);

                    if (!ok)  {
                        log("Multiply operation failed");
                    }

                    break;

                case LSL:
                    if (inst.args.size() < 3) {
                        ok = false;
                        log("Not enough arguments for logical shift left operation");
                        break;
                    }

                    if (inst.args[0].type != REGISTER || inst.args[1].type != REGISTER) {
                        ok = false;
                        log("Only register addresses can be provided to the first 2 arguments of a logical shift left operation");
                        break;
                    }

                    if (inst.args[2].type == ADDRESS) {
                        ok = false;
                        log("Memory addresses can not be provided as an operand to a logical shift left operation");
                        break;
                    }

                    ok = shift(process, inst.args[0], inst.args[1], inst.args[2], 1);

                    if (!ok)  {
                        log("Logical shift left operation failed");
                    }

                    break;

                case LSR:
                    if (inst.args.size() < 3) {
                        ok = false;
                        log("Not enough arguments for logical shift right operation");
                        break;
                    }

                    if (inst.args[0].type != REGISTER || inst.args[1].type != REGISTER) {
                        ok = false;
                        log("Only register addresses can be provided to the first 2 arguments of a logical shift right operation");
                        break;
                    }

                    if (inst.args[2].type == ADDRESS) {
                        ok = false;
                        log("Memory addresses can not be provided as an operand to a logical shift right operation");
                        break;
                    }

                    ok = shift(process, inst.args[0], inst.args[1], inst.args[2], -1);

                    if (!ok)  {
                        log("Logical shift right operation failed");
                    }

                    break;

                case CMP:
                    if (inst.args.size() < 2) {
                        ok = false;
                        log("Not enough arguments for compare operation");
                        break;
                    }

                    if (inst.args[0].type != REGISTER) {
                        ok = false;
                        log("Only a register address can be provided as the first argument of a compare operation");
                        break;
                    }

                    if (inst.args[1].type == ADDRESS) {
                        ok = false;
                        log("A memory address can not be provided as the operarond of a compare operation");
                        break;
                    }

                    ok = compare(process, inst.args[0], inst.args[1]);

                    if (!ok) {
                        log("Compare failed");
                    }

                    break;

                case B:
                    if (inst.args.size() < 1) {
                        ok = false;
                        log("Not enough arguments for branch operation");
                        break;
                    }

                    ok = branch(process, inst.args[0]);

                    if (ok) {
                        incrementPC = false;
                    } else {
                        log("Branch failed");
                    }
                    
                    break;

                case BEQ:
                    if (inst.args.size() < 1) {
                        ok = false;
                        log("Not enough arguments for branch operation");
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
                        log("Branch failed");
                    }

                    break;

                case BNE:
                    if (inst.args.size() < 1) {
                        ok = false;
                        log("Not enough arguments for branch operation");
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
                        log("Branch failed");
                    }

                    break;

                case BGT:
                    if (inst.args.size() < 1) {
                        ok = false;
                        log("Not enough arguments for branch operation");
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
                        log("Branch failed");
                    }

                    break;

                case BLT:
                    if (inst.args.size() < 1) {
                        ok = false;
                        log("Not enough arguments for branch operation");
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
                        log("Branch failed");
                    }

                    break;
                
                case FLUSH:
                    ok = flushFrame(process);

                    if (!ok) {
                        log("Flush frame failed");
                    }

                    break;

                case WAIT:
                    if (inst.args.size() < 1) {
                        ok = false;
                        log("Not enough arguments for wait operation");
                        break;
                    }

                    int delay;

                    if (!getValue(process, inst.args[0], delay)) {
                        ok = false;
                        log("Wait delay argument parsing failed");
                        break;
                    }

                    this_thread::sleep_for(chrono::milliseconds(delay));
                    ok = true;

                    if (!ok) {
                        log("Wait failed");
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
    initscr();
    noecho();
    curs_set(0);

    Kernel kernel(PAGE_COUNT, PAGE_SIZE);

    bool ok;
    bool isGitlabCI = checkGitLabCI();

    if (!fs::exists(programsDirectory)) {
        kernel.log("Programs directory does not exist");
        if (!isGitlabCI) {
            getch();
        }
        endwin();
        return 0;
    }

    for (const auto& entry : fs::directory_iterator(programsDirectory)) {
        fs::path filePath = entry.path();
        string fileName = filePath.filename().string();
        string extension = filePath.extension().string();
        
        if (extension != ".asm") {
            kernel.log(" File not loaded as it is not an .asm file: " + fileName);
            continue;
        }

        ok = kernel.loadProgram(programsDirectory, fileName, REGISTER_COUNT);
        
        if (!ok) {
            kernel.log("Error while loading .asm file: " + fileName);
        } else {
            kernel.log("Succesfully loaded .asm file: " + fileName);
        }
    }

    kernel.log("Programs loaded succesfully");

    if (isGitlabCI) {
        ok = kernel.startExecution(MAX_TEST_INSTRUCTIONS);
    } else {
        ok = kernel.startExecution();
    }

    if (ok) {
        kernel.log("All programs executed succesfully");
    } else {
        kernel.log("Error during exectuion");
    }

    if (!isGitlabCI) {
        getch();
    }

    endwin();

    return 0;
};
