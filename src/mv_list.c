#include "mv_list.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *const VIDEO_EXT[] = {
    ".mp4", ".m4v", ".mkv", ".mov", ".avi", ".ts", ".m2ts", ".webm", ".mpg", ".mpeg",
};

static int lower(int c) { return (c >= 'A' && c <= 'Z') ? c + 32 : c; }

static int ci_cmp(const char *a, const char *b) {
    while (*a && *b && lower((unsigned char)*a) == lower((unsigned char)*b)) { a++; b++; }
    return lower((unsigned char)*a) - lower((unsigned char)*b);
}

int mv_is_video(const char *name) {
    const char *dot = strrchr(name, '.');
    if (!dot) return 0;
    for (size_t i = 0; i < sizeof(VIDEO_EXT) / sizeof(VIDEO_EXT[0]); i++)
        if (ci_cmp(dot, VIDEO_EXT[i]) == 0) return 1;
    return 0;
}

void mv_list_init(mv_list_t *l) { l->items = NULL; l->count = 0; l->cap = 0; }

static int push(mv_list_t *l, const mv_entry_t *e) {
    if (l->count == l->cap) {
        size_t nc = l->cap ? l->cap * 2 : 64;
        mv_entry_t *n = (mv_entry_t *)realloc(l->items, nc * sizeof(*n));
        if (!n) return -1;
        l->items = n; l->cap = nc;
    }
    l->items[l->count++] = *e;
    return 0;
}

int mv_list_add_raw(mv_list_t *l, const char *label, const char *path, int is_dir, long long size) {
    mv_entry_t e;
    memset(&e, 0, sizeof(e));
    snprintf(e.name, sizeof(e.name), "%s", label);
    snprintf(e.path, sizeof(e.path), "%s", path);
    e.is_dir = is_dir;
    e.size = size;
    return push(l, &e);
}

int mv_list_add(mv_list_t *l, const char *dir, const char *name, int is_dir, long long size) {
    if (!name || name[0] == '\0' || name[0] == '.') return 1;
    if (!is_dir && !mv_is_video(name)) return 1;
    char path[1024];
    size_t dl = strlen(dir);
    snprintf(path, sizeof(path), "%s%s%s", dir, (dl > 0 && dir[dl - 1] == '/') ? "" : "/", name);
    return mv_list_add_raw(l, name, path, is_dir, size);
}

static int entry_cmp(const void *pa, const void *pb) {
    const mv_entry_t *a = (const mv_entry_t *)pa, *b = (const mv_entry_t *)pb;
    if (a->is_dir != b->is_dir) return b->is_dir - a->is_dir;
    return ci_cmp(a->name, b->name);
}

void mv_list_sort(mv_list_t *l) {
    if (l->count > 1) qsort(l->items, l->count, sizeof(mv_entry_t), entry_cmp);
}

void mv_list_free(mv_list_t *l) {
    free(l->items);
    mv_list_init(l);
}

int mv_path_parent(const char *path, char *out, size_t n) {
    size_t len = strlen(path);
    while (len > 1 && path[len - 1] == '/') len--;
    while (len > 0 && path[len - 1] != '/') len--;   /* drop last component */
    while (len > 1 && path[len - 1] == '/') len--;   /* drop the slash */
    if (len == 0 || (len == 1 && path[0] == '/')) {
        if (n) out[0] = '\0';
        return 0;
    }
    if (len >= n) len = n - 1;
    memcpy(out, path, len);
    out[len] = '\0';
    return 1;
}

void mv_format_size(long long bytes, char *out, size_t n) {
    if (bytes < 0) { snprintf(out, n, "?"); return; }
    const char *units[] = {"B", "KB", "MB", "GB", "TB"};
    int u = 0;
    long long whole = bytes, rem = 0;
    while (whole >= 1024 && u < 4) { rem = whole % 1024; whole /= 1024; u++; }
    if (u == 0) snprintf(out, n, "%lld B", whole);
    else snprintf(out, n, "%lld.%lld %s", whole, (rem * 10) / 1024, units[u]);
}
