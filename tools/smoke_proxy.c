#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char** argv) {
    if (argc != 2) {
        return 2;
    }
    // Loading the bridge runs its constructor, which writes a run record.
    // Refuse to load it unless that record is redirected away from the
    // player's production log (see TESO4M4_LOG_DIR in mvk_log_file.h).
    const char* log_dir = getenv("TESO4M4_LOG_DIR");
    if (!log_dir || log_dir[0] != '/') {
        fprintf(stderr, "smoke_proxy: set TESO4M4_LOG_DIR to an absolute "
                        "temporary directory before loading the bridge\n");
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
