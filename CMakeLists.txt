cmake_minimum_required(VERSION 3.10)

project(Sokoban)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

include_directories(include)

add_executable(Sokoban src/Sokoban.cpp src/Board.cpp src/Solver.cpp src/DistanceTable.cpp)
