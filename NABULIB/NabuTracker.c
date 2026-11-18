// ****************************************************************************************
// NABU Tracker implementation
//
// Like NABU-LIB, every function is hand written Z80 assembly (__naked) with the C
// version kept in the comments above it. The tracker is included in every program
// that plays music, and SDCC links in every static function even when the program
// never calls it, so small code matters here. (The C version of the sound effect
// support was 1200 bytes, this is a little over half of that.)
//
// - Functions with one argument use __z88dk_fastcall, more than one __z88dk_callee.
// - IX is never touched. The shadow registers are not used.
// - Internal helpers start with ntr_ and are documented with their register contract.
// - See NOTE ON (void) PARAMETERS in NABU-LIB.c.
// ****************************************************************************************

// ***************************************************************************
// Internal helpers (register contracts in the comments)
// ***************************************************************************
static void nt_core(void) __naked {

  __asm

    ; ------------------------------------------------------------------------
    ; ntr_index: hl = hl + c
    ;   out: a changed. everything else kept
    ; ------------------------------------------------------------------------
    ntr_index:

      ld a, l
      add a, c
      ld l, a
      ret nc
      inc h
      ret

    ; ------------------------------------------------------------------------
    ; ntr_bit: a = 1 << c (c = channel 0 - 2)
    ;   out: everything else kept
    ; ------------------------------------------------------------------------
    ntr_bit:

      ld a, c
      inc a                   ; 1, 2, 3
      cp 3
      ret c
      inc a                   ; 3 -> 4
      ret

    ; ------------------------------------------------------------------------
    ; ntr_noiseDefault: give the noise period (R6) back to the song
    ;   out: a changed. everything else kept
    ; ------------------------------------------------------------------------
    ntr_noiseDefault:

      ld a, 6
      out (_IO_AYLATCH), a
      ld a, NT_NOISE_PERIOD
      out (_IO_AYDATA), a
      ret

    ; ------------------------------------------------------------------------
    ; ntr_volume: the volume a music channel should be at right now
    ;
    ; if (_ntPaused || _ntVol[channel] <= _ntAtten)
    ;   return 0;
    ;
    ; return _ntVol[channel] - _ntAtten;
    ;
    ;   in:  c = channel
    ;   out: a = volume. hl changed. bc, de kept
    ; ------------------------------------------------------------------------
    ntr_volume:

      ld a, (__ntPaused)
      or a
      jr nz, ntr_volMute

      ld hl, __ntVol
      call ntr_index
      ld a, (hl)
      ld hl, __ntAtten
      sub (hl)
      ret nc

    ntr_volMute:

      xor a
      ret

    ; ------------------------------------------------------------------------
    ; ntr_restore: write the note and volume to the AY from the copy the
    ; tracker keeps of a music channel. Only for a channel that no sound effect owns.
    ;
    ; AY[channel * 2]     = _NOTES_FINE[_ntNote[channel]];
    ; AY[channel * 2 + 1] = _NOTES_COURSE[_ntNote[channel]];
    ; AY[8 + channel]     = nt_volume(channel);
    ;
    ;   in:  c = channel
    ;   out: a, de, hl changed. bc kept
    ; ------------------------------------------------------------------------
    ntr_restore:

      ld hl, __ntNote
      call ntr_index
      ld e, (hl)
      ld d, 0                 ; de = note

      ; tone fine
      ld hl, __NOTES_FINE
      add hl, de
      ld a, c
      add a, a
      out (_IO_AYLATCH), a
      ld a, (hl)
      out (_IO_AYDATA), a

      ; tone coarse
      ld hl, __NOTES_COURSE
      add hl, de
      ld a, c
      add a, a
      inc a
      out (_IO_AYLATCH), a
      ld a, (hl)
      out (_IO_AYDATA), a

      ; volume
      call ntr_volume
      ld e, a
      ld a, c
      add a, 8
      out (_IO_AYLATCH), a
      ld a, e
      out (_IO_AYDATA), a
      ret

    ; ------------------------------------------------------------------------
    ; ntr_writeMixer: write the mixer (R7). This is the mixer setting of the song,
    ; except that a channel owned by a sound effect gets tone only (nt_sfxNote)
    ; or noise only (nt_sfxNoise). So a drum track that turned channel 0 into
    ; noise does not turn a tone effect on channel 0 into hiss, and the other
    ; way round. Bit 6 (AY port A = output, it carries the NABU interrupt
    ; mask) always comes from _ntMixer, which always has it set.
    ;
    ; uint8_t mixer   = _ntMixer;
    ; uint8_t toneBit = 0b00000001;   tone disable bit, noise disable is << 3
    ;
    ; for (ch = 0; ch < 3; ch++) {
    ;
    ;   if (_ntSfxTicks[ch]) {
    ;
    ;     mixer &= ~(toneBit | (toneBit << 3));   enable both
    ;
    ;     if (_ntSfxNoiseMask & toneBit)           disable the unused one
    ;       mixer |= toneBit;
    ;     else
    ;       mixer |= toneBit << 3;
    ;   }
    ;
    ;   toneBit <<= 1;
    ; }
    ;
    ; AY[7] = mixer;
    ;
    ;   out: a, bc, de, hl changed
    ; ------------------------------------------------------------------------
    ntr_writeMixer:

      ld a, (__ntMixer)
      ld e, a                 ; e = mixer
      ld a, (__ntSfxNoiseMask)
      ld d, a                 ; d = noise effect mask
      ld hl, __ntSfxTicks
      ld bc, 0x0301           ; b = 3 channels, c = tone bit

    ntr_wmLoop:

      ld a, (hl)
      or a
      jr z, ntr_wmNext

      ; enable tone and noise
      ld a, c
      add a, a
      add a, a
      add a, a
      or c
      cpl
      and e
      ld e, a

      ; then disable the one the effect does not use
      ld a, d
      and c
      ld a, c
      jr nz, ntr_wmSet
      add a, a
      add a, a
      add a, a

    ntr_wmSet:

      or e
      ld e, a

    ntr_wmNext:

      inc hl
      sla c
      djnz ntr_wmLoop

      ld a, 7
      out (_IO_AYLATCH), a
      ld a, e
      out (_IO_AYDATA), a
      ret

    ; ------------------------------------------------------------------------
    ; ntr_reserve: give a channel to a sound effect
    ;
    ; if (holdTicks == 0)          0 would mean "free"
    ;   holdTicks = 1;
    ;
    ; _ntSfxTicks[channel] = holdTicks;
    ;
    ; if (isNoise)
    ;   _ntSfxNoiseMask |= 1 << channel;
    ; else
    ;   _ntSfxNoiseMask &= ~(1 << channel);
    ;
    ; nt_writeMixer();
    ;
    ;   in:  e = channel (0 - 2), a = holdTicks, l = isNoise
    ;   out: a, hl changed. bc, de kept
    ; ------------------------------------------------------------------------
    ntr_reserve:

      or a
      jr nz, ntr_rsTicks
      inc a

    ntr_rsTicks:

      push bc
      push de

      ld c, e                 ; c = channel
      ld b, l                 ; b = isNoise
      ld e, a                 ; e = holdTicks

      ld hl, __ntSfxTicks
      call ntr_index
      ld (hl), e

      call ntr_bit
      ld e, a                 ; e = channel bit
      ld hl, __ntSfxNoiseMask
      ld a, b
      or a
      ld a, e
      jr nz, ntr_rsNoise

      cpl
      and (hl)
      jr ntr_rsStore

    ntr_rsNoise:

      or (hl)

    ntr_rsStore:

      ld (hl), a
      call ntr_writeMixer

      pop de
      pop bc
      ret

    ; ------------------------------------------------------------------------
    ; ntr_release: hand a channel back to the music. _ntSfxTicks[channel] must
    ; already be 0.
    ;
    ; if (_ntSfxNoiseMask & bit) {        the last noise effect gives the
    ;                                     noise period back to the song
    ;   _ntSfxNoiseMask &= ~bit;
    ;
    ;   if (_ntSfxNoiseMask == 0)
    ;     AY[6] = NT_NOISE_PERIOD;
    ; }
    ;
    ; nt_writeMixer();                    song mixer, note and volume, so the
    ; nt_restoreChannel(channel);         music comes back right now
    ;
    ;   in:  c = channel
    ;   out: a, de, hl changed. bc kept
    ; ------------------------------------------------------------------------
    ntr_release:

      call ntr_bit
      ld hl, __ntSfxNoiseMask
      ld e, a
      and (hl)
      jr z, ntr_rlMix

      ld a, e
      cpl
      and (hl)
      ld (hl), a
      call z, ntr_noiseDefault

    ntr_rlMix:

      push bc
      call ntr_writeMixer
      pop bc
      jp ntr_restore

    ; ------------------------------------------------------------------------
    ; ntr_clear3: write 0 to the 3 bytes at hl
    ;   out: a = 0, hl + 2. everything else kept
    ; ------------------------------------------------------------------------
    ntr_clear3:

      xor a
      ld (hl), a
      inc hl
      ld (hl), a
      inc hl
      ld (hl), a
      ret

  __endasm;
}

