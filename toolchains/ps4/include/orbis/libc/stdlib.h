#pragma once
/* OpenOrbis v0.5.3 compatibility shim.
 *
 * reXGlue's memory_ps4.cpp and LibertyRecomp's user_malloc_ps4.cpp /
 * mspace_ps4.cpp include <orbis/libc/stdlib.h> for the SCE direct-memory
 * constants and the sceLibcMspace API.  The release SDK used by CI does not
 * ship that header (it only has orbis/libkernel.h and orbis/libc.h), so we
 * provide the minimal declarations here.  Kernel entry points come from the
 * real SDK header; the mspace functions are implemented in-tree by
 * LibertyRecomp/os/ps4/mspace_ps4.cpp (C++ linkage, not extern "C").
 */
#include <orbis/libkernel.h>

#include <stddef.h>
#include <stdint.h>

#ifndef SCE_KERNEL_PROT_CPU_READ
#  define SCE_KERNEL_PROT_CPU_READ 0x01
#endif
#ifndef SCE_KERNEL_PROT_CPU_RW
#  define SCE_KERNEL_PROT_CPU_RW 0x02
#endif
#ifndef SCE_KERNEL_PROT_CPU_WRITE
#  define SCE_KERNEL_PROT_CPU_WRITE SCE_KERNEL_PROT_CPU_RW
#endif
#ifndef SCE_KERNEL_PROT_CPU_EXEC
#  define SCE_KERNEL_PROT_CPU_EXEC 0x04
#endif
#ifndef SCE_KERNEL_PROT_CPU_ALL
#  define SCE_KERNEL_PROT_CPU_ALL 0x07
#endif
#ifndef SCE_KERNEL_PROT_GPU_READ
#  define SCE_KERNEL_PROT_GPU_READ 0x10
#endif
#ifndef SCE_KERNEL_PROT_GPU_WRITE
#  define SCE_KERNEL_PROT_GPU_WRITE 0x20
#endif
#ifndef SCE_KERNEL_PROT_GPU_RW
#  define SCE_KERNEL_PROT_GPU_RW 0x30
#endif

#ifndef SCE_KERNEL_WB_ONION
#  define SCE_KERNEL_WB_ONION 0
#endif
#ifndef SCE_KERNEL_WC_GARLIC
#  define SCE_KERNEL_WC_GARLIC 3
#endif
#ifndef SCE_KERNEL_WB_GARLIC
#  define SCE_KERNEL_WB_GARLIC 10
#endif

/* Base PS4 direct-memory ceiling; Neo mode adds 512 MiB. */
#ifndef SCE_KERNEL_MAIN_DMEM_SIZE
#  define SCE_KERNEL_MAIN_DMEM_SIZE (4608ULL * 1024ULL * 1024ULL)
#endif

typedef void* SceLibcMspace;

typedef struct SceLibcMallocManagedSize {
  uint16_t size;
  uint16_t version;
  uint32_t reserved;
  size_t maxSystemSize;
  size_t currentSystemSize;
  size_t maxInuseSize;
  size_t currentInuseSize;
} SceLibcMallocManagedSize;

SceLibcMspace sceLibcMspaceCreate(const char* name, void* base, size_t capacity,
                                  uint32_t flags);
int sceLibcMspaceDestroy(SceLibcMspace msp);
void* sceLibcMspaceMalloc(SceLibcMspace msp, size_t size);
void sceLibcMspaceFree(SceLibcMspace msp, void* ptr);
void* sceLibcMspaceCalloc(SceLibcMspace msp, size_t nelem, size_t size);
void* sceLibcMspaceRealloc(SceLibcMspace msp, void* ptr, size_t size);
void* sceLibcMspaceMemalign(SceLibcMspace msp, size_t boundary, size_t size);
int sceLibcMspacePosixMemalign(SceLibcMspace msp, void** ptr, size_t boundary,
                               size_t size);
void* sceLibcMspaceReallocalign(SceLibcMspace msp, void* ptr, size_t boundary,
                                size_t size);
int sceLibcMspaceMallocStats(SceLibcMspace msp,
                             SceLibcMallocManagedSize* mmsize);
int sceLibcMspaceMallocStatsFast(SceLibcMspace msp,
                                 SceLibcMallocManagedSize* mmsize);
size_t sceLibcMspaceMallocUsableSize(void* ptr);
int sceLibcMspaceIsHeapEmpty(SceLibcMspace msp);
