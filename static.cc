#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <unistd.h> // getopt()

typedef struct TraceLineDetail {
    unsigned int programCounter;
    unsigned short branchType;
    unsigned int targetAddress;
    bool branchTaken;
} TraceInfo;

const unsigned short CONDITIONAL_BRANCH = 1;

void usage(char *baseName);
void simulate(std::string filePath, bool verbose);
void parseLine(std::string *line, TraceInfo *trace);
void printLine(TraceInfo *trace);

int main(int argc, char *argv[]) {
    std::string traceFilePath;
    bool vflag = false;   // verbose mode flag
    int option;
    
    // parse command line args
    while ((option = getopt(argc, argv, "v")) != -1) {
        switch (option) {
        case 'v':
            vflag = true;
            break;
        default:
            usage(argv[0]);
            exit(1);
        }
    }
    
    if (optind >= argc) {
        usage(argv[0]);
        exit(1);
    }
    
    traceFilePath = argv[optind];
    
    simulate(traceFilePath, vflag);
    
    return 0;
}

void usage(char *baseName) {
    std::cerr << "Usage: " << baseName << " [-v] TRACE_FILE" << std::endl;
}

void simulate(std::string filePath, bool verbose) {
    std::ifstream traceFile;
    std::string line;
    
    traceFile.open(filePath);
    if (traceFile.fail()) {
        std::cerr << "Error: failed to open: " << filePath << std::endl;
        exit(1);
    }
    
    while(std::getline(traceFile, line)) {
        TraceInfo trace;
        parseLine(&line, &trace);
        
        if(verbose && trace.branchType == CONDITIONAL_BRANCH) {
            std::cout << line << std::endl;
        }
    }
    
    traceFile.close();
}

void parseLine(std::string *line, TraceInfo *trace) {
    std::stringstream sstream(*line);
    
    sstream >> std::hex >> trace->programCounter;
    sstream >> std::dec >> trace->branchType;
    sstream >> std::hex >> trace->targetAddress;
    sstream >> std::dec >> trace->branchTaken;
}
