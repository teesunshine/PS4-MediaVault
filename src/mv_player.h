#ifndef MV_PLAYER_H
#define MV_PLAYER_H
#include "graphics.h"

/* Phase 2: video playback through the system libSceAvPlayer (hardware decode).
 * See docs/DESIGN.md and src/mv_avplayer.h for the struct-layout caveat. */

enum MvPlayerState { MV_PLAYER_IDLE, MV_PLAYER_PLAYING, MV_PLAYER_PAUSED, MV_PLAYER_ERROR };

/* One-time setup of the memory/file/event callbacks. Call once at startup. */
void mv_player_init();

/* Opens and starts playing `path`. Returns true on success; on failure call
 * mv_player_error() for a short message to show the user. */
bool mv_player_open(const char *path);

/* Call once per frame while a player is open. Pumps the decoder and, when a
 * new video frame has arrived, decodes it into the internal RGB buffer. */
void mv_player_update();

/* Draws the current frame letterboxed into the given screen rectangle.
 * Safe to call even before the first frame arrives (draws nothing). */
void mv_player_blit(Scene2D *s, int x, int y, int w, int h);

void mv_player_toggle_pause();
MvPlayerState mv_player_state();
/* True once playback has run out of data (end of file). */
bool mv_player_finished();
const char *mv_player_error();

/* Stops playback and releases the AvPlayer handle and frame buffer. */
void mv_player_close();

#endif
