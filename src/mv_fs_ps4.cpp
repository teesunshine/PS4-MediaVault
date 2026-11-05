#include "mv_fs_ps4.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <orbis/libkernel.h>

/* Classic FreeBSD dirent layout as used by PS4 getdents (verified on hardware). */
struct mv_dirent {
    uint32_t d_fileno;
    uint16_t d_reclen;
    uint8_t  d_type;
    uint8_t  d_namlen;
    char     d_name[256];
};

#define MV_DT_DIR 4
#define MV_DT_REG 8

static int stat_is_dir(const OrbisKernelStat &st) { return (st.st_mode & 0170000) == 0040000; }

int mv_probe_path(const char *path) {
    OrbisKernelStat st;
    return sceKernelStat(path, &st);
}

int mv_scan_dir(const char *path, mv_list_t *out) {
    int fd = sceKernelOpen(path, 0, 0);
    if (fd < 0) return fd;

    static char buf[16384];
    for (;;) {
        int n = sceKernelGetdents(fd, buf, sizeof(buf));
        if (n <= 0) break;
        int off = 0;
        while (off < n) {
            struct mv_dirent *d = (struct mv_dirent *)(buf + off);
            if (d->d_reclen == 0) break;
            off += d->d_reclen;

            if (d->d_name[0] == '.' || d->d_name[0] == '\0') continue;

            int is_dir = (d->d_type == MV_DT_DIR);
            long long size = -1;
            if (d->d_type != MV_DT_DIR && d->d_type != MV_DT_REG) {
                /* Unknown type: ask stat. */
                char full[1024];
                snprintf(full, sizeof(full), "%s/%s", path, d->d_name);
                OrbisKernelStat st;
                if (sceKernelStat(full, &st) != 0) continue;
                is_dir = stat_is_dir(st);
                size = (long long)st.st_size;
            } else if (!is_dir && mv_is_video(d->d_name)) {
                char full[1024];
                snprintf(full, sizeof(full), "%s/%s", path, d->d_name);
                OrbisKernelStat st;
                if (sceKernelStat(full, &st) == 0) size = (long long)st.st_size;
            }
            if (mv_list_add(out, path, d->d_name, is_dir, size) < 0) {
                sceKernelClose(fd);
                return -1;
            }
        }
    }
    sceKernelClose(fd);
    mv_list_sort(out);
    return 0;
}
