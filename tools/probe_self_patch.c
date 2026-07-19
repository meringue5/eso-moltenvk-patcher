#include <mach/mach.h>
#include <mach/mach_error.h>
#include <mach/mach_vm.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

__attribute__((noinline)) static int original(void) {
    return 1;
}

__attribute__((noinline)) static int replacement(void) {
    return 2;
}

int main(void) {
    int before = original();
    long page_size = sysconf(_SC_PAGESIZE);
    uintptr_t page = (uintptr_t)(void*)&original & ~((uintptr_t)page_size - 1);
    kern_return_t result = mach_vm_protect(
        mach_task_self(), page, (mach_vm_size_t)page_size, FALSE,
        VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXECUTE | VM_PROT_COPY);
    if (result != KERN_SUCCESS) {
        fprintf(stderr, "mach_vm_protect RWX+COW: %s\n", mach_error_string(result));
        return 1;
    }
    uint8_t patch[12] = {0x48, 0xb8};
    uint64_t destination = (uint64_t)(uintptr_t)(void*)&replacement;
    memcpy(&patch[2], &destination, sizeof(destination));
    patch[10] = 0xff;
    patch[11] = 0xe0;
    memcpy((void*)(uintptr_t)(void*)&original, patch, sizeof(patch));
    __builtin___clear_cache((char*)(void*)&original, (char*)(void*)&original + sizeof(patch));
    mach_vm_protect(mach_task_self(), page, (mach_vm_size_t)page_size, FALSE,
                    VM_PROT_READ | VM_PROT_EXECUTE);
    int after = original();
    printf("Rosetta entry patch: before=%d after=%d\n", before, after);
    return before == 1 && after == 2 ? 0 : 1;
}
