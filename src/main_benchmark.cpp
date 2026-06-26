#include "../include/SimulationEngine.h"
#include "../include/Maze.h"
#include "../include/Agent.h"
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

namespace {
constexpr int kDefaultMazeCount = 100;
constexpr int kMazeSize = 16;
constexpr unsigned kSeedBase = 42;

struct AlgoStats {
    AlgorithmType algorithm;
    int runs = 0;
    int successes = 0;
    double steps_sum = 0.0;
    double cells_sum = 0.0;
    double turns_sum = 0.0;
    double time_sum = 0.0;
    double wall_time_sum = 0.0;
    double exploration_cells_sum = 0.0;
    double speed_run_steps_sum = 0.0;
};

double percentImprovement(double baseline, double value) {
    if (baseline <= 0.0) return 0.0;
    return ((baseline - value) / baseline) * 100.0;
}

double percentIncrease(double baseline, double value) {
    if (baseline <= 0.0) return 0.0;
    return ((value - baseline) / baseline) * 100.0;
}

void accumulate(AlgoStats& stats, const BenchmarkResult& result) {
    stats.runs++;
    if (!result.success) return;
    stats.successes++;
    stats.steps_sum += result.steps_to_goal;
    stats.cells_sum += result.cells_explored;
    stats.turns_sum += result.turn_count;
    stats.time_sum += result.simulated_time;
    stats.wall_time_sum += result.wall_time_ms;
    stats.exploration_cells_sum += result.exploration_cells;
    stats.speed_run_steps_sum += result.speed_run_steps;
}

double average(double sum, int count) {
    return count > 0 ? sum / count : 0.0;
}

std::string csvEscape(const std::string& value) {
    if (value.find(',') == std::string::npos && value.find('"') == std::string::npos) {
        return value;
    }
    std::string escaped = "\"";
    for (char ch : value) {
        if (ch == '"') escaped += "\"\"";
        else escaped += ch;
    }
    escaped += "\"";
    return escaped;
}
}

