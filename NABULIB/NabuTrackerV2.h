// ****************************************************************************************
// NABU-LIB NABU TRACKER V2 - MAKE SOMETHING
//
// DJ Sures (c) 2026
// https://nabu.ca
//
// Plays NabuTracker v2 songs and sound effects on the AY-3-8910. Write songs in the
// music tracker at https://nabu.ca/music (it imports MIDI files too), download the
// C header, and play it:
//
//   #define BIN_TYPE BIN_HOMEBREW
//   #include "NABU-LIB.h"
//   #include "NabuTrackerV2.h"
//   #include "mysong.h"              // const uint8_t mysong[] = { ... } from the tracker
//
//   void main() {
//
//     initNABULib();
//     vdp_enableVDPReadyInt();
//
//     nt2_init(mysong);
//
//     while (true) {
//
//       vdp_waitVDPReadyInt();       // once per frame (60 times a second)
//       nt2_tick();
//
//       if (fired)
//         nt2_sfx(mysong, MYSONG_SFX_LASER, NT2_SFX_AUTO);
//     }
//   }
//
// Call nt2_tick() exactly once per video frame: the tracker plays songs at the
// NABU's 59.94 frames a second, the same as the editor.
//
// WHAT IS NEW COMPARED TO NabuTracker (v1)
// ----------------------------------------
// - Songs are bytes instead of 16 bit words, and patterns are split per channel and
//   shared, so a song is usually 5 - 10 times smaller.
// - Instruments: volume envelopes, arpeggios (chords), pitch sweeps and vibrato, noise
//   (drums) and the AY hardware envelope ("buzzer" bass), all stepped once per frame.
// - Sound effects are part of the same file and play on top of the music. The music
//   keeps running underneath and comes back on that channel when the effect ends.
// - Pause, resume, a master music volume for fades, and the play position for syncing
//   your game to the music.
// - v1 (NabuTracker.h) still works. The two can be included together.
//
// HOW IT WORKS
// ------------
// Every frame the tracker works out what each of the three channels should play
// (music, or a sound effect when one is playing on that channel) and writes AY
// registers 0 - 12 (13, the envelope shape, only when a note starts an envelope).
// So nothing else should write the AY while the tracker is running, or the next
// nt2_tick() overwrites it. Use nt2_sfx() for sound effects instead.
//
// It takes about 6000 T-states a frame on average (10% of a 60Hz frame), up to about
// 17000 on a row where all three channels start new notes. The player is about 1.4 KB
// of code and 183 bytes of RAM.
//
// SONG FORMAT
// -----------
// Documented at the top of nt2core.js (the editor) and in the comments of
// NabuTrackerV2.c. You don't need it to use the tracker.
//
// Make something,
// DJ Sures
// ****************************************************************************************

#ifndef RETRONET_NABUTRACKERV2
#define RETRONET_NABUTRACKERV2

// nt2_sfx() channel: use a channel that has no sound effect (2, then 1, then 0)
#define NT2_SFX_AUTO 0xFF

// Voice layout (one per channel for music, one per channel for sound effects).
// Used by the assembly in NabuTrackerV2.c.
#define NT2_V_ACTIVE   0     // a note is playing
#define NT2_V_NOTE     1     // note 0 - 95
#define NT2_V_CHANVOL  2     // channel volume 0 - 15
#define NT2_V_SLIDE    3     // signed period change per tick
#define NT2_V_PACC     4     // word: accumulated period offset (pitch macro + slide)
#define NT2_V_VOLM     6     // macro: word pointer, then position byte
#define NT2_V_ARPM     9
#define NT2_V_PITCHM   12
#define NT2_V_MODEM    15
#define NT2_V_ENV      18    // instrument envelope byte
#define NT2_V_ENVTRIG  19    // restart the hardware envelope on the next output
#define NT2_V_PERIOD   20    // word: tone period to output
#define NT2_V_AMP      22    // amplitude to output (0 - 15, 0x10 = envelope)
#define NT2_V_MODE     23    // tone / noise bits and noise period to output
#define NT2_V_TRACK    24    // word: music track read pointer
#define NT2_V_WAIT     26    // music rows left to wait
#define NT2_V_INST     27    // music instrument
#define NT2_V_SIZE     28

// Player state. Static so it stays private to the program that includes this header
// (the tracker is header only, like NabuTracker v1).
static const uint8_t *_nt2Song       = 0;
static uint8_t        _nt2Playing    = 0;
static uint8_t        _nt2Paused     = 0;
static uint8_t        _nt2Atten      = 0;
static uint8_t        _nt2Noise      = 0;
static uint8_t        _nt2Mixer      = 0x7F;
static uint8_t        _nt2Speed      = 6;
static uint8_t        _nt2TickCtr    = 0;
static uint8_t        _nt2Order      = 0;
static uint8_t        _nt2Row        = 0;
static uint8_t        _nt2Rows       = 0;
static uint8_t        _nt2SfxTicks[3] = { 0, 0, 0 };
static uint8_t        _nt2Voices[6 * NT2_V_SIZE] = { 0 };

// ***************************************************************************
// Start playing a song from the beginning. Sound effects that are playing
// carry on.
// ***************************************************************************
static void nt2_init(const uint8_t *song) __z88dk_fastcall;

// ***************************************************************************
// Advance the music and sound effects by one frame and write the AY.
// Call it once per video frame, also while paused or when only sound
// effects are used.
// ***************************************************************************
static void nt2_tick(void);

// ***************************************************************************
// Stop the music and all sound effects, and silence the AY.
// ***************************************************************************
static void nt2_stop(void);

// ***************************************************************************
// Pause / resume the music (silent while paused). Sound effects still play.
// ***************************************************************************
static void nt2_pause(void);
static void nt2_resume(void);

// ***************************************************************************
// Turn the music down: attenuation 0 = full volume, 15 = silent. The AY
// volume is logarithmic, so each step is about 3dB and a fade sounds even.
// Sound effects are not affected.
// ***************************************************************************
static void nt2_setAttenuation(uint8_t attenuation) __z88dk_fastcall;

// ***************************************************************************
// Play sound effect number sfx from bank (a song made in the tracker; its
// sound effect list) on channel 0 - 2, or NT2_SFX_AUTO. The music keeps
// playing on the other channels, and on this one again when the effect ends.
// The bank does not have to be the song that is playing.
// ***************************************************************************
static void nt2_sfx(const uint8_t *bank, uint8_t sfx, uint8_t channel) __z88dk_callee;

// ***************************************************************************
// Stop the sound effect on a channel now (the music comes back on it).
// ***************************************************************************
static void nt2_sfxStop(uint8_t channel) __z88dk_fastcall;

// ***************************************************************************
// Status (macros)
// ***************************************************************************

// Non-zero while the song is playing (it stops at the end if it does not loop)
#define nt2_isPlaying() (_nt2Playing)

// Non-zero while a sound effect plays on the channel
#define nt2_sfxIsActive(channel) (_nt2SfxTicks[(channel)] != 0)

// Position of the next row to play, for syncing the game to the music:
//   if (nt2_getRow() == 0 && nt2_getOrder() == 4) bossAppears();
#define nt2_getOrder() (_nt2Order)
#define nt2_getRow() (_nt2Row)

#include "NabuTrackerV2.c"

#endif
