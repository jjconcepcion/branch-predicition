#include <iostream>
#include <string>
#include <unistd.h> // getopt()

void usage(char *baseName);

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
    
    
    return 0;
}

void usage(char *baseName) {
    std::cerr << "Usage: " << baseName << " [-v] TRACE_FILE" << std::endl;
}
