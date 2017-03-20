#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <unistd.h> // getopt()

typedef struct TraceLineDetail {
    unsigned int programCounter;
    unsigned short branchType;
    unsigned int targetAddress;
    bool branchTaken;
} TraceInfo;

typedef struct BranchStatistics {
    unsigned int branches;
    unsigned int forward;
    unsigned int forwardTaken;
    unsigned int backward;
    unsigned int backwardTaken;
    unsigned int misprediction;
} BranchStats;

typedef struct BranchTargetBufferEntry {
    unsigned int tag;
    unsigned int targetAddress;
    bool valid;
} BtbEntry;

const unsigned short CONDITIONAL_BRANCH = 1;

void usage(char *baseName);
void simulate(std::string filePath, unsigned int pbSize,
              unsigned int btbSize, bool verbose);
void parseLine(std::string *line, TraceInfo *trace);
void printLine(TraceInfo *trace);
void evaluate(TraceInfo *trace, BranchStats *stats);
void printSummary(BranchStats *stats);

int main(int argc, char *argv[]) {
    std::string traceFilePath;
    bool vflag = false;   // verbose mode flag
    int option;
    unsigned int pbSize;    // size of prediction buffer
    unsigned int btbSize;  // size of branch target buffer
    
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
    
    if ((optind + 2) >= argc) {
        usage(argv[0]);
        exit(1);
    }
    
    traceFilePath = argv[optind];
    pbSize = atoi(argv[optind + 1]);
    btbSize = atoi(argv[optind + 2]);
    
    simulate(traceFilePath, pbSize, btbSize, vflag);
    
    return 0;
}

void usage(char *baseName) {
    std::cerr << "Usage: " << baseName << " [-v] TRACE_FILE ";
    std::cerr << "PB_SIZE BTB_SIZE" << std::endl;
    std::cerr << "Option:" << std::endl;
    std::cerr << "\t-v,\tverbose mode" << std::endl;
    std::cerr << "Arguments:" << std::endl;
    std::cerr << "\tTRACE_FILE: path to trace file" << std::endl;
    std::cerr << "\tPB_SIZE: size of prediction buffer (power of 2)";
    std::cerr << std::endl;
    std::cerr << "\tBTB_SIZE: size of taget buffer (power of 2)" << std::endl;

}
void simulate(std::string filePath, unsigned int pbSize,
              unsigned int btbSize, bool verbose) {
    std::ifstream traceFile;
    std::string line;
    BranchStats stats = {0};
    unsigned char predictions[pbSize];
    BtbEntry btb[btbSize];
    
    traceFile.open(filePath.c_str(), std::ifstream::in);
    if (traceFile.fail()) {
        std::cerr << "Error: failed to open: " << filePath << std::endl;
        exit(1);
    }
    
    while(std::getline(traceFile, line)) {
        TraceInfo trace;
        parseLine(&line, &trace);
        evaluate(&trace, &stats);
        
        if (verbose && trace.branchType == CONDITIONAL_BRANCH) {
            std::cout << line << std::endl;
        }
    }
    traceFile.close();

    printSummary(&stats);
}

void parseLine(std::string *line, TraceInfo *trace) {
    std::stringstream sstream(*line);
    
    sstream >> std::hex >> trace->programCounter;
    sstream >> std::dec >> trace->branchType;
    sstream >> std::hex >> trace->targetAddress;
    sstream >> std::dec >> trace->branchTaken;
}

void evaluate(TraceInfo *trace, BranchStats *stats) {
    if(trace->branchType != CONDITIONAL_BRANCH) {
        return;
    }

    bool branchTaken = trace->branchTaken;
    bool forwardBranch = (trace->programCounter < trace->targetAddress);
    bool backwardBranch = !forwardBranch;

    stats->branches += 1;
    if (forwardBranch) {
        stats->forward += 1;

        if (branchTaken) {
            stats->forwardTaken += 1;
            stats->misprediction += 1;
        }
    } else if (backwardBranch) {
        stats->backward += 1;

        if (branchTaken) {
            stats->backwardTaken += 1;
        } else {
            stats->misprediction += 1;
        }
    }
}

void printSummary(BranchStats *stats) {
    using namespace std;
    double mispredicitonRate;

    mispredicitonRate = static_cast<double>(stats->misprediction)
        / stats->branches;

    cout << "Number of branches = " << stats->branches << endl;
    cout << "Number of forward branches = ";
    cout << stats->forward << endl;
    cout << "Number of forward taken branches = ";
    cout << stats->forwardTaken << endl;
    cout << "Number of backward branches = ";
    cout <<  stats->backward << endl;
    cout << "Number of backward taken branches = ";
    cout << stats->backwardTaken << endl;
    cout << "Number of mispredictions = ";
    cout << stats->misprediction << " " << mispredicitonRate << endl;
}