// ***************************************************************************
// Public functions (documented in NabuTracker.h)
// ***************************************************************************

static void nt_init(const uint16_t *song) __z88dk_fastcall __naked {

  // see NOTE ON (void) PARAMETERS in NABU-LIB.c
  (void)song;

  // _song = song;
  //
  // _itemCntr = 0;
  // _beatCntr = 0;
  // _ntPaused = 0;
  //
  // // Forget the notes of the previous song
  // _ntNote[0..2] = 0;
  // _ntVol[0..2]  = 0;
  //
  // // Enable only the Tone generators on A B C, disable all noise
  // _ntMixer = 0b01111000;
  //
  // // Noise period (R6), mixer, and every channel that is not playing a sound
  // // effect gets note 0 at volume zero. A sound effect that is still playing is
  // // left alone, so starting a song does not cut it off.
  // nt_refresh();
  __asm

      ld (__song), hl

      ld hl, 0
      ld (__itemCntr), hl
      ld (__beatCntr), hl

      ld hl, __ntNote
      call ntr_clear3
      ld hl, __ntVol
      call ntr_clear3

      ld (__ntPaused), a      ; a = 0 from ntr_clear3

      ld a, 0x78
      ld (__ntMixer), a

      jp _nt_refresh

  __endasm;
}

