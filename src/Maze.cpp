#include "../include/Maze.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <cmath>
#include <random>
#include <functional>
#include <utility>

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
            grid[x][y] = Cell(x, y, false, false, false, false, false);
        }
    }
}

const Cell& Maze::getCell(int x, int y) const {
    return grid[x][y];
}

void Maze::setCell(int x, int y, bool n, bool e, bool s, bool w) {
    if (!isValid(x, y)) return;
    grid[x][y] = Cell(x, y, n, e, s, w, true);
}

void Maze::removeWallBetween(int x1, int y1, int x2, int y2) {
    if (!isValid(x1, y1) || !isValid(x2, y2)) return;
    int dx = x2 - x1;
    int dy = y2 - y1;

    Cell c1 = grid[x1][y1];
    Cell c2 = grid[x2][y2];
    if (dx == 1 && dy == 0) {
        c1.e = false;
        c2.w = false;
    } else if (dx == -1 && dy == 0) {
        c1.w = false;
        c2.e = false;
    } else if (dx == 0 && dy == 1) {
        c1.n = false;
        c2.s = false;
    } else if (dx == 0 && dy == -1) {
        c1.s = false;
        c2.n = false;
    } else {
        return;
    }
    c1.known = true;
    c2.known = true;
    grid[x1][y1] = c1;
    grid[x2][y2] = c2;
}

std::pair<int, int> Maze::getCenter() const {
    return {width / 2, height / 2};
}

Maze Maze::generate(int width, int height, unsigned seed) {
    Maze maze(width, height);
    for (int x = 0; x < width; ++x) {
        for (int y = 0; y < height; ++y) {
            maze.setCell(x, y, true, true, true, true);
        }
    }

    std::mt19937 rng(seed);
    std::vector<std::vector<bool>> visited(width, std::vector<bool>(height, false));

    std::function<void(int, int)> carve = [&](int cx, int cy) {
        visited[cx][cy] = true;
        std::vector<std::pair<int, int>> dirs = {{0, 1}, {1, 0}, {0, -1}, {-1, 0}};
        std::shuffle(dirs.begin(), dirs.end(), rng);

        for (const auto& dir : dirs) {
            int nx = cx + dir.first;
            int ny = cy + dir.second;
            if (!maze.isValid(nx, ny) || visited[nx][ny]) continue;
            maze.removeWallBetween(cx, cy, nx, ny);
            carve(nx, ny);
        }
    };

    carve(0, 0);

    for (int x = 0; x < width; ++x) {
        maze.setCell(x, 0, maze.getCell(x, 0).n, maze.getCell(x, 0).e, true, maze.getCell(x, 0).w);
        maze.setCell(x, height - 1, true, maze.getCell(x, height - 1).e, maze.getCell(x, height - 1).s, maze.getCell(x, height - 1).w);
    }
    for (int y = 0; y < height; ++y) {
        maze.setCell(0, y, maze.getCell(0, y).n, maze.getCell(0, y).e, maze.getCell(0, y).s, true);
        maze.setCell(width - 1, y, maze.getCell(width - 1, y).n, true, maze.getCell(width - 1, y).s, maze.getCell(width - 1, y).w);
    }

    return maze;
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

bool Maze::canMove(int from_x, int from_y, int to_x, int to_y, bool allow_diagonal) const {
    if (!isValid(from_x, from_y) || !isValid(to_x, to_y)) return false;

    const int dx = to_x - from_x;
    const int dy = to_y - from_y;
    if (dx == 0 && dy == 0) return false;

    if (std::abs(dx) + std::abs(dy) == 1) {
        if (dx == 1) return !hasWall(from_x, from_y, 'e');
        if (dx == -1) return !hasWall(from_x, from_y, 'w');
        if (dy == 1) return !hasWall(from_x, from_y, 'n');
        if (dy == -1) return !hasWall(from_x, from_y, 's');
        return false;
    }

    if (!allow_diagonal || std::abs(dx) != 1 || std::abs(dy) != 1) {
        return false;
    }

    if (dx == 1 && dy == 1) {
        return !hasWall(from_x, from_y, 'n') && !hasWall(from_x, from_y, 'e')
            && !hasWall(from_x, to_y, 'e') && !hasWall(to_x, from_y, 'n');
    }
    if (dx == 1 && dy == -1) {
        return !hasWall(from_x, from_y, 's') && !hasWall(from_x, from_y, 'e')
            && !hasWall(from_x, to_y, 'e') && !hasWall(to_x, from_y, 's');
    }
    if (dx == -1 && dy == -1) {
        return !hasWall(from_x, from_y, 's') && !hasWall(from_x, from_y, 'w')
            && !hasWall(from_x, to_y, 'w') && !hasWall(to_x, from_y, 's');
    }
    if (dx == -1 && dy == 1) {
        return !hasWall(from_x, from_y, 'n') && !hasWall(from_x, from_y, 'w')
            && !hasWall(from_x, to_y, 'w') && !hasWall(to_x, from_y, 'n');
    }

    return false;
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
