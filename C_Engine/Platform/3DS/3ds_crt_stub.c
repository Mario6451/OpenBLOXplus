#include <3ds.h>
#include <3ds/env.h>
#include <stdlib.h>
#include <reent.h>

// APT/libctru required symbols (normally provided by crt0)
#ifdef CRT_CIA_MODE
// For CIA: __apt_appid=0 means APT will auto-register the app
// __service_ptr must be NULL for CIA (envIsHomebrew returns false)
u32 __apt_appid          = 0;
u32 __system_runflags    = RUNFLAG_APTREINIT;
void* __service_ptr      = NULL;
#else
// For 3DSX: homebrew mode (no special runflags needed)
u32 __apt_appid          = 0;
u32 __system_runflags    = 0;
void* __service_ptr      = NULL;
#endif
const char* __system_arglist = NULL;

// Heap sizes (used by aptInit/linearMemAlloc)
u32 __ctru_heap_size       = 0x04000000;   // 64MB app heap
u32 __ctru_linear_heap_size = 0x00400000;  // 4MB linear heap

// Linker-provided symbols for BSS initialization
extern char __bss_start__;
extern char __bss_end__;

// C++ global constructors (linker-provided pointers, not C labels)
extern void *const __init_array_start;
extern void *const __init_array_end;

// TLS symbols (linker-provided)
extern char __tls_start;
extern char __tls_end;
extern char __tdata_lma;
extern char __tdata_lma_end;

// Declare main from our codebase
extern int main(int argc, char** argv);

// libctru CRT functions (defined in libctru.a)
void __system_initSyscalls(void);
void __system_allocateHeaps(int argc, void* argv);
void __libc_init_array(void);

// libctru services
Result aptInit(void);

// Entry point for CIA - runs at base address
__attribute__((section(".text.main"), used))
void _start(void) {
    // Initialize kernel syscalls FIRST (required before any svc calls)
    __system_initSyscalls();

#ifdef CRT_CIA_MODE
    // CIA mode: aptInit sets up APT shared memory, heaps, and process memory mappings
    // This MUST happen before BSS zeroing, because the CIA loader only maps p_filesz,
    // not p_memsz. aptInit() maps the full process region via svcControlMemory.
    aptInit();
#else
    // 3DSX mode: allocate heaps manually (no APT shared memory needed)
    __system_allocateHeaps(0, NULL);
#endif

    // Set up TLS for the main thread
    // The linker places TLS in .bss at __tls_start (after regular BSS).
    // Copy .tdata from LMA to VMA, then set TPIDRURO.
    // For CIA: TLS is accessed via __tls_start offset from our data segment.
    unsigned tls_size = (unsigned)(&__tls_end - &__tls_start);

    // Copy .tdata to the TLS VMA location (in .bss)
    unsigned char* tls_src = (unsigned char*)&__tdata_lma;
    unsigned char* tls_dst = (unsigned char*)&__tls_start;
    unsigned tdata_size = (unsigned)(&__tdata_lma_end - &__tdata_lma);
    for (unsigned i = 0; i < tdata_size; i++) {
        tls_dst[i] = tls_src[i];
    }

    // Zero .tbss portion of TLS
    for (unsigned i = tdata_size; i < tls_size; i++) {
        tls_dst[i] = 0;
    }

    // Zero BSS (only up to __tls_start, TLS handles its own zeroing)
    // Before zeroing, ensure the BSS region is mapped. Azahar's CIA loader
    // doesn't map p_memsz > p_filesz, so we explicitly allocate it.
    char* p = &__bss_start__;
    char* end = &__tls_start;
    {
        u32 bss_size = (u32)(end - p);
        if (bss_size > 0) {
            u32 pages = (bss_size + 0xFFF) >> 12;
            u32 base = (u32)(uintptr_t)p & ~0xFFF;
            u32 i;
            for (i = 0; i < pages; i++) {
                u32 addr = base + (i << 12);
                u32 handle = 0;
                Result res = svcControlMemory(&handle, addr, 0, 0x1000,
                    0x3, 0x3); // MapUnmapped(0x3) -> IoMemStatic(0x3)
                // Ignore errors (page may already be mapped)
                (void)res;
                (void)handle;
            }
        }
    }
    while (p < end) {
        *p++ = 0;
    }

    // Run C++ global constructors / libc init (must be after aptInit)
    __libc_init_array();

    // Call main and exit if it returns
    exit(main(0, NULL));
}