static void nt_stopSounds(void) __naked {

  // // Forget the music volumes and any sound effects, so nothing comes
  // // back later from nt_refresh() or a sound effect ending.
  // _ntVol[0..2]      = 0;
  // _ntSfxTicks[0..2] = 0;
  // _ntSfxNoiseMask   = 0;
  //
  // // Immediately mute all channels by setting amplitudes to zero.
  // // Mixer and tone/noise configuration are left as-is on purpose,
  // // so a subsequent nt_init() or manual AY configuration can reuse it.
  // ayWrite(8,  0);
  // ayWrite(9,  0);
  // ayWrite(10, 0);
  __asm

      ld hl, __ntVol
      call ntr_clear3
      ld hl, __ntSfxTicks
      call ntr_clear3
      ld (__ntSfxNoiseMask), a

      ld c, 8

    ntr_ssLoop:

      ld a, c
      out (_IO_AYLATCH), a
      xor a
      out (_IO_AYDATA), a
      inc c
      ld a, c
      cp 11
      jr nz, ntr_ssLoop

      ret

  __endasm;
}

static void nt_refresh(void) __naked {

  // if (_ntSfxNoiseMask == 0)
  //   ayWrite(6, NT_NOISE_PERIOD);
  //
  // nt_writeMixer();
  //
  // for (uint8_t ch = 0; ch < 3; ch++)
  //   if (_ntSfxTicks[ch] == 0)
  //     nt_restoreChannel(ch);
  __asm

      ld a, (__ntSfxNoiseMask)
      or a
      call z, ntr_noiseDefault

      call ntr_writeMixer

      ld c, 0

    ntr_rfLoop:

      ld hl, __ntSfxTicks
      call ntr_index
      ld a, (hl)
      or a
      call z, ntr_restore

      inc c
      ld a, c
      cp 3
      jr c, ntr_rfLoop

      ret

  __endasm;
}

static void nt_pause(void) __naked {

  // _ntPaused = 1;
  //
  // nt_refresh();
  __asm

      ld a, 1
      ld (__ntPaused), a
      jp _nt_refresh

  __endasm;
}

static void nt_resume(void) __naked {

  // _ntPaused = 0;
  //
  // nt_refresh();
  __asm

      xor a
      ld (__ntPaused), a
      jp _nt_refresh

  __endasm;
}

