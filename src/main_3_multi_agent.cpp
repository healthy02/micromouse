#include "../include/SimulationEngine.h"
#include <iostream>
#include <stdexcept>
#include <string>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <maze.num> [case]" << std::endl;
        std::cerr << "case 1: Same destination. case 2: Unique destinations." << std::endl;
        return 1;
    }
    
    std::string maze_path = argv[1];
    try {
        SimulationEngine engine(maze_path);
        
        int sim_case = 1;
        if (argc > 2) {
            sim_case = std::stoi(argv[2]);
        }

        if (sim_case == 1) {
            // Case A: Same destination
            engine.addAgent(0, 0, 7, 7, false, AlgorithmType::TIME_A_STAR);
            engine.addAgent(15, 0, 7, 7, false, AlgorithmType::TIME_A_STAR);
        } else {
            // Case B: Unique destinations. Both return to their entrance before speed run.
            engine.addAgent(0, 0, 7, 7, false, AlgorithmType::TIME_A_STAR);
            engine.addAgent(15, 0, 0, 15, false, AlgorithmType::TIME_A_STAR);
        }
        
        engine.printInitState();
        
        int max_ticks = 10000;
        int tick = 0;
        while (tick < max_ticks && engine.tick()) {
            engine.printState();
            std::cout << std::flush;
            tick++;
        }
        
        engine.printState();
        std::cout << "{\"type\": \"done\"}" << std::endl;
    } catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << std::endl;
        return 1;
    }
    
    return 0;
}
