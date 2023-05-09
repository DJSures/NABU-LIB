// ****************************************************************************************
// NABU-LIB NABU TRACKER - MAKE SOMETHING
//
// DJ Sures (c) 2023
// https://nabu.ca
// 
// Last updated on March 22, 2023 (v2023.03.22.00)
// 
// Get latest copy and examples from: https://github.com/DJSures/NABU-LIB
//
// This is a lightweight NABU music tracker. 
// 
// Read the summaries below for details of each function in this file.
// 
// Make something,
// DJ Sures
// **********************************************************************************************

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

// No touchy
uint16_t _itemCntr;
uint16_t _beatCntr;
uint16_t *_song;

// ***************************************************************************
// Initialize the tracker with the specified song
// ***************************************************************************
void nt_init(uint16_t *song);

// ***************************************************************************
// Called on each iteration of your program that advances the music by 1/16
// ***************************************************************************
void nt_handleNote();

// ***************************************************************************
// Stop all sounds becuase you're done playing music
// ***************************************************************************
void nt_stopSounds();

#include "NabuTracker.c"

#endif