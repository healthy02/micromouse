#include "../include/Maze.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>

namespace {
bool hasWallValue(int value) {
    return value == 0 || value == 1;
}

bool isEnclosed(const Maze& maze) {
    for (int x = 0; x < maze.getWidth(); ++x) {
        if (!maze.hasWall(x, 0, 's')) return false;
        if (!maze.hasWall(x, maze.getHeight() - 1, 'n')) return false;
    }
    for (int y = 0; y < maze.getHeight(); ++y) {
        if (!maze.hasWall(0, y, 'w')) return false;
        if (!maze.hasWall(maze.getWidth() - 1, y, 'e')) return false;
    }
    return true;
}

bool isConsistent(const Maze& maze) {
    for (int x = 0; x < maze.getWidth(); ++x) {
        for (int y = 0; y < maze.getHeight(); ++y) {
            if (x + 1 < maze.getWidth() && maze.hasWall(x, y, 'e') != maze.hasWall(x + 1, y, 'w')) return false;
            if (y + 1 < maze.getHeight() && maze.hasWall(x, y, 'n') != maze.hasWall(x, y + 1, 's')) return false;
        }
    }
    return true;
}
}

Maze::Maze(int width, int height) : width(width), height(height) {
    grid.resize(width, std::vector<Cell>(height));
    for(int x = 0; x < width; ++x) {
        for(int y = 0; y < height; ++y) {
            grid[x][y] = {x, y, false, false, false, false};
        }
    }
}

const Cell& Maze::getCell(int x, int y) const {
    return grid[x][y];
}

void Maze::setCell(int x, int y, bool n, bool e, bool s, bool w) {
    if (!isValid(x, y)) return;
    grid[x][y] = {x, y, n, e, s, w};
}

bool Maze::isValid(int x, int y) const {
    return x >= 0 && x < width && y >= 0 && y < height;
}

bool Maze::hasWall(int x, int y, char dir) const {
    if (!isValid(x, y)) return true; // walls out of bounds
    const Cell& c = grid[x][y];
    switch (dir) {
        case 'n': return c.n;
        case 'e': return c.e;
        case 's': return c.s;
        case 'w': return c.w;
    }
    return true;
}

Maze* Maze::fromNumFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) return nullptr;
    
    std::string line;
    int max_x = -1, max_y = -1;
    std::vector<std::vector<int>> data;
    
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        int x, y, n, e, s, w;
        if (iss >> x >> y >> n >> e >> s >> w) {
            std::string extra;
            if (iss >> extra) return nullptr;
            if (x < 0 || y < 0 || !hasWallValue(n) || !hasWallValue(e) || !hasWallValue(s) || !hasWallValue(w)) {
                return nullptr;
            }
            data.push_back({x, y, n, e, s, w});
            max_x = std::max(max_x, x);
            max_y = std::max(max_y, y);
        } else if (line.find_first_not_of(" \t\r\n") != std::string::npos) {
            return nullptr;
        }
    }

    if (data.empty()) return nullptr;
    
    Maze* maze = new Maze(max_x + 1, max_y + 1);
    std::vector<std::vector<bool>> seen(max_x + 1, std::vector<bool>(max_y + 1, false));
    for (const auto& d : data) {
        if (seen[d[0]][d[1]]) {
            delete maze;
            return nullptr;
        }
        seen[d[0]][d[1]] = true;
        maze->setCell(d[0], d[1], d[2], d[3], d[4], d[5]);
    }

    for (int x = 0; x <= max_x; ++x) {
        for (int y = 0; y <= max_y; ++y) {
            if (!seen[x][y]) {
                delete maze;
                return nullptr;
            }
        }
    }

    if (!isEnclosed(*maze) || !isConsistent(*maze)) {
        delete maze;
        return nullptr;
    }
    
    return maze;
}