int main(int argc, char* argv[]) {
    int maze_count = kDefaultMazeCount;
    if (argc >= 2) {
        maze_count = std::max(1, std::atoi(argv[1]));
    }

    const std::vector<AlgorithmType> algorithms = {
        AlgorithmType::TIME_A_STAR,
        AlgorithmType::FLOOD_FILL,
        AlgorithmType::HAND_ON_WALL
    };

    std::vector<BenchmarkResult> all_results;
    all_results.reserve(static_cast<std::size_t>(maze_count * algorithms.size()));

    std::cout << "Generating " << maze_count << " random " << kMazeSize << "x" << kMazeSize
              << " mazes (seed base " << kSeedBase << ")..." << std::endl;

    for (int maze_id = 0; maze_id < maze_count; ++maze_id) {
        Maze maze = Maze::generate(kMazeSize, kMazeSize, kSeedBase + static_cast<unsigned>(maze_id));
        const auto center = maze.getCenter();

        for (AlgorithmType algorithm : algorithms) {
            BenchmarkResult result = SimulationEngine::runBenchmark(
                maze, algorithm, 0, 0, center.first, center.second, maze_id);
            all_results.push_back(result);
        }
    }

    std::cout << std::endl;
    std::cout << "=== CSV Results ===" << std::endl;
    std::cout << "maze_id,algorithm,success,steps_to_goal,cells_explored,exploration_cells,turn_count,simulated_time_s,wall_time_ms,exploration_steps,speed_run_steps"
              << std::endl;

    for (const BenchmarkResult& result : all_results) {
        std::cout << result.maze_id << ","
                  << csvEscape(Agent::algorithmTypeName(result.algorithm)) << ","
                  << (result.success ? "true" : "false") << ","
                  << result.steps_to_goal << ","
                  << result.cells_explored << ","
                  << result.exploration_cells << ","
                  << result.turn_count << ","
                  << std::fixed << std::setprecision(4) << result.simulated_time << ","
                  << std::setprecision(2) << result.wall_time_ms << ","
                  << result.exploration_steps << ","
                  << result.speed_run_steps << std::endl;
    }

    std::map<AlgorithmType, AlgoStats> summary;
    for (AlgorithmType algorithm : algorithms) {
        AlgoStats stats;
        stats.algorithm = algorithm;
        summary[algorithm] = stats;
    }
    for (const BenchmarkResult& result : all_results) {
        accumulate(summary[result.algorithm], result);
    }

    const AlgoStats& astar = summary[AlgorithmType::TIME_A_STAR];
    const AlgoStats& flood = summary[AlgorithmType::FLOOD_FILL];
    const AlgoStats& wall = summary[AlgorithmType::HAND_ON_WALL];

    const double astar_steps = average(astar.speed_run_steps_sum, astar.successes);
    const double flood_steps = average(flood.speed_run_steps_sum, flood.successes);
    const double wall_steps = average(wall.speed_run_steps_sum, wall.successes);

    const double astar_expl_cells = average(astar.exploration_cells_sum, astar.successes);
    const double flood_expl_cells = average(flood.exploration_cells_sum, flood.successes);
    const double wall_expl_cells = average(wall.exploration_cells_sum, wall.successes);

    const double astar_time = average(astar.time_sum, astar.successes);
    const double flood_time = average(flood.time_sum, flood.successes);
    const double wall_time = average(wall.time_sum, wall.successes);

    std::cout << std::endl;
    std::cout << "=== Benchmark Summary (" << maze_count << " mazes, "
              << kMazeSize << "x" << kMazeSize << ") ===" << std::endl;
    std::cout << std::fixed << std::setprecision(1);

    for (AlgorithmType algorithm : algorithms) {
        const AlgoStats& stats = summary[algorithm];
        std::cout << Agent::algorithmTypeName(algorithm)
                  << ": success " << stats.successes << "/" << stats.runs
                  << ", avg speed-run steps " << average(stats.speed_run_steps_sum, stats.successes)
                  << ", avg exploration cells " << average(stats.exploration_cells_sum, stats.successes)
                  << ", avg turns " << average(stats.turns_sum, stats.successes)
                  << ", avg simulated time " << average(stats.time_sum, stats.successes) << "s"
                  << ", avg compute " << average(stats.wall_time_sum, stats.successes) << "ms"
                  << std::endl;
    }

    std::cout << std::endl;
    if (astar.successes > 0 && wall.successes > 0) {
        std::cout << "- Time-Aware A* reached the goal in "
                  << percentImprovement(wall_steps, astar_steps)
                  << "% fewer speed-run steps than Wall Follower";
        if (wall_expl_cells > astar_expl_cells) {
            std::cout << " while exploring "
                      << percentIncrease(astar_expl_cells, wall_expl_cells)
                      << "% fewer cells during the discovery phase";
        } else {
            std::cout << " but explored "
                      << percentIncrease(wall_expl_cells, astar_expl_cells)
                      << "% more cells during the discovery phase";
        }
        std::cout << "." << std::endl;
    }
    if (astar.successes > 0 && flood.successes > 0) {
        std::cout << "- Time-Aware A* completed speed runs in "
                  << percentImprovement(flood_steps, astar_steps)
                  << "% fewer steps than Flood Fill and finished "
                  << percentImprovement(flood_time, astar_time)
                  << "% faster in simulated time (diagonal-aware routing with turn costs)."
                  << std::endl;
    }
    if (flood.successes > 0 && wall.successes > 0) {
        std::cout << "- Flood Fill took "
                  << percentImprovement(wall_steps, flood_steps)
                  << "% fewer speed-run steps than Wall Follower and explored "
                  << percentImprovement(wall_expl_cells, flood_expl_cells)
                  << "% fewer cells during discovery."
                  << std::endl;
        std::cout << "- Wall Follower used "
                  << percentIncrease(flood_time, wall_time)
                  << "% more simulated time end-to-end than Flood Fill across the full explore-and-sprint cycle."
                  << std::endl;
    }

    std::cout << std::endl;
    std::cout << "=== Aggregate Table ===" << std::endl;
    std::cout << std::left << std::setw(18) << "Algorithm"
              << std::setw(14) << "Speed Steps"
              << std::setw(16) << "Explored Cells"
              << std::setw(10) << "Turns"
              << std::setw(14) << "Sim Time (s)"
              << std::setw(14) << "Compute (ms)" << std::endl;

    for (AlgorithmType algorithm : algorithms) {
        const AlgoStats& stats = summary[algorithm];
        std::cout << std::setw(18) << Agent::algorithmTypeName(algorithm)
                  << std::setw(14) << average(stats.speed_run_steps_sum, stats.successes)
                  << std::setw(16) << average(stats.exploration_cells_sum, stats.successes)
                  << std::setw(10) << average(stats.turns_sum, stats.successes)
                  << std::setw(14) << average(stats.time_sum, stats.successes)
                  << std::setw(14) << average(stats.wall_time_sum, stats.successes)
                  << std::endl;
    }

    const int total_failures = static_cast<int>(all_results.size()) -
        (astar.successes + flood.successes + wall.successes);
    if (total_failures > 0) {
        std::cerr << std::endl << "Warning: " << total_failures
                  << " runs did not reach the goal within the tick limit." << std::endl;
        return 2;
    }

    return 0;
}
