#ifndef RETRONET_NABUTRACKER
#define RETRONET_NABUTRACKER

// Channel
// -------
// 0) noise (drum)
// 1) notes
// 2) notes
//
// Commands       Parameters
// -----          ----------
// NT_NOTE_OFF    channel
// NT_NOTE_ON     channel, note, volume (0x00-0x0f)
// NT_SND_CHNG    TRUE = note | FALSE = noise
// NT_LOOP        (Note: a loop MUST be placed at the end of your track)

#define NT_SND_CHNG 0
#define NT_NOTE_OFF 10
#define NT_NOTE_ON  20
#define NT_LOOP     99

uint16_t _itemCntr;
uint8_t _beatCntr;

uint8_t *_song;

void nt_init(uint8_t *song);

void nt_stopSounds();

void nt_handleNote();

#include "NabuTracker.c"

#endif