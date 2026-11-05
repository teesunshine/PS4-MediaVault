#include "../src/mv_list.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

int main(void) {
    /* video extension detection */
    assert(mv_is_video("x.M4V") && mv_is_video("a.b.mkv") && !mv_is_video("x.jpg") && !mv_is_video("noext"));

    /* filtering + sorting */
    mv_list_t l;
    mv_list_init(&l);
    assert(mv_list_add(&l, "/data", "Zebra", 1, -1) == 0);
    assert(mv_list_add(&l, "/data", "alpha", 1, -1) == 0);
    assert(mv_list_add(&l, "/data", "b.MKV", 0, 100) == 0);
    assert(mv_list_add(&l, "/data", "a.mp4", 0, 50) == 0);
    assert(mv_list_add(&l, "/data", "notes.txt", 0, 5) == 1);
    assert(mv_list_add(&l, "/data", ".hidden.mp4", 0, 5) == 1);
    assert(mv_list_add(&l, "/data", ".cache", 1, 0) == 1);
    assert(l.count == 4);
    mv_list_sort(&l);
    assert(strcmp(l.items[0].name, "alpha") == 0 && l.items[0].is_dir);
    assert(strcmp(l.items[1].name, "Zebra") == 0);
    assert(strcmp(l.items[2].name, "a.mp4") == 0);
    assert(strcmp(l.items[3].name, "b.MKV") == 0);
    assert(strcmp(l.items[2].path, "/data/a.mp4") == 0);
    mv_list_free(&l);

    /* trailing slash handling */
    mv_list_init(&l);
    mv_list_add(&l, "/data/", "x.mp4", 0, 1);
    assert(strcmp(l.items[0].path, "/data/x.mp4") == 0);
    mv_list_free(&l);

    /* parent paths */
    char p[64];
    assert(mv_path_parent("/data/videos/a", p, sizeof p) == 1 && strcmp(p, "/data/videos") == 0);
    assert(mv_path_parent("/data/videos/", p, sizeof p) == 1 && strcmp(p, "/data") == 0);
    assert(mv_path_parent("/data", p, sizeof p) == 0 && p[0] == '\0');
    assert(mv_path_parent("/", p, sizeof p) == 0);

    /* sizes */
    char s[32];
    mv_format_size(512, s, sizeof s);              assert(strcmp(s, "512 B") == 0);
    mv_format_size(1536, s, sizeof s);             assert(strcmp(s, "1.5 KB") == 0);
    mv_format_size(3LL * 1024 * 1024 * 1024, s, sizeof s); assert(strcmp(s, "3.0 GB") == 0);
    mv_format_size(-1, s, sizeof s);               assert(strcmp(s, "?") == 0);

    puts("all tests passed");
    return 0;
}
