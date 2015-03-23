#ifndef ARENA_ALLOCATOR_H_INCLUDED
#define ARENA_ALLOCATOR_H_INCLUDED

#include <memory>
#include <type_traits>
#include <utility>
#include <cstddef>

#include "BlockProvider.h"

inline constexpr std::size_t firstSetBit(std::size_t i, std::size_t s = 0)
{
    return i & 1ULL ? s : firstSetBit(i >> 1ULL, s + 1);
}

template <class BlockProviderIn>
class ArenaAllocator
{
public:
    using BlockProvider = BlockProviderIn;
    static constexpr std::size_t BlockSize = BlockProvider::BlockSize;

    // alignment MUST be a power of two!
    static constexpr std::size_t alignIndex(std::size_t index, std::size_t alignment) noexcept
    {
        return ((index + alignment - 1) & ~(alignment - 1));
    }

private:
    struct alignas(std::max_align_t) AllocBlock
    {
        char mem[BlockProvider::BlockSize - sizeof(AllocBlock*)];
        AllocBlock *next;
    };
    static_assert(1ULL << firstSetBit(offsetof(AllocBlock, next)) >= alignof(AllocBlock*), "Incompatible block size!");
    static_assert(sizeof(AllocBlock::mem) > 0, "Block size too small!");

    AllocBlock *base;
    AllocBlock *headBlock;
    std::size_t headIndex;

    void* getHintPtr() noexcept
    {
        return static_cast<char*>(static_cast<void*>(headBlock)) + BlockProvider::BlockSize;
    }

    void appendBlock()
    {
        AllocBlock *newBlock = static_cast<AllocBlock*>(BlockProvider::allocateBlock(getHintPtr()));
        if(newBlock == nullptr)
            newBlock = static_cast<AllocBlock*>(BlockProvider::allocateBlock(nullptr));
        if(newBlock == nullptr)
            throw std::bad_alloc();

        newBlock->next = nullptr;
        headBlock->next = newBlock;
        headBlock = newBlock;
        headIndex = 0;
    }
    void nextBlock()
    {
        if(headBlock->next)
        {
            headBlock = headBlock->next;
            headIndex = 0;
        }
        else
            appendBlock();
    }
//
//    static constexpr std::size_t alignIndex(std::size_t index, std::size_t alignment) noexcept
//    {
//        return (alignment * ((index + offsetof(AllocBlock, mem) + alignment - 1) / alignment)) - offsetof(AllocBlock, mem);
//    }

public:
    static constexpr std::size_t MaxAllocationSize = sizeof(AllocBlock::mem);
    static constexpr std::size_t StorageAlignment = 1ULL << firstSetBit(offsetof(AllocBlock, mem));

    ArenaAllocator()
    {
        base = static_cast<AllocBlock*>(BlockProvider::allocateBlock(nullptr));
        if(base == nullptr)
            throw std::bad_alloc();

        base->next = nullptr;
        headBlock = base;
        headIndex = 0;
    }
    ~ArenaAllocator()
    {
        AllocBlock *cur = base, *next;
        while(cur != nullptr)
        {
            next = cur->next;
            BlockProvider::freeBlock(cur);
            cur = next;
        }
    }

    static constexpr bool isStorable(std::size_t size, std::size_t alignment) noexcept
    {
        return alignIndex(0, alignment) + size < MaxAllocationSize;
    }

    void reset() noexcept
    {
        headBlock = base;
        headIndex = 0;
    }

    void* allocateUnsafe(std::size_t s)
    {
        if(s > MaxAllocationSize - headIndex)
            nextBlock();

        void *ptr = &headBlock->mem[headIndex];
        headIndex += s;
        if(headIndex >= MaxAllocationSize)
            nextBlock();

        return ptr;
    }
    void* allocate(std::size_t s)
    {
        if(s == 0) s = 1;

        if(s > MaxAllocationSize)
            throw std::bad_alloc();

        return allocateUnsafe(s);
    }
    void* allocateAlignedUnsafe(std::size_t s, std::size_t alignment)
    {
        std::size_t alignedIndex = alignIndex(headIndex, alignment);
        if(alignedIndex + s >= MaxAllocationSize)
        {
            nextBlock();
            alignedIndex = alignIndex(headIndex, alignment);
        }

        headIndex = alignedIndex;
        return allocateUnsafe(s);
    }
    void* allocateAligned(std::size_t s, std::size_t alignment)
    {
        if(s == 0) s = 1;

        if(!isStorable(s, alignment))
            throw std::bad_alloc();
        return allocateAlignedUnsafe(s, alignment);
    }

    template <typename T, typename... Args>
    T* construct(Args... args)
    {
        static_assert(std::is_trivially_destructible<T>::value, "Type must be trivially destructible!");
        static_assert(isStorable(sizeof(T), alignof(T)), "Type must not be larger than max allocation size!");

        T *result = static_cast<T*>(allocateAlignedUnsafe(sizeof(T), alignof(T)));
        new (result) T(std::forward<Args>(args)...);
        return result;
    }

    template <typename T, typename... Args>
    T* constructUnaligned(Args... args)
    {
        static_assert(std::is_trivially_destructible<T>::value, "Type must be trivially destructible!");
        static_assert(sizeof(T) <= MaxAllocationSize, "Type must not be larger than max allocation size!");

        T *result = static_cast<T*>(allocateUnsafe(sizeof(T)));
        new (result) T(std::forward<Args>(args)...);
        return result;
    }
};

template <std::size_t BlockSize>
using ArenaAllocatorDefault = ArenaAllocator<BlockProviderNewDelete<BlockSize>>;

using ArenaAllocatorSystem = ArenaAllocator<BlockProviderSystem>;

#endif // ARENA_ALLOCATOR_H_INCLUDED
