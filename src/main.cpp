#include "../include/SimulationEngine.h"
#include <iostream>
#include <stdexcept>
#include <string>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <maze.num>" << std::endl;
        return 1;
    }
    
    std::string maze_path = argv[1];
    try {
        SimulationEngine engine(maze_path);
        
        // 2 Mice, start at (0,0), destination (7,7)
        // Mouse 1: Naive (Orthogonal only)
        engine.addAgent(0, 0, 7, 7, false, AlgorithmType::TIME_A_STAR);
        
        // Mouse 2: Smooth (Allows diagonal)
        engine.addAgent(0, 0, 7, 7, true, AlgorithmType::TIME_A_STAR);
        
        engine.printInitState();
        
        int max_ticks = 10000;
        int tick = 0;
        while (tick < max_ticks && engine.tick()) {
            engine.printState();
            std::cout << std::flush;
            tick++;
        }
        
        // Final state
        engine.printState();
        std::cout << "{\"type\": \"done\"}" << std::endl;
    } catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << std::endl;
        return 1;
    }
    
    return 0;
}
