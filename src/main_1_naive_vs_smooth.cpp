#include "../include/SimulationEngine.h"
#include <iostream>
#include <stdexcept>
#include <string>

int main(int argc, char* argv[]) {
    
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <maze.num>" << std::endl;
        return 1;
    }
    
    std::string maze_path = argv[1];
    try {
        SimulationEngine engine(maze_path);
        
        // Mouse 0: Naive (Orthogonal only) - Time Aware A*
        engine.addAgent(0, 0, 7, 7, false, AlgorithmType::TIME_A_STAR);
        
        // Mouse 1: Smooth (Allows diagonal) - Time Aware A*
        engine.addAgent(0, 0, 7, 7, true, AlgorithmType::TIME_A_STAR);
        
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
