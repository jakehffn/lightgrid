#pragma once

#include <cassert>
#include <vector>
#include <array>
#include <algorithm>
#include <span>
#include <type_traits>

#if defined(__BMI2__) && (defined(__GNUC__) || defined(__llvm__)) && defined(__x86_64__)
    #include <immintrin.h>
#endif

#if defined(__GNUC__)
    #define UNLIKELY(expr) (__builtin_expect((bool)(expr), 0))
    #define LIKELY(expr) (__builtin_expect((bool)(expr), 1))
#else
    #define UNLIKELY(expr) ((bool)(expr))
    #define LIKELY(expr) ((bool)(expr))
#endif

namespace lightgrid {

    template<typename C, typename T>
    concept insertable = requires(C& c, T t) {
        {c.insert(c.end(), std::forward<T>(t))};
    };

    struct bounds {
        int x,y,w,h;
    };

    /**
    * @brief Data-structure for spatial lookup.
    * Divides 2D coordinates into cells, allowing for insertion and lookup for 
    *   an arbitrary type T, based on position.
    *   CellSize determines the number of bounds coordinate units mapped to a single node
    *   ZBitWidth is the number of bits used for z-ordering. This will determine the number of nodes used (2^ZBitWidth)
    */    
    template<class T, int CellSize, size_t ZBitWidth=16u>
    requires (ZBitWidth <= sizeof(size_t)*8)
    class grid {
    public:
        void clear();

        int insert(T element, const bounds& bounds);
        void remove(int element_node, const bounds& bounds);
        void update(int element_node, const bounds& old_bounds, const bounds& new_bounds);
        void reserve(int num);

        template<typename R> 
        requires insertable<R, T>
        R& query(const bounds& bounds, R& results);
        
    private:
        // A mask for wrapping z-orders outside the bounds of the grid
        static constinit const uint64_t wrapping_bit_mask{(1 << ZBitWidth) - 1};

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

        int elementInsert(T element);
        void elementRemove(int element_node);

        void cellInsert(int cell_node, int element_node);
        void cellRemove(int cell_node, int element_node);
        void cellQuery(int cell_node);

        grid<T, CellSize, ZBitWidth>::cell_bounds getCellBounds(const bounds& bounds);
        unsigned modulo(int value, unsigned m);
        unsigned modulo2048(int value);
        void resetQuerySet();

        inline uint64_t z_order(uint32_t x, uint32_t y) const;
        inline uint64_t interleave_with_zeros(uint32_t input) const;
        inline uint64_t interleave(uint32_t x, uint32_t y) const;

        std::vector<T> elements;
        std::vector<node> element_nodes;
        std::vector<node> cell_nodes{std::vector<node>(wrapping_bit_mask)}; // The first cells in this list will never change and will be accessed directly, acting as the 2D list of cells

        std::vector<int> last_query;
        std::vector<bool> query_set;
        size_t query_size{0}; // Used to avoid clearing the vector every frame;

        int free_element_nodes{-1}; // singly linked-list of the free nodes
        int free_cell_nodes{-1}; 
        int num_elements{0};
    };

    // template<class T, int CellSize, size_t ZBitWidth>
    // void grid<T, CellSize, ZBitWidth>::init(int width, int height, int cell_size) {

    //     this->width = width;
    //     this->height = height;
    //     this->cell_size = cell_size;
    //     this->cell_row_size = (width+cell_size-1)/cell_size;
    //     this->cell_column_size = (height+cell_size-1)/cell_size;

    //     this->clear();
    // }

    template<class T, int CellSize, size_t ZBitWidth>
    requires (ZBitWidth <= sizeof(size_t)*8)
    void grid<T, CellSize, ZBitWidth>::clear() {
        this->elements.clear();
        this->element_nodes.clear();
        this->cell_nodes.clear();
        this->cell_nodes.resize(wrapping_bit_mask);
    }

    template<class T, int CellSize, size_t ZBitWidth>
    requires (ZBitWidth <= sizeof(size_t)*8)
    int grid<T, CellSize, ZBitWidth>::insert(T element, const bounds& bounds) {
        assert(this->cell_nodes.size() > 0 && "Insert attempted on uninitialized grid");

        int new_element_node = this->elementInsert(element);

        cell_bounds scaled{getCellBounds(bounds)};

        for (int yy{scaled.y_start}; yy <= scaled.y_end; yy++) {
            for (int xx{scaled.x_start}; xx <= scaled.x_end; xx++) {
                this->cellInsert(this->z_order(xx, yy), new_element_node);     
            }
        }

        this->num_elements++;

        if (this->query_set.size() < this->num_elements) {
            
            this->last_query.resize(this->num_elements);
            this->query_set.resize(this->num_elements);
        }

        return new_element_node;
    }

