#include "../include/SimulationEngine.h"
#include <iostream>
#include <stdexcept>

SimulationEngine::SimulationEngine(const std::string& maze_path)
    : real_maze(Maze::fromNumFile(maze_path)), owns_maze(true), tick_count(0) {
    if (!real_maze) {
        throw std::runtime_error("Unable to load maze file: " + maze_path);
    }
}

SimulationEngine::SimulationEngine(Maze maze)
    : real_maze(new Maze(std::move(maze))), owns_maze(true), tick_count(0) {}

SimulationEngine::~SimulationEngine() {
    if (owns_maze) {
        delete real_maze;
    }
}

void SimulationEngine::addAgent(int start_x, int start_y, int dest_x, int dest_y, bool smooth, AlgorithmType algo) {
    if (!real_maze) {
        throw std::runtime_error("Cannot add an agent without a maze");
    }
    if (!real_maze->isValid(start_x, start_y) || !real_maze->isValid(dest_x, dest_y)) {
        throw std::runtime_error("Agent start or destination is outside the maze");
    }
    int id = static_cast<int>(agents.size());
    agents.emplace_back(id, start_x, start_y, dest_x, dest_y, real_maze->getWidth(), real_maze->getHeight(), smooth, algo);
    agents.back().discover(*real_maze);
}

void SimulationEngine::setAgentDisplayName(const std::string& name) {
    if (!agents.empty()) {
        agents.back().setDisplayName(name);
    }
}

bool SimulationEngine::tick() {
    if (!real_maze) return false;
    bool any_active = false;
    
    if (agents.size() > 1) {
        for (size_t i = 0; i < agents.size(); ++i) {
            for (size_t j = 0; j < agents.size(); ++j) {
                if (i != j) {
                    agents[i].syncMemory(agents[j]);
                }
            }
        }
    }
    
    for (auto& agent : agents) {
        agent.newly_discovered.clear();

        if (agent.isDone()) {
            continue;
        }
        
        any_active = true;
        
        if (agent.path_index >= agent.path.size()) {
            agent.planPath();
        }
        
        bool moved = agent.moveNext(*real_maze);
        if (!moved) {
            agent.planPath();
        }
        
        if (agent.mode == AgentMode::EXPLORATION && agent.returned_to_start) {
            agent.beginSpeedRun();
            agent.mode = AgentMode::SPEED_RUN;
            agent.path.clear();
            agent.path_index = 0;
            agent.planPath();
        }
    }
    
    tick_count++;
    return any_active;
}

bool SimulationEngine::allAgentsDone() const {
    if (agents.empty()) return true;
    for (const auto& agent : agents) {
        if (!agent.isDone()) return false;
    }
    return true;
}

const Agent& SimulationEngine::getAgent(std::size_t index) const {
    return agents.at(index);
}

BenchmarkResult SimulationEngine::runBenchmark(
    const Maze& maze,
    AlgorithmType algorithm,
    int start_x,
    int start_y,
    int dest_x,
    int dest_y,
    int maze_id,
    int max_ticks) {

    const auto start_clock = std::chrono::steady_clock::now();

    SimulationEngine engine(maze);
    engine.addAgent(start_x, start_y, dest_x, dest_y, algorithm == AlgorithmType::TIME_A_STAR, algorithm);

    int ticks = 0;
    while (ticks < max_ticks && engine.tick()) {
        ++ticks;
    }

    const auto end_clock = std::chrono::steady_clock::now();
    const double wall_time_ms = std::chrono::duration<double, std::milli>(end_clock - start_clock).count();

    const Agent& agent = engine.getAgent(0);
    BenchmarkResult result;
    result.maze_id = maze_id;
    result.algorithm = algorithm;
    result.success = agent.isDone();
    result.steps_to_goal = agent.getStepsToGoal();
    result.cells_explored = agent.cells_explored;
    result.turn_count = agent.total_turns;
    result.simulated_time = agent.simulated_time;
    result.wall_time_ms = wall_time_ms;
    result.exploration_steps = agent.exploration_steps;
    result.speed_run_steps = agent.speed_run_steps;
    result.exploration_cells = agent.exploration_cells > 0 ? agent.exploration_cells : agent.cells_explored;
    return result;
}

