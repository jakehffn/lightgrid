cmake_minimum_required(VERSION 3.23)

project(light_grid)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_executable(${PROJECT_NAME}_benchmark test/benchmark/benchmark.cpp)
add_executable(${PROJECT_NAME}_test test/fast_grid/grid.cpp)
add_executable(${PROJECT_NAME}_example test/example/fast_grid_example.cpp)

target_include_directories(${PROJECT_NAME}_benchmark PUBLIC include/light_grid)
target_include_directories(${PROJECT_NAME}_test PUBLIC include/light_grid)
target_include_directories(${PROJECT_NAME}_example PUBLIC include/light_grid)

add_subdirectory(test/benchmark)
add_subdirectory(test/example)
add_subdirectory(test/fast_grid)