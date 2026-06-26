#include "../include/VisualSimRunner.h"
#include "../include/SimulationEngine.h"
#include "../include/Maze.h"
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <maze.num> [case]" << std::endl;
        std::cerr << "case 1: Same destination. case 2: Unique destinations." << std::endl;
        return 1;
    }

    try {
        SimulationEngine probe(argv[1]);
        const auto center = probe.getMaze().getCenter();
        const int max_x = probe.getMaze().getWidth() - 1;
        const int max_y = probe.getMaze().getHeight() - 1;

        int sim_case = 1;
        if (argc > 2) {
            sim_case = std::stoi(argv[2]);
        }

        std::vector<VisualRunSpec> runs;
        if (sim_case == 1) {
            runs.push_back({0, 0, center.first, center.second, false, AlgorithmType::TIME_A_STAR,
                            "Co-op mouse from (0,0)"});
            runs.push_back({max_x, 0, center.first, center.second, false, AlgorithmType::TIME_A_STAR,
                            "Co-op mouse from (" + std::to_string(max_x) + ",0)"});
        } else {
            runs.push_back({0, 0, center.first, center.second, false, AlgorithmType::TIME_A_STAR,
                            "Mouse A -> center"});
            runs.push_back({max_x, 0, 0, max_y, false, AlgorithmType::TIME_A_STAR,
                            "Mouse B -> opposite corner"});
        }

        return runVisualSimSequence(argv[1], runs);
    } catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << std::endl;
        return 1;
    }
}
