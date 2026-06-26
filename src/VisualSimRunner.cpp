#include "../include/VisualSimRunner.h"
#include "../include/SimulationEngine.h"
#include <iostream>

int runVisualSimSequence(const std::string& maze_path, const std::vector<VisualRunSpec>& runs, int max_ticks) {
    if (runs.empty()) {
        std::cerr << "No simulation runs configured." << std::endl;
        return 1;
    }

    const std::size_t run_total = runs.size();
    for (std::size_t run_index = 0; run_index < run_total; ++run_index) {
        const VisualRunSpec& run = runs[run_index];
        const std::string label = run.label.empty()
            ? Agent::algorithmTypeName(run.algorithm)
            : run.label;

        SimulationEngine engine(maze_path);
        engine.printRunInit(static_cast<int>(run_index), static_cast<int>(run_total), label);
        engine.addAgent(run.start_x, run.start_y, run.dest_x, run.dest_y, run.smooth, run.algorithm);
        if (!run.label.empty()) {
            engine.setAgentDisplayName(run.label);
        }

        int tick = 0;
        while (tick < max_ticks && engine.tick()) {
            engine.printState();
            std::cout << std::flush;
            tick++;
        }

        engine.printState();
        std::cout << "{\"type\": \"segment_done\", \"algorithm\": \"" << label << "\"}" << std::endl;
    }

    std::cout << "{\"type\": \"done\"}" << std::endl;
    return 0;
}