static void nt_setAttenuation(uint8_t attenuation) __z88dk_fastcall __naked {

  // see NOTE ON (void) PARAMETERS in NABU-LIB.c
  (void)attenuation;

  // // 15 or more is silent
  // if (attenuation > 15)
  //   attenuation = 15;
  //
  // _ntAtten = attenuation;
  //
  // nt_refresh();
  __asm

      ld a, l
      cp 16
      jr c, ntr_saStore
      ld a, 15

    ntr_saStore:

      ld (__ntAtten), a
      jp _nt_refresh

  __endasm;
}

static void nt_sfxNote(uint8_t channel, uint8_t note, uint16_t delayLength, uint8_t holdTicks) __z88dk_callee __naked {

  // see NOTE ON (void) PARAMETERS in NABU-LIB.c
  (void)channel;
  (void)note;
  (void)delayLength;
  (void)holdTicks;

  // if (channel > 2)
  //   return;
  //
  // nt_reserve(channel, holdTicks, 0);
  //
  // // Only touches this channel (unlike playNoteDelay), so the music
  // // keeps playing on the other two
  // playNoteEnvelope(channel, note, delayLength);
  __asm

      pop hl
      pop de                  ; e = channel, d = note
      pop bc                  ; bc = delayLength
      dec sp
      pop af                  ; a = holdTicks
      push hl

      ld h, a
      ld a, e
      cp 3
      ret nc
      ld a, h

      ld l, 0                 ; tone
      call ntr_reserve

      jp nl_playNoteEnv       ; NABU-LIB playNoteEnvelope(), e = channel, d = note, bc = delayLength

  __endasm;
}

static void nt_sfxNoise(uint8_t channel, uint8_t noisePeriod, uint16_t delayLength, uint8_t holdTicks) __z88dk_callee __naked {

  // see NOTE ON (void) PARAMETERS in NABU-LIB.c
  (void)channel;
  (void)noisePeriod;
  (void)delayLength;
  (void)holdTicks;

  // if (channel > 2)
  //   return;
  //
  // nt_reserve(channel, holdTicks, 1);
  //
  // // Noise pitch: 0 = highest hiss, 31 = lowest rumble
  // ayWrite(6, noisePeriod & 0x1F);
  //
  // // Envelope length, same register order (and same sound) as playNoteDelay()
  // ayWrite(11, delayLength >> 8);
  // ayWrite(12, delayLength & 0xff);
  //
  // // Envelope mode on this channel only, then start a single decay
  // ayWrite(8 + channel, 0b00010000);
  // ayWrite(13, 0b00000000);
  __asm

      pop hl
      pop de                  ; e = channel, d = noisePeriod
      pop bc                  ; bc = delayLength
      dec sp
      pop af                  ; a = holdTicks
      push hl

      ld h, a
      ld a, e
      cp 3
      ret nc
      ld a, h

      ld l, 1                 ; noise
      call ntr_reserve

      ld a, 6
      out (_IO_AYLATCH), a
      ld a, d
      and 0x1f
      out (_IO_AYDATA), a

      ld a, 11
      out (_IO_AYLATCH), a
      ld a, b
      out (_IO_AYDATA), a

      ld a, 12
      out (_IO_AYLATCH), a
      ld a, c
      out (_IO_AYDATA), a

      ld a, e
      add a, 8
      out (_IO_AYLATCH), a
      ld a, 0x10
      out (_IO_AYDATA), a

      ld a, 13
      out (_IO_AYLATCH), a
      xor a
      out (_IO_AYDATA), a

      ret

  __endasm;
}

static void nt_sfxRelease(uint8_t channel) __z88dk_fastcall __naked {

  // see NOTE ON (void) PARAMETERS in NABU-LIB.c
  (void)channel;

  // if (channel > 2 || _ntSfxTicks[channel] == 0)
  //   return;
  //
  // _ntSfxTicks[channel] = 0;
  //
  // (then ntr_release, see its comment)
  __asm

      ld a, l
      cp 3
      ret nc

      ld c, a
      ld hl, __ntSfxTicks
      call ntr_index
      ld a, (hl)
      or a
      ret z

      ld (hl), 0
      jp ntr_release

  __endasm;
}

