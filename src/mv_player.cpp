#include "mv_player.h"
#include "mv_avplayer.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <orbis/libkernel.h>
#include <orbis/Sysmodule.h>

/* libSceAvPlayer is not one of the "always loaded" base modules (unlike
 * VideoOut/Pad/UserService, which already appear in a fresh process's module
 * list) - it has to be brought in explicitly, or every sceAvPlayer* call
 * traps with PRX_NOT_RESOLVED_FUNCTION because the import was never
 * resolved. Confirmed on hardware: see docs/NOTES.md. */
static bool mv_avplayer_module_loaded = false;

/* Caps the decoded-frame buffer; matches the app's 1920x1080 framebuffer
 * (see FRAME_W/FRAME_H in main.cpp). A source larger than this is rejected
 * rather than overflowing mv_rgb. */
#define MV_MAX_W 1920
#define MV_MAX_H 1080

static SceAvPlayerHandle mv_handle = -1;
static MvPlayerState mv_state = MV_PLAYER_IDLE;
static bool mv_finished = false;
static char mv_err[160] = "";

static uint8_t mv_rgb[MV_MAX_W * MV_MAX_H * 3];
static uint32_t mv_frame_w = 0, mv_frame_h = 0;

/* Single in-flight source file; AvPlayer only ever has one open at a time here. */
static int mv_file_fd = -1;

/* ---- memory callbacks: a small over-allocating aligned allocator ---- */
static void *mv_av_alloc(void *, uint32_t alignment, uint32_t size) {
    if (alignment < sizeof(void *)) alignment = sizeof(void *);
    uint8_t *base = (uint8_t *)malloc((size_t)size + alignment + sizeof(void *));
    if (!base) return nullptr;
    uintptr_t raw = (uintptr_t)(base + sizeof(void *));
    uintptr_t aligned = (raw + alignment - 1) & ~((uintptr_t)alignment - 1);
    ((void **)aligned)[-1] = base;
    return (void *)aligned;
}
static void mv_av_free(void *, void *ptr) {
    if (!ptr) return;
    free(((void **)ptr)[-1]);
}

/* ---- file callbacks: PS4 kernel calls, matching mv_fs_ps4.cpp's approach ---- */
static int mv_av_file_open(void *, const char *filename) {
    mv_file_fd = sceKernelOpen(filename, 0, 0);
    return mv_file_fd >= 0 ? 0 : -1;
}
static int mv_av_file_close(void *) {
    if (mv_file_fd >= 0) sceKernelClose(mv_file_fd);
    mv_file_fd = -1;
    return 0;
}
static int mv_av_file_read(void *, uint8_t *buffer, uint64_t position, uint32_t length) {
    if (mv_file_fd < 0) return -1;
    return (int)sceKernelPread(mv_file_fd, buffer, length, (off_t)position);
}
static uint64_t mv_av_file_size(void *) {
    if (mv_file_fd < 0) return 0;
    OrbisKernelStat st;
    if (sceKernelFstat(mv_file_fd, &st) != 0) return 0;
    return (uint64_t)st.st_size;
}

/* ---- event callback: log only, for now ---- */
static void mv_av_event(void *, int32_t eventId, int32_t sourceId, void *) {
    printf("[MediaVault] AvPlayer event %d (source %d)\n", (int)eventId, (int)sourceId);
}

void mv_player_init() {
    if (mv_avplayer_module_loaded) return;
    int32_t rc = sceSysmoduleLoadModule(ORBIS_SYSMODULE_AV_PLAYER);
    if (rc != 0) {
        printf("[MediaVault] sceSysmoduleLoadModule(AV_PLAYER) failed (0x%08X) - "
               "playback will not work\n", (unsigned)rc);
        return;
    }
    mv_avplayer_module_loaded = true;
}

static void set_err(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(mv_err, sizeof(mv_err), fmt, ap);
    va_end(ap);
    mv_state = MV_PLAYER_ERROR;
}

bool mv_player_open(const char *path) {
    mv_player_close();
    mv_err[0] = '\0';

    if (!mv_avplayer_module_loaded) {
        set_err("libSceAvPlayer module is not loaded (see startup log)");
        return false;
    }

    SceAvPlayerInitData init;
    memset(&init, 0, sizeof(init));
    init.memoryReplacement.allocate = mv_av_alloc;
    init.memoryReplacement.deallocate = mv_av_free;
    init.memoryReplacement.allocateTexture = mv_av_alloc;
    init.memoryReplacement.deallocateTexture = mv_av_free;
    init.fileReplacement.open = mv_av_file_open;
    init.fileReplacement.close = mv_av_file_close;
    init.fileReplacement.readOffset = mv_av_file_read;
    init.fileReplacement.size = mv_av_file_size;
    init.eventReplacement.eventCallback = mv_av_event;
    init.debugLevel = 0;
    init.basePriority = 1000;
    init.numOutputVideoFrameBuffers = 4;
    init.autoStart = 0; /* we call sceAvPlayerStart ourselves, below */
    init.defaultLanguage = "eng";

    mv_handle = sceAvPlayerInit(&init);
    if (mv_handle < 0) { set_err("sceAvPlayerInit failed (0x%08X)", mv_handle); return false; }

    int rc = sceAvPlayerAddSource(mv_handle, path);
    if (rc < 0) {
        set_err("sceAvPlayerAddSource failed (0x%08X)", rc);
        sceAvPlayerClose(mv_handle);
        mv_handle = -1;
        return false;
    }

    sceAvPlayerStart(mv_handle);
    mv_state = MV_PLAYER_PLAYING;
    mv_finished = false;
    mv_frame_w = mv_frame_h = 0;
    return true;
}

