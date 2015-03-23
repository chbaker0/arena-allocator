#ifndef BLOCK_PROVIDER_H_INLUCDED
#define BLOCK_PROVIDER_H_INLUCDED

#include <new>

#ifdef _WIN32
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef NOMINMAX
#endif // _WIN32

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

#ifdef _WIN32
template <std::size_t BlockSizeIn = 2097152>    // 2 MiB
struct BlockProviderWindows
{
    static constexpr std::size_t BlockSize = BlockSizeIn;

    static void* allocateBlock(void *hint) noexcept
    {
        return VirtualAlloc(hint, BlockSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    }
    static void freeBlock(void *block) noexcept
    {
        VirtualFree(block, 0, MEM_RELEASE);
    }
};

using BlockProviderSystem = BlockProviderWindows<>;
#else
using BlockProviderSystem = BlockProviderNewDelete<>;
#endif // _WIN32

#endif // BLOCK_PROVIDER_H_INLUCDED
