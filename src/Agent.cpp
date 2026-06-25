#include "../include/Agent.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <queue>

namespace {
const int INF_DISTANCE = 1000000000;

char directionForDelta(int dx, int dy) {
    if (dx == 0 && dy == 1) return 'n';
    if (dx == 1 && dy == 0) return 'e';
    if (dx == 0 && dy == -1) return 's';
    if (dx == -1 && dy == 0) return 'w';
    return '?';
}

bool isOrthogonalNeighbor(int dx, int dy) {
    return std::abs(dx) + std::abs(dy) == 1;
}
}

Agent::Agent(int id, int start_x, int start_y, int dest_x, int dest_y, int maze_width, int maze_height, bool smooth_movement, AlgorithmType algo)
    : id(id), x(start_x), y(start_y), start_x(start_x), start_y(start_y), dest_x(dest_x), dest_y(dest_y), 
      angle(90.0), smooth_movement(smooth_movement), mode(AgentMode::EXPLORATION), algorithm(algo),
      memory(maze_width, maze_height), path_index(0), total_steps(0), total_turns(0), simulated_time(0.0), max_speed(2.0),
      returned_to_start(false), reached_exploration_destination(false), exploration_target(ExplorationTarget::DESTINATION),
      distance_grid(maze_width, std::vector<int>(maze_height, INF_DISTANCE)) {
    updateDistanceGrid(dest_x, dest_y);
}

std::string Agent::getAlgorithmName() const {
    if (algorithm == AlgorithmType::TIME_A_STAR) return "Time-Aware A*";
    if (algorithm == AlgorithmType::FLOOD_FILL) return "Flood Fill";
    if (algorithm == AlgorithmType::HAND_ON_WALL) return "Hand on Wall";
    return "Unknown";
}

void Agent::discover(const Maze& real_maze) {
    const Cell& c = real_maze.getCell(x, y);
    updateMemory(x, y, c.n, c.e, c.s, c.w);
    
    // We can just send the currently occupied cell. 
    // In a real micromouse, it sees 1 cell ahead usually, but let's keep it simple: it discovers the cell it stands on.
    newly_discovered.push_back({x, y, c.n, c.e, c.s, c.w});
}

void Agent::learnMaze(const Maze& real_maze) {
    for (int cx = 0; cx < real_maze.getWidth(); ++cx) {
        for (int cy = 0; cy < real_maze.getHeight(); ++cy) {
            const Cell& c = real_maze.getCell(cx, cy);
            updateMemory(cx, cy, c.n, c.e, c.s, c.w);
            newly_discovered.push_back({cx, cy, c.n, c.e, c.s, c.w});
        }
    }
}

void Agent::updateMemory(int cx, int cy, bool n, bool e, bool s, bool w) {
    memory.setCell(cx, cy, n, e, s, w);
}

void Agent::syncMemory(Agent& other) {
    for (int cx = 0; cx < memory.getWidth(); ++cx) {
        for (int cy = 0; cy < memory.getHeight(); ++cy) {
            const Cell& other_c = other.memory.getCell(cx, cy);
            if (other_c.n || other_c.e || other_c.s || other_c.w) { 
                memory.setCell(cx, cy, other_c.n, other_c.e, other_c.s, other_c.w);
            }
        }
    }
}

void Agent::planPath() {
    if (mode == AgentMode::EXPLORATION) {
        planExplorationPath();
        return;
    }

    int target_x = dest_x;
    int target_y = dest_y;
    exploration_target = ExplorationTarget::DESTINATION;
    updateDistanceGrid(target_x, target_y);
    
    PathResult res;
    if (algorithm == AlgorithmType::TIME_A_STAR) {
        res = Pathfinder::findPathTimeAStar(memory, x, y, target_x, target_y, angle, smooth_movement, max_speed);
    } else if (algorithm == AlgorithmType::FLOOD_FILL) {
        res = Pathfinder::findPathFloodFill(memory, x, y, target_x, target_y);
    } else if (algorithm == AlgorithmType::HAND_ON_WALL) {
        res = Pathfinder::findPathFloodFill(memory, x, y, target_x, target_y);
    }

    path = res.path;
    path_index = 0;
    if (path.size() > 0 && path[0].x == x && path[0].y == y) {
        path_index = 1;
    }
}

void Agent::planExplorationPath() {
    const int target_x = exploration_target == ExplorationTarget::DESTINATION ? dest_x : start_x;
    const int target_y = exploration_target == ExplorationTarget::DESTINATION ? dest_y : start_y;
    updateDistanceGrid(target_x, target_y);

    path.clear();
    path.push_back({x, y});
    path_index = 1;

    int best_distance = distance_grid[x][y];
    Position best = {x, y};
    double best_turn = std::numeric_limits<double>::max();
    bool found_better = false;
    const std::vector<std::pair<int, int>> moves = {{0, 1}, {1, 0}, {0, -1}, {-1, 0}};

    for (auto move : moves) {
        int nx = x + move.first;
        int ny = y + move.second;
        if (!memory.isValid(nx, ny)) continue;
        if (memory.hasWall(x, y, directionForDelta(move.first, move.second))) continue;

        double target_angle = std::atan2(move.second, move.first) * 180.0 / M_PI;
        if (target_angle < 0) target_angle += 360.0;
        double turn = std::abs(target_angle - angle);
        if (turn > 180.0) turn = 360.0 - turn;

        int candidate_distance = distance_grid[nx][ny];
        if (candidate_distance < best_distance ||
            (found_better && candidate_distance == best_distance && turn < best_turn)) {
            best_distance = candidate_distance;
            best = {nx, ny};
            best_turn = turn;
            found_better = true;
        }
    }

    if (best.x != x || best.y != y) {
        path.push_back(best);
    }
}

