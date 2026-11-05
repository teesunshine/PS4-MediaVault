/*
 * PS4-MediaVault - Phase 1 file browser + Phase 2 playback.
 * Browses internal storage (/data) and, where the sandbox allows, USB drives.
 * Selecting a video plays it through the system AvPlayer (src/mv_player.*).
 */
#include <stdio.h>
#include <string.h>
#include <sstream>
#include <orbis/libkernel.h>
#include <orbis/Pad.h>

#include "graphics.h"
#include "log.h"
#include "mv_list.h"
#include "mv_fs_ps4.h"
#include "mv_player.h"
#include "pad.h"
#include "text.h"

std::stringstream debugLogStream;   /* required by the toolchain's log.h */

#define FRAME_W 1920
#define FRAME_H 1080
#define ROW_H   44
#define LIST_Y  150
#define VISIBLE 18
#define MARGIN  60

static const Color C_BG   = {18, 18, 26};
static const Color C_BAR  = {50, 90, 160};
static const Color C_TEXT = {235, 235, 240};
static const Color C_DIR  = {120, 200, 255};
static const Color C_DIM  = {140, 140, 155};
static const Color C_ERR  = {255, 130, 130};
static const Color C_OK   = {150, 230, 160};

struct Root { const char *path; const char *label; };
static const Root ROOTS[] = {
    {"/data",     "Internal storage"},
    {"/mnt/usb0", "USB drive 1"},
    {"/mnt/usb1", "USB drive 2"},
};
static const int ROOT_COUNT = sizeof(ROOTS) / sizeof(ROOTS[0]);

enum Mode { MODE_ROOTS, MODE_DIR, MODE_PLAYER };

struct App {
    Mode mode = MODE_ROOTS;
    mv_list_t list;
    char path[1024] = "";
    int sel = 0, top = 0;
    char status[320] = "";
    bool statusErr = false;
    char nowPlaying[256] = "";
};

static void set_status(App &a, bool err, const char *fmt, const char *s1 = "", unsigned v = 0) {
    snprintf(a.status, sizeof(a.status), fmt, s1, v);
    a.statusErr = err;
}

static void load_roots(App &a) {
    mv_list_free(&a.list);
    mv_list_init(&a.list);
    for (int i = 0; i < ROOT_COUNT; i++) {
        char label[256];
        int rc = mv_probe_path(ROOTS[i].path);
        snprintf(label, sizeof(label), "%-18s %s%s", ROOTS[i].label, ROOTS[i].path,
                 rc == 0 ? "" : "   [not accessible]");
        mv_list_add_raw(&a.list, label, ROOTS[i].path, 1, -1);
    }
    a.mode = MODE_ROOTS;
    a.path[0] = '\0';
    a.sel = a.top = 0;
}

static void enter_dir(App &a, const char *path) {
    mv_list_t nl;
    mv_list_init(&nl);
    int rc = mv_scan_dir(path, &nl);
    if (rc < 0) {
        mv_list_free(&nl);
        set_status(a, true, "Cannot open %s (error 0x%08X)", path, (unsigned)rc);
        return;
    }
    mv_list_free(&a.list);
    a.list = nl;
    snprintf(a.path, sizeof(a.path), "%s", path);
    a.mode = MODE_DIR;
    a.sel = a.top = 0;
    a.status[0] = '\0';
}

static void go_back(App &a) {
    if (a.mode == MODE_ROOTS) return;
    for (int i = 0; i < ROOT_COUNT; i++) {
        if (strcmp(a.path, ROOTS[i].path) == 0) { load_roots(a); a.status[0] = '\0'; return; }
    }
    char parent[1024];
    if (mv_path_parent(a.path, parent, sizeof(parent))) enter_dir(a, parent);
    else load_roots(a);
}

static void activate(App &a) {
    if (a.list.count == 0) return;
    const mv_entry_t &e = a.list.items[a.sel];
    if (e.is_dir) { enter_dir(a, e.path); return; }
    if (mv_player_open(e.path)) {
        snprintf(a.nowPlaying, sizeof(a.nowPlaying), "%s", e.name);
        a.mode = MODE_PLAYER;
    } else {
        char msg[320];
        snprintf(msg, sizeof(msg), "Could not play %s: %s", e.name, mv_player_error());
        set_status(a, true, "%s", msg);
    }
}

static void stop_playback(App &a) {
    mv_player_close();
    a.mode = MODE_DIR;
    a.status[0] = '\0';
}

/* Returns true if anything changed and the screen needs redrawing. */
static bool handle_input(App &a, const Pad &pad) {
    if (a.mode == MODE_PLAYER) {
        if (pad.Pressed(ORBIS_PAD_BUTTON_CROSS))  mv_player_toggle_pause();
        if (pad.Pressed(ORBIS_PAD_BUTTON_CIRCLE)) stop_playback(a);
        return true; /* video is animated - always redraw */
    }

    int count = (int)a.list.count;
    int oldSel = a.sel;
    if (pad.Repeat(ORBIS_PAD_BUTTON_UP))   a.sel--;
    if (pad.Repeat(ORBIS_PAD_BUTTON_DOWN)) a.sel++;
    if (pad.Pressed(ORBIS_PAD_BUTTON_L1))  a.sel -= VISIBLE;
    if (pad.Pressed(ORBIS_PAD_BUTTON_R1))  a.sel += VISIBLE;
    if (count > 0) {
        if (a.sel < 0) a.sel = 0;
        if (a.sel >= count) a.sel = count - 1;
    } else a.sel = 0;
    if (a.sel < a.top) a.top = a.sel;
    if (a.sel >= a.top + VISIBLE) a.top = a.sel - VISIBLE + 1;
    bool changed = (a.sel != oldSel);

    if (pad.Pressed(ORBIS_PAD_BUTTON_CROSS))  { activate(a); changed = true; }
    if (pad.Pressed(ORBIS_PAD_BUTTON_CIRCLE)) { go_back(a);  changed = true; }
    return changed;
}

