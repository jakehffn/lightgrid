#include <iostream>
#include <iomanip>
#include <vector>
#include <random>
#include <chrono>
#include <string>

#include <lightgrid/grid.hpp>

lightgrid::bounds genValidBounds(int map_width, int map_height);

std::mt19937 gen_rand;

struct Entity {
    lightgrid::bounds bounds;
    int id{-1};
    int element_node{-1};

    // Entity(Entity&& entity) = default;
};

struct Test {
    size_t num_tests{100};
    size_t num_test_entities{800};
    int cell_size{16};
    int map_width{3200};
    int map_height{3200};

    std::vector<int64_t> function_durations;
    std::vector<std::string> function_labels;

    std::vector<Entity> test_entities;
    lightgrid::grid<Entity*> grid;

    Test(size_t num_tests, size_t num_test_entities, int cell_size) :
        num_tests{ num_tests }, num_test_entities{ num_test_entities }, cell_size{ cell_size } {

            this->test_entities.reserve(this->num_test_entities);

            for (size_t i{0}; i < this->num_test_entities; i++) {
                this->test_entities.emplace_back(genValidBounds(this->map_width, this->map_height), i);
            }

            // Initialize the test grid with the test entities
            this->grid.init(this->map_width, this->map_height, this->cell_size);
            this->grid.reserve(this->num_test_entities);

            for (auto& entity : this->test_entities) {
                entity.element_node = grid.insert(&entity, entity.bounds);
            }
    }
};

void printPercentage(int part, int total) {

    int total_marks = 60;
    float percent = part/(float)total;
    int num_marks = (int)(percent*total_marks);
    std::cout << std::right << "\r[";
    std::cout << std::setfill('=') << std::setw(num_marks + 1) << ">"; // One is added becuase the minimum number of characters is always at least one
    std::cout << std::setfill('-') << std::setw(total_marks - num_marks + 1);
    std::cout << "]";

    int total_string_length = std::to_string(total).length();
    std::cout << "[";
    std::cout << std::setfill(' ') << std::setw(total_string_length);
    std::cout << part << "/" << total << "] ";
    std::cout << std::setw(4) << (int)(percent*100) << "% ";  
}

void printEntity(const Entity& test_entity) {

    std::cout << std::right << std::setfill('-') << std::setw(10) << "\n";
    std::cout << std::left << std::setfill(' ');
    std::cout << "x: " << std::setw(10) << test_entity.bounds.x << "    ";
    std::cout << "y: " << std::setw(10) << test_entity.bounds.y << "    ";
    std::cout << "w: " << std::setw(10) << test_entity.bounds.w << "    ";
    std::cout << "h: " << std::setw(10) << test_entity.bounds.h << "\n";

    std::cout << "id: " << std::setw(12) << test_entity.id << "    ";
    std::cout << "element_node: " << std::setw(30) << test_entity.element_node << "\n";
}

void printTest(const Test& test) {

    int col_width{20};
    int precision{8};

    std::cout << "\n";
    std::cout << "-----------------------------------------------\n";
    std::cout << "|                  Summary                    |\n";
    std::cout << "-----------------------------------------------\n\n";

    std::cout << std::left << std::setfill(' ');
    std::cout << std::setw(col_width) << "Number of Tests: " << test.num_tests << "\n";
    std::cout << std::setw(col_width) << "Number of Entities: " << test.num_test_entities << "\n\n";

    for (size_t i{0}; i < test.function_labels.size(); i++) {

        auto duration = test.function_durations[i];
        auto average = duration/(float)test.num_tests;

        std::cout << test.function_labels[i] << ": \n";
        std::cout << "    " << std::setw(col_width-4) << "Total Time: " << std::setprecision(precision) << duration/1000000.0 << " ms" << "\n";
        std::cout << "    " << std::setw(col_width-4) << "Per Test: " << average/1000000.0 << std::setprecision(precision) << " ms\n\n";
    }

    std::cout << "-----------------------------------------------\n\n";
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

lightgrid::bounds genValidBounds(int map_width, int map_height) {

    int max_width{64};
    int min_width{16};

    int max_height{64};
    int min_height{16};

    int w{((int)(gen_rand()%(uint_fast32_t)max_width)-min_width)+min_width};
    int h{((int)(gen_rand()%(uint_fast32_t)max_height)-min_height)+min_height};

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

    return std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
}

template<typename F>
void testFunction(F&& func, Test& test, std::string label) {

    test.function_durations.push_back(0);
    test.function_labels.push_back(label);

    std::cout << label << "\n";

    for (int i{0}; i < test.num_tests; i++) {

        if (i%(test.num_tests/100 + 1) == 0) {
            printPercentage(i, test.num_tests);
        }

        std::forward<F>(func)(test);
    }

    printPercentage(test.num_tests, test.num_tests);

    std::cout << "\n\n";
}

void naiveCollisionsTime(Test& test) {

    // std::cout << "\n---No grid collision test---\n";

    std::vector<Entity*> results;
    int i{0};

    for (int xx{0}; xx < test.num_test_entities; xx++) {
        
        test.function_durations[test.function_durations.size()-1] += timeFunction([&test, &results, &xx]() {

            for (int yy{0}; yy < test.num_test_entities; yy++) {

                if (xx != yy && isColliding(test.test_entities[xx].bounds, test.test_entities[yy].bounds)) {
                    results.push_back(&(test.test_entities[xx]));
                }
            }

        });

        i += results.size();
        results.clear();
    }
}

void gridCollisionsTime(Test& test) {

    std::vector<Entity*> results;
    int i{0};

    for (int xx{0}; xx < test.test_entities.size(); xx++) {

        test.function_durations[test.function_durations.size()-1] += timeFunction([&test, &results, &i, &xx]() {

            test.grid.query(test.test_entities[xx].bounds, results);

            for (auto entity : results) {
                if (entity->id != xx && isColliding(test.test_entities[xx].bounds, entity->bounds)) {
                    i++;
                }
            }
        });

        results.clear();
    }
}

int main() {

    std::cout << "\n";
    std::cout << "===============================================\n";
    std::cout << "|                                             |\n";
    std::cout << "|           Light Grid Benchmark              |\n";
    std::cout << "|               Jake Hoffman                  |\n";
    std::cout << "|                   2023                      |\n";
    std::cout << "|                                             |\n";
    std::cout << "===============================================\n\n";

    Test test(400, 10000, 40);

    // testFunction(naiveCollisionsTime, test, "Collision tests");
    testFunction(gridCollisionsTime, test, "Collision tests w/ grid");

    printTest(test);
}