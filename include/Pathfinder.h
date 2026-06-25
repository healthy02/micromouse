#pragma once

#include "Maze.h"
#include <vector>
#include <tuple>

struct Position {
    int x, y;
    bool operator==(const Position& other) const { return x == other.x && y == other.y; }
    bool operator!=(const Position& other) const { return !(*this == other); }
    bool operator<(const Position& other) const {
        if (x != other.x) return x < other.x;
        return y < other.y;
    }
};

struct PathResult {
    std::vector<Position> path;
    double cost;
};

class Pathfinder {
public:
    static PathResult findPathTimeAStar(const Maze& maze, int start_x, int start_y, int dest_x, int dest_y, double start_angle, bool smooth, double max_speed);
    
    static PathResult findPathFloodFill(const Maze& maze, int start_x, int start_y, int dest_x, int dest_y);
    
    // Hand on wall returns the immediate next move only
    static PathResult findPathHandOnWall(const Maze& maze, int start_x, int start_y, double current_angle);
};