    template<class T, int CellSize, size_t ZBitWidth>
    requires (ZBitWidth <= sizeof(size_t)*8)
    void grid<T, CellSize, ZBitWidth>::remove(int element_node, const bounds& bounds) {

        assert(this->cell_nodes.size() > 0 && "Remove attempted on uninitialized grid");

        cell_bounds scaled{getCellBounds(bounds)};

        for (int yy{scaled.y_start}; yy <= scaled.y_end; yy++) {
            for (int xx{scaled.x_start}; xx <= scaled.x_end; xx++) {
                this->cellRemove(this->z_order(xx, yy), element_node);     
            }
        }

        this->elementRemove(element_node);
        this->num_elements--;
    }

    template<class T, int CellSize, size_t ZBitWidth>
    requires (ZBitWidth <= sizeof(size_t)*8)
    void grid<T, CellSize, ZBitWidth>::update(int element_node, const bounds& old_bounds, const bounds& new_bounds) {

        assert(this->cell_nodes.size() > 0 && "Update attempted on uninitialized grid");

        // Remove from old bounds
        cell_bounds scaled_old{getCellBounds(old_bounds)};

        for (int yy{scaled_old.y_start}; yy <= scaled_old.y_end; yy++) {
            for (int xx{scaled_old.x_start}; xx <= scaled_old.x_end; xx++) {
                this->cellRemove(this->z_order(xx, yy), element_node);     
            }
        }

        // Insert into new bounds
        cell_bounds scaled_new{getCellBounds(new_bounds)};

        for (int yy{scaled_new.y_start}; yy <= scaled_new.y_end; yy++) {
            for (int xx{scaled_new.x_start}; xx <= scaled_new.x_end; xx++) {
                this->cellInsert(this->z_order(xx, yy), element_node);     
            }
        }
    }

    template<class T, int CellSize, size_t ZBitWidth>
    requires (ZBitWidth <= sizeof(size_t)*8)
    void grid<T, CellSize, ZBitWidth>::reserve(int num) {

        this->elements.reserve(num);
        this->cell_nodes.reserve(wrapping_bit_mask + num);
        this->element_nodes.reserve(num);
    }

