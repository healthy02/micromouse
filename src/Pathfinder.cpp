#include "../include/Pathfinder.h"
#include <queue>
#include <map>
#include <cmath>
#include <algorithm>
#include <iostream>

int snapAngle(double angle) {
    int snapped = static_cast<int>(std::round(angle / 90.0)) * 90;
    return ((snapped % 360) + 360) % 360;
}

struct State {
    int x, y;
    int angle;
    double g_cost;
    double f_cost;
    
    bool operator>(const State& other) const { return f_cost > other.f_cost; }
    bool operator<(const State& other) const {
        if (x != other.x) return x < other.x;
        if (y != other.y) return y < other.y;
        return angle < other.angle;
    }
};

namespace {
double timeHeuristic(int x, int y, int dest_x, int dest_y, bool smooth, double max_speed) {
    const double dx = static_cast<double>(dest_x - x);
    const double dy = static_cast<double>(dest_y - y);
    if (smooth) {
        return std::sqrt(dx * dx + dy * dy) / max_speed;
    }
    return (std::abs(dx) + std::abs(dy)) / max_speed;
}
}

PathResult Pathfinder::findPathTimeAStar(const Maze& maze, int start_x, int start_y, int dest_x, int dest_y, double start_angle, bool smooth, double max_speed) {
    std::priority_queue<State, std::vector<State>, std::greater<State>> pq;
    std::map<State, double> min_cost;
    std::map<State, State> parent;
    
    const double start_h = timeHeuristic(start_x, start_y, dest_x, dest_y, smooth, max_speed);
    State start_state = {start_x, start_y, snapAngle(start_angle), 0.0, start_h};
    pq.push(start_state);
    min_cost[start_state] = 0.0;
    
    State best_end_state = {-1, -1, 0, 1e9, 1e9};
    
    while (!pq.empty()) {
        State current = pq.top();
        pq.pop();
        
        if (current.x == dest_x && current.y == dest_y) {
            if (current.g_cost < best_end_state.g_cost) {
                best_end_state = current;
            }
            continue;
        }
        
        if (current.g_cost > min_cost[current]) continue;
        
        std::vector<std::pair<int, int>> moves = {
            {0, 1}, {1, 0}, {0, -1}, {-1, 0}
        };
        if (smooth) {
            moves.push_back({1, 1});
            moves.push_back({1, -1});
            moves.push_back({-1, -1});
            moves.push_back({-1, 1});
        }
        
        for (const auto& m : moves) {
            int nx = current.x + m.first;
            int ny = current.y + m.second;
            
            if (!maze.isValid(nx, ny)) continue;
            if (!maze.canMove(current.x, current.y, nx, ny, smooth)) continue;
            
            int target_angle = snapAngle(std::atan2(m.second, m.first) * 180.0 / M_PI);
            if (target_angle < 0) target_angle += 360.0;
            
            double angle_diff = std::abs(target_angle - current.angle);
            if (angle_diff > 180.0) angle_diff = 360.0 - angle_diff;
            
            double distance = std::sqrt(m.first * m.first + m.second * m.second);
            double move_cost = (distance / max_speed) + (angle_diff / 90.0) * 0.5;
            
            const double next_g = current.g_cost + move_cost;
            State next_key = {nx, ny, target_angle, 0.0, 0.0};
            
            if (min_cost.find(next_key) == min_cost.end() || next_g < min_cost[next_key]) {
                min_cost[next_key] = next_g;
                parent[next_key] = {current.x, current.y, current.angle, 0.0, 0.0};
                const double h = timeHeuristic(nx, ny, dest_x, dest_y, smooth, max_speed);
                pq.push({nx, ny, target_angle, next_g, next_g + h});
            }
        }
    }
    
    PathResult res;
    res.cost = best_end_state.g_cost;
    if (res.cost < 1e9) {
        State curr = {best_end_state.x, best_end_state.y, best_end_state.angle, 0.0, 0.0};
        while (curr.x != start_x || curr.y != start_y) {
            if (parent.find(curr) == parent.end()) break;  // safety guard
            res.path.push_back({curr.x, curr.y});
            curr = parent[curr];
        }
        res.path.push_back({start_x, start_y});
        std::reverse(res.path.begin(), res.path.end());
    }
    return res;
}

