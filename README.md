# lightgrid

**lightgrid is an header-only implementation of a grid data-structure for spacial lookups, useful for collision detection and render-culling, implemented using C++20 features.**

This branch avoids the usage of a linked-list. Currently, the performance is comparable to the main branch, but is still slightly slower. For now, the benefits are that there is no need to initialize the grid, the interface is much simpler, and there is a traversal function.

## Installation

lightgrid is header-only, so just copy `grid.hpp` into your project and you're ready to go.

## Usage

Check out the [example project](./example/lightgrid_example.cpp) to see lightgrid in action, along with some explanation regarding implementation in your own project.

## Build

### Example

The example project uses SDL2. If it is installed on your machine, the included `FindSDL2.cmake` module should be able to find it.

The example can be built with the following commands:

```console
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build . --target lightgrid_example
```