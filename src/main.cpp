// usage
// ./event_engine_app fifo 2 2 5 logs.txt
// ./event_engine_app rr 2 2 5 logs.txt
// ./event_engine_app weighted 2 2 5 logs.txt

#include "event_engine.h"
#include <iostream>
#include <cstdlib>
#include <string>

int main(int argc, char* argv[]) {
    bool isJson = false;
    if (argc >= 7 && std::string(argv[6]) == "--json") {
        isJson = true;
    } else if (argc != 6) {
        std::cerr << "Usage: " << argv[0]
                  << " <fifo|weighted|rr> <num_producers> <num_consumers> <buffer_size> <log_file> [--json]\n";
        return 1;
    }

    std::string modeArg = argv[1];

    SchedulerMode mode; // for metrics later, we can go between different modes (i hope lmao)
                        // I am just too lazy to evaluate that right now
    if (modeArg == "fifo") {
        mode = SchedulerMode::FIFO;
    } else if (modeArg == "weighted") {
        mode = SchedulerMode::WEIGHTED;
    } 
    else if (modeArg == "rr") {
        mode = SchedulerMode::RR;
    }
    else {
        std::cerr << "Invalid scheduler mode. Use fifo, rr, or weighted.\n";
        return 1;
    }

    int p = std::atoi(argv[2]);
    int c = std::atoi(argv[3]);
    int size = std::atoi(argv[4]);

    InitEventEngine(mode, p, c, size, argv[5], isJson);

    return 0;
}