static void nt_handleNote(void) __naked {

  // // Sound effects count down even when no song is loaded or the music
  // // is paused, so a game can use them on their own.
  // for (uint8_t ch = 0; ch < 3; ch++) {
  //
  //   const uint8_t t = _ntSfxTicks[ch];
  //
  //   if (t == 0 || t == NT_SFX_HOLD)
  //     continue;
  //
  //   if (t == 1)
  //     nt_sfxRelease(ch);
  //   else
  //     _ntSfxTicks[ch] = t - 1;
  // }
  //
  // if (_ntPaused)
  //   return;
  //
  // // Defensive: if nt_init() was never called, _song is NULL (static
  // // zero-init) and dereferencing it would feed garbage into the AY
  // // registers. Cheap to guard against.
  // if (_song == 0)
  //   return;
  //
  // // Walk the song with a pointer instead of indexing song[_itemCntr + n].
  // const uint16_t *p = _song + _itemCntr;
  //
  // // Process all events scheduled for the current beat.
  // // Multiple items can share the same time value and will be processed
  // // sequentially before advancing the beat counter.
  // //
  // // Every command updates the tracker's copy of the channel (_ntNote,
  // // _ntVol, _ntMixer) and only writes the AY when no sound effect owns the
  // // channel. When the effect ends, nt_sfxRelease() plays the copy.
  // while (*p == _beatCntr) {
  //
  //   p++;
  //
  //   switch (*p) {
  //
  //     case NT_NOTE_OFF: {
  //
  //       // [time, NT_NOTE_OFF, channel]
  //       // Bounds-check the channel before writing: an out-of-range
  //       // value would otherwise hit AY registers 11+ (envelope period
  //       // and shape) and corrupt the chip's audio state.
  //       const uint8_t channel = (uint8_t)p[1];
  //
  //       if (channel < 3) {
  //
  //         _ntVol[channel] = 0;
  //
  //         if (_ntSfxTicks[channel] == 0)
  //           ayWrite(8 + channel, 0);
  //       }
  //
  //       p += 2;
  //
  //       break;
  //     }
  //
  //     case NT_NOTE_ON: {
  //
  //       // [time, NT_NOTE_ON, channel, noteIndex, volume]
  //       // Volume is masked to 4 bits because bit 4 of the volume
  //       // register is the AY's envelope-mode select - a stray high
  //       // bit in song data would otherwise switch the channel into
  //       // envelope mode and the note would go silent.
  //       const uint8_t channel = (uint8_t)p[1];
  //
  //       if (channel < 3) {
  //
  //         _ntNote[channel] = (uint8_t)p[2];
  //         _ntVol[channel]  = (uint8_t)p[3] & 0x0F;
  //
  //         if (_ntSfxTicks[channel] == 0)
  //           nt_restoreChannel(channel);
  //       }
  //
  //       p += 4;
  //
  //       break;
  //     }
  //
  //     case NT_SND_CHNG: {
  //
  //       // [time, NT_SND_CHNG, isToneMode]
  //       // When isToneMode is non-zero, the mixer is configured for
  //       // pure tone (no noise). When zero, channel A is switched to
  //       // noise mode (drum) while B and C remain tone channels.
  //       // Volume for channel A is reset to zero so the caller can
  //       // explicitly set the desired volume for the next hit.
  //       _ntVol[0] = 0;
  //
  //       if (_ntSfxTicks[0] == 0)
  //         ayWrite(8, 0);
  //
  //       if (p[1])
  //         _ntMixer = 0b01111000;   // Note (tone) mode on all channels
  //       else
  //         _ntMixer = 0b01110001;   // Noise on A (drum), tone on B and C
  //
  //       nt_writeMixer();
  //
  //       p += 2;
  //
  //       break;
  //     }
  //
  //     case NT_LOOP: {
  //
  //       // Loop back to the beginning of the song, and keep processing
  //       // within the same tick so the first beat plays without a gap.
  //       p = _song;
  //
  //       _beatCntr = 0;
  //
  //       break;
  //     }
  //
  //     default: {
  //
  //       // Unknown command in song data. To avoid walking off into
  //       // random memory, stop processing this tick.
  //       // (_itemCntr is left on the command.)
  //       _itemCntr = p - _song;
  //
  //       return;
  //     }
  //   }
  // }
  //
  // _itemCntr = p - _song;
  //
  // _beatCntr++;
  __asm

      ; ---- sound effect countdown
      ld c, 0

    ntr_hnSfx:

      ld hl, __ntSfxTicks
      call ntr_index
      ld a, (hl)
      or a
      jr z, ntr_hnSfxNext

      inc a
      jr z, ntr_hnSfxNext     ; NT_SFX_HOLD (255)

      dec (hl)
      call z, ntr_release     ; keeps c

    ntr_hnSfxNext:

      inc c
      ld a, c
      cp 3
      jr c, ntr_hnSfx

      ; ---- paused or no song?
      ld a, (__ntPaused)
      or a
      ret nz

      ld hl, (__song)
      ld a, h
      or l
      ret z

      ; hl = p = _song + _itemCntr (16 bit entries)
      ld de, (__itemCntr)
      ex de, hl
      add hl, hl
      add hl, de

    ntr_hnLoop:

      ; while (*p == _beatCntr)
      ld de, (__beatCntr)
      ld a, (hl)
      cp e
      jr nz, ntr_hnDone
      inc hl
      ld a, (hl)
      dec hl
      cp d
      jr nz, ntr_hnDone

      inc hl
      inc hl                  ; p++, hl = command

      ; commands are 16 bit, so the high byte must be 0
      inc hl
      ld a, (hl)
      dec hl
      or a
      jr nz, ntr_hnUnknown

      ld a, (hl)
      inc hl
      inc hl                  ; hl = first parameter

      cp NT_NOTE_ON
      jp z, ntr_hnNoteOn
      cp NT_NOTE_OFF
      jp z, ntr_hnNoteOff
      cp NT_SND_CHNG
      jp z, ntr_hnSndChng
      cp NT_LOOP
      jr nz, ntr_hnBadCmd

      ; NT_LOOP: back to the start of the song, same tick
      ld hl, 0
      ld (__beatCntr), hl
      ld hl, (__song)
      jp ntr_hnLoop

    ntr_hnBadCmd:

      dec hl
      dec hl                  ; back on the command

    ntr_hnUnknown:

      ; _itemCntr = p - _song, the beat does not advance
      jr ntr_hnStoreItem

    ntr_hnDone:

      call ntr_hnStoreItem

      ld hl, (__beatCntr)
      inc hl
      ld (__beatCntr), hl
      ret

    ntr_hnStoreItem:

      ; _itemCntr = (hl - _song) / 2
      ld de, (__song)
      or a
      sbc hl, de
      srl h
      rr l
      ld (__itemCntr), hl
      ret

    ntr_hnNoteOff:

      ld c, (hl)              ; c = channel
      inc hl
      inc hl                  ; p += 2

      ld a, c
      cp 3
      jp nc, ntr_hnLoop

      push hl

      ld hl, __ntVol
      call ntr_index
      ld (hl), 0

      ld hl, __ntSfxTicks
      call ntr_index
      ld a, (hl)
      or a
      jr nz, ntr_hnPop

      ld a, c
      add a, 8
      out (_IO_AYLATCH), a
      xor a
      out (_IO_AYDATA), a

    ntr_hnPop:

      pop hl
      jp ntr_hnLoop

    ntr_hnNoteOn:

      ld c, (hl)              ; c = channel
      inc hl
      inc hl
      ld e, (hl)              ; e = note
      inc hl
      inc hl
      ld d, (hl)              ; d = volume
      inc hl
      inc hl                  ; p += 4

      ld a, c
      cp 3
      jp nc, ntr_hnLoop

      push hl

      ld hl, __ntNote
      call ntr_index
      ld (hl), e

      ld hl, __ntVol
      call ntr_index
      ld a, d
      and 0x0f
      ld (hl), a

      ld hl, __ntSfxTicks
      call ntr_index
      ld a, (hl)
      or a
      call z, ntr_restore

      pop hl
      jp ntr_hnLoop

    ntr_hnSndChng:

      ld a, (hl)
      inc hl
      or (hl)                 ; isToneMode (16 bit) != 0 ?
      inc hl                  ; p += 2

      push hl

      ld b, 0x78              ; Note (tone) mode on all channels
      jr nz, ntr_hnMixer
      ld b, 0x71              ; Noise on A (drum), tone on B and C

    ntr_hnMixer:

      ld a, b
      ld (__ntMixer), a

      xor a
      ld (__ntVol), a         ; _ntVol[0] = 0

      ld a, (__ntSfxTicks)
      or a
      jr nz, ntr_hnWriteMixer

      ld a, 8
      out (_IO_AYLATCH), a
      xor a
      out (_IO_AYDATA), a

    ntr_hnWriteMixer:

      call ntr_writeMixer

      pop hl
      jp ntr_hnLoop

  __endasm;
}
