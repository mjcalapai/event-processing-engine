#include "event_engine.h"
#include <iostream>
#include <cstdlib>

int main(int argc, char* argv[]) {
    if (argc != 5) {
        std::cerr << "Usage: " << argv[0]
                  << " <num_producers> <num_consumers> <bb_size> <log_file>\n";
        return 1;
    }

    int p = std::atoi(argv[1]);
    int c = std::atoi(argv[2]);
    int size = std::atoi(argv[3]);

    InitEventEngine(p, c, size, argv[4]);

    return 0;
}