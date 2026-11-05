#ifndef MV_AVPLAYER_H
#define MV_AVPLAYER_H
/*
 * Reconstructed SceAvPlayer declarations.
 *
 * The toolchain's own orbis/AvPlayer.h (include/orbis/AvPlayer.h) only has
 * bare, parameterless names (see docs/NOTES.md) - it exists to satisfy the
 * linker's symbol list, not to describe the ABI. AvPlayer is the same
 * library family Sony shipped on PS Vita and PS3 before PS4, and its public
 * struct layout and calling convention did not change across that family,
 * so this header is transcribed from the published Vita AvPlayer headers
 * (see https://docs.vitasdk.org/group__SceAvPlayerUser.html) with PS4-only
 * extensions (EnableStream/GetStreamInfo/JumpToTime/Ex variants) added from
 * their symbol names.
 *
 * NOT YET VERIFIED ON HARDWARE. If sceAvPlayerInit() crashes or returns a
 * garbage handle, start by checking this file's field order/sizes against
 * a disassembly of libSceAvPlayer.sprx on your firmware before anything else.
 *
 * Do not also include <orbis/AvPlayer.h> in a translation unit that includes
 * this file: its parameterless prototypes conflict with the real ones below.
 */
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef int32_t SceAvPlayerHandle;
typedef int32_t SceAvPlayerBool; /* 0 / 1 */

typedef enum SceAvPlayerStreamType {
    SCE_AVPLAYER_VIDEO = 0,
    SCE_AVPLAYER_AUDIO = 1,
    SCE_AVPLAYER_TIMEDTEXT = 2,
} SceAvPlayerStreamType;

/* --- memory callbacks --- */
typedef void *(*SceAvPlayerAllocate)(void *arg, uint32_t alignment, uint32_t size);
typedef void  (*SceAvPlayerDeallocate)(void *arg, void *ptr);
typedef void *(*SceAvPlayerAllocateTexture)(void *arg, uint32_t alignment, uint32_t size);
typedef void  (*SceAvPlayerDeallocateTexture)(void *arg, void *ptr);

typedef struct SceAvPlayerMemAllocator {
    void *objectPointer;
    SceAvPlayerAllocate allocate;
    SceAvPlayerDeallocate deallocate;
    SceAvPlayerAllocateTexture allocateTexture;
    SceAvPlayerDeallocateTexture deallocateTexture;
} SceAvPlayerMemAllocator;

/* --- file callbacks --- */
typedef int      (*SceAvPlayerOpenFile)(void *arg, const char *filename);
typedef int      (*SceAvPlayerCloseFile)(void *arg);
typedef int      (*SceAvPlayerReadOffsetFile)(void *arg, uint8_t *buffer, uint64_t position, uint32_t length);
typedef uint64_t (*SceAvPlayerSizeFile)(void *arg);

typedef struct SceAvPlayerFileReplacement {
    void *objectPointer;
    SceAvPlayerOpenFile open;
    SceAvPlayerCloseFile close;
    SceAvPlayerReadOffsetFile readOffset;
    SceAvPlayerSizeFile size;
} SceAvPlayerFileReplacement;

/* --- event callback --- */
typedef void (*SceAvPlayerEventCallback)(void *arg, int32_t eventId, int32_t sourceId, void *eventData);

typedef struct SceAvPlayerEventReplacement {
    void *objectPointer;
    SceAvPlayerEventCallback eventCallback;
} SceAvPlayerEventReplacement;

typedef struct SceAvPlayerInitData {
    SceAvPlayerMemAllocator memoryReplacement;
    SceAvPlayerFileReplacement fileReplacement;
    SceAvPlayerEventReplacement eventReplacement;
    int32_t debugLevel;
    uint32_t basePriority;
    int32_t numOutputVideoFrameBuffers;
    SceAvPlayerBool autoStart;
    uint8_t reserved[3];
    const char *defaultLanguage;
} SceAvPlayerInitData;

typedef struct SceAvPlayerAudio {
    uint16_t channelCount;
    uint8_t  reserved[2];
    uint32_t sampleRate;
    uint32_t size;
    uint32_t languageCode;
} SceAvPlayerAudio;

typedef struct SceAvPlayerVideo {
    uint32_t width;
    uint32_t height;
    float    aspectRatio;
    uint32_t languageCode;
} SceAvPlayerVideo;

typedef union SceAvPlayerStreamDetails {
    uint32_t reserved[4];
    SceAvPlayerAudio audio;
    SceAvPlayerVideo video;
} SceAvPlayerStreamDetails;

/* Video frames arrive with pData pointing at an NV12 buffer: a full-size Y
 * plane (width*height bytes, row pitch == width) followed immediately by a
 * half-height, interleaved U/V plane. That layout is the Vita/PS4 AvPlayer
 * default - unconfirmed on this project's target hardware. */
typedef struct SceAvPlayerFrameInfo {
    uint8_t *pData;
    uint32_t reserved;
    uint64_t timeStamp;
    SceAvPlayerStreamDetails details;
} SceAvPlayerFrameInfo;

typedef struct SceAvPlayerStreamInfo {
    uint32_t type; /* SceAvPlayerStreamType */
    uint32_t reserved;
    SceAvPlayerStreamDetails details;
    uint64_t duration;
    uint64_t startTime;
} SceAvPlayerStreamInfo;

SceAvPlayerHandle sceAvPlayerInit(SceAvPlayerInitData *pInit);
int32_t  sceAvPlayerAddSource(SceAvPlayerHandle h, const char *pFilename);
int32_t  sceAvPlayerClose(SceAvPlayerHandle h);
uint64_t sceAvPlayerCurrentTime(SceAvPlayerHandle h);
int32_t  sceAvPlayerDisableStream(SceAvPlayerHandle h, uint32_t streamID);
int32_t  sceAvPlayerEnableStream(SceAvPlayerHandle h, uint32_t streamID);
SceAvPlayerBool sceAvPlayerGetAudioData(SceAvPlayerHandle h, SceAvPlayerFrameInfo *pFrame);
SceAvPlayerBool sceAvPlayerGetStreamInfo(SceAvPlayerHandle h, uint32_t streamID, SceAvPlayerStreamInfo *pInfo);
SceAvPlayerBool sceAvPlayerGetVideoData(SceAvPlayerHandle h, SceAvPlayerFrameInfo *pFrame);
SceAvPlayerBool sceAvPlayerIsActive(SceAvPlayerHandle h);
int32_t  sceAvPlayerJumpToTime(SceAvPlayerHandle h, uint64_t argTime);
int32_t  sceAvPlayerPause(SceAvPlayerHandle h);
int32_t  sceAvPlayerResume(SceAvPlayerHandle h);
int32_t  sceAvPlayerSetLooping(SceAvPlayerHandle h, SceAvPlayerBool loop);
int32_t  sceAvPlayerStart(SceAvPlayerHandle h);
int32_t  sceAvPlayerStop(SceAvPlayerHandle h);
int32_t  sceAvPlayerStreamCount(SceAvPlayerHandle h);

#ifdef __cplusplus
}
#endif
#endif
