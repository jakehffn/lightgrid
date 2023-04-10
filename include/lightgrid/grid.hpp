// When using GCC, compile with the -mbmi2 flag in order to take advantage of 

#pragma once

#include <cstdlib>
#include <cstdint>
#include <cassert>
#include <vector>
#include <array>
#include <algorithm>
#include <memory>
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
    template<class T, size_t Scale, size_t ZBitWidth=8u, size_t CellDepth=16u>
    requires (ZBitWidth <= sizeof(size_t)*8)
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

        inline uint64_t interleave_with_zeros(uint32_t input) const;
        inline uint64_t interleave(uint32_t x, uint32_t y) const;

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

            // Count of entities in global_overflow for cell
            std::uint32_t overflow_count = 0;

        public:
            void insert(StorageT new_entity) {

                for (auto& entity : this->entities) {
                    if (!entity) {
                        entity = new_entity;
                        return;
                    }
                }  

                global_overflow.emplace_back(entity, this);
                this->overflow_count++;
            }

            void remove(StorageT old_entity) {

                // Swap the old entity with the last in the cell
                size_t old_pos;
                
                for (size_t curr{0}; curr < CellDepth; curr++) {
                    if (this->entities[curr] == old_entity) {
                        old_pos = curr;

                    } else if (!this->entities[curr]) {

                        this->entities[old_pos] = this->entities[curr-1];
                        return;
                    }
                }

                for (auto it{global_overflow.begin()}; it != global_overflow.end();) {
                    if UNLIKELY(entity.owner == this) [[unlikely]] {

                        global_overflow.erase(it);
                        this->overflow_count--;
                        return;
                    }
                    it++;
                }
            }

            void traverse(traversal_function<StorageT> auto callback) const {

                for (auto* entity : this->entities) {
                    if (entity) {
                        callback(*entity);
                    }
                    // You can avoid this in cases where fixed_count is small enough
                    // that the 'else' is detrimental
                    else if constexpr (CellDepth >= early_out_min_count) {
                        break;
                    }
                }

                if UNLIKELY(this->overflow_count) [[unlikely]] {
                    // I tried building a span to avoid multiple calls into non-pure
                    // functions of std::vector, but the codegen was universally worse
                    for (auto& entity : this->global_overflow) {
                        if UNLIKELY(entity.owner == this) [[unlikely]] {
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

    template<class T, size_t Scale, size_t ZBitWidth, size_t CellDepth>
    inline uint64_t grid<T, Scale, ZBitWidth, CellDepth>::interleave_with_zeros(uint32_t input) const {

        uint64_t res = input;
        res = (res | (res << 16)) & 0x0000ffff0000ffff;
        res = (res | (res << 8 )) & 0x00ff00ff00ff00ff;
        res = (res | (res << 4 )) & 0x0f0f0f0f0f0f0f0f;
        res = (res | (res << 2 )) & 0x3333333333333333;
        res = (res | (res << 1 )) & 0x5555555555555555;
        return res;
    }

    // In the case that _pdep_u64 is unavailable, use a traditional algorithm for interleaving
    template<class T, size_t Scale, size_t ZBitWidth, size_t CellDepth>
    #if defined(__GNUC__) || defined(__llvm__)
        __attribute__ ((target ("default")))
    #endif
    inline uint64_t grid<T, Scale, ZBitWidth, CellDepth>::interleave(uint32_t x, uint32_t y) const {
        return interleave_with_zeros(x) | (interleave_with_zeros(y) << 1);
    }

    #if defined(__BMI2__) && (defined(__GNUC__) || defined(__llvm__)) && defined(__x86_64__)
        template<class T, size_t Scale, size_t ZBitWidth, size_t CellDepth>
        __attribute__ ((target ("bmi2")))
        inline uint64_t grid<T, Scale, ZBitWidth, CellDepth>::interleave(uint32_t x, uint32_t y) const {
            return _pdep_u64(y,0xaaaaaaaaaaaaaaaa) | _pdep_u64(x, 0x5555555555555555);
        }
    #endif
}
