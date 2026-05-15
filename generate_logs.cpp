#include "log_entry.h"
#include <iostream>
#include <fstream>
#include <random>

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <num_lines> <output_file>\n";
        return 1;
    }

    int num_lines = std::atoi(argv[1]);
    std::string output_file = argv[2];

    std::ofstream out(output_file);
    if (!out) {
        std::cerr << "Failed to open " << output_file << " for writing.\n";
        return 1;
    }

    std::mt19937 rng(1337);
    std::uniform_int_distribution<int> dist(1, 100);

    for (int i = 0; i < num_lines; i++) {
        uint64_t id = nextLogID();
        int roll = dist(rng);
        LogEntry entry;

        if (roll <= 80) {
            entry = generateMixedNoise(id);
        } else if (roll <= 90) {
            entry = generateNormalTraffic(id);
        } else if (roll <= 95) {
            entry = generateFailedLoginBurst(id, "10.9.9.9");
        } else {
            entry = generatePortScanEvent(id, "10.8.8.8", 80 + roll);
        }

        out << entry.toLogLine() << "\n";
    }

    out.close();
    std::cout << "Successfully generated " << num_lines << " logs to " << output_file << "\n";
    return 0;
}
