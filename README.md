# lightgrid

lightgrid is a header-only implementation of a grid data-structure for lookups on spacial data.

## Examples 
---

![1,000 entities colliding](./example/gifs/grid_example_1000.gif)

**1,000 entities colliding**

*~2200 FPS ~10MB 900x900 grid w/ cell size 25*

---
![5,000 entities colliding](./example/gifs/grid_example_5000.gif)

**5,000 entities colliding**

*~540 FPS ~17MB 900x900 grid w/ cell size 10*

---

![10,000 entities colliding](./example/gifs/grid_example_10000.gif)

**10,000 entities colliding**

*~300 FPS ~25MB 900x900 grid w/ cell size 8*

---

![50,000 entities colliding](./example/gifs/grid_example_50000.gif)

**50,000 entities colliding**

*~58 FPS ~88MB 900x900 grid w/ cell size 3*

---

GIFs for the following looks like noise so they won't be shown.

**100,000 entities colliding**

*~30 FPS ~165MB 900x900 grid w/ cell size 2*

---

**200,000 entities colliding**

*~14 FPS ~320MB 900x900 grid w/ cell size 1*

---

**500,000 entities colliding**

*~4 FPS ~1200MB 1920x1080 grid w/ cell size 1*

---
### Note

The performace of these examples is somewhat sensitive to the velocities of the entities. Cramming causes many more collision checks, and cramming is much more likely when the velocities are high relative to the size of the entities. To make the tests more *fair*, the velocity was decreased as the size of the entities was decreased.

## Implementation
ToDo

## Usage

To use lightgrid, just copy `grid.hpp` into your project.

## Build

### Example

The example project uses SDL2. If it is installed on your machine, the included `FindSDL2.cmake` module should be able to find it.

The example can be built with the following commands:

```console
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build . --target lightgrid_example
```

### Benchmarks

Some preliminary benchmarks can be seen [here](./test/benchmark/README.md). The desired benchmarks have not been created yet, but in the future, feel free contribute benchmarks for your machine.

The benchmark *(as unhelpful as it is at the moment)* can be built with the following commands:

```console
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build . --target lightgrid_benchmark
```

### Tests

There are currently no tests.

If you would really like to be sure that there are no tests, they can be built with the following commands:

```console
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build . --target lightgrid_test
```
