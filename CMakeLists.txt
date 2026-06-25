cmake_minimum_required(VERSION 3.10)

project(micromouse_sim_backend LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
    set(CMAKE_CXX_COMPILER_LAUNCHER
        "${CMAKE_COMMAND}" -E env "CCACHE_DIR=${CMAKE_BINARY_DIR}/.ccache" "${CCACHE_PROGRAM}"
    )
endif()

set(SIM_BACKEND_SOURCES
    src/Maze.cpp
    src/Pathfinder.cpp
    src/Agent.cpp
    src/SimulationEngine.cpp
)

add_executable(micromouse_sim src/main.cpp ${SIM_BACKEND_SOURCES})
add_executable(sim1 src/main_1_naive_vs_smooth.cpp ${SIM_BACKEND_SOURCES})
add_executable(sim2 src/main_2_algorithms.cpp ${SIM_BACKEND_SOURCES})
add_executable(sim3 src/main_3_multi_agent.cpp ${SIM_BACKEND_SOURCES})

if(MSVC)
    target_compile_options(micromouse_sim PRIVATE /W4)
    target_compile_options(sim1 PRIVATE /W4)
    target_compile_options(sim2 PRIVATE /W4)
    target_compile_options(sim3 PRIVATE /W4)
else()
    target_compile_options(micromouse_sim PRIVATE -Wall -Wextra -pedantic)
    target_compile_options(sim1 PRIVATE -Wall -Wextra -pedantic)
    target_compile_options(sim2 PRIVATE -Wall -Wextra -pedantic)
    target_compile_options(sim3 PRIVATE -Wall -Wextra -pedantic)
endif()
