#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include <ctype.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#include "plugins/plugin_sdk.h"
#if defined(__GLIBC__)
  #include <link.h>
#endif

typedef const char* (*fn_get_name)(void);
typedef const char* (*fn_init)(int);
typedef const char* (*fn_fini)(void);
typedef const char* (*fn_place_work)(const char*);
typedef void (*fn_attach)(const char* (*next_place_work)(const char*));
typedef const char* (*fn_wait_finished)(void);

typedef struct {
    void* handle;
    char* name;
    char* so_path;
    int temp_path;
    fn_init init;
    fn_fini fini;
    fn_place_work place_work;
    fn_attach attach;
    fn_wait_finished wait_finished;
} plugin_handle_t;

static void print_usage(FILE *out) {
    fprintf(out,
"Usage: ./analyzer <queue_size> <plugin1> <plugin2> ... <pluginN>\n"
"\nArguments:\n"
"  queue_size    Maximum number of items in each plugin's queue\n"
"  plugin1..N    Names of plugins to load (without .so extension)\n"
"\nAvailable plugins:\n"
"  logger        - Logs all strings that pass through\n"
"  typewriter    - Simulates typewriter effect with delays\n"
"  uppercaser    - Converts strings to uppercase\n"
"  rotator       - Move every character to the right. Last character moves to the beginning.\n"
"  flipper       - Reverses the order of characters\n"
"  expander      - Expands each character with spaces\n"
"\nExample:\n"
"  ./analyzer 20 uppercaser rotator logger\n"
"  echo 'hello' | ./analyzer 20 uppercaser rotator logger\n"
"  echo '<END>' | ./analyzer 20 uppercaser rotator logger\n"
    );
}

static int resolve_sym(void *handle, const char *name, void *out_fnptr, size_t out_sz) {
    dlerror();
    void *p = dlsym(handle, name);
    const char *e = dlerror();
    if (e || !p) {
        return -1;
    }
    memcpy(out_fnptr, &p, out_sz);
    return 0;
}

typedef struct {
    char* key; 
    int count;
} name_count_t;

static void free_name_counts(name_count_t *counts, int counts_n) {
    if (!counts) {
        return;
    }
    for (int k = 0; k < counts_n; ++k) {
        free(counts[k].key);
    }
}

static void free_plugin(plugin_handle_t *p) {
    if (!p) {
        return;
    }
    if (p->handle) {
        dlclose(p->handle);
    }
    if (p->temp_path && p->so_path) {
        (void)unlink(p->so_path);
    }
    free(p->so_path);
    free(p->name);
    memset(p, 0, sizeof(*p));
}

static void cleanup_plugins_upto(plugin_handle_t *P, int upto_inclusive) {
    if (!P || upto_inclusive < 0) {
        return;
    }
    for (int k = 0; k <= upto_inclusive; ++k) {
        free_plugin(&P[k]);
    }
}

static int file_exists(const char* p) {
    struct stat st; 
    return stat(p, &st) == 0;
}

static int bump_count(name_count_t** arr, int* n, const char* key) {
    for (int i = 0; i < *n; ++i) {
        if (strcmp((*arr)[i].key, key) == 0) {
            (*arr)[i].count += 1;
            return (*arr)[i].count;
        }
    }
    name_count_t* tmp = (name_count_t*)realloc(*arr, (size_t)(*n + 1) * sizeof(**arr));
    if (!tmp) {
        return -1;
    }
    *arr = tmp;
    (*arr)[*n].key = strdup(key);
    if (!(*arr)[*n].key) {
        return -1;
    }
    (*arr)[*n].count = 1;
    *n += 1;
    return 1;
}

static int make_instance_path(const char* base, const char* plug, int inst_idx, char* out, size_t out_sz, int* made_temp) {
    *made_temp = 0;
    if (inst_idx <= 1) {
        snprintf(out, out_sz, "%s", base); 
        return 0; 
    }
    snprintf(out, out_sz, "output/%s__inst%d.so", plug, inst_idx);
    if (file_exists(out)) {
        *made_temp = 1;
        return 0;
    }
    FILE *src = fopen(base, "rb"); 
    if (!src) {
        return -1;
    }
    FILE *dst = fopen(out,  "wb"); 
    if (!dst) { 
        fclose(src); 
        return -1;
    }
    char buf[8192]; 
    size_t n;
    while ((n = fread(buf, 1, sizeof buf, src)) > 0) {
        if (fwrite(buf, 1, n, dst) != n) {
            fclose(src); fclose(dst);
            return -1;
        }
    }
    fclose(src); 
    fclose(dst);
    *made_temp = 1;
    return 0;
}

