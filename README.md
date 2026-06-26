# Micromouse Algorithm Benchmark

A C++ micromouse simulation with a Pygame visualizer that compares three maze-solving strategies through a full **explore → return to start → speed run** cycle—the same lifecycle used in real micromouse competitions.

## Algorithms

| Algorithm | Exploration | Speed run |
|-----------|-------------|-----------|
| **Time-Aware A\*** | Flood-fill distance grid (goal-directed) | Time-optimal path with diagonal moves, turn penalties, and an admissible heuristic |
| **Flood Fill** | Flood-fill distance grid | Shortest cell-count BFS path on the discovered map |
| **Wall Follower** | Right-hand wall following | Continues wall-following on the discovered map |

## Benchmark Results (100 random 16×16 mazes)

Each maze is generated with a deterministic seed, and all three algorithms run on the **same** mazes from `(0,0)` to the center cell.

| Algorithm | Avg speed-run steps | Avg cells explored | Avg turns | Avg simulated time |
|-----------|--------------------:|-------------------:|----------:|-------------------:|
| Time-Aware A* | 19.0 | 243.1 | 321.4 | 496.3 s |
| Flood Fill | 101.5 | 243.1 | 373.2 | 573.1 s |
| Wall Follower | 171.5 | 256.0 | 425.1 | 655.8 s |

**Resume-ready bullets:**

- Time-Aware A* reached the goal in **88.9% fewer speed-run steps** than Wall Follower while exploring **5.3% fewer cells** during the discovery phase.
- Time-Aware A* completed speed runs in **81.3% fewer steps** than Flood Fill and finished **13.4% faster** in simulated time (diagonal-aware routing with turn costs).
- Flood Fill took **40.8% fewer speed-run steps** than Wall Follower and explored **5.0% fewer cells** during discovery.

Full CSV output: [`benchmark/results.txt`](benchmark/results.txt)

## Quick Start

**Requirements:** CMake 3.10+, C++11 compiler, Python 3, Pygame

```bash
./run.sh          # interactive menu (visual demos + benchmark)
```

Or manually:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel

# Visual comparison of all three algorithms
./build/sim2 src/resources/mazes/example1.num | python3 visualizer.py

# Headless benchmark (default: 100 mazes)
./build/benchmark 100
```

## Visualizer Controls

- **Tab** — cycle which agent's flood-fill distance grid is shown
- Walls appear only as agents discover them (no cheat overlay)
- Sidebar shows steps, turns, simulated time, and cells explored per agent

## Project Structure

```
src/
  Maze.cpp           — .num loader + procedural maze generator
  Pathfinder.cpp     — Time-Aware A*, Flood Fill, Wall Follower
  Agent.cpp          — exploration/speed-run lifecycle + metrics
  SimulationEngine.cpp
  main_benchmark.cpp — 100-maze batch runner with CSV + summary
visualizer.py        — Pygame frontend (JSON over stdin)
```

## Architecture

```
┌─────────────┐    JSON lines     ┌──────────────┐
│ C++ sim     │ ────────────────▶ │ visualizer.py│
│ (sim1/2/3)  │    init/state/done│ (Pygame)     │
└─────────────┘                   └──────────────┘

┌─────────────┐
│ benchmark   │  headless — no visualizer, emits CSV + summary
└─────────────┘
```

Each agent runs independently: discover cells → build memory map → return to start → sprint to goal using only discovered knowledge (no post-exploration map cheat).