void SimulationEngine::printInitState() const {
    printRunInit(0, 1, agents.empty() ? "Micromouse" : agents[0].getAlgorithmName());
}

void SimulationEngine::printRunInit(int run_index, int run_total, const std::string& algorithm_label) const {
    if (!real_maze) return;
    std::cout << "{\"type\": \"init\", \"run_index\": " << run_index
              << ", \"run_total\": " << run_total
              << ", \"algorithm\": \"" << algorithm_label << "\""
              << ", \"maze_w\": " << real_maze->getWidth()
              << ", \"maze_h\": " << real_maze->getHeight() << "}" << std::endl;
}

void SimulationEngine::printState() {
    if (!real_maze) return;
    std::cout << "{\"type\": \"state\", \"tick\": " << tick_count << ", \"maze_w\": " << real_maze->getWidth() << ", \"maze_h\": " << real_maze->getHeight() << ", \"agents\": [";
    for (size_t i = 0; i < agents.size(); ++i) {
        const auto& a = agents[i];
        std::cout << "{\"id\": " << a.id << ", \"x\": " << a.x << ", \"y\": " << a.y;
        std::cout << ", \"dest_x\": " << a.dest_x << ", \"dest_y\": " << a.dest_y;
        int target_x = a.mode == AgentMode::EXPLORATION && a.exploration_target == ExplorationTarget::START ? a.start_x : a.dest_x;
        int target_y = a.mode == AgentMode::EXPLORATION && a.exploration_target == ExplorationTarget::START ? a.start_y : a.dest_y;
        std::cout << ", \"target_x\": " << target_x << ", \"target_y\": " << target_y;
        std::cout << ", \"angle\": " << a.angle;
        std::cout << ", \"mode\": " << (a.mode == AgentMode::EXPLORATION ? "\"EXPLORATION\"" : "\"SPEED_RUN\"");
        std::cout << ", \"exploration_target\": " << (a.exploration_target == ExplorationTarget::DESTINATION ? "\"DESTINATION\"" : "\"START\"");
        std::cout << ", \"algorithm\": \"" << a.getAlgorithmName() << "\"";
        std::cout << ", \"total_steps\": " << a.total_steps;
        std::cout << ", \"total_turns\": " << a.total_turns;
        std::cout << ", \"simulated_time\": " << a.simulated_time;
        std::cout << ", \"cells_explored\": " << a.cells_explored;
        
        std::cout << ", \"discovered_cells\": [";
        for (size_t j = 0; j < a.newly_discovered.size(); ++j) {
            const auto& c = a.newly_discovered[j];
            std::cout << "{\"x\":" << c.x << ",\"y\":" << c.y 
                      << ",\"n\":" << (c.n ? 1 : 0) 
                      << ",\"e\":" << (c.e ? 1 : 0) 
                      << ",\"s\":" << (c.s ? 1 : 0) 
                      << ",\"w\":" << (c.w ? 1 : 0) << "}";
            if (j < a.newly_discovered.size() - 1) std::cout << ", ";
        }
        std::cout << "]";

        std::cout << ", \"distance_grid\": [";
        for (int y = 0; y < real_maze->getHeight(); ++y) {
            if (y > 0) std::cout << ", ";
            std::cout << "[";
            for (int x = 0; x < real_maze->getWidth(); ++x) {
                if (x > 0) std::cout << ", ";
                int value = a.distance_grid[x][y];
                if (value >= 1000000000) {
                    std::cout << "null";
                } else {
                    std::cout << value;
                }
            }
            std::cout << "]";
        }
        std::cout << "]";

        std::cout << "}";
        if (i < agents.size() - 1) std::cout << ", ";
    }
    std::cout << "]}" << std::endl;
}
