#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <cstdint>
#include <vector>
#include <unistd.h> // getopt()

typedef struct TraceLineDetail {
    uint32_t programCounter;
    unsigned short branchType;
    uint32_t targetAddress;
    bool branchTaken;
    uint32_t predictionIndex;
    unsigned char currentPrediction;
    unsigned char nextPrediction;
    uint32_t btbIndex;
    uint32_t btbTag;
} TraceInfo;

typedef struct BranchStatistics {
    uint32_t branches;
    uint32_t forward;
    uint32_t forwardTaken;
    uint32_t backward;
    uint32_t backwardTaken;
    uint32_t predictCorrect;
    uint32_t predictIncorrect;
    uint32_t btbHit;
    uint32_t btbMiss;
} BranchStats;

typedef struct BranchTargetBufferEntry {
    uint32_t tag;
    uint32_t targetAddress;
    bool valid;
} BtbEntry;

const unsigned short CONDITIONAL_BRANCH = 1;
const unsigned char DEFAULT_PREDICTION = 1;
const BtbEntry DEFAULT_BTB_ENTRY = {0,0,0};
const int BITS_WITHIN_WORD = 2;

void usage(char *baseName);
void simulate(std::string filePath, uint32_t pbSize,
              uint32_t btbSize, bool verbose);
void parseLine(std::string *line, TraceInfo *trace);
void evaluate(TraceInfo *trace, BranchStats *stats,
              std::vector<unsigned char> &predictionBuffer,
              std::vector<BtbEntry> &branchTargetBuffer);
void printSummary(BranchStats *stats);
uint32_t log2(uint32_t x);
uint32_t bufferIndex(uint32_t bufferSize, uint32_t address);
void printVerboseMessages(TraceInfo &trace, BranchStats &stats);
void updatePrediction(std::vector<unsigned char> &predictionBuffer,
                      TraceInfo &trace, BranchStats &stats);

int main(int argc, char *argv[]) {
    std::string traceFilePath;
    bool vflag = false;   // verbose mode flag
    int option;
    uint32_t pbSize;    // size of prediction buffer
    uint32_t btbSize;  // size of branch target buffer
    
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
    std::cerr << "\tBTB_SIZE: size of target buffer (power of 2)" << std::endl;
}

uint32_t log2(uint32_t x) {
    uint32_t retval = 0;
    while( x>>= 1) {
        retval++;
    }
    return retval;
}

void simulate(std::string filePath, uint32_t pbSize,
              uint32_t btbSize, bool verbose) {
    std::ifstream traceFile;
    std::string line;
    BranchStats stats = {0};
    std::vector<unsigned char> predictions (pbSize, DEFAULT_PREDICTION);
    std::vector<BtbEntry> btb (btbSize, DEFAULT_BTB_ENTRY);
    
    traceFile.open(filePath.c_str(), std::ifstream::in);
    if (traceFile.fail()) {
        std::cerr << "Error: failed to open: " << filePath << std::endl;
        exit(1);
    }
    
    while(std::getline(traceFile, line)) {
        TraceInfo trace;
        parseLine(&line, &trace);
        evaluate(&trace, &stats, predictions, btb);
        
        if (verbose && trace.branchType == CONDITIONAL_BRANCH) {
            printVerboseMessages(trace, stats);
        }
    }
    traceFile.close();

    printSummary(&stats);
}

uint32_t bufferIndex(uint32_t bufferSize, uint32_t address) {
    uint32_t index, nbits;

    nbits = log2(bufferSize);
    index = address << (32 - nbits - BITS_WITHIN_WORD);
    index >>= (32 - nbits);

    return index;
}

void parseLine(std::string *line, TraceInfo *trace) {
    std::stringstream sstream(*line);
    
    sstream >> std::hex >> trace->programCounter;
    sstream >> std::dec >> trace->branchType;
    sstream >> std::hex >> trace->targetAddress;
    sstream >> std::dec >> trace->branchTaken;
}

bool predictTaken(unsigned char prediction) {
    return (prediction == 2 || prediction == 3);
}

bool validTag(uint32_t tag, BtbEntry &entry) {
    return (entry.valid && tag == entry.tag);
}