static void truncate_to(const char *src, char *dst, size_t maxChars) {
    size_t len = strlen(src);
    if (len <= maxChars) { snprintf(dst, maxChars + 1, "%s", src); return; }
    snprintf(dst, maxChars + 1, "%.*s...", (int)maxChars - 3, src);
}

static void draw_player(Scene2D *s, const App &a) {
    s->FrameBufferFill({0, 0, 0});
    mv_player_blit(s, 0, 0, FRAME_W, FRAME_H);

    char line[300];
    const char *state = mv_player_state() == MV_PLAYER_PAUSED ? "Paused" : "Playing";
    snprintf(line, sizeof(line), "%s - %s", state, a.nowPlaying);
    char shown[140];
    truncate_to(line, shown, 110);
    ui_text(s, MARGIN, 30, shown, 2, C_TEXT);
    ui_text(s, MARGIN, FRAME_H - 50, "[X] Play/Pause     [O] Stop", 2, C_DIM);
}

static void draw(Scene2D *s, const App &a) {
    if (a.mode == MODE_PLAYER) { draw_player(s, a); return; }

    s->FrameBufferFill(C_BG);

    ui_text(s, MARGIN, 30, "PS4 MediaVault", 3, C_TEXT);
    char pathLine[200];
    truncate_to(a.mode == MODE_ROOTS ? "Storage locations" : a.path, pathLine, 100);
    ui_text(s, MARGIN, 92, pathLine, 2, C_DIM);
    ui_fill(s, MARGIN, 134, FRAME_W - 2 * MARGIN, 2, C_DIM);

    if (a.list.count == 0) {
        ui_text(s, MARGIN + 20, LIST_Y + 20, "(no folders or video files here)", 2, C_DIM);
    }
    for (int r = 0; r < VISIBLE; r++) {
        int idx = a.top + r;
        if (idx >= (int)a.list.count) break;
        const mv_entry_t &e = a.list.items[idx];
        int y = LIST_Y + r * ROW_H;
        if (idx == a.sel) ui_fill(s, MARGIN, y - 4, FRAME_W - 2 * MARGIN, ROW_H - 4, C_BAR);

        char name[200];
        char shown[256];
        snprintf(shown, sizeof(shown), "%s%s", e.name,
                 (e.is_dir && a.mode == MODE_DIR) ? "/" : "");
        truncate_to(shown, name, 84);
        ui_text(s, MARGIN + 20, y, name, 2, e.is_dir ? C_DIR : C_TEXT);

        if (!e.is_dir) {
            char size[32];
            mv_format_size(e.size, size, sizeof(size));
            ui_text(s, FRAME_W - MARGIN - 20 - ui_text_width(size, 2), y, size, 2, C_DIM);
        }
    }

    if (a.list.count > 0) {
        char pos[48];
        snprintf(pos, sizeof(pos), "%d / %d", a.sel + 1, (int)a.list.count);
        ui_text(s, FRAME_W - MARGIN - ui_text_width(pos, 2), 92, pos, 2, C_DIM);
    }

    if (a.status[0]) {
        char st[140];
        truncate_to(a.status, st, 110);
        ui_text(s, MARGIN, 950, st, 2, a.statusErr ? C_ERR : C_OK);
    }
    ui_text(s, MARGIN, 1000,
            a.mode == MODE_ROOTS ? "[X] Open     [Up/Down] Move"
                                 : "[X] Open     [O] Back     [L1/R1] Page     [Up/Down] Move",
            2, C_DIM);
}

int main(void) {
    setvbuf(stdout, NULL, _IONBF, 0);
    printf("[MediaVault] Phase 1+2 starting\n");

    Scene2D *scene = new Scene2D(FRAME_W, FRAME_H, 4);
    if (!scene->Init(0xC000000, 2)) {
        printf("[MediaVault] failed to init video\n");
        for (;;) sceKernelSleep(1);
    }

    Pad pad;
    if (!pad.Init()) printf("[MediaVault] failed to init controller\n");

    mv_player_init();

    App app;
    mv_list_init(&app.list);
    load_roots(app);

    int frameID = 0, dirty = 2;
    for (;;) {
        pad.Poll();
        if (handle_input(app, pad)) dirty = 2;

        if (app.mode == MODE_PLAYER) {
            mv_player_update();
            if (mv_player_finished() || mv_player_state() == MV_PLAYER_ERROR) {
                bool wasErr = mv_player_state() == MV_PLAYER_ERROR;
                char err[160];
                snprintf(err, sizeof(err), "%s", mv_player_error());
                stop_playback(app);
                if (wasErr) set_status(app, true, "%s", err);
                dirty = 2;
            }
        }

        if (dirty > 0) {
            draw(scene, app);
            scene->SubmitFlip(frameID);
            scene->FrameWait(frameID);
            scene->FrameBufferSwap();
            frameID++;
            dirty--;
        } else {
            sceKernelUsleep(16000);
        }
    }
    /* Never return from main: returning crashed the app with SIGSYS on exit. */
    return 0;
}
