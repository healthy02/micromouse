#include "../include/Pathfinder.h"
#include <queue>
#include <map>
#include <cmath>
#include <algorithm>
#include <iostream>

struct State {
    int x, y;
    double angle;
    double cost;
    
    bool operator>(const State& other) const { return cost > other.cost; }
    bool operator<(const State& other) const {
        if (x != other.x) return x < other.x;
        if (y != other.y) return y < other.y;
        return angle < other.angle;
    }
};

PathResult Pathfinder::findPathTimeAStar(const Maze& maze, int start_x, int start_y, int dest_x, int dest_y, double start_angle, bool smooth, double max_speed) {
    std::priority_queue<State, std::vector<State>, std::greater<State>> pq;
    std::map<State, double> min_cost;
    std::map<State, State> parent;
    
    State start_state = {start_x, start_y, start_angle, 0.0};
    pq.push(start_state);
    min_cost[start_state] = 0.0;
    
    State best_end_state = {-1, -1, 0.0, 1e9};
    
    while (!pq.empty()) {
        State current = pq.top();
        pq.pop();
        
        if (current.x == dest_x && current.y == dest_y) {
            if (current.cost < best_end_state.cost) {
                best_end_state = current;
            }
            continue;
        }
        
        if (current.cost > min_cost[current]) continue;
        
        std::vector<std::pair<int, int>> moves = {
            {0, 1}, {1, 0}, {0, -1}, {-1, 0}
        };
        if (smooth) {
            moves.push_back({1, 1});
            moves.push_back({1, -1});
            moves.push_back({-1, -1});
            moves.push_back({-1, 1});
        }
        
        for (auto m : moves) {
            int nx = current.x + m.first;
            int ny = current.y + m.second;
            
            if (!maze.isValid(nx, ny)) continue;
            
            bool blocked = false;
            if (m.first == 0 && m.second == 1 && maze.hasWall(current.x, current.y, 'n')) blocked = true;
            else if (m.first == 1 && m.second == 0 && maze.hasWall(current.x, current.y, 'e')) blocked = true;
            else if (m.first == 0 && m.second == -1 && maze.hasWall(current.x, current.y, 's')) blocked = true;
            else if (m.first == -1 && m.second == 0 && maze.hasWall(current.x, current.y, 'w')) blocked = true;
            else if (smooth && abs(m.first) == 1 && abs(m.second) == 1) {
                if (m.first == 1 && m.second == 1 && (maze.hasWall(current.x, current.y, 'n') || maze.hasWall(current.x, current.y, 'e'))) blocked = true;
                if (m.first == 1 && m.second == -1 && (maze.hasWall(current.x, current.y, 's') || maze.hasWall(current.x, current.y, 'e'))) blocked = true;
                if (m.first == -1 && m.second == -1 && (maze.hasWall(current.x, current.y, 's') || maze.hasWall(current.x, current.y, 'w'))) blocked = true;
                if (m.first == -1 && m.second == 1 && (maze.hasWall(current.x, current.y, 'n') || maze.hasWall(current.x, current.y, 'w'))) blocked = true;
            }
            
            if (blocked) continue;
            
            double target_angle = atan2(m.second, m.first) * 180.0 / M_PI;
            if (target_angle < 0) target_angle += 360.0;
            
            double angle_diff = abs(target_angle - current.angle);
            if (angle_diff > 180.0) angle_diff = 360.0 - angle_diff;
            
            double distance = sqrt(m.first * m.first + m.second * m.second);
            double move_cost = (distance / max_speed) + (angle_diff / 90.0) * 0.5; // 0.5s per 90 deg turn
            
            State next_state = {nx, ny, target_angle, current.cost + move_cost};
            State next_key = {nx, ny, target_angle, 0.0};
            
            if (min_cost.find(next_key) == min_cost.end() || next_state.cost < min_cost[next_key]) {
                min_cost[next_key] = next_state.cost;
                parent[next_key] = {current.x, current.y, current.angle, 0.0};
                pq.push(next_state);
            }
        }
    }
    
    PathResult res;
    res.cost = best_end_state.cost;
    if (res.cost < 1e9) {
        State curr = {best_end_state.x, best_end_state.y, best_end_state.angle, 0.0};
        while (curr.x != start_x || curr.y != start_y || abs(curr.angle - start_angle) > 1e-4) {
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
            
            bool blocked = false;
            if (m.first == 0 && m.second == 1 && maze.hasWall(curr.x, curr.y, 'n')) blocked = true;
            else if (m.first == 1 && m.second == 0 && maze.hasWall(curr.x, curr.y, 'e')) blocked = true;
            else if (m.first == 0 && m.second == -1 && maze.hasWall(curr.x, curr.y, 's')) blocked = true;
            else if (m.first == -1 && m.second == 0 && maze.hasWall(curr.x, curr.y, 'w')) blocked = true;
            
            if (blocked) continue;
            
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
        // Turn around (U-turn), effectively going back to where we came from if not blocked
        if (!maze.hasWall(start_x, start_y, back_w)) {
            res.path.push_back({start_x - dx, start_y - dy});
        }
    }
    
    return res;
}