PathResult Pathfinder::findPathFloodFill(const Maze& maze, int start_x, int start_y, int dest_x, int dest_y) {
    std::queue<Position> q;
    std::map<Position, Position> parent;
    std::map<Position, int> dist;
    
    Position start = {start_x, start_y};
    q.push(start);
    dist[start] = 0;
    
    bool found = false;
    
    while (!q.empty()) {
        Position curr = q.front();
        q.pop();
        
        if (curr.x == dest_x && curr.y == dest_y) {
            found = true;
            break;
        }
        
        std::vector<std::pair<int, int>> moves = {{0,1}, {1,0}, {0,-1}, {-1,0}};
        for (auto m : moves) {
            int nx = curr.x + m.first;
            int ny = curr.y + m.second;
            
            if (!maze.isValid(nx, ny)) continue;
            if (!maze.canMove(curr.x, curr.y, nx, ny, false)) continue;
            
            Position next_pos = {nx, ny};
            if (dist.find(next_pos) == dist.end()) {
                dist[next_pos] = dist[curr] + 1;
                parent[next_pos] = curr;
                q.push(next_pos);
            }
        }
    }
    
    PathResult res;
    res.cost = found ? dist[{dest_x, dest_y}] : 1e9;
    if (found) {
        Position curr = {dest_x, dest_y};
        while (curr != start) {
            res.path.push_back(curr);
            curr = parent[curr];
        }
        res.path.push_back(start);
        std::reverse(res.path.begin(), res.path.end());
    }
    return res;
}

PathResult Pathfinder::findPathHandOnWall(const Maze& maze, int start_x, int start_y, double current_angle) {
    // Determine current direction
    int dx = 0, dy = 0;
    char front_w = '\0', right_w = '\0', left_w = '\0', back_w = '\0';
    PathResult res;
    
    int ang = (int(current_angle + 0.5)) % 360;
    
    if (ang == 0) { // East
        dx = 1; dy = 0;
        front_w = 'e'; right_w = 's'; left_w = 'n'; back_w = 'w';
    } else if (ang == 90) { // North
        dx = 0; dy = 1;
        front_w = 'n'; right_w = 'e'; left_w = 'w'; back_w = 's';
    } else if (ang == 180) { // West
        dx = -1; dy = 0;
        front_w = 'w'; right_w = 'n'; left_w = 's'; back_w = 'e';
    } else if (ang == 270) { // South
        dx = 0; dy = -1;
        front_w = 's'; right_w = 'w'; left_w = 'e'; back_w = 'n';
    } else {
        res.cost = 1e9;
        res.path.push_back({start_x, start_y});
        return res;
    }
    
    res.cost = 0;
    res.path.push_back({start_x, start_y}); // Current pos
    
    // Right wall logic:
    if (!maze.hasWall(start_x, start_y, right_w)) {
        // Turn right and move
        if (ang == 0) res.path.push_back({start_x, start_y - 1});
        else if (ang == 90) res.path.push_back({start_x + 1, start_y});
        else if (ang == 180) res.path.push_back({start_x, start_y + 1});
        else if (ang == 270) res.path.push_back({start_x - 1, start_y});
    } else if (!maze.hasWall(start_x, start_y, front_w)) {
        // Go straight
        res.path.push_back({start_x + dx, start_y + dy});
    } else if (!maze.hasWall(start_x, start_y, left_w)) {
        // Turn left and move
        if (ang == 0) res.path.push_back({start_x, start_y + 1});
        else if (ang == 90) res.path.push_back({start_x - 1, start_y});
        else if (ang == 180) res.path.push_back({start_x, start_y - 1});
        else if (ang == 270) res.path.push_back({start_x + 1, start_y});
    } else {
        if (!maze.hasWall(start_x, start_y, back_w)) {
            res.path.push_back({start_x - dx, start_y - dy});
        }
    }

    if (res.path.size() > 1) {
        const Position& next = res.path[1];
        if (!maze.canMove(start_x, start_y, next.x, next.y, false)) {
            res.path.resize(1);
        }
    }
    
    return res;
}
