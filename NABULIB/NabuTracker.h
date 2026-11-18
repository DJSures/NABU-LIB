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

// Sound effects and music together
// --------------------------------
// The AY has only three channels, so a sound effect has to borrow one from
// the music. playNoteDelay() silences all three channels, and even
// playNoteEnvelope() (which only touches its own channel) has two problems
// when music is playing on that channel: the song's next NT_NOTE_ON cuts
// the effect off, and when the effect ends the channel stays silent until
// the song's next NT_NOTE_ON on it.
//
// The nt_sfx functions fix both. They reserve the channel for a number of
// ticks (calls to nt_handleNote). While reserved, the song keeps running
// and the tracker remembers what the channel should be playing, but does
// not write it to the AY. When the reservation ends, the remembered note
// and volume are put back straight away, so the music comes back in the
// middle of the note instead of waiting for the next one.
//
//   nt_sfxNote(2, 48, 10, 22);    // ch 2 beeps, music resumes on ch 2 after 22 ticks
//
// How many ticks? delayLength values below 256 decay in about
// delayLength * 37ms. At one nt_handleNote() per 60Hz frame that is about
// delayLength * 2.2 ticks (10 -> 22 ticks).
//
// Pick the channel the music needs least (often the drum or harmony
// channel). The envelope generator is shared by all channels, so two
// effects at the same time restart each other's envelope.

// holdTicks value for nt_sfxNote() / nt_sfxNoise(): keep the channel until
// nt_sfxRelease() is called (e.g. an engine sound while a button is held)
#define NT_SFX_HOLD 255

// Noise period (AY R6) the tracker uses for songs (drums on channel 0)
#define NT_NOISE_PERIOD 1

// Internal tracker state
// ----------------------
// These are defined as static so that including this header in multiple
// translation units does not cause multiple-definition linker errors.
// The tracker is effectively header-only and the state is private to
// the translation unit that includes this header.

static uint16_t _itemCntr = 0;
static uint16_t _beatCntr = 0;
static const uint16_t *_song = 0;

// What the song wants each channel to play, so it can be put back after a
// sound effect, a pause or an attenuation change. Volume 0 = note off.
static uint8_t _ntNote[3] = { 0, 0, 0 };
static uint8_t _ntVol[3]  = { 0, 0, 0 };

// What the song wants the mixer (R7) to be. Bit 6 must stay set (AY port A
// is an output, it carries the NABU interrupt mask).
static uint8_t _ntMixer = 0b01111000;

// Sound effect reservation per channel: 0 = free (music plays), 1-254 =
// ticks left, NT_SFX_HOLD = until nt_sfxRelease()
static uint8_t _ntSfxTicks[3] = { 0, 0, 0 };

// Bit per channel (bit 0 = channel 0): the effect on it is noise, not tone
static uint8_t _ntSfxNoiseMask = 0;

// Master attenuation subtracted from every music volume (0 - 15)
static uint8_t _ntAtten = 0;

// Non-zero while the music is paused by nt_pause()
static uint8_t _ntPaused = 0;

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
static void nt_init(const uint16_t *song) __z88dk_fastcall;

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
// This immediately mutes all AY-3-8910 channels used by the tracker,
// including any sound effect that is playing.
// ***************************************************************************
static void nt_stopSounds(void);

// ***************************************************************************
// Play a note with a decay envelope as a sound effect, borrowing the
// channel from the music for holdTicks calls of nt_handleNote() (or until
// nt_sfxRelease() with NT_SFX_HOLD). The music comes back on that channel
// as soon as the effect ends. The other two channels are not touched.
//
// - channel, note, delayLength: same as playNoteDelay()
// - holdTicks: 1-254, or NT_SFX_HOLD. See "Sound effects and music together"
//
// The channel plays tone only while it is borrowed, even if the song turned
// it into a drum (noise) channel.
//
// You can use the nt_sfx functions without a song. Keep calling
// nt_handleNote() so the reservation counts down.
// ***************************************************************************
static void nt_sfxNote(uint8_t channel, uint8_t note, uint16_t delayLength, uint8_t holdTicks) __z88dk_callee;

// ***************************************************************************
// Same as nt_sfxNote() but plays noise (explosions, crashes, thrust).
//
// - noisePeriod: 0 = highest hiss, 31 = lowest rumble
//
// The noise period is shared by all channels, so a song's drums sound
// different while a noise effect is playing. It goes back when it ends.
// ***************************************************************************
static void nt_sfxNoise(uint8_t channel, uint8_t noisePeriod, uint16_t delayLength, uint8_t holdTicks) __z88dk_callee;

// ***************************************************************************
// End the sound effect on the channel now and give it back to the music.
// Does nothing if the channel has no sound effect.
// ***************************************************************************
static void nt_sfxRelease(uint8_t channel) __z88dk_fastcall;

// ***************************************************************************
// Non-zero while a sound effect owns the channel (macro)
// ***************************************************************************
#define nt_sfxIsActive(channel) (_ntSfxTicks[(channel)] != 0)

// ***************************************************************************
// Pause the music: mutes the music and stops the song from advancing.
// Sound effects still play, so keep calling nt_handleNote() (e.g. for
// menu beeps on a pause screen).
// ***************************************************************************
static void nt_pause(void);

// ***************************************************************************
// Resume the music from where nt_pause() stopped it. Notes that were
// sounding when it was paused come back straight away.
// ***************************************************************************
static void nt_resume(void);

// ***************************************************************************
// Turn the music down without touching the song data: attenuation is
// subtracted from every music volume. 0 = full volume, 15 = silent.
// Each step is about 3dB (the AY volume is logarithmic), so this sounds
// even. Call it once per few ticks with 1, 2, 3... for a fade out.
// Sound effects are not affected.
// ***************************************************************************
static void nt_setAttenuation(uint8_t attenuation) __z88dk_fastcall;

// ***************************************************************************
// Write everything the music needs back to the AY (mixer, noise period,
// notes and volumes). Use it after your own code changed the AY
// registers, e.g. after a playNoteDelay() or initNABULIBAudio(), so the
// music does not wait for the next notes to come back.
// ***************************************************************************
static void nt_refresh(void);

// ***************************************************************************
// The beat the next nt_handleNote() will play (macro). Use it to sync game
// events to the music, e.g. flash something on every 4th beat:
//   if ((nt_getBeat() & 3) == 0)
// ***************************************************************************
#define nt_getBeat() (_beatCntr)

#include "NabuTracker.c"

#endif
