#include <string>
#include <iostream>
#include <vector>
#include <random>
#include <iomanip>

#include <SDL.h>

#include <lightgrid/grid.hpp>

#define WINDOW_WIDTH 900
#define WINDOW_HEIGHT 900

#define NUM_ENTITIES 10000
#define LAYOUT_PADDING 1 // Number of pixels of separation between entities

#define MAX_ENTITY_WIDTH 16
#define MIN_ENTITY_WIDTH 16
#define MAX_ENTITY_HEIGHT (MAX_ENTITY_WIDTH)
#define MIN_ENTITY_HEIGHT (MIN_ENTITY_WIDTH)

// This collision simulation doesn't do anything fancy to deal with high-speed
//      entities. For tiny entities, these speeds need to be fairly low to prevent
//      entities from constantly flying through each other.
#define MAX_ENTITY_VELOCITY 20.0f
#define MIN_ENTITY_VELOCITY -20.0f

// SDL seems to use a massive amount of memory when drawing many rectangles. 
//      Still not sure if this is my issue, or SDL's issue; regardless, the memory 
//      usage is measured separately without the rectangles being rendered. 
//      I may switch to OpenGL in the future.
// Setting this to true will disable SDL drawing
#define MEASURE_MEMORY false

SDL_Renderer *renderer;
SDL_Window *window;
SDL_Event event;

bool quit{false};

struct entity {
    lightgrid::bounds bounds;

    float velocity_x;
    float velocity_y;

    float real_x;   // For pixel-width entities, the position must have 
    float real_y;   //      sub-pixel precision

    SDL_Color color;
};

// It is prefered to insert pointers or indicies into other lists
//      than it is to store an instance of that type.
lightgrid::grid<int, 20, 10> grid;
std::vector<entity> entities;

std::mt19937 gen_rand;

lightgrid::bounds genBounds(int map_width, int map_height);

bool isColliding(const entity& e1, const entity& e2);
void resolveCollisionX(entity& e1, entity& e2);
void resolveCollisionY(entity& e1, entity& e2);
void resolveCollision(entity& e1, entity& e2);
void resolveCollisions();

void updatePositions(float delta_time);

void createEntities(int num_entities);
void prepareGrid();
void drawRects();
void pollEvents();

lightgrid::bounds genBounds(int map_width, int map_height) {
    int w,h;

    if (MAX_ENTITY_WIDTH != MIN_ENTITY_WIDTH) {

        int rand_mod{MAX_ENTITY_WIDTH-MIN_ENTITY_WIDTH};
        w = (int)(gen_rand()%(uint_fast32_t)(rand_mod)+MIN_ENTITY_WIDTH);

    } else {
        w = MAX_ENTITY_WIDTH;
    }

    if (MAX_ENTITY_HEIGHT != MIN_ENTITY_HEIGHT) {

        int rand_mod{MAX_ENTITY_HEIGHT-MIN_ENTITY_HEIGHT};
        h = (int)(gen_rand()%(uint_fast32_t)(rand_mod)+MIN_ENTITY_HEIGHT);

    } else {
        h = MAX_ENTITY_HEIGHT;
    }

    lightgrid::bounds new_bounds {
        -1,-1,
        w,h
    };

    return std::move(new_bounds);
}

// Simple aabb collision detection
bool isColliding(const entity& e1, const entity& e2) {
    float top_1 = e1.real_y;
    float bottom_1 = e1.real_y + e1.bounds.h;
    float left_1 = e1.real_x;
    float right_1 = e1.real_x + e1.bounds.w;

    float top_2 = e2.real_y;
    float bottom_2 = e2.real_y + e2.bounds.h;
    float left_2 = e2.real_x;
    float right_2 = e2.real_x + e2.bounds.w;

    return (bottom_1 > top_2 && bottom_2 > top_1 && right_1 > left_2 && right_2 > left_1);
}

void resolveCollisionX(entity& e1, entity& e2) {
    // All masses are assumed to be equal, so the resolution in either
    //      axis is as simple as setting the position of one entity
    //      to the edge of the other and swapping the velocities in that
    //      axis.

    float tmp;

    if (e1.real_x < e2.real_x) {

        if (e1.velocity_x > e2.velocity_x) {
            e1.real_x = e2.real_x - e1.bounds.w;
            tmp = e1.velocity_x;
            e1.velocity_x = e2.velocity_x;
            e2.velocity_x = tmp;
        }

    } else {

        if (e2.velocity_x > e1.velocity_x) {
            e2.real_x = e1.real_x - e2.bounds.w;
            tmp = e2.velocity_x;
            e2.velocity_x = e1.velocity_x;
            e1.velocity_x = tmp;
        }
    }
}


