static void nt_init(const uint16_t *song) {

  _song = song;
  
  _itemCntr = 0;
  _beatCntr = 0;

  // Noise envelope initial configuration
  ayWrite(6, 0b00000001);

  // Enable only the Tone generators on A B C, disable all noise
  ayWrite(7, 0b01111000);

  // Set the amplitude (volume) to zero on all channels
  ayWrite(8,  0);
  ayWrite(9,  0);
  ayWrite(10, 0);
}

static void nt_stopSounds(void) {

  // Immediately mute all channels by setting amplitudes to zero.
  // Mixer and tone/noise configuration are left as-is on purpose,
  // so a subsequent nt_init() or manual AY configuration can reuse it.
  ayWrite(8,  0);
  ayWrite(9,  0);
  ayWrite(10, 0);
}

static void nt_handleNote(void) {

  // Defensive: if nt_init() was never called, _song is NULL (static
  // zero-init) and dereferencing it would feed garbage into the AY
  // registers. Cheap to guard against.
  if (_song == 0)
    return;

  // Local alias so the compiler can keep the song pointer in a register
  // instead of reloading the global each time.
  const uint16_t *song = _song;

  // Process all events scheduled for the current beat.
  // Multiple items can share the same time value and will be processed
  // sequentially before advancing the beat counter.
  while (song[_itemCntr] == _beatCntr) {

    _itemCntr++;

    // command
    switch (song[_itemCntr]) {

      case NT_NOTE_OFF: {

        // Parameters:
        //   [time, NT_NOTE_OFF, channel]
        //
        // Bounds-check the channel before writing: an out-of-range
        // value would otherwise hit AY registers 11+ (envelope period
        // and shape) and corrupt the chip's audio state.
        const uint8_t channel = (uint8_t)song[_itemCntr + 1];

        if (channel < 3)
          ayWrite(8 + channel, 0);

        _itemCntr += 2;

        break;
      }

      case NT_NOTE_ON: {

        // Parameters:
        //   [time, NT_NOTE_ON, channel, noteIndex, volume]
        //
        // Channel mapping (0=A, 1=B, 2=C):
        //   tone period   -> AY registers (channel*2)   and (channel*2 + 1)
        //   channel volume -> AY register  (8 + channel)
        //
        // Volume is masked to 4 bits because bit 4 of the volume
        // register is the AY's envelope-mode select - a stray high
        // bit in song data would otherwise switch the channel into
        // envelope mode and the note would go silent.
        //
        // _NOTES_FINE and _NOTES_COURSE are external lookup tables in
        // NABU-LIB that map note index to tone period for the AY-3-8910.
        const uint8_t channel = (uint8_t)song[_itemCntr + 1];
        const uint8_t note    = (uint8_t)song[_itemCntr + 2];
        const uint8_t volume  = (uint8_t)song[_itemCntr + 3];

        if (channel < 3) {

          const uint8_t toneReg = channel << 1;

          ayWrite(toneReg,     _NOTES_FINE[note]);
          ayWrite(toneReg + 1, _NOTES_COURSE[note]);
          ayWrite(8 + channel, volume & 0x0F);
        }

        _itemCntr += 4;

        break;
      }

      case NT_SND_CHNG: {

        // Parameters:
        //   [time, NT_SND_CHNG, isToneMode]
        //
        // When isToneMode is non-zero, the mixer is configured for
        // pure tone (no noise). When zero, channel A is switched to
        // noise mode (drum) while B and C remain tone channels.
        //
        // Volume for channel A is reset to zero so the caller can
        // explicitly set the desired volume for the next hit.

        ayWrite(8,  0);
        
        if (song[_itemCntr + 1]) {

          // Note (tone) mode on all channels
          ayWrite(7, 0b01111000);
        } else {

          // Noise mode enabled on channel A, tone disabled on A and
          // still enabled on B and C. This matches the original design
          // of using channel 0 as a drum/noise source.
          ayWrite(7, 0b01110001);
        }

        _itemCntr += 2;
        
        break;
      }
      
      case NT_LOOP: {

        // Loop back to the beginning of the song.
        // The beat counter is reset and the item counter is rewound,
        // then processing continues within the same tick so that
        // the first beat of the loop can be played without a gap.
        _itemCntr = 0;
        
        _beatCntr = 0;

        break;
      }

      default: {

        // Unknown command in song data.
        // To avoid walking off into random memory, stop processing
        // this tick. You could also choose to call nt_stopSounds()
        // or null out _song here if you prefer a hard fail.
        return;
      }
    }
  }

  _beatCntr++;
}
