#pragma once

#include <cassert>
#include <vector>
#include <unordered_set>
#include <algorithm>
#include <span>

namespace lightgrid {

    template<typename C, typename T>
    concept insertable = requires(C& c, T t) {
        {c.insert(c.end(), std::forward<T>(t))};
    };

    struct bounds {
        int x,y,w,h;
    };

    struct cell_bounds {
        int x_start, x_end, y_start, y_end;
    };

    struct node {
        node() {};
        node(int element) : element{ element } {};
        node(int element, int next) : element{ element }, next{ next } {};
        // Index of element in element list
        int element=-1;
        // Either the index of the next element in the cell or the next element in the free list
        // -1 if the end of either list
        int next=-1; 
    };
    /**
    * @brief Data-structure for spacial lookup.
    * Divides 2D coordinates into cells, allowing for insertion and lookup for 
    *   an arbitrary type T, based on position.
    */    
    template<class T>
    class grid {
    public:
        void init(int width, int height, int cell_size);
        void clear();

        int insert(T element, const bounds& bounds);
        void remove(int element_node, const bounds& bounds);
        void update(int element_node, const bounds& old_bounds, const bounds& new_bounds);
        void reserve(int num);

        template<typename R> 
        requires insertable<R, T>
        R& query(const bounds& bounds, R& results);
        
    private:
        int elementInsert(T element);
        void elementRemove(int element_node);

        void cellInsert(int cell_node, int element_node);
        void cellRemove(int cell_node, int element_node);
        void cellQuery(int cell_node);

        cell_bounds clampCellBounds(const bounds& bounds);
        void resetQuerySet();

        std::vector<T> elements;
        std::vector<node> element_nodes;
        std::vector<node> cell_nodes; // The first cells in this list will never change and will be accessed directly, acting as the 2D list of cells

        std::vector<bool> parent_grid;

        std::vector<int> last_query;
        std::vector<bool> query_set;
        size_t query_size{0}; // Used to avoid clearing the vector every frame;

        int free_element_nodes{-1}; // singly linked-list of the free nodes
        int free_cell_nodes{-1}; 

        int width{0};
        int height{0};
        int cell_size{0};
        int cell_row_size{0};
        int cell_column_size{0};
        int num_cells{0};
        int num_elements{0};
    };

    template<class T>
    void grid<T>::init(int width, int height, int cell_size) {

        this->width = width;
        this->height = height;
        this->cell_size = cell_size;
        this->cell_row_size = (width+cell_size-1)/cell_size;
        this->cell_column_size = (height+cell_size-1)/cell_size;

        this->clear();
    }

    template<class T>
    void grid<T>::clear() {

        this->elements.clear();
        this->element_nodes.clear();
        this->cell_nodes.clear();

        this->num_cells = this->cell_row_size * ((this->height+this->cell_size-1)/this->cell_size);
        this->cell_nodes.resize(num_cells);
    }

    template<class T>
    int grid<T>::insert(T element, const bounds& bounds) {

        assert(this->cell_nodes.size() > 0 && "Insert attempted on uninitialized grid");

        int new_element_node = this->elementInsert(element);

        cell_bounds clamped{clampCellBounds(bounds)};

        for (int yy{clamped.y_start}; yy <= clamped.y_end; yy++) {
            for (int xx{clamped.x_start}; xx <= clamped.x_end; xx++) {
                this->cellInsert(yy*this->cell_row_size + xx, new_element_node);     
            }
        }

        this->num_elements++;

        if (this->query_set.size() < this->num_elements) {
            
            this->last_query.resize(this->num_elements);
            this->query_set.resize(this->num_elements);
        }

        return new_element_node;
    }

    template<class T>
    void grid<T>::remove(int element_node, const bounds& bounds) {

        assert(this->cell_nodes.size() > 0 && "Remove attempted on uninitialized grid");

        cell_bounds clamped{clampCellBounds(bounds)};

        for (int yy{clamped.y_start}; yy <= clamped.y_end; yy++) {
            for (int xx{clamped.x_start}; xx <= clamped.x_end; xx++) {
                this->cellRemove(yy*this->cell_row_size + xx, element_node);     
            }
        }
        this->elementRemove(element_node);

        this->num_elements--;
    }

    template<class T>
    void grid<T>::update(int element_node, const bounds& old_bounds, const bounds& new_bounds) {

        assert(this->cell_nodes.size() > 0 && "Update attempted on uninitialized grid");

        // TODO: make a specialized function for updates
        //      Element node could be immediately reused without it every being added to freenode lists

        this->remove(element_node, old_bounds);
        this->insert(this->elements[this->element_nodes[element_node].element], new_bounds);
    }

    template<class T>
    void grid<T>::reserve(int num) {

        this->elements.reserve(num);
        this->cell_nodes.reserve(num);
        this->element_nodes.reserve(num);
    }

