#pragma once

#include "Maze.h"
#include "Agent.h"
#include <vector>
#include <string>

class SimulationEngine {
public:
    SimulationEngine(const std::string& maze_path);
    ~SimulationEngine();

    void addAgent(int start_x, int start_y, int dest_x, int dest_y, bool smooth, AlgorithmType algo);
    
    // Runs one simulation tick for all agents. Returns false if all agents have finished speed run.
    bool tick(); 
    
    // Output states
    void printInitState() const;
    void printState();

private:
    Maze* real_maze;
    std::vector<Agent> agents;
    int tick_count;
};