void resolveCollisionY(entity& e1, entity& e2) {
    float tmp;

    if (e1.real_y < e2.real_y) {

        if (e1.velocity_y > e2.velocity_y) {
            e1.real_y = e2.real_y - e1.bounds.h;
            tmp = e1.velocity_y;
            e1.velocity_y = e2.velocity_y;
            e2.velocity_y = tmp;
        }

    } else {

        if (e2.velocity_y > e1.velocity_y) {
            e2.real_y = e1.real_y - e2.bounds.h;
            tmp = e2.velocity_y;
            e2.velocity_y = e1.velocity_y;
            e1.velocity_y = tmp;
        }
    }
}

void resolveCollision(entity& e1, entity& e2) {
    // For any collision between two aabb's, a collision only needs 
    //      to be resolved in one axis. 

    // An intuitive solution for deciding which axis to resolve for
    //      is to imagine the rectangle create by the intersection
    //      of the two bounding boxes, and choose the axis perpendicular
    //      to the largest side of that intersection.

    float collisions_width;
    if (e1.real_x < e2.real_x) {
        collisions_width = std::min((e1.real_x + e1.bounds.w) - e2.real_x, (float)e2.bounds.w);
    } else {
        collisions_width = std::min((e2.real_x + e2.bounds.w) - e1.real_x, (float)e1.bounds.w);
    }

    float collisions_height;
    if (e1.real_y < e2.real_y) {
        collisions_height = std::min((e1.real_y + e1.bounds.h) - e2.real_y, (float)e2.bounds.h);
    } else {
        collisions_height = std::min((e2.real_y + e2.bounds.h) - e1.real_y, (float)e1.bounds.h);
    }

    if (collisions_width > collisions_height) {
        resolveCollisionY(e1, e2);
    } else {
        resolveCollisionX(e1,e2);
    }
}

void resolveCollisions() {
    for (int it{0}; it < entities.size(); it++) {
        entity& entity{entities[it]};

        grid.traverse(entity.bounds, [it, &entity](int other_entity) {
            if (it != other_entity && isColliding(entity, entities[other_entity])) {
                resolveCollision(entity, entities[other_entity]);
            }
        });

        if (entity.real_x <= 0) {
            entity.real_x = 0;
            entity.velocity_x = -entity.velocity_x;
        }
        if (entity.real_y <= 0) {
            entity.real_y = 0;
            entity.velocity_y = -entity.velocity_y;
        }
        if (entity.real_x + entity.bounds.w > WINDOW_WIDTH) {
            entity.real_x = WINDOW_WIDTH - entity.bounds.w;
            entity.velocity_x = -entity.velocity_x;
        }
        if (entity.real_y + entity.bounds.h > WINDOW_HEIGHT) {
            entity.real_y = WINDOW_HEIGHT - entity.bounds.h;
            entity.velocity_y = -entity.velocity_y;
        }
    }
}

void updatePositions(float delta_time) {
    lightgrid::bounds old_bounds;

    for (int it{0}; it < entities.size(); it++) {

        entity& entity{entities[it]};

        old_bounds = entity.bounds;

        entity.real_x += (delta_time/1000.0)*entity.velocity_x;
        entity.real_y += (delta_time/1000.0)*entity.velocity_y;
        entity.bounds.x = entity.real_x;
        entity.bounds.y = entity.real_y;

        // The previous bounds are needed when updating the position of an
        //      element in the grid, along with the index of the element node,
        //      within the grid. This is the only overhead that must be 
        //      considered when implementing the grid.
        grid.update(it, old_bounds, entity.bounds);
    }
}

