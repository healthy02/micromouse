#pragma once

#include "Maze.h"
#include "Pathfinder.h"
#include <cstddef>
#include <set>
#include <tuple>
#include <vector>
#include <string>

enum class AgentMode { EXPLORATION, SPEED_RUN };
enum class AlgorithmType { TIME_A_STAR, FLOOD_FILL, HAND_ON_WALL };
enum class ExplorationTarget { DESTINATION, START };

class Agent {
public:
    Agent(int id, int start_x, int start_y, int dest_x, int dest_y, int maze_width, int maze_height, bool smooth_movement, AlgorithmType algo);

    int id;
    int x, y;
    int start_x, start_y;
    int dest_x, dest_y;
    double angle; 

    bool smooth_movement;
    AgentMode mode;
    AlgorithmType algorithm;
    Maze memory; 

    std::vector<Position> path;
    std::size_t path_index;

    int total_steps;
    int total_turns;
    double simulated_time;
    double max_speed;
    bool returned_to_start;
    bool reached_exploration_destination;
    ExplorationTarget exploration_target;
    std::vector<std::vector<int>> distance_grid;
    std::vector<std::vector<bool>> visited;

    int exploration_steps;
    int speed_run_steps;
    int exploration_turns;
    int speed_run_turns;
    double exploration_time;
    double speed_run_time;
    int cells_explored;
    int exploration_cells;
    std::set<std::tuple<int, int, int>> wall_follow_states;
    int wall_escape_steps;
    std::string display_name;

    void planFloodFillStep(int target_x, int target_y);
    bool wallFollowLoopDetected();
    void triggerWallEscape();
    void beginSpeedRun();
    void learnBlockedMove(const Maze& real_maze, int next_x, int next_y);

    // We track newly discovered cells in the current tick to send minimal JSON
    std::vector<Cell> newly_discovered;

    void discover(const Maze& real_maze);
    void updateMemory(int cx, int cy, bool n, bool e, bool s, bool w);
    void markVisited(int cx, int cy);
    void syncMemory(Agent& other);
    
    void planPath();
    void planExplorationPath();
    void updateDistanceGrid(int target_x, int target_y);
    bool moveNext(const Maze& real_maze); 
    bool reachedDestination() const;
    bool isDone() const;

    void setDisplayName(const std::string& name);
    std::string getAlgorithmName() const;
    int getStepsToGoal() const;
    static std::string algorithmTypeName(AlgorithmType algo);
};
