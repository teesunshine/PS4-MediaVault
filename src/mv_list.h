#ifndef MV_LIST_H
#define MV_LIST_H
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Portable (host-testable) list/path helpers. No PS4 APIs in here. */

typedef struct {
    char name[256];   /* display name */
    char path[1024];  /* full path */
    int  is_dir;
    long long size;   /* bytes, -1 unknown */
} mv_entry_t;

typedef struct {
    mv_entry_t *items;
    size_t count;
    size_t cap;
} mv_list_t;

int  mv_is_video(const char *filename);
void mv_list_init(mv_list_t *l);
/* Adds an entry found while scanning `dir`. Keeps directories and video files only,
 * skips names starting with '.'. Returns 0 if added, 1 if skipped, -1 on out-of-memory. */
int  mv_list_add(mv_list_t *l, const char *dir, const char *name, int is_dir, long long size);
/* Adds an entry with no filtering (used for the storage-roots screen). */
int  mv_list_add_raw(mv_list_t *l, const char *label, const char *path, int is_dir, long long size);
/* Directories first, then case-insensitive A-Z. */
void mv_list_sort(mv_list_t *l);
void mv_list_free(mv_list_t *l);
/* Strips the last path component. Returns 1 on success, 0 if there is no parent. */
int  mv_path_parent(const char *path, char *out, size_t n);
void mv_format_size(long long bytes, char *out, size_t n);

#ifdef __cplusplus
}
#endif
#endif
