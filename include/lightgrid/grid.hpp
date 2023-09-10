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

namespace lightgrid {
    template<typename Callable, typename T>
    concept traversal_function = requires(const Callable a, T &b) {
        { a(b) } -> std::convertible_to<void>;
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
    template<class T, int CellSize, size_t ZBitWidth=16u, size_t CellDepth=16u>
    requires (ZBitWidth <= sizeof(size_t)*8 && CellDepth < 256)
    class grid {
    public:
        void clear();
        void insert(T entity, const bounds& bounds);
        void remove(T entity, const bounds& bounds);
        void update(T entity, const bounds& old_bounds, const bounds& new_bounds);
        void traverse(const bounds& bounds, traversal_function<T> auto callback);
        
    private:
        // A mask for wrapping z-orders outside the bounds of the grid
        static constinit const uint64_t wrapping_bit_mask{(1 << ZBitWidth) - 1};

        class node;

        struct overflow_entity final {
            T entity;
            node* owner;
        };

        inline void iter_bounds(const bounds& bounds, traversal_function<uint64_t> auto iter_func);
        inline uint64_t z_order(uint32_t x, uint32_t y) const;
        inline uint64_t interleave_with_zeros(uint32_t input) const;
        inline uint64_t interleave(uint32_t x, uint32_t y) const;

        std::array<node, 1 << ZBitWidth> nodes;
        // overflow for everything else.
        inline static std::vector<overflow_entity> global_overflow;

        class node final {
            // The minimum count where the inner loop will 'break' instead of continuing to iterate
            // upon finding a null entity.
            // This is done for improved branch prediction
            static constinit const size_t early_out_min_count = 8;

            // Fixed-size array for the common case.
            // Is kept tightly-packed.
            std::array<T, CellDepth> entities;
            std::uint8_t size{0};

            // Count of entities in global_overflow for cell
            std::uint32_t overflow_count{0};

        public:
            void insert(T new_entity) {
                if LIKELY(this->size < CellDepth) [[likely]] {
                    this->entities[this->size] = new_entity;
                    this->size++;
                } else {
                    grid<T, CellSize, ZBitWidth, CellDepth>::global_overflow.emplace_back(new_entity, this);
                    this->overflow_count++;
                }
            }

            void remove(T old_entity) {
                // Swap the old entity with the last in the cell     
                for (size_t curr{0}; curr < CellDepth; curr++) {
                    if (this->entities[curr] == old_entity) {
                        this->entities[curr] = this->entities[this->size-1];
                        this->size--;
                        return;
                    }
                }
                for (
                    auto it{grid<T, CellSize, ZBitWidth, CellDepth>::global_overflow.begin()}; 
                    it != grid<T, CellSize, ZBitWidth, CellDepth>::global_overflow.end();
                    it++
                ) {
                    if UNLIKELY(it->owner == this) [[unlikely]] {
                        grid<T, CellSize, ZBitWidth, CellDepth>::global_overflow.erase(it);
                        this->overflow_count--;
                        return;
                    }
                }
            }

            void traverse(traversal_function<T> auto callback) const {
                if constexpr (CellDepth >= early_out_min_count) {
                    for (size_t curr{0}; curr < this->size; curr++) {
                        callback(this->entities[curr]);
                    }
                } else {
                    for (size_t curr{0}; curr < CellDepth; curr++) {
                        if (curr < this->size) {
                            callback(this->entities[curr]);
                        }
                    }
                }

                if UNLIKELY(this->overflow_count) [[unlikely]] {
                    // (From Reddit guy)
                    // I tried building a span to avoid multiple calls into non-pure
                    // functions of std::vector, but the codegen was universally worse
                    for (auto& entity : grid<T, CellSize, ZBitWidth, CellDepth>::global_overflow) {
                        if UNLIKELY(entity.owner == this) [[unlikely]] {
                            callback(entity.entity);
                        }
                    }
                }
            }
        };
    };

    template<class T, int CellSize, size_t ZBitWidth, size_t CellDepth>
    requires (ZBitWidth <= sizeof(size_t)*8 && CellDepth < 256)
    void grid<T, CellSize, ZBitWidth, CellDepth>::clear() {
        for (auto& node : this->nodes) {
            node.size = 0;
            node.overflow_count = 0;
        }
        grid<T, CellSize, ZBitWidth, CellDepth>::global_overflow.clear();
    }

    template<class T, int CellSize, size_t ZBitWidth, size_t CellDepth>
    requires (ZBitWidth <= sizeof(size_t)*8 && CellDepth < 256)
    void grid<T, CellSize, ZBitWidth, CellDepth>::insert(T entity, const bounds& bounds) {
        this->iter_bounds(bounds, [this, entity](int cell) {
            this->nodes[cell].insert(entity);
        });
    }

    template<class T, int CellSize, size_t ZBitWidth, size_t CellDepth>
    requires (ZBitWidth <= sizeof(size_t)*8 && CellDepth < 256)
    void grid<T, CellSize, ZBitWidth, CellDepth>::remove(T entity, const bounds& bounds) {
        this->iter_bounds(bounds, [this, entity](int cell) {
            this->nodes[cell].remove(entity);
        });
    }

    template<class T, int CellSize, size_t ZBitWidth, size_t CellDepth>
    requires (ZBitWidth <= sizeof(size_t)*8 && CellDepth < 256)
    void grid<T, CellSize, ZBitWidth, CellDepth>::update(T entity, const bounds& old_bounds, const bounds& new_bounds) {
        // I tried a few ways to only update what has changed, but nothing I tested was faster than this
        this->remove(entity, old_bounds);
        this->insert(entity, new_bounds);
    }

    template<class T, int CellSize, size_t ZBitWidth, size_t CellDepth>
    requires (ZBitWidth <= sizeof(size_t)*8 && CellDepth < 256)
    void grid<T, CellSize, ZBitWidth, CellDepth>::traverse(const bounds& bounds, traversal_function<T> auto callback) {
        this->iter_bounds(bounds, [this, callback](int cell) {
            this->nodes[cell].traverse(callback);
        });
    }

    template<class T, int CellSize, size_t ZBitWidth, size_t CellDepth>
    requires (ZBitWidth <= sizeof(size_t)*8 && CellDepth < 256)
    inline void grid<T, CellSize, ZBitWidth, CellDepth>::iter_bounds(const bounds& bounds, traversal_function<uint64_t> auto iter_func) {
        lightgrid::bounds iter_bounds{
            (bounds.x + CellSize - 1) / CellSize,
            (bounds.y + CellSize - 1) / CellSize,
            (bounds.w + CellSize - 1) / CellSize,
            (bounds.h + CellSize - 1) / CellSize,
        };
        for (int it_y{0}; it_y <= iter_bounds.h; it_y++) {
            for (int it_x{0}; it_x <= iter_bounds.w; it_x++) {
                iter_func(z_order(iter_bounds.x + it_x, iter_bounds.y + it_y));
            }
        }
    }

    template<class T, int CellSize, size_t ZBitWidth, size_t CellDepth>
    requires (ZBitWidth <= sizeof(size_t)*8 && CellDepth < 256)
    inline uint64_t grid<T, CellSize, ZBitWidth, CellDepth>::z_order(uint32_t x, uint32_t y) const {
        return this->interleave(x, y) & grid<T, CellSize, ZBitWidth, CellDepth>::wrapping_bit_mask;
    }

    template<class T, int CellSize, size_t ZBitWidth, size_t CellDepth>
    requires (ZBitWidth <= sizeof(size_t)*8 && CellDepth < 256)
    inline uint64_t grid<T, CellSize, ZBitWidth, CellDepth>::interleave_with_zeros(uint32_t input) const {
        uint64_t res = input;
        res = (res | (res << 16)) & 0x0000ffff0000ffff;
        res = (res | (res << 8 )) & 0x00ff00ff00ff00ff;
        res = (res | (res << 4 )) & 0x0f0f0f0f0f0f0f0f;
        res = (res | (res << 2 )) & 0x3333333333333333;
        res = (res | (res << 1 )) & 0x5555555555555555;
        return res;
    }

    // In the case that _pdep_u64 is unavailable, use a traditional algorithm for interleaving
    template<class T, int CellSize, size_t ZBitWidth, size_t CellDepth>
    requires (ZBitWidth <= sizeof(size_t)*8 && CellDepth < 256)
    #if defined(__GNUC__) || defined(__llvm__)
        __attribute__ ((target ("default")))
    #endif
    inline uint64_t grid<T, CellSize, ZBitWidth, CellDepth>::interleave(uint32_t x, uint32_t y) const {
        return this->interleave_with_zeros(x) | (this->interleave_with_zeros(y) << 1);
    }

    #if defined(__BMI2__) && (defined(__GNUC__) || defined(__llvm__)) && defined(__x86_64__)
        template<class T, int CellSize, size_t ZBitWidth, size_t CellDepth>
        __attribute__ ((target ("bmi2")))
        inline uint64_t grid<T, CellSize, ZBitWidth, CellDepth>::interleave(uint32_t x, uint32_t y) const {
            return _pdep_u64(y,0xaaaaaaaaaaaaaaaa) | _pdep_u64(x, 0x5555555555555555);
        }
    #endif
}
