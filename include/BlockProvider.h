#ifndef BLOCK_PROVIDER_H_INLUCDED
#define BLOCK_PROVIDER_H_INLUCDED

#include <new>

#include <windows.h>

//class PageSizeChecker
//{
//    std::size_t pageSize;
//
//public:
//    PageSizeChecker() noexcept
//    {
//        SYSTEM_INFO sysinfo;
//        GetSystemInfo(&sysinfo);
//        pageSize = sysinfo.dwPageSize;
//    }
//
//    std::size_t getPageSize() const noexcept {return pageSize;}
//};

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

//template <std::size_t BlockSizeIn>
//PageSizeChecker BlockProviderWindows<BlockSizeIn>::pageSizeChecker;

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
