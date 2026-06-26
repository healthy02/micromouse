#pragma once

#include "Maze.h"
#include "Agent.h"
#include <chrono>
#include <memory>
#include <vector>
#include <string>

struct BenchmarkResult {
    int maze_id;
    AlgorithmType algorithm;
    bool success;
    int steps_to_goal;
    int cells_explored;
    int turn_count;
    double simulated_time;
    double wall_time_ms;
    int exploration_steps;
    int speed_run_steps;
    int exploration_cells;
};

class SimulationEngine {
public:
    explicit SimulationEngine(const std::string& maze_path);
    explicit SimulationEngine(Maze maze);
    ~SimulationEngine();

    void addAgent(int start_x, int start_y, int dest_x, int dest_y, bool smooth, AlgorithmType algo);
    void setAgentDisplayName(const std::string& name);
    
    bool tick();
    bool allAgentsDone() const;
    const Agent& getAgent(std::size_t index) const;
    int getTickCount() const { return tick_count; }
    const Maze& getMaze() const { return *real_maze; }
    
    void printInitState() const;
    void printRunInit(int run_index, int run_total, const std::string& algorithm_label) const;
    void printState();

    static BenchmarkResult runBenchmark(
        const Maze& maze,
        AlgorithmType algorithm,
        int start_x,
        int start_y,
        int dest_x,
        int dest_y,
        int maze_id,
        int max_ticks = 50000
    );

private:
    Maze* real_maze;
    bool owns_maze;
    std::vector<Agent> agents;
    int tick_count;
};
