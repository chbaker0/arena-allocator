#ifndef ARENA_ALLOCATOR_H_INCLUDED
#define ARENA_ALLOCATOR_H_INCLUDED

#include <type_traits>
#include <utility>

#include "BlockProvider.h"

template <class BlockProviderIn>
class ArenaAllocator
{
public:
    using BlockProvider = BlockProviderIn;

private:
    struct AllocBlock
    {
        AllocBlock *next;
        char mem[BlockProvider::BlockSize - sizeof(AllocBlock*)];
    };
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

public:
    static constexpr std::size_t BlockSize = BlockProvider::BlockSize;
    static constexpr std::size_t MaxAllocationSize = sizeof(AllocBlock::mem);

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

    void reset() noexcept
    {
        headBlock = base;
        headIndex = 0;
    }

    void* allocate(std::size_t s)
    {
        if(s == 0) s = 1;

        if(s > MaxAllocationSize)
            return nullptr;
        else if(s > MaxAllocationSize - headIndex)
            nextBlock();

        void *ptr = &headBlock->mem[headIndex];
        headIndex += s;
        if(headIndex >= MaxAllocationSize)
            nextBlock();

        return ptr;
    }

    template <typename T, typename... Args>
    T* construct(Args... args)
    {
        static_assert(std::is_trivially_destructible<T>::value, "Type must be trivially destructible!");
        static_assert(sizeof(T) <= MaxAllocationSize, "Type must not be larger than max allocation size!");

        T *result = static_cast<T*>(allocate(sizeof(T)));
        new (result) T(std::forward<Args>(args)...);
        return result;
    }
};

template <std::size_t BlockSize>
using ArenaAllocatorDefault = ArenaAllocator<BlockProviderNewDelete<BlockSize>>;

using ArenaAllocatorSystem = ArenaAllocator<BlockProviderWindows<>>;

#endif // ARENA_ALLOCATOR_H_INCLUDED