void createEntities(int num_entities) {
    // This function attempts to create the specified number of entities,
    //      centering them in the window. If specified number will not fit in
    //      the window, it will create as many entities as it can fit.

    float ratio{WINDOW_WIDTH/(float)WINDOW_HEIGHT};

    int padding{LAYOUT_PADDING};

    int max_num_entities_y{WINDOW_HEIGHT/(MAX_ENTITY_HEIGHT+padding)};
    int max_num_entities_x{WINDOW_WIDTH/(MAX_ENTITY_WIDTH+padding)};
    int max_num_entities{max_num_entities_x*max_num_entities_y};

    int width, start_x, start_y;

    if (num_entities < max_num_entities) {

        // Get the number of entities on a side
        width = std::sqrt(ratio*num_entities);
        int height{num_entities/width};

        start_x = (max_num_entities_x - width)/2;
        start_y = (max_num_entities_y - height)/2;

    } else {
        width = max_num_entities_x;
        start_x = 0;
        start_y = 0;
    }

    int entity_count{0};
    uint8_t lowest_color{100};

    for (int entity_count{0}; entity_count < num_entities && entity_count < max_num_entities; entity_count++) {

        lightgrid::bounds new_bounds{genBounds(WINDOW_WIDTH, WINDOW_HEIGHT)};

        float max_velocity{MAX_ENTITY_VELOCITY};
        float min_velocity{MIN_ENTITY_VELOCITY};

        float velocity_x{(gen_rand()%((uint_fast32_t)(max_velocity*100-min_velocity*100))/100.0f+min_velocity)};
        float velocity_y{(gen_rand()%((uint_fast32_t)(max_velocity*100-min_velocity*100))/100.0f+min_velocity)};
        
        int curr_x{(entity_count%width + start_x)*(MAX_ENTITY_WIDTH+padding)};
        int curr_y{(entity_count/width + start_y)*(MAX_ENTITY_HEIGHT+padding)};

        new_bounds.x = curr_x;
        new_bounds.y = curr_y;

        entities.emplace_back(
            new_bounds,
            velocity_x,
            velocity_y, // velocity
            curr_x,
            curr_y,
            SDL_Color {
                (uint8_t)(lowest_color + (gen_rand()%(256 - lowest_color))), 
                (uint8_t)(lowest_color + (gen_rand()%(256 - lowest_color))), 
                (uint8_t)(lowest_color + (gen_rand()%(256 - lowest_color))), 
                255
            }
        );
    }
}

void prepareGrid() {
    for (int it{0}; it < entities.size(); it++) {
        grid.insert(it, entities[it].bounds);
    }
}

void drawRects() {
    for (auto entity : entities) {
        SDL_SetRenderDrawColor(renderer, entity.color.r, entity.color.g, entity.color.b, 255);
        SDL_RenderFillRect(renderer, reinterpret_cast<SDL_Rect*>(&(entity.bounds)));
    }
}

void pollEvents() {
    while (SDL_PollEvent(&event) == 1) {
        switch(event.type) {
            case SDL_QUIT:
                quit = true;
                break;
            case SDL_KEYDOWN:
                switch(event.key.keysym.sym) {
                    case SDLK_ESCAPE: 
                        quit = true;
                        break;
                }
                break;
        }
    }
}

int main(int argc, char **argv) {
    SDL_Init(SDL_INIT_TIMER | SDL_INIT_VIDEO);
    SDL_CreateWindowAndRenderer(WINDOW_WIDTH, WINDOW_HEIGHT, 0, &window, &renderer);
    
    createEntities(NUM_ENTITIES);
    prepareGrid();
    
    std::cout << "Number of entities: " << entities.size() << "\n";

    uint64_t last_frame{SDL_GetTicks64()};
    uint64_t current_frame{SDL_GetTicks64()};
    uint64_t delta_time{0};

    int frame_count{0};
    uint64_t interval_time{0};

    while (!quit) {
        pollEvents();

        current_frame = SDL_GetTicks64();
        delta_time = current_frame - last_frame;
        last_frame = current_frame;

        SDL_SetRenderDrawColor(renderer, 40, 35, 30, 0);
        SDL_RenderClear(renderer);
        
        updatePositions(delta_time);
        resolveCollisions();

        #if !(MEASURE_MEMORY)
            drawRects();
        #endif

        SDL_RenderPresent(renderer);

        frame_count++;
        interval_time += delta_time;
        
        if (interval_time >= 1000) {
            std::cout << "\r" << std::fixed << std::setprecision(2) << "FPS: " << std::setw(10) << (frame_count/(float)interval_time)*1000.0f;
            frame_count = 0;
            interval_time = 0;
        }
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}