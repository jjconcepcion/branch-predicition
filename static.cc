#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <unistd.h> // getopt()

void usage(char *baseName);
void simulate(std::string filePath, bool verbose);

int main(int argc, char * argv[]) {
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
    unsigned int programCounter, targetAddress;
    unsigned short branchType;
    bool branchTaken;
    
    traceFile.open(filePath);
    if (traceFile.fail()) {
        std::cerr << "Error: failed to open: " << filePath << std::endl;
        exit(1);
    }
    
    while(std::getline(traceFile, line)) {
        std::stringstream sstream(line);
        sstream >> std::hex >> programCounter;
        sstream >> std::dec >> branchType;
        sstream >> std::hex >> targetAddress;
        sstream >> std::dec >> branchTaken;
        
        std::cout << programCounter << " " << branchType << " ";
        std::cout << targetAddress << " " << branchTaken << std::endl;
    }
    
    traceFile.close();
}
