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

double snapToCardinalAngle(double angle) {
    int snapped = static_cast<int>(std::round(angle / 90.0)) * 90;
    snapped = ((snapped % 360) + 360) % 360;
    return static_cast<double>(snapped);
}
}

Agent::Agent(int id, int start_x, int start_y, int dest_x, int dest_y, int maze_width, int maze_height, bool smooth_movement, AlgorithmType algo)
    : id(id), x(start_x), y(start_y), start_x(start_x), start_y(start_y), dest_x(dest_x), dest_y(dest_y), 
      angle(90.0), smooth_movement(smooth_movement), mode(AgentMode::EXPLORATION), algorithm(algo),
      memory(maze_width, maze_height), path_index(0), total_steps(0), total_turns(0), simulated_time(0.0), max_speed(2.0),
      returned_to_start(false), reached_exploration_destination(false), exploration_target(ExplorationTarget::DESTINATION),
      distance_grid(maze_width, std::vector<int>(maze_height, INF_DISTANCE)),
      visited(maze_width, std::vector<bool>(maze_height, false)),
      exploration_steps(0), speed_run_steps(0), exploration_turns(0), speed_run_turns(0),
      exploration_time(0.0), speed_run_time(0.0), cells_explored(0), exploration_cells(0),
      wall_escape_steps(0) {
    updateDistanceGrid(dest_x, dest_y);
}

std::string Agent::algorithmTypeName(AlgorithmType algo) {
    if (algo == AlgorithmType::TIME_A_STAR) return "Time-Aware A*";
    if (algo == AlgorithmType::FLOOD_FILL) return "Flood Fill";
    if (algo == AlgorithmType::HAND_ON_WALL) return "Wall Follower";
    return "Unknown";
}

std::string Agent::getAlgorithmName() const {
    if (!display_name.empty()) return display_name;
    return algorithmTypeName(algorithm);
}

void Agent::setDisplayName(const std::string& name) {
    display_name = name;
}

int Agent::getStepsToGoal() const {
    return speed_run_steps > 0 ? speed_run_steps : total_steps;
}

void Agent::markVisited(int cx, int cy) {
    if (!memory.isValid(cx, cy) || visited[cx][cy]) return;
    visited[cx][cy] = true;
    cells_explored++;
}

void Agent::discover(const Maze& real_maze) {
    const Cell& c = real_maze.getCell(x, y);
    updateMemory(x, y, c.n, c.e, c.s, c.w);
    markVisited(x, y);
    newly_discovered.push_back(Cell(x, y, c.n, c.e, c.s, c.w, true));
}

void Agent::updateMemory(int cx, int cy, bool n, bool e, bool s, bool w) {
    memory.setCell(cx, cy, n, e, s, w);
}

void Agent::syncMemory(Agent& other) {
    for (int cx = 0; cx < memory.getWidth(); ++cx) {
        for (int cy = 0; cy < memory.getHeight(); ++cy) {
            const Cell& other_c = other.memory.getCell(cx, cy);
            if (other_c.known) {
                memory.setCell(cx, cy, other_c.n, other_c.e, other_c.s, other_c.w);
            }
            if (other.visited[cx][cy]) {
                markVisited(cx, cy);
            }
        }
    }
}

void Agent::planPath() {
    if (mode == AgentMode::EXPLORATION) {
        planExplorationPath();
        return;
    }

    const int target_x = dest_x;
    const int target_y = dest_y;
    exploration_target = ExplorationTarget::DESTINATION;
    updateDistanceGrid(target_x, target_y);

    if (algorithm == AlgorithmType::HAND_ON_WALL) {
        if (wall_escape_steps > 0) {
            wall_escape_steps--;
            planFloodFillStep(target_x, target_y);
            return;
        }
        if (wallFollowLoopDetected()) {
            triggerWallEscape();
            planFloodFillStep(target_x, target_y);
            return;
        }

        PathResult res = Pathfinder::findPathHandOnWall(memory, x, y, snapToCardinalAngle(angle));
        path = res.path;
        path_index = 0;
        if (!path.empty() && path[0].x == x && path[0].y == y) {
            path_index = 1;
        }
        return;
    }

    PathResult res;
    if (algorithm == AlgorithmType::TIME_A_STAR) {
        res = Pathfinder::findPathTimeAStar(memory, x, y, target_x, target_y, angle, smooth_movement, max_speed);
        if (res.path.size() < 2 || res.cost >= 1e9) {
            res = Pathfinder::findPathFloodFill(memory, x, y, target_x, target_y);
        }
    } else {
        res = Pathfinder::findPathFloodFill(memory, x, y, target_x, target_y);
    }

    path = res.path;
    path_index = 0;
    if (!path.empty() && path[0].x == x && path[0].y == y) {
        path_index = 1;
    }
}

