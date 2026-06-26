#pragma once

#include "Agent.h"
#include <string>
#include <vector>

struct VisualRunSpec {
    int start_x;
    int start_y;
    int dest_x;
    int dest_y;
    bool smooth;
    AlgorithmType algorithm;
    std::string label;
};

int runVisualSimSequence(const std::string& maze_path, const std::vector<VisualRunSpec>& runs, int max_ticks = 50000);