int main(int argc, char** argv) {
    if (argc < 3) {
        fprintf(stderr, "error: missing queue size or plugins\n");
        print_usage(stdout);
        return 1;
    }
    char* endp = NULL;
    long q = strtol(argv[1], &endp, 10);
    if (!endp || *endp != '\0' || q <= 0) {
        fprintf(stderr, "error: invalid queue size '%s'\n", argv[1]);
        print_usage(stdout);
        return 1;
    }
    int queue_size = (int)q;
    const int n = argc - 2;
    plugin_handle_t* P = (plugin_handle_t*)calloc((size_t)n, sizeof(*P));
    if (!P) {
        fprintf(stderr, "error: oom\n");
        return 1;
    }
    name_count_t* counts = NULL;
    int counts_n = 0;
    for (int i = 0; i < n; ++i) {
        const char* plug = argv[2 + i];
        int inst_idx = bump_count(&counts, &counts_n, plug);
        if (inst_idx < 0) {
            fprintf(stderr, "error: oom (counts)\n");
            print_usage(stdout);
            cleanup_plugins_upto(P, i - 1);
            free_name_counts(counts, counts_n);
            free(counts);
            free(P);
            return 1;
        }
        char base[256];  snprintf(base, sizeof(base), "output/%s.so", plug);
        char actual[256]; int made_temp = 0;
        if (make_instance_path(base, plug, inst_idx, actual, sizeof actual, &made_temp) != 0) {
            fprintf(stderr, "error: cannot prepare instance path for %s\n", plug);
            print_usage(stdout);
            cleanup_plugins_upto(P, i - 1);
            free_name_counts(counts, counts_n);
            free(counts);
            free(P);
            return 1;
        }
        void* h = NULL;
        #if defined(__GLIBC__) && defined(LM_ID_NEWLM)
        if (inst_idx > 1) {
            h = dlmopen(LM_ID_NEWLM, actual, RTLD_NOW | RTLD_LOCAL);
        }
        #endif
        if (!h) {
            h = dlopen(actual, RTLD_NOW | RTLD_LOCAL);
        }
        if (!h) {
            fprintf(stderr, "dlopen %s failed: %s\n", actual, dlerror());
            print_usage(stdout);
            cleanup_plugins_upto(P, i - 1);
            free_name_counts(counts, counts_n);
            free(counts);
            free(P);
            return 1;
        }
        P[i].handle = h;
        P[i].name = strdup(plug);
        P[i].so_path = strdup(actual);
        P[i].temp_path = made_temp;

        if (resolve_sym(h, "plugin_init", &P[i].init, sizeof P[i].init) ||
            resolve_sym(h, "plugin_fini", &P[i].fini, sizeof P[i].fini) ||
            resolve_sym(h, "plugin_place_work", &P[i].place_work, sizeof P[i].place_work) ||
            resolve_sym(h, "plugin_attach", &P[i].attach, sizeof P[i].attach) ||
            resolve_sym(h, "plugin_wait_finished", &P[i].wait_finished, sizeof P[i].wait_finished)) {
            fprintf(stderr, "dlsym failed for required plugin symbols in %s\n", actual);
            print_usage(stdout);
            free_plugin(&P[i]);
            cleanup_plugins_upto(P, i - 1);
            free_name_counts(counts, counts_n);
            free(counts);
            free(P);
            return 1;
        }
    }
    for (int i = 0; i < n; ++i) {
        const char* err = P[i].init(queue_size);
        if (err) {
            fprintf(stderr, "init(%s) failed: %s\n", P[i].name, err);
            for (int k = 0; k <= i; ++k) {
                (void)P[k].fini();
            }
            cleanup_plugins_upto(P, n - 1);
            free_name_counts(counts, counts_n);
            free(counts);
            free(P);
            return 2;
        }
    }
    for (int i = 0; i + 1 < n; ++i) {
        P[i].attach(P[i+1].place_work);
    }
    char buf[1025];
    while (fgets(buf, sizeof(buf), stdin)) {
        size_t len = strlen(buf);
        if (len > 0 && buf[len-1] == '\n') {
            buf[len-1] = '\0';
        }
        const char* err = P[0].place_work(buf);
        if (err) {
            fprintf(stderr, "place_work error: %s\n", err);
            break;
        }
        if (strcmp(buf, "<END>") == 0) {
            break;
        }
    }
    for (int i = 0; i < n; ++i) {
        const char* err = P[i].wait_finished();
        if (err) {
            fprintf(stderr, "wait_finished(%s): %s\n", P[i].name, err);
        }
    }
    for (int i = 0; i < n; ++i) {
        const char* err = P[i].fini();
        if (err) {
            fprintf(stderr, "fini(%s): %s\n", P[i].name, err);
        }
    }
    cleanup_plugins_upto(P, n - 1);
    free_name_counts(counts, counts_n);
    free(counts);
    free(P);
    fprintf(stderr, "Pipeline shutdown complete\n");
    return 0;
}