void Agent::planExplorationPath() {
    const int target_x = exploration_target == ExplorationTarget::DESTINATION ? dest_x : start_x;
    const int target_y = exploration_target == ExplorationTarget::DESTINATION ? dest_y : start_y;

    if (algorithm == AlgorithmType::HAND_ON_WALL) {
        if (wall_escape_steps > 0) {
            wall_escape_steps--;
            planFloodFillStep(target_x, target_y);
            return;
        }
        if (wallFollowLoopDetected()) {
            triggerWallEscape();
            planFloodFillStep(target_x, target_y);
            return;
        }

        PathResult res = Pathfinder::findPathHandOnWall(memory, x, y, snapToCardinalAngle(angle));
        path = res.path;
        path_index = 0;
        if (!path.empty() && path[0].x == x && path[0].y == y) {
            path_index = 1;
        }
        return;
    }
    

    planFloodFillStep(target_x, target_y);
}

void Agent::planFloodFillStep(int target_x, int target_y) {
    updateDistanceGrid(target_x, target_y);

    path.clear();
    path.push_back({x, y});
    path_index = 1;

    int best_distance = distance_grid[x][y];
    Position best = {x, y};
    double best_turn = std::numeric_limits<double>::max();
    bool found_better = false;
    const std::vector<std::pair<int, int>> moves = {{0, 1}, {1, 0}, {0, -1}, {-1, 0}};

    for (const auto& move : moves) {
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
    } else {
        for (const auto& move : moves) {
            int nx = x + move.first;
            int ny = y + move.second;
            if (!memory.isValid(nx, ny)) continue;
            if (memory.hasWall(x, y, directionForDelta(move.first, move.second))) continue;
            path.push_back({nx, ny});
            break;
        }
    }
}

bool Agent::wallFollowLoopDetected() {
    const int ang = static_cast<int>(snapToCardinalAngle(angle));
    const auto key = std::make_tuple(x, y, ang);
    if (wall_follow_states.find(key) != wall_follow_states.end()) {
        return true;
    }
    wall_follow_states.insert(key);
    return false;
}

void Agent::learnBlockedMove(const Maze& real_maze, int next_x, int next_y) {
    const Cell& current = real_maze.getCell(x, y);
    updateMemory(x, y, current.n, current.e, current.s, current.w);

    if (real_maze.isValid(next_x, next_y)) {
        const Cell& next = real_maze.getCell(next_x, next_y);
        updateMemory(next_x, next_y, next.n, next.e, next.s, next.w);
    }
}

void Agent::triggerWallEscape() {
    wall_follow_states.clear();
    wall_escape_steps = 64;
}

void Agent::beginSpeedRun() {
    wall_follow_states.clear();
    wall_escape_steps = 0;
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

        for (const auto& move : moves) {
            int nx = curr.x + move.first;
            int ny = curr.y + move.second;
            if (!memory.isValid(nx, ny)) continue;
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
    if (next_pos.x == x && next_pos.y == y) {
        path_index++;
        return false;
    }

    if (!real_maze.isValid(next_pos.x, next_pos.y)) {
        discover(real_maze);
        return false;
    }

    const int dx = next_pos.x - x;
    const int dy = next_pos.y - y;
    if (std::abs(dx) > 1 || std::abs(dy) > 1 || (dx == 0 && dy == 0)) {
        learnBlockedMove(real_maze, next_pos.x, next_pos.y);
        path_index = path.size();
        return false;
    }

    if (!real_maze.canMove(x, y, next_pos.x, next_pos.y, smooth_movement)) {
        learnBlockedMove(real_maze, next_pos.x, next_pos.y);
        discover(real_maze);
        return false;
    }
    
    double target_angle = std::atan2(dy, dx) * 180.0 / M_PI;
    if (target_angle < 0) target_angle += 360.0;
    
    double angle_diff = std::abs(target_angle - angle);
    if (angle_diff > 180.0) angle_diff = 360.0 - angle_diff;
    
    if (angle_diff > 1e-4) total_turns++;
    
    total_steps++;
    
    double distance = std::sqrt(dx * dx + dy * dy);
    double turn_penalty = (angle_diff / 90.0) * 0.5;
    double accel_penalty = (angle_diff > 1e-4) ? 0.2 : 0.0;
    const double move_time = (distance / max_speed) + turn_penalty + accel_penalty;
    
    simulated_time += move_time;

    if (mode == AgentMode::EXPLORATION) {
        exploration_steps++;
        if (angle_diff > 1e-4) exploration_turns++;
        exploration_time = simulated_time;
    } else {
        speed_run_steps++;
        if (angle_diff > 1e-4) speed_run_turns++;
        speed_run_time += move_time;
    }
    
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
        exploration_cells = cells_explored;
    }
    
    return true;
}

bool Agent::reachedDestination() const {
    return x == dest_x && y == dest_y;
}

bool Agent::isDone() const {
    return mode == AgentMode::SPEED_RUN && reachedDestination();
}