    template<class T, int CellSize, size_t ZBitWidth>
    requires (ZBitWidth <= sizeof(size_t)*8)
    template<typename R> 
    requires insertable<R, T>
    R& grid<T, CellSize, ZBitWidth>::query(const bounds& bounds, R& results) {

        assert(this->cell_nodes.size() > 0 && "Query attempted on uninitialized grid");

        cell_bounds scaled{getCellBounds(bounds)};

        for (int yy{scaled.y_start}; yy <= scaled.y_end; yy++) {
            for (int xx{scaled.x_start}; xx <= scaled.x_end; xx++) {
                this->cellQuery(this->z_order(xx, yy));     
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

    template<class T, int CellSize, size_t ZBitWidth>
    requires (ZBitWidth <= sizeof(size_t)*8)
    inline int grid<T, CellSize, ZBitWidth>::elementInsert(T element) {

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

    template<class T, int CellSize, size_t ZBitWidth>
    requires (ZBitWidth <= sizeof(size_t)*8)
    inline void grid<T, CellSize, ZBitWidth>::elementRemove(int element_node) {

        // Make the given element_node the head of the free_element_nodes list
        this->element_nodes[element_node].next = this->free_element_nodes;
        this->free_element_nodes = element_node;
    }

    template<class T, int CellSize, size_t ZBitWidth>
    requires (ZBitWidth <= sizeof(size_t)*8)
    inline void grid<T, CellSize, ZBitWidth>::cellInsert(int cell_node, int element_node) {

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

    template<class T, int CellSize, size_t ZBitWidth>
    requires (ZBitWidth <= sizeof(size_t)*8)
    inline void grid<T, CellSize, ZBitWidth>::cellRemove(int cell_node, int element_node) {

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

    template<class T, int CellSize, size_t ZBitWidth>
    requires (ZBitWidth <= sizeof(size_t)*8)
    inline void grid<T, CellSize, ZBitWidth>::cellQuery(int cell_node) {

        int current_node{this->cell_nodes[cell_node].next};

        while (current_node != -1) {
            assert(current_node < this->cell_nodes.size() && "current_node out of bounds");

            int current_element{this->cell_nodes[current_node].element};

            // Only add to the current query if it has not already been added
            if (!this->query_set[current_element]) {

                this->last_query[this->query_size] = current_element;
                this->query_size++;

                this->query_set[current_element] = true;
            }

            current_node = this->cell_nodes[current_node].next;
        }
    }

    template<class T, int CellSize, size_t ZBitWidth>
    requires (ZBitWidth <= sizeof(size_t)*8)
    inline grid<T, CellSize, ZBitWidth>::cell_bounds grid<T, CellSize, ZBitWidth>::getCellBounds(const bounds& bounds) {
    
        cell_bounds scaled;

        scaled.x_start = bounds.x/CellSize;
        scaled.y_start = bounds.y/CellSize;
        scaled.x_end = (bounds.x + bounds.w)/CellSize;
        scaled.y_end = (bounds.y + bounds.h)/CellSize;

        return scaled;
    }

    // Returns a positive modulo
    template<class T, int CellSize, size_t ZBitWidth>
    requires (ZBitWidth <= sizeof(size_t)*8)
    inline unsigned grid<T, CellSize, ZBitWidth>::modulo(int value, unsigned m) {
        int mod = value % (int)m;
        if (mod < 0) {
            mod += m;
        }
        return mod;
    }

    template<class T, int CellSize, size_t ZBitWidth>
    requires (ZBitWidth <= sizeof(size_t)*8)
    inline unsigned grid<T, CellSize, ZBitWidth>::modulo2048(int value) {
        return this->modulo(value, 2048);
    }

    template<class T, int CellSize, size_t ZBitWidth>
    requires (ZBitWidth <= sizeof(size_t)*8)
    inline void grid<T, CellSize, ZBitWidth>::resetQuerySet() {

        for (int i{0}; i < this->query_size; i++) {
            this->query_set[this->last_query[i]] = false;
        }

        this->query_size = 0;
    }

    template<class T, int CellSize, size_t ZBitWidth>
    requires (ZBitWidth <= sizeof(size_t)*8)
    inline uint64_t grid<T, CellSize, ZBitWidth>::z_order(uint32_t x, uint32_t y) const {
        return this->interleave(x, y) & wrapping_bit_mask;
    }

    template<class T, int CellSize, size_t ZBitWidth>
    requires (ZBitWidth <= sizeof(size_t)*8)
    inline uint64_t grid<T, CellSize, ZBitWidth>::interleave_with_zeros(uint32_t input) const {
        uint64_t res = input;
        res = (res | (res << 16)) & 0x0000ffff0000ffff;
        res = (res | (res << 8 )) & 0x00ff00ff00ff00ff;
        res = (res | (res << 4 )) & 0x0f0f0f0f0f0f0f0f;
        res = (res | (res << 2 )) & 0x3333333333333333;
        res = (res | (res << 1 )) & 0x5555555555555555;
        return res;
    }

    // In the case that _pdep_u64 is unavailable, use a traditional algorithm for interleaving
    template<class T, int CellSize, size_t ZBitWidth>
    requires (ZBitWidth <= sizeof(size_t)*8)
    #if defined(__GNUC__) || defined(__llvm__)
        __attribute__ ((target ("default")))
    #endif
    inline uint64_t grid<T, CellSize, ZBitWidth>::interleave(uint32_t x, uint32_t y) const {
        return this->interleave_with_zeros(x) | (this->interleave_with_zeros(y) << 1);
    }

    #if defined(__BMI2__) && (defined(__GNUC__) || defined(__llvm__)) && defined(__x86_64__)
        template<class T, int CellSize, size_t ZBitWidth>
        __attribute__ ((target ("bmi2")))
        inline uint64_t grid<T, CellSize, ZBitWidth>::interleave(uint32_t x, uint32_t y) const {
            return _pdep_u64(y,0xaaaaaaaaaaaaaaaa) | _pdep_u64(x, 0x5555555555555555);
        }
    #endif
}
