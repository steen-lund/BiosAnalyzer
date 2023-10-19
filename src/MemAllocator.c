#include "MemAllocator.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// if on a 64bit system, we need to use mmap to allocate memory within the first 4GB of memory
#if __x86_64__ || __ppc64__ || __powerpc64__ || __aarch64__
#include <sys/mman.h>
#endif

uintptr_t allocateIn4GBRange(uint32_t size)
{
#if __x86_64__ || __ppc64__ || __powerpc64__ || __aarch64__
    // Using mmap to allocate within the first 4GB of memory'
    // This is because the PCI configuration only supports 32 bit addresses
    void* addr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_32BIT | MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (addr == MAP_FAILED)
    {
        printf("Could not allocate memory\n");
        return 0;
    }

    uint64_t _addr = (uint64_t)addr;
    uint32_t addr32 = (uint32_t)_addr & 0x00000000ffffffff;
    if (_addr != addr32)
    {
        munmap(addr, size);
        printf("Could not allocate memory\n");
        return 0;
    }
#else
    // On 32 bit systems, we can just use malloc
    void* addr = malloc(size);
    if (addr == NULL)
    {
        printf("Could not allocate memory\n");
        return NULL;
    }

    uint32_t addr32 = (uint32_t)addr;
#endif

    return (uintptr_t)addr32;   
}

void freeIn4GBRange(void* addr, uint32_t size)
{
#if __x86_64__ || __ppc64__ || __powerpc64__ || __aarch64__
    munmap(addr, size);
#else
    free(addr);
#endif
}
