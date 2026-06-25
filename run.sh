#!/bin/bash
set -e

cd "$(dirname "$0")"

# Ensure Pygame is installed
if ! python3 -c "import pygame" &> /dev/null; then
    echo "Installing pygame..."
    pip3 install pygame
fi

echo "Building C++ backend..."
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel

echo "============================================="
echo "Select a simulation to run:"
echo "1) Naive vs Smooth (Diagonal) Approach"
echo "2) Algorithms Comparison (FloodFill, Hand On Wall, A*)"
echo "3) Multi-Agent Cooperation (Case A: Same Destination)"
echo "4) Multi-Agent Cooperation (Case B: Unique Destinations)"
echo "============================================="

read -p "Enter choice (1-4): " choice

case $choice in
    1)
        echo "Running Naive vs Smooth..."
        ./build/sim1 src/resources/mazes/example1.num | python3 visualizer.py
        ;;
    2)
        echo "Running Algorithms Comparison..."
        ./build/sim2 src/resources/mazes/example1.num | python3 visualizer.py
        ;;
    3)
        echo "Running Multi-Agent (Same Destination)..."
        ./build/sim3 src/resources/mazes/example1.num 1 | python3 visualizer.py
        ;;
    4)
        echo "Running Multi-Agent (Unique Destinations)..."
        ./build/sim3 src/resources/mazes/example1.num 2 | python3 visualizer.py
        ;;
    *)
        echo "Invalid choice. Exiting."
        ;;
esac
