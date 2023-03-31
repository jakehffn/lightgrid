#pragma once

#include <cstdlib>
#include <cstdint>
#include <cassert>
#include <vector>
#include <array>
#include <algorithm>
#include <memory>
#include <type_traits>

#if _GNUC_
#   define _unlikely(expr) (__builtin_expect(!!(expr), 0))
#   define _likely(expr) (__builtin_expect(!!(expr), 1)
#else
#   define _unlikely(expr) (!!(expr))
#   define _likely(expr) (!!(expr))
#endif


// using entity_t = int;
// static constinit const size_t fixed_count = 16;
// using callback_t = void (const entity_t&);

// void traverse(const node<entity_t, fixed_count> &node, callback_t callback) {
//     node.traverse(callback);
// }

// volatile entity_t volatile_store;

// struct callback final {
//     static void function(const entity_t & entity) {
//         volatile_store = entity;
//     }

//     void operator()(const entity_t & __restrict entity) const {
//         function(entity);
//     }
// };

// void traverse2(const node<entity_t, fixed_count> &node) {
//     node.traverse(callback{});
// }

// void traverse3(const node<entity_t, fixed_count> &node) {
//     traverse(node, &callback::function);
// }


namespace lightgrid {

    template<typename Callable, typename StorageT>
    concept traversal_function = requires(const Callable a, StorageT &b) {
        { a(b) } -> std::convertible_to<void>;
    };

    struct bounds {
        int x,y,w,h;
    };

    /**
    * @brief Data-structure for spatial lookup.
    * Divides 2D coordinates into cells, allowing for insertion and lookup for 
    *   an arbitrary type T, based on position. 
    *   Scale is the factor to divide by when converting from world coordinates to entity coordinates. 
    *   ZBitWidth is the number of bits used for z-ordering. This will determine the size of the grid, as the number of cells will be the same of the max value with that number of bits
    */    
    template<class T, size_t Scale, size_t ZBitWidth=8, size_t CellDepth=16u>
    requires (ZBitWidth <= 32)
    class grid {
    public:

        // A pointer type must be stored, but a pointer pointer can also be avoided
        using StorageT = std::remove_pointer_t<T>*;

        void clear();
        void insert(StorageT entity, const bounds& bounds);
        void remove(StorageT entity, const bounds& bounds);
        void update(const bounds& old_bounds, const bounds& new_bounds);
        void traverse(const bounds& bounds, traversal_function<StorageT> auto callback) const;
        
    private:

        class node;

        struct overflow_entity final {
            StorageT entity;
            node* owner;
        };

        // overflow for everything else.
        static std::vector<overflow_entity> global_overflow;

        class node final {
            // The minimum count where the inner loop will 'break' instead of continuing to iterate
            // upon finding a null entity.
            // This is done for improved branch prediction
            static constinit const size_t early_out_min_count = 8;

            // Fixed-size array for the common case.
            // Is kept tightly-packed.
            std::array<StorageT, CellDepth> entities;

            // Count of entities for this cell in the global grid overflow.
            std::uint32_t overflow_count = 0;

        public:
            void traverse(traversal_function<StorageT> auto callback) const {
                for (auto* entity : entities) {
                    if (entity) {
                        callback(*entity);
                    }
                    // You can avoid this in cases where fixed_count is small enough
                    // that the 'else' is detrimental
                    else if constexpr (CellDepth >= early_out_min_count) {
                        break;
                    }
                }

                if _unlikely(overflow_count) [[unlikely]] {
                    // I tried building a span to avoid multiple calls into non-pure
                    // functions of std::vector, but the codegen was universally worse
                    for (auto& entity : global_overflow) {
                        if _unlikely(entity.owner == this) [[unlikely]] {
                            callback(*entity.entity);
                        }
                    }
                }
            }
        };
    };

    template<class T, size_t Scale, size_t ZBitWidth, size_t CellDepth>
    void grid<T, Scale, ZBitWidth, CellDepth>::clear() {

        
    }

    template<class T, size_t Scale, size_t ZBitWidth, size_t CellDepth>
    void grid<T, Scale, ZBitWidth, CellDepth>::insert(StorageT entity, const bounds& bounds) {

       
    }

    template<class T, size_t Scale, size_t ZBitWidth, size_t CellDepth>
    void grid<T, Scale, ZBitWidth, CellDepth>::remove(StorageT entity, const bounds& bounds) {

        
    }

    template<class T, size_t Scale, size_t ZBitWidth, size_t CellDepth>
    void grid<T, Scale, ZBitWidth, CellDepth>::update(const bounds& old_bounds, const bounds& new_bounds) {

        
    }

    template<class T, size_t Scale, size_t ZBitWidth, size_t CellDepth>
    void grid<T, Scale, ZBitWidth, CellDepth>::traverse(const bounds& bounds, traversal_function<StorageT> auto callback) const {

    }
}
