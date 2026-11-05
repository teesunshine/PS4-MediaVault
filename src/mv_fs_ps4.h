#ifndef MV_FS_PS4_H
#define MV_FS_PS4_H
#include "mv_list.h"

/* Directory scanning using PS4 kernel file calls (POSIX opendir/access do not work
 * reliably in the PS4 sandbox). Returns 0 on success, or a negative kernel error
 * code (e.g. 0x80020002 = not found) if the directory cannot be opened. */
int mv_scan_dir(const char *path, mv_list_t *out);

/* Returns 0 if the path exists and is reachable, else the kernel error code. */
int mv_probe_path(const char *path);

#endif
