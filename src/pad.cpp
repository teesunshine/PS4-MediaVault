#include "pad.h"
#include <orbis/Pad.h>
#include <orbis/UserService.h>

bool Pad::Init() {
    if (scePadInit() != 0) return false;
    OrbisUserServiceInitializeParams param;
    param.priority = ORBIS_KERNEL_PRIO_FIFO_LOWEST;
    sceUserServiceInitialize(&param);
    int userId = 0;
    if (sceUserServiceGetInitialUser(&userId) != 0) return false;
    handle = scePadOpen(userId, 0, 0, NULL);
    return handle >= 0;
}

void Pad::Poll() {
    OrbisPadData data;
    if (handle < 0 || scePadReadState(handle, &data) != 0) return;
    prev = cur;
    cur = data.buttons;
    for (int i = 0; i < 32; i++) {
        if (cur & (1u << i)) hold[i]++;
        else hold[i] = 0;
    }
}

bool Pad::Pressed(uint32_t button) const {
    return (cur & button) && !(prev & button);
}

bool Pad::Repeat(uint32_t button) const {
    if (!(cur & button)) return false;
    int idx = __builtin_ctz(button);
    int h = hold[idx];
    if (h == 1) return true;
    return h > 24 && ((h - 24) % 4 == 0);   /* ~0.4s delay, then ~15 steps/s */
}