    template<class T>
    template<typename R> 
    requires insertable<R, T>
    R& grid<T>::query(const bounds& bounds, R& results) {

        assert(this->cell_nodes.size() > 0 && "Query attempted on uninitialized grid");

        cell_bounds clamped{clampCellBounds(bounds)};

        for (int yy{clamped.y_start}; yy <= clamped.y_end; yy++) {
            for (int xx{clamped.x_start}; xx <= clamped.x_end; xx++) {
                this->cellQuery(yy*this->cell_row_size + xx);     
            }
        }

        std::span query_span{last_query.begin(), this->query_size};
        
        std::transform(query_span.begin(), query_span.end(), std::inserter(results, results.end()), 
            ([this](const auto& element) {
                return this->elements[this->element_nodes[element].element];
            })
        );

        this->resetQuerySet();

        return results;
    }

    template<class T>
    inline int grid<T>::elementInsert(T element) {

        int new_element_node;

        if (this->free_element_nodes != -1) {

            // Use the first item in the linked list and move the head to the next free node
            new_element_node = this->free_element_nodes;
            free_element_nodes = this->element_nodes[this->free_element_nodes].next;

            this->elements[element_nodes[new_element_node].element] = element;

        } else {

            // Create new element node and add reference to index into elements list
            new_element_node = this->element_nodes.size();
            this->element_nodes.emplace_back(this->elements.size());
            this->elements.push_back(element);
        }

        return new_element_node;
    }

    template<class T>
    inline void grid<T>::elementRemove(int element_node) {

        // Make the given element_node the head of the free_element_nodes list
        this->element_nodes[element_node].next = this->free_element_nodes;
        this->free_element_nodes = element_node;
    }

    template<class T>
    inline void grid<T>::cellInsert(int cell_node, int element_node) {

        if (this->free_cell_nodes != -1) {

            // Use element of free node as scratchpad for next free node
            this->cell_nodes[this->free_cell_nodes].element = this->cell_nodes[this->free_cell_nodes].next;

            // Move head of cell's linked list to the free node
            this->cell_nodes[this->free_cell_nodes].next = this->cell_nodes[cell_node].next;
            this->cell_nodes[cell_node].next = this->free_cell_nodes;

            // Move head of free nodes to the value in scratchpad and set head of cell to the element node
            this->free_cell_nodes = this->cell_nodes[this->free_cell_nodes].element;
            this->cell_nodes[this->cell_nodes[cell_node].next].element = element_node;

        } else {

            // Create new cell node and add reference to index into element_nodes list
            this->cell_nodes.emplace_back(element_node, this->cell_nodes[cell_node].next);
            this->cell_nodes[cell_node].next = this->cell_nodes.size() - 1;

        }
    }

    template<class T>
    inline void grid<T>::cellRemove(int cell_node, int element_node) {

        int previous_node{cell_node};
        int current_node{this->cell_nodes[cell_node].next};

        // Find the element_node in the cell_node's list
        while (this->cell_nodes[current_node].element != element_node) {
            previous_node = current_node;
            current_node = this->cell_nodes[current_node].next;
        }

        // Remove the cell_node containing element_node
        this->cell_nodes[previous_node].next = this->cell_nodes[current_node].next;
        // Make the currentNode the head of the free_cell_nodes list 
        this->cell_nodes[current_node].next = this->free_cell_nodes;
        this->free_cell_nodes = current_node;
    }

    template<class T>
    inline void grid<T>::cellQuery(int cell_node) {

        int current_node{this->cell_nodes[cell_node].next};

        while (current_node != -1) {
            assert(current_node < this->cell_nodes.size() && "current_node out of bounds");

            int current_element{this->cell_nodes[current_node].element};

            // Only add to the current query if it has not already been added
            if (!this->query_set[current_element]) { [[unlikely]] // Don't know if this is really doing anything?

                this->last_query[this->query_size] = current_element;
                this->query_size++;

                this->query_set[current_element] = true;
            }

            current_node = this->cell_nodes[current_node].next;
        }
    }

    template<class T>
    inline cell_bounds grid<T>::clampCellBounds(const bounds& bounds) {
    
        cell_bounds clamped;

        clamped.x_start = std::clamp(bounds.x/this->cell_size, 
            0, this->cell_row_size-1);
        clamped.y_start = std::clamp(bounds.y/this->cell_size, 
            0, this->cell_column_size-1);
        clamped.x_end = std::clamp((bounds.x + bounds.w)/this->cell_size,
            0, this->cell_row_size-1);
        clamped.y_end = std::clamp((bounds.y + bounds.h)/this->cell_size,
            0, this->cell_column_size-1);

        return clamped;
    }

    template<class T>
    inline void grid<T>::resetQuerySet() {

        for (int i{0}; i < this->query_size; i++) {
            this->query_set[this->last_query[i]] = false;
        }

        this->query_size = 0;
    }
}