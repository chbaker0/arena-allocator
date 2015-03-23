#ifndef BLOCK_PROVIDER_H_INLUCDED
#define BLOCK_PROVIDER_H_INLUCDED

#include <new>

template <std::size_t BlockSizeIn = 1024>
struct BlockProviderNewDelete
{
    static constexpr std::size_t BlockSize = BlockSizeIn;

    static void* allocateBlock(void *hint) noexcept
    {
        return ::operator new(BlockSize, std::nothrow);
    }
    static void freeBlock(void *block) noexcept
    {
        ::operator delete(block, std::nothrow);
    }
};

#endif // BLOCK_PROVIDER_H_INLUCDED
