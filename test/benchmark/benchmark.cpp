#include <iostream>
#include <iomanip>
#include <vector>
#include <random>
#include <chrono>
#include <string>

#include <lightgrid/grid.hpp>

struct TestEntity {
    lightgrid::bounds bounds;
    int id{-1};
    int element_node{-1};
};

size_t num_test_entities{40000};
int map_width{3200};
int map_height{3200};
int cell_size{16};

std::vector<TestEntity> test_entities;
// std::vector<lightgrid::bounds> test_bounds;
lightgrid::grid<TestEntity*> grid;
std::mt19937 gen_rand;

void printPercentage(int part, int total) {

    int total_possible_marks = 60;
    float percent = part/(float)total;
    int num_marks = (int)(percent*total_possible_marks);
    std::cout << "\r[";
    std::cout << std::setfill('=') << std::setw(num_marks) << ">";
    std::cout << std::setfill('-') << std::setw(total_possible_marks - num_marks);
    std::cout << "]";

    int total_string_length = std::to_string(total).length();
    std::cout << "[";
    std::cout << std::setfill(' ') << std::setw(total_string_length);
    std::cout << part << "/" << total << "] ";
    std::cout << std::setw(4) << (int)(percent*100) << "% ";  
}

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

template<typename F>
int64_t timeFunction(F&& func) {

    auto start = std::chrono::high_resolution_clock::now();
    std::forward<F>(func)();
    auto end = std::chrono::high_resolution_clock::now();

    return std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
}

int64_t naiveCollisionsTime() {

    std::cout << "\n---No grid collision test---\n";

    std::vector<TestEntity*> results;
    int i{0};

    int64_t duration_sum{0};

    for (int xx{0}; xx < num_test_entities; xx++) {

        if (xx%(num_test_entities/100) == 0) {
            printPercentage(xx, num_test_entities);
        }
        
        duration_sum += timeFunction([&results, &i, &xx]() {

            for (int yy{0}; yy < num_test_entities; yy++) {

                if (xx != yy && isColliding(test_entities[xx].bounds, test_entities[yy].bounds)) {
                    results.push_back(&test_entities[xx]);
                }
            }

            i += results.size();
            results.clear();
        });
    }

    printPercentage(num_test_entities, num_test_entities);

    std::cout << "\n| Collisions detected: " << i << "\n";
    std::cout << "-------------------------\n";

    return duration_sum;
}

int64_t gridCollisionsTime() {

    std::cout << "\n---Grid collision test---\n";

    std::vector<TestEntity*> results;
    int i{0};

    int64_t duration_sum{0};

    for (int xx{0}; xx < test_entities.size(); xx++) {

        if (xx%(num_test_entities/100) == 0) {
            printPercentage(xx, num_test_entities);
        }

        duration_sum += timeFunction([&results, &i, &xx]() {

            grid.query(test_entities[xx].bounds, results);

            for (auto entity : results) {
                if (entity->id != xx && isColliding(test_entities[xx].bounds, entity->bounds)) {
                    i++;
                }
            }

            results.clear();
        });
    }

    printPercentage(num_test_entities, num_test_entities);

    std::cout << "\n| Collisions detected: " <<  i << "\n";
    std::cout << "-------------------------\n";

    return duration_sum;
}

int main() {

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

    auto no_grid_duration = naiveCollisionsTime();
    auto no_grid_average = no_grid_duration/(float)num_test_entities;

    auto grid_duration = gridCollisionsTime();
    auto grid_average = grid_duration/(float)num_test_entities;

    std::cout << std::left << std::setfill(' ');
    std::cout << "\nNumber of Entities: " << num_test_entities << "\n\n";
    std::cout << std::setw(34) << "Finding collisions without grid: " << std::setprecision(6) << no_grid_duration << " ms" << "\n";
    std::cout << "Average: " << no_grid_average << std::setprecision(6) << " ms\n\n";
    std::cout << std::setw(34) << "Finding collisions with grid: " << std::setprecision(6) << grid_duration << " ms" << "\n";
    std::cout << "Average: " << grid_average << std::setprecision(6) << " ms\n";
}