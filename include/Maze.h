#pragma once

#include <vector>
#include <string>

struct Cell {
    int x, y;
    bool n, e, s, w;
};

class Maze {
public:
    Maze(int width, int height);
    static Maze* fromNumFile(const std::string& path);
    
    int getWidth() const { return width; }
    int getHeight() const { return height; }
    
    const Cell& getCell(int x, int y) const;
    void setCell(int x, int y, bool n, bool e, bool s, bool w);

    bool isValid(int x, int y) const;
    bool hasWall(int x, int y, char dir) const;

private:
    int width;
    int height;
    std::vector<std::vector<Cell>> grid;
};
