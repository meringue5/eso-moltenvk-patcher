#include <dlfcn.h>
#include <stdio.h>

int main(int argc, char** argv) {
    if (argc != 2) {
        return 2;
    }
    void* library = dlopen(argv[1], RTLD_NOW | RTLD_LOCAL);
    if (!library) {
        fprintf(stderr, "dlopen: %s\n", dlerror());
        return 1;
    }
    void* bink_open = dlsym(library, "BinkOpen");
    printf("BinkOpen re-export: %s\n", bink_open ? "yes" : "NO");
    return bink_open ? 0 : 1;
}
