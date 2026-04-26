// ****************************************************************************************
// NABU-LIB NABU TRACKER - MAKE SOMETHING
//
// DJ Sures (c) 2024
// https://nabu.ca
//
// Last updated on 2024.03.31.00
//
// Get latest copy and examples from: https://github.com/DJSures/NABU-LIB
//
// This is a lightweight NABU music tracker for the AY-3-8910 sound chip.
//
// REQUIREMENTS
// ------------
// NABU-LIB.h must be #included BEFORE NabuTracker.h. The tracker uses
// ayWrite(), the _NOTES_FINE / _NOTES_COURSE tone tables, and uint8_t /
// uint16_t typedefs - all of which come from NABU-LIB. NabuTracker.h
// intentionally does not include NABU-LIB.h itself, because NABU-LIB.h
// pulls in NABU-LIB.c at the bottom and must therefore be included from
// exactly one translation unit (your main.c).
//
// MINIMAL USAGE
// -------------
//   #define BIN_TYPE BIN_HOMEBREW
//   #include "NABU-LIB.h"
//   #include "NabuTracker.h"
//
//   // Song format: a flat array of uint16_t triplets/quads:
//   //   { time, command, params... , time, command, params... , ... }
//   //
//   // The last entry should be an NT_LOOP so the song wraps cleanly.
//   const uint16_t mySong[] = {
//     0,  NT_SND_CHNG, 1,           // tick 0: tone mode on all channels
//     0,  NT_NOTE_ON,  0, 24, 15,   // tick 0: ch 0 plays note 24 at vol 15
//     8,  NT_NOTE_OFF, 0,           // tick 8: ch 0 silenced
//     16, NT_LOOP,                  // tick 16: loop back to start
//   };
//
//   void main() {
//
//     initNABULib();
//     nt_init(mySong);
//
//     while (gameRunning) {
//
//       nt_handleNote();   // advance song by one beat (1/16 note)
//       waitForVSync();    // pace your tick however you like
//     }
//
//     nt_stopSounds();
//   }
//
// Make something,
// DJ Sures
// **********************************************************************************************

#ifndef RETRONET_NABUTRACKER
#define RETRONET_NABUTRACKER

// Channel / mixer model
// ---------------------
// The AY-3-8910 has three independent channels (A, B, C). Each channel
// can produce a tone, noise, or both, independently controlled by the
// mixer (register 7). The tracker exposes them as channel indices 0, 1,
// and 2 and uses NT_SND_CHNG to flip channel 0 between tone and noise:
//
//   NT_SND_CHNG 1  -> all three channels are tone (default after nt_init)
//   NT_SND_CHNG 0  -> channel 0 is noise (drum), channels 1/2 stay tone
//
// So a typical drum-and-melody track uses NT_SND_CHNG 0 to turn channel 0
// into a percussive voice while channels 1 and 2 carry the melody/harmony.
//
// Commands       Parameters
// --------       ----------
// NT_NOTE_OFF    channel                       (channel 0-2)
// NT_NOTE_ON     channel, noteIndex, volume    (volume 0-15; index into NABU-LIB note table)
// NT_SND_CHNG    isToneMode                    (1 = tone on ch 0, 0 = noise on ch 0)
// NT_LOOP        (must be the final entry of your track)

#define NT_SND_CHNG 0
#define NT_NOTE_OFF 10
#define NT_NOTE_ON  20
#define NT_LOOP     99

// Internal tracker state
// ----------------------
// These are defined as static so that including this header in multiple
// translation units does not cause multiple-definition linker errors.
// The tracker is effectively header-only and the state is private to
// the translation unit that includes this header.

static uint16_t _itemCntr;
static uint16_t _beatCntr;
static const uint16_t *_song;

// ***************************************************************************
// Initialize the tracker with the specified song.
//
// The song data is an array of uint16_t encoded as:
//   [time, command, params...]
// where "time" is the beat index (1/16th note resolution) and "command"
// is one of NT_NOTE_ON, NT_NOTE_OFF, NT_SND_CHNG, or NT_LOOP.
//
// A NT_LOOP command must be placed at the end of the track so playback
// can safely loop back to the beginning.
// ***************************************************************************
static void nt_init(const uint16_t *song);

// ***************************************************************************
// Called once per tick of your main loop to advance the music by 1/16 note.
//
// Typical usage pattern:
//   nt_init(mySong);
//   while (gameIsRunning) {
//     nt_handleNote();   // advance song by one tick
//     // ... other game logic ...
//   }
// ***************************************************************************
static void nt_handleNote(void);

// ***************************************************************************
// Stop all sounds because you're done playing music.
// This immediately mutes all AY-3-8910 channels used by the tracker.
// ***************************************************************************
static void nt_stopSounds(void);

#include "NabuTracker.c"

#endif
