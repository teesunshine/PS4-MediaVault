#ifndef MV_PAD_H
#define MV_PAD_H
#include <stdint.h>

/* Minimal controller wrapper: edge detection plus key-repeat for held buttons. */
class Pad {
public:
    bool Init();
    void Poll();                       /* call once per loop iteration (~60 Hz) */
    bool Pressed(uint32_t button) const;   /* true only on the frame it went down */
    bool Repeat(uint32_t button) const;    /* pressed, or auto-repeating while held */
private:
    int handle = -1;
    uint32_t cur = 0, prev = 0;
    int hold[32] = {0};
};

#endif
