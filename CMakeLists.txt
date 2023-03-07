cmake_minimum_required(VERSION 3.23)

project(lightgrid)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_executable(${PROJECT_NAME}_benchmark test/benchmark/benchmark.cpp)
add_executable(${PROJECT_NAME}_test test/lightgrid/grid.cpp)
add_executable(${PROJECT_NAME}_example test/example/lightgrid_example.cpp)

target_include_directories(${PROJECT_NAME}_benchmark PUBLIC include/lightgrid)
target_include_directories(${PROJECT_NAME}_test PUBLIC include/lightgrid)
target_include_directories(${PROJECT_NAME}_example PUBLIC include/lightgrid)

add_subdirectory(test/benchmark)
add_subdirectory(test/example)
add_subdirectory(test/lightgrid)