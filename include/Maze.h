#pragma once

#include <vector>
#include <string>

struct Cell {
    int x, y;
    bool n, e, s, w;
    bool known;

    Cell(int x = 0, int y = 0, bool n = false, bool e = false, bool s = false, bool w = false, bool known = false)
        : x(x), y(y), n(n), e(e), s(s), w(w), known(known) {}
};

class Maze {
public:
    Maze(int width, int height);
    static Maze* fromNumFile(const std::string& path);
    static Maze generate(int width, int height, unsigned seed);
    
    int getWidth() const { return width; }
    int getHeight() const { return height; }
    
    const Cell& getCell(int x, int y) const;
    void setCell(int x, int y, bool n, bool e, bool s, bool w);
    void removeWallBetween(int x1, int y1, int x2, int y2);

    bool isValid(int x, int y) const;
    bool hasWall(int x, int y, char dir) const;
    bool canMove(int from_x, int from_y, int to_x, int to_y, bool allow_diagonal) const;
    std::pair<int, int> getCenter() const;

private:
    int width;
    int height;
    std::vector<std::vector<Cell>> grid;
};
