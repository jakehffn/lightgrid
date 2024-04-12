# lightgrid

**lightgrid is an header-only implementation of a grid data-structure for spacial lookups, useful for collision detection and render-culling, implemented using C++20 features.**

lightgrid allows for rapid insertion of arbitrary data types, very stable memory usage, and the ability to take avantage of the known characteristics of the inserted data.

Currently lightgrid offers a simple interface for insertion, removal, updating, and querying of entities in the grid.

## Examples

![1,000 entities colliding](./example/gifs/grid_example_1000.gif)

**1,000 entities colliding**

*~2200 FPS ~0.1MB 900x900 grid w/ cell size 25*

---
![5,000 entities colliding](./example/gifs/grid_example_5000.gif)

**5,000 entities colliding**

*~540 FPS ~0.3MB 900x900 grid w/ cell size 10*

---

![10,000 entities colliding](./example/gifs/grid_example_10000.gif)

**10,000 entities colliding**

*~300 FPS ~0.5MB 900x900 grid w/ cell size 8*

---

### **GIFs for the following look like noise so they won't be shown:**

**50,000 entities colliding**

*~58 FPS ~4.7MB 900x900 grid w/ cell size 3*

---

**100,000 entities colliding**

*~30 FPS ~10.0MB 900x900 grid w/ cell size 2*

---

**200,000 entities colliding**

*~14 FPS ~22.7MB 900x900 grid w/ cell size 1*

---

**500,000 entities colliding**

*~4 FPS ~57.2MB 1920x1080 grid w/ cell size 1*

---

*All examples ran on Intel Core i7-10750H @ 2.60GHz*

### Notes

Initializing a SDL with a window of 900x900 on my machine uses ~11.7MB of memory, and ~16.5MB for 1920x1080. These values were subtracted from the above memory usage stats.

SDL seems to have significant memory overhead (>1000B per rectangle) when drawing to the screen. Still not sure if this is my issue or SDL's issue; regardless, the memory usage is measured separately without the rectangles being rendered. SDL is still initialized for its timer though, so I did not bother to disable window creation for the memory benchmarks, opting to subtract the baseline memory usage of SDL.

The performace of these examples is somewhat sensitive to the velocities of the entities. Cramming causes many more collision checks, and cramming is much more likely when the velocities are high relative to the size of the entities. To make the tests more *fair*, the velocity was decreased as the size of the entities was decreased.

## Implementation

ToDo

## Installation

lightgrid is header-only, so just copy `grid.hpp` into your project and you're ready to go.

## Usage

Check out the [example project](./example/lightgrid_example.cpp) to see lightgrid in action, along with some explanation regarding implementation in your own project.

### Usage Considerations

From some basic testing, lightgrid has the best performance when the grid cells are around the size of the smallest entities for dense grids, and around the size of the average entity for more sparse grids. If few collisions are expected, about the same performace will be acheived using cells the size of the space between entities. Regardless, be sure to profile for your own data to get the best results.

While lightgrid makes some considerations to avoid poor performance for large types, the best performace will be achieved by inserting a reference or index to objects rather than the objects themselves. This will improve the performace of insertion and querying.

## Build

### Example

The example project uses SDL2. If it is installed on your machine, the included `FindSDL2.cmake` module should be able to find it.

The example can be built with the following commands:

```console
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build . --target lightgrid_example
```

### Tests

There are currently no tests.

If you would really like to be sure that there are no tests, they can be built with the following commands:

```console
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build . --target lightgrid_test
```

## Future Plans

- Iterating function which is applied to each result of a query wwithout the need to copy the results into another container.
- Quad tree implementation.