void Agent::updateDistanceGrid(int target_x, int target_y) {
    distance_grid.assign(memory.getWidth(), std::vector<int>(memory.getHeight(), INF_DISTANCE));
    if (!memory.isValid(target_x, target_y)) return;

    std::queue<Position> q;
    distance_grid[target_x][target_y] = 0;
    q.push({target_x, target_y});

    const std::vector<std::pair<int, int>> moves = {{0, 1}, {1, 0}, {0, -1}, {-1, 0}};
    while (!q.empty()) {
        Position curr = q.front();
        q.pop();

        for (auto move : moves) {
            int nx = curr.x + move.first;
            int ny = curr.y + move.second;
            if (!memory.isValid(nx, ny)) continue;

            // Check the wall from the neighbor back into the current cell.
            if (memory.hasWall(nx, ny, directionForDelta(-move.first, -move.second))) continue;

            if (distance_grid[nx][ny] > distance_grid[curr.x][curr.y] + 1) {
                distance_grid[nx][ny] = distance_grid[curr.x][curr.y] + 1;
                q.push({nx, ny});
            }
        }
    }
}

bool Agent::moveNext(const Maze& real_maze) {
    if (path_index >= path.size()) return false;
    
    Position next_pos = path[path_index];
    
    int dx = next_pos.x - x;
    int dy = next_pos.y - y;
    if (!real_maze.isValid(next_pos.x, next_pos.y)) {
        discover(real_maze);
        return false;
    }
    
    bool blocked = false;
    if (!isOrthogonalNeighbor(dx, dy) && !(smooth_movement && abs(dx) == 1 && abs(dy) == 1)) blocked = true;
    else if (dx == 0 && dy == 1 && real_maze.hasWall(x, y, 'n')) blocked = true;
    else if (dx == 1 && dy == 0 && real_maze.hasWall(x, y, 'e')) blocked = true;
    else if (dx == 0 && dy == -1 && real_maze.hasWall(x, y, 's')) blocked = true;
    else if (dx == -1 && dy == 0 && real_maze.hasWall(x, y, 'w')) blocked = true;
    else if (smooth_movement && abs(dx) == 1 && abs(dy) == 1) {
        if (dx == 1 && dy == 1 && (real_maze.hasWall(x, y, 'n') || real_maze.hasWall(x, y, 'e'))) blocked = true;
        if (dx == 1 && dy == -1 && (real_maze.hasWall(x, y, 's') || real_maze.hasWall(x, y, 'e'))) blocked = true;
        if (dx == -1 && dy == -1 && (real_maze.hasWall(x, y, 's') || real_maze.hasWall(x, y, 'w'))) blocked = true;
        if (dx == -1 && dy == 1 && (real_maze.hasWall(x, y, 'n') || real_maze.hasWall(x, y, 'w'))) blocked = true;
    }
    
    if (blocked) {
        discover(real_maze); 
        return false; 
    }
    
    double target_angle = atan2(dy, dx) * 180.0 / M_PI;
    if (target_angle < 0) target_angle += 360.0;
    
    double angle_diff = abs(target_angle - angle);
    if (angle_diff > 180.0) angle_diff = 360.0 - angle_diff;
    
    if (angle_diff > 1e-4) total_turns++;
    total_steps++;
    
    double distance = sqrt(dx * dx + dy * dy);
    // Include acceleration/deceleration constraints (simple heuristic)
    // Turning costs 0.5s per 90 degrees.
    // If moving straight after a turn, there's an acceleration penalty (e.g. +0.2s).
    double turn_penalty = (angle_diff / 90.0) * 0.5;
    double accel_penalty = (angle_diff > 1e-4) ? 0.2 : 0.0;
    
    simulated_time += (distance / max_speed) + turn_penalty + accel_penalty;
    
    x = next_pos.x;
    y = next_pos.y;
    angle = target_angle;
    
    path_index++;
    
    discover(real_maze);
    
    if (mode == AgentMode::EXPLORATION && !reached_exploration_destination && reachedDestination()) {
        reached_exploration_destination = true;
        exploration_target = ExplorationTarget::START;
        path.clear();
        path_index = 0;
    } else if (mode == AgentMode::EXPLORATION && reached_exploration_destination && x == start_x && y == start_y) {
        returned_to_start = true;
    }
    
    return true;
}

bool Agent::reachedDestination() const {
    return x == dest_x && y == dest_y;
}

bool Agent::isDone() const {
    return mode == AgentMode::SPEED_RUN && reachedDestination();
}
