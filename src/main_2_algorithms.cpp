#include "../include/VisualSimRunner.h"
#include "../include/SimulationEngine.h"
#include "../include/Maze.h"
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <maze.num>" << std::endl;
        return 1;
    }

    try {
        SimulationEngine probe(argv[1]);
        const auto center = probe.getMaze().getCenter();

        std::vector<VisualRunSpec> runs;
        runs.push_back({0, 0, center.first, center.second, false, AlgorithmType::FLOOD_FILL, "Flood Fill"});
        runs.push_back({0, 0, center.first, center.second, false, AlgorithmType::HAND_ON_WALL, "Wall Follower"});
        runs.push_back({0, 0, center.first, center.second, false, AlgorithmType::TIME_A_STAR, "Time-Aware A*"});

        return runVisualSimSequence(argv[1], runs);
    } catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << std::endl;
        return 1;
    }
}