void evaluate(TraceInfo *trace, BranchStats *stats,
              std::vector<unsigned char> &predictionBuffer,
              std::vector<BtbEntry> &branchTargetBuffer) {
    if(trace->branchType != CONDITIONAL_BRANCH) {
        return;
    }
    bool branchTaken, forwardBranch, backwardBranch;
    uint32_t tag, addressLowOrderBits;
    BtbEntry btbEntry;
    
    /*
     * Check the prediction for the current branch instruction. When branch
     * taken, check the branch tag against the BTB, and record the BTB
     * hits and misses. Do nothing if branch is not taken.
     */
    trace->predictionIndex = bufferIndex(predictionBuffer.size(), 
                                        trace->programCounter);
    trace->btbIndex = bufferIndex(branchTargetBuffer.size(),
                                 trace->programCounter);
    trace->currentPrediction = predictionBuffer[trace->predictionIndex];
    addressLowOrderBits = log2(branchTargetBuffer.size()) + BITS_WITHIN_WORD;
    tag = trace->programCounter >> addressLowOrderBits;
    btbEntry = branchTargetBuffer[trace->btbIndex];
    if (predictTaken(trace->currentPrediction)) {
        if(validTag(tag, btbEntry))
            stats->btbHit++;
        else
            stats->btbMiss++;
    }

    updatePrediction(predictionBuffer, *trace, *stats);

    branchTaken = trace->branchTaken;
    forwardBranch = (trace->programCounter < trace->targetAddress);
    backwardBranch = !forwardBranch;

    // update BTB
    if (trace->branchTaken) {
        branchTargetBuffer[trace->btbIndex].tag = tag;
        branchTargetBuffer[trace->btbIndex].valid = true;
    }
    trace->btbTag = branchTargetBuffer[trace->btbIndex].tag;

    // record branch statistics
    stats->branches += 1;
    if (forwardBranch) {
        stats->forward += 1;

        if (branchTaken) {
            stats->forwardTaken += 1;
        }
    } else if (backwardBranch) {
        stats->backward += 1;

        if (branchTaken) {
            stats->backwardTaken += 1;
        }
    }
}

void printSummary(BranchStats *stats) {
    using namespace std;
    double mispredicitonRate, btbMissRate;

    mispredicitonRate = static_cast<double>(stats->predictIncorrect)
        / (stats->predictCorrect + stats->predictIncorrect);

    btbMissRate = static_cast<double>(stats->btbMiss)
        / (stats->btbMiss + stats->btbHit);

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
    cout << stats->predictIncorrect << " " << mispredicitonRate << endl;
    cout << "Number of BTB misses = ";
    cout << stats->btbMiss << " " << btbMissRate << endl;
}

void printVerboseMessages(TraceInfo &trace, BranchStats &stats) {
    static int order = 0;
    std::cout << order << " ";
    order += 1;
    std::cout.setf(std::ios::hex, std::ios::basefield);
    std::cout << trace.predictionIndex << " ";
    std::cout.unsetf(std::ios::hex);
    std::cout << (int) trace.currentPrediction << " ";
    std::cout << (int) trace.nextPrediction << " ";
    std::cout << trace.btbIndex << " ";
    std::cout.setf(std::ios::hex, std::ios::basefield);
    std::cout << trace.btbTag << " ";
    std::cout.unsetf(std::ios::hex);
    std::cout << stats.btbHit + stats.btbMiss << " ";
    std::cout << stats.btbMiss << std::endl;
}

void updatePrediction(std::vector<unsigned char> &predictionBuffer,
                      TraceInfo &trace, BranchStats &stats) {
    unsigned char prediction, newPrediction;
    bool branchTaken;

    prediction = trace.currentPrediction;
    branchTaken = trace.branchTaken;

    // Wrong predictions
    if ((prediction == 0 || prediction == 1) && branchTaken) {
        newPrediction = prediction + 1;
        stats.predictIncorrect += 1;
    } else if ((prediction == 3 || prediction == 2) && !branchTaken) {
        newPrediction = prediction - 1;
        stats.predictIncorrect += 1;
    }
    // Correct predictions
    else if (prediction == 2 && branchTaken) {
        newPrediction = prediction + 1;
        stats.predictCorrect += 1;
    }
    else if (prediction == 1 && !branchTaken) {
        newPrediction = prediction - 1;
        stats.predictCorrect += 1;
    }// Correct prediction and keep same prediction state
    else {
        newPrediction = prediction;
        stats.predictCorrect += 1;
    }

    //record change
    predictionBuffer[trace.predictionIndex] = trace.nextPrediction = newPrediction;
}
