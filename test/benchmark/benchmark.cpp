#include <iostream>
#include <iomanip>
#include <vector>
#include <random>
#include <chrono>

#include <lightgrid/grid.hpp>

struct TestEntity {
    lightgrid::bounds bounds;
    int id{-1};
    int element_node{-1};
};

std::vector<TestEntity> test_entities;
// std::vector<lightgrid::bounds> test_bounds;
lightgrid::grid<TestEntity*> grid;
std::mt19937 gen_rand;

void printEntity(const TestEntity& test_entity) {

    std::cout << std::right << std::setfill('-') << std::setw(10) << "\n";
    std::cout << std::left << std::setfill(' ');
    std::cout << "x: " << std::setw(10) << test_entity.bounds.x << "    ";
    std::cout << "y: " << std::setw(10) << test_entity.bounds.y << "    ";
    std::cout << "w: " << std::setw(10) << test_entity.bounds.w << "    ";
    std::cout << "h: " << std::setw(10) << test_entity.bounds.h << "\n";

    std::cout << "id: " << std::setw(12) << test_entity.id << "    ";
    std::cout << "element_node: " << std::setw(30) << test_entity.element_node << "\n";
}

bool isColliding(const lightgrid::bounds& bounds_1, const lightgrid::bounds& bounds_2) {

    float top_1 = bounds_1.y;
    float bottom_1 = bounds_1.y + bounds_1.h;
    float left_1 = bounds_1.x;
    float right_1 = bounds_1.x + bounds_1.w;

    float top_2 = bounds_2.y;
    float bottom_2 = bounds_2.y + bounds_2.h;
    float left_2 = bounds_2.x;
    float right_2 = bounds_2.x + bounds_2.w;

    return (bottom_1 > top_2 && bottom_2 > top_1 && right_1 > left_2 && right_2 > left_1);
}

lightgrid::bounds genValidBounds(int map_width, int map_height, int cell_size) {

    int w{((int)(gen_rand()%(uint_fast32_t)100)-10)+10};
    int h{((int)(gen_rand()%(uint_fast32_t)100)-10)+10};

    lightgrid::bounds new_bounds {
        ((int)(gen_rand()%(uint_fast32_t)(map_width-w-1))+1),
        ((int)(gen_rand()%(uint_fast32_t)(map_height-h-1))+1),
        w,
        h
    };

    return std::move(new_bounds);
}

int getCollisionsNaive(std::vector<TestEntity*>& results) {

    int i{0};
    for (int xx{0}; xx < test_entities.size(); xx++) {

        for (int yy{0}; yy < test_entities.size(); yy++) {

            if (xx != yy && isColliding(test_entities[xx].bounds, test_entities[yy].bounds)) {
                results.push_back(&test_entities[xx]);
            }
        }

        i += results.size();
        results.clear();
    }

    return i;
}

int getCollisionsGrid(std::vector<TestEntity*>& results) {

    int i{0};

    for (int xx{0}; xx < test_entities.size(); xx++) {

        grid.query(test_entities[xx].bounds, results);

        for (auto entity : results) {
            if (entity->id != xx && isColliding(test_entities[xx].bounds, entity->bounds)) {
                i++;
            }
        }
        results.clear();
    }
    
    return i;
}

int main() {

    size_t num_test_entities{4000};
    int map_width{2000};
    int map_height{2000};
    int cell_size{10};

    // Create all the test entities
    test_entities.reserve(num_test_entities);

    for (size_t i{0}; i < num_test_entities; i++) {
        test_entities.emplace_back(genValidBounds(map_width, map_height, cell_size),i);
    }

    // Initialize the test grid with the test entities
    grid.init(map_width, map_height, cell_size);
    grid.reserve(num_test_entities);

    for (auto& entity : test_entities) {
        entity.element_node = grid.insert(&entity, entity.bounds);
    }

    std::vector<TestEntity*> no_grid_results;
    int no_grid_sum{0};
    auto start = std::chrono::high_resolution_clock::now();

    no_grid_sum = getCollisionsNaive(no_grid_results);

    auto end = std::chrono::high_resolution_clock::now();
    auto no_grid_duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    auto no_grid_average = no_grid_duration/(float)num_test_entities;

    std::cout << "No grid collision count: " << no_grid_sum << "\n";

    std::vector<TestEntity*> grid_results;
    int grid_sum{0};
    start = std::chrono::high_resolution_clock::now();

    grid_sum += getCollisionsGrid(grid_results);

    end = std::chrono::high_resolution_clock::now();
    auto grid_duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    auto grid_average = grid_duration/(float)num_test_entities;

    std::cout << "Grid collision count: " <<  grid_sum << "\n";

    std::cout << std::left;
    std::cout << "\nNumber of Entities: " << num_test_entities << "\n\n";
    std::cout << std::setw(34) << "Finding collisions without grid: " << std::setprecision(6) << no_grid_duration << " ms" << "\n";
    std::cout << "Average: " << no_grid_average << std::setprecision(6) << " ms\n\n";
    std::cout << std::setw(34) << "Finding collisions with grid: " << std::setprecision(6) << grid_duration << " ms" << "\n";
    std::cout << "Average: " << grid_average << std::setprecision(6) << " ms\n";
}