/* BT.601 limited-range NV12 -> RGB, clamped. */
static inline uint8_t clamp255(int v) { return v < 0 ? 0 : (v > 255 ? 255 : (uint8_t)v); }

static void nv12_to_rgb(const uint8_t *src, uint32_t w, uint32_t h) {
    const uint8_t *yPlane = src;
    const uint8_t *uvPlane = src + (size_t)w * h;
    for (uint32_t y = 0; y < h; y++) {
        const uint8_t *yRow = yPlane + (size_t)y * w;
        const uint8_t *uvRow = uvPlane + (size_t)(y / 2) * w;
        uint8_t *out = mv_rgb + (size_t)y * w * 3;
        for (uint32_t x = 0; x < w; x++) {
            int c = yRow[x] - 16;
            int d = uvRow[(x / 2) * 2 + 0] - 128;
            int e = uvRow[(x / 2) * 2 + 1] - 128;
            out[x * 3 + 0] = clamp255((298 * c + 409 * e + 128) >> 8);
            out[x * 3 + 1] = clamp255((298 * c - 100 * d - 208 * e + 128) >> 8);
            out[x * 3 + 2] = clamp255((298 * c + 516 * d + 128) >> 8);
        }
    }
}

void mv_player_update() {
    if (mv_handle < 0 || mv_state != MV_PLAYER_PLAYING) return;

    if (!sceAvPlayerIsActive(mv_handle)) { mv_finished = true; return; }

    SceAvPlayerFrameInfo info;
    memset(&info, 0, sizeof(info));
    if (sceAvPlayerGetVideoData(mv_handle, &info) && info.pData) {
        uint32_t w = info.details.video.width, h = info.details.video.height;
        if (w > 0 && h > 0 && w <= MV_MAX_W && h <= MV_MAX_H) {
            nv12_to_rgb(info.pData, w, h);
            mv_frame_w = w;
            mv_frame_h = h;
        } else if (w > MV_MAX_W || h > MV_MAX_H) {
            set_err("video resolution %ux%u too large for the player buffer", w, h);
        }
    }
}

void mv_player_blit(Scene2D *s, int x, int y, int w, int h) {
    if (mv_frame_w == 0 || mv_frame_h == 0) return;

    /* Letterbox: fit the source into (w,h) keeping its aspect ratio. */
    int dstW = w, dstH = (int)((int64_t)mv_frame_h * w / mv_frame_w);
    if (dstH > h) { dstH = h; dstW = (int)((int64_t)mv_frame_w * h / mv_frame_h); }
    int offX = x + (w - dstW) / 2;
    int offY = y + (h - dstH) / 2;

    for (int dy = 0; dy < dstH; dy++) {
        uint32_t sy = (uint32_t)((int64_t)dy * mv_frame_h / dstH);
        const uint8_t *row = mv_rgb + (size_t)sy * mv_frame_w * 3;
        for (int dx = 0; dx < dstW; dx++) {
            uint32_t sx = (uint32_t)((int64_t)dx * mv_frame_w / dstW);
            const uint8_t *p = row + sx * 3;
            Color c; c.r = p[0]; c.g = p[1]; c.b = p[2];
            s->DrawPixel(offX + dx, offY + dy, c);
        }
    }
}

void mv_player_toggle_pause() {
    if (mv_handle < 0) return;
    if (mv_state == MV_PLAYER_PLAYING) { sceAvPlayerPause(mv_handle); mv_state = MV_PLAYER_PAUSED; }
    else if (mv_state == MV_PLAYER_PAUSED) { sceAvPlayerResume(mv_handle); mv_state = MV_PLAYER_PLAYING; }
}

MvPlayerState mv_player_state() { return mv_state; }
bool mv_player_finished() { return mv_finished; }
const char *mv_player_error() { return mv_err; }

void mv_player_close() {
    if (mv_handle >= 0) { sceAvPlayerStop(mv_handle); sceAvPlayerClose(mv_handle); }
    mv_handle = -1;
    mv_state = MV_PLAYER_IDLE;
    mv_finished = false;
    mv_frame_w = mv_frame_h = 0;
}
