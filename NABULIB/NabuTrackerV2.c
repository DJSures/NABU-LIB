// ****************************************************************************************
// NABU Tracker v2 implementation
//
// Hand written Z80 assembly (__naked) like NABU-LIB. This does exactly what the Player
// class in the editor (Web/.../js/music/nt2core.js) does, register write for register
// write, so the editor plays what the NABU plays. Change one, change both, and run the
// differential test (C Programs/Tests/Test NT2) that compares them.
//
// - Functions with one argument use __z88dk_fastcall, more than one __z88dk_callee.
// - IY points at the voice being worked on (NT2_V_* offsets). IX is never touched.
//   The shadow registers are not used.
// - Internal helpers start with ntr2_ and are documented with their register contract.
// - Text in ; comments inside __asm goes through the C preprocessor, so no apostrophes
//   and no C macro names in them.
//
// SONG FORMAT (offsets from the start of the song, words little endian)
//  0  'N' '2'   2 flags   3 speed (ticks per row)   4 orderCount   5 loopOrder (0xFF = stop)
//  6  word orders   8 word tracks   10 word instruments   12 word sfx (0 = none)
//  Order: rows, track A, track B, track C.   Track table / instrument table: word offsets.
//  Track bytes: 00-5F note, 60 off, 80-9F instrument, A0-AF volume, B0 s slide,
//               B1 s speed, C0-FF wait (n & 63) + 1 rows. Others end the row.
//  Instrument: env byte (bit 7 on, bits 4-6 shift, 0-3 shape), then the volume, arpeggio,
//              pitch and mode macros, each: len, loop, len bytes.
//  Sfx table: count, then instrument, note, ticks for each.
// ****************************************************************************************

// ***************************************************************************
// Internal helpers (register contracts in the comments)
// ***************************************************************************
static void nt2_core(void) __naked {

  __asm

    ; ------------------------------------------------------------------------
    ; ntr2_field: hl = bc + word at (bc + a). Reads a header offset of a song.
    ;   in:  bc = song, a = header offset
    ;   out: hl = address. a, de changed, bc kept
    ; ------------------------------------------------------------------------
    ntr2_field:

      ld l, a
      ld h, 0
      add hl, bc
      ld e, (hl)
      inc hl
      ld d, (hl)
      ld h, b
      ld l, c
      add hl, de
      ret

    ; ------------------------------------------------------------------------
    ; ntr2_bit: a = 1 shifted left by a (a = 0 - 2)
    ; ------------------------------------------------------------------------
    ntr2_bit:

      inc a
      cp 3
      ret c
      inc a
      ret

    ; ------------------------------------------------------------------------
    ; ntr2_iyOff: hl = iy + a
    ;   out: a changed
    ; ------------------------------------------------------------------------
    ntr2_iyOff:

      push iy
      pop hl
      add a, l
      ld l, a
      ret nc
      inc h
      ret

    ; ------------------------------------------------------------------------
    ; ntr2_voice: iy = voice a (0 - 2 music, 3 - 5 sound effects)
    ;   out: a, de changed
    ; ------------------------------------------------------------------------
    ntr2_voice:

      ld iy, __nt2Voices
      ld de, NT2_V_SIZE
      or a
      ret z

    ntr2_voLoop:

      add iy, de
      dec a
      jr nz, ntr2_voLoop
      ret

    ; ------------------------------------------------------------------------
    ; ntr2_skipMacro: hl = hl + 2 + (hl), the next macro of an instrument
    ;   out: de changed
    ; ------------------------------------------------------------------------
    ntr2_skipMacro:

      ld e, (hl)
      ld d, 0
      inc hl
      inc hl
      add hl, de
      ret

    ; ------------------------------------------------------------------------
    ; ntr2_noteOn: start a note on a voice
    ;
    ; rec = bank + instrumentTable[inst]
    ; env = rec[0], then the 4 macro pointers, positions 0, pitch offset 0,
    ; restart the hardware envelope when the instrument uses it
    ;
    ;   in:  iy = voice, bc = bank (song), l = instrument, h = note
    ;   out: everything but ix, iy, bc changed
    ; ------------------------------------------------------------------------
    ntr2_noteOn:

      ld (iy + NT2_V_NOTE), h
      ld (iy + NT2_V_ACTIVE), 1

      push hl
      ld a, 10
      call ntr2_field         ; hl = instrument table
      ex de, hl
      pop hl
      ld h, 0                 ; hl = instrument
      add hl, hl
      add hl, de
      ld e, (hl)
      inc hl
      ld d, (hl)
      ld h, b
      ld l, c
      add hl, de              ; hl = instrument record

      ld a, (hl)
      ld (iy + NT2_V_ENV), a
      rla
      ld a, 0
      rla                     ; a = env bit 7
      ld (iy + NT2_V_ENVTRIG), a

      inc hl
      ld (iy + NT2_V_VOLM), l
      ld (iy + NT2_V_VOLM + 1), h
      call ntr2_skipMacro
      ld (iy + NT2_V_ARPM), l
      ld (iy + NT2_V_ARPM + 1), h
      call ntr2_skipMacro
      ld (iy + NT2_V_PITCHM), l
      ld (iy + NT2_V_PITCHM + 1), h
      call ntr2_skipMacro
      ld (iy + NT2_V_MODEM), l
      ld (iy + NT2_V_MODEM + 1), h

      xor a
      ld (iy + NT2_V_VOLM + 2), a
      ld (iy + NT2_V_ARPM + 2), a
      ld (iy + NT2_V_PITCHM + 2), a
      ld (iy + NT2_V_MODEM + 2), a
      ld (iy + NT2_V_PACC), a
      ld (iy + NT2_V_PACC + 1), a
      ret

    ; ------------------------------------------------------------------------
    ; ntr2_step: one step of a macro
    ;
    ; if (len == 0) return default;
    ; if (pos >= len) return data[len - 1] with carry set (finished);
    ; value = data[pos++];
    ; if (pos >= len && loop < len) pos = loop;
    ;
    ;   in:  hl = address of the macro field in the voice (pointer, then position)
    ;        b = default value for an empty macro
    ;   out: a = value. carry set = the macro has finished (no loop).
    ;        bc, de, hl changed
    ; ------------------------------------------------------------------------
    ntr2_step:

      ld e, (hl)
      inc hl
      ld d, (hl)
      inc hl                  ; de = macro, hl = position
      ld a, (de)
      or a
      jr nz, ntr2_stHave

      ld a, b                 ; empty, carry clear
      ret

    ntr2_stHave:

      ld c, a                 ; c = len
      ld b, (hl)              ; b = position
      ex de, hl               ; hl = macro, de = position
      inc hl
      ld a, (hl)              ; loop
      inc hl                  ; hl = data
      push af

      ld a, b
      cp c
      jr nc, ntr2_stFin

      add a, l
      ld l, a
      jr nc, ntr2_stNc
      inc h

    ntr2_stNc:

      inc b
      ld a, b
      cp c
      jr c, ntr2_stKeep

      pop af                  ; a = loop
      cp c
      jr nc, ntr2_stStore     ; no loop: stay at len
      ld b, a
      jr ntr2_stStore

    ntr2_stKeep:

      pop af

    ntr2_stStore:

      ld a, b
      ld (de), a
      ld a, (hl)
      or a                    ; carry clear
      ret

    ntr2_stFin:

      pop af
      ld a, c
      dec a
      add a, l
      ld l, a
      jr nc, ntr2_stNc2
      inc h

    ntr2_stNc2:

      ld a, (hl)
      scf
      ret

    ; ------------------------------------------------------------------------
    ; ntr2_update: step the instrument of a voice and work out its output
    ;
    ; vol  = volume macro (15), arp = arpeggio macro (0), pd = pitch macro
    ; (0, also 0 once finished), mode = mode macro (0x40)
    ; pitchOffset += pd + slide
    ; period = noteTable[clamp(note + arp, 0, 95)] + pitchOffset, clamped 1 - 4095
    ; amp = envelope ? (vol ? 0x10 : 0) : max(0, vol + chanVol - 15)
    ;
    ;   in:  iy = voice
    ;   out: everything but ix, iy changed
    ; ------------------------------------------------------------------------
    ntr2_update:

      ld a, (iy + NT2_V_ACTIVE)
      or a
      ret z

      ld a, NT2_V_VOLM
      call ntr2_iyOff
      ld b, 15
      call ntr2_step
      ld (iy + NT2_V_AMP), a  ; volume, until the end

      ld a, NT2_V_ARPM
      call ntr2_iyOff
      ld b, 0
      call ntr2_step
      push af                 ; arpeggio

      ld a, NT2_V_PITCHM
      call ntr2_iyOff
      ld b, 0
      call ntr2_step
      jr nc, ntr2_upPd
      xor a

    ntr2_upPd:

      ; pitch offset += signed pd + signed slide
      ld e, a
      rla
      sbc a, a
      ld d, a
      ld l, (iy + NT2_V_PACC)
      ld h, (iy + NT2_V_PACC + 1)
      add hl, de
      ld a, (iy + NT2_V_SLIDE)
      ld e, a
      rla
      sbc a, a
      ld d, a
      add hl, de
      ld (iy + NT2_V_PACC), l
      ld (iy + NT2_V_PACC + 1), h
      push hl                 ; pitch offset

      ld a, NT2_V_MODEM
      call ntr2_iyOff
      ld b, 0x40
      call ntr2_step
      ld (iy + NT2_V_MODE), a

      pop de                  ; de = pitch offset
      pop af                  ; a = arpeggio

      ; note + signed arpeggio, clamped 0 - 95
      ld l, a
      rla
      sbc a, a
      ld h, a
      ld c, (iy + NT2_V_NOTE)
      ld b, 0
      add hl, bc
      bit 7, h
      jr nz, ntr2_upN0
      ld a, h
      or a
      jr nz, ntr2_upNMax
      ld a, l
      cp 96
      jr c, ntr2_upNOk

    ntr2_upNMax:

      ld a, 95
      jr ntr2_upNOk

    ntr2_upN0:

      xor a

    ntr2_upNOk:

      ; period = note table + pitch offset
      ld l, a
      ld h, 0
      add hl, hl
      ld bc, ntr2_notes
      add hl, bc
      ld a, (hl)
      inc hl
      ld h, (hl)
      ld l, a
      add hl, de

      ; clamp to 1 - 4095 (signed)
      bit 7, h
      jr nz, ntr2_upP1
      ld a, h
      or l
      jr z, ntr2_upP1
      ld a, h
      cp 0x10
      jr c, ntr2_upPOk
      ld hl, 4095
      jr ntr2_upPOk

    ntr2_upP1:

      ld hl, 1

    ntr2_upPOk:

      ld (iy + NT2_V_PERIOD), l
      ld (iy + NT2_V_PERIOD + 1), h

      ; amplitude
      ld a, (iy + NT2_V_AMP)
      bit 7, (iy + NT2_V_ENV)
      jr z, ntr2_upVol
      or a
      jr z, ntr2_upStore
      ld a, 0x10
      jr ntr2_upStore

    ntr2_upVol:

      add a, (iy + NT2_V_CHANVOL)
      sub 15
      jr nc, ntr2_upStore
      xor a

    ntr2_upStore:

      ld (iy + NT2_V_AMP), a
      ret

    ; ------------------------------------------------------------------------
    ; ntr2_readTrack: read one row of a music track
    ;   in:  iy = music voice
    ;   out: everything but ix, iy changed
    ; ------------------------------------------------------------------------
    ntr2_readTrack:

      ld a, (iy + NT2_V_WAIT)
      or a
      jr z, ntr2_rtRead
      dec (iy + NT2_V_WAIT)
      ret

    ntr2_rtRead:

      ld l, (iy + NT2_V_TRACK)
      ld h, (iy + NT2_V_TRACK + 1)

    ntr2_rtLoop:

      ld a, (hl)
      inc hl
      cp 0x60
      jr c, ntr2_rtNote
      jr z, ntr2_rtOff
      cp 0x80
      jr c, ntr2_rtEnd
      cp 0xA0
      jr c, ntr2_rtInst
      cp 0xB0
      jr c, ntr2_rtVol
      jr z, ntr2_rtSlide
      cp 0xB1
      jr z, ntr2_rtSpeed
      cp 0xC0
      jr c, ntr2_rtEnd

      and 0x3F
      ld (iy + NT2_V_WAIT), a

    ntr2_rtEnd:

      ld (iy + NT2_V_TRACK), l
      ld (iy + NT2_V_TRACK + 1), h
      ret

    ntr2_rtOff:

      ld (iy + NT2_V_ACTIVE), 0
      jr ntr2_rtEnd

    ntr2_rtInst:

      and 31
      ld (iy + NT2_V_INST), a
      jr ntr2_rtLoop

    ntr2_rtVol:

      and 15
      ld (iy + NT2_V_CHANVOL), a
      jr ntr2_rtLoop

    ntr2_rtSlide:

      ld a, (hl)
      inc hl
      ld (iy + NT2_V_SLIDE), a
      jr ntr2_rtLoop

    ntr2_rtSpeed:

      ld a, (hl)
      inc hl
      ld (__nt2Speed), a
      jr ntr2_rtLoop

    ntr2_rtNote:

      ld (iy + NT2_V_TRACK), l
      ld (iy + NT2_V_TRACK + 1), h
      ld h, a
      ld l, (iy + NT2_V_INST)
      ld bc, (__nt2Song)
      jp ntr2_noteOn

    ; ------------------------------------------------------------------------
    ; ntr2_loadOrder: start order a (row 0, the 3 track pointers, waits and
    ; slides cleared)
    ;   out: everything but ix changed (iy too)
    ; ------------------------------------------------------------------------
    ntr2_loadOrder:

      ld (__nt2Order), a
      push af
      xor a
      ld (__nt2Row), a

      ld bc, (__nt2Song)
      ld a, 6
      call ntr2_field         ; hl = order list
      pop af
      ld e, a
      ld d, 0
      ex de, hl
      add hl, hl
      add hl, hl
      add hl, de              ; hl = order entry

      ld a, (hl)
      ld (__nt2Rows), a
      inc hl
      push hl
      ld a, 8
      call ntr2_field         ; hl = track table
      ex de, hl
      pop hl                  ; hl = track numbers

      ld iy, __nt2Voices
      ld b, 3

    ntr2_loLoop:

      push bc
      push hl
      ld l, (hl)
      ld h, 0
      add hl, hl
      add hl, de
      ld c, (hl)
      inc hl
      ld b, (hl)
      ld hl, (__nt2Song)
      add hl, bc
      ld (iy + NT2_V_TRACK), l
      ld (iy + NT2_V_TRACK + 1), h
      ld (iy + NT2_V_WAIT), 0
      ld (iy + NT2_V_SLIDE), 0
      ld bc, NT2_V_SIZE
      add iy, bc
      pop hl
      inc hl
      pop bc
      djnz ntr2_loLoop

      ret

    ; ------------------------------------------------------------------------
    ; ntr2_row: play one row of the song, then move to the next row / order
    ;   out: everything but ix changed
    ; ------------------------------------------------------------------------
    ntr2_row:

      xor a
      call ntr2_voice
      call ntr2_readTrack
      ld a, 1
      call ntr2_voice
      call ntr2_readTrack
      ld a, 2
      call ntr2_voice
      call ntr2_readTrack

      ld hl, __nt2Row
      inc (hl)
      ld a, (hl)
      ld hl, __nt2Rows
      cp (hl)
      ret c

      ; next order, or the loop order, or stop
      ld hl, (__nt2Song)
      inc hl
      inc hl
      inc hl
      inc hl                  ; hl = order count
      ld a, (__nt2Order)
      inc a
      cp (hl)
      jr c, ntr2_rwLoad

      inc hl
      ld a, (hl)              ; loop order
      cp 0xFF
      jr nz, ntr2_rwLoad

      xor a
      ld (__nt2Playing), a
      ld (__nt2Voices + NT2_V_ACTIVE), a
      ld (__nt2Voices + NT2_V_SIZE + NT2_V_ACTIVE), a
      ld (__nt2Voices + NT2_V_SIZE * 2 + NT2_V_ACTIVE), a
      ret

    ntr2_rwLoad:

      jp ntr2_loadOrder

    ; ------------------------------------------------------------------------
    ; ntr2_outCh: write one channel to the AY: the sound effect voice when one
    ; is playing on it, else the music voice
    ;
    ; R(2c), R(2c+1) = period
    ; if the voice plays (and it is a sound effect or the music is not paused):
    ;   amp = voice amp, music minus the attenuation
    ;   enable tone / noise in the mixer, noise period, hardware envelope
    ;   period (period shifted right), envelope shape when a note started it
    ; R(8+c) = amp (0 when not playing)
    ;
    ;   in:  c = channel
    ;   out: everything but ix changed (iy too)
    ; ------------------------------------------------------------------------
    ntr2_outCh:

      ld hl, __nt2SfxTicks
      ld b, 0
      add hl, bc
      ld a, (hl)
      or a
      ld a, c
      jr z, ntr2_ocMusic
      add a, 3
      ld b, 1                 ; b = 1: sound effect

    ntr2_ocMusic:

      call ntr2_voice

      ld a, c
      add a, a
      out (_IO_AYLATCH), a
      ld a, (iy + NT2_V_PERIOD)
      out (_IO_AYDATA), a
      ld a, c
      add a, a
      inc a
      out (_IO_AYLATCH), a
      ld a, (iy + NT2_V_PERIOD + 1)
      out (_IO_AYDATA), a

      ld e, 0                 ; e = amp
      ld a, (iy + NT2_V_ACTIVE)
      or a
      jp z, ntr2_ocAmp
      ld a, b
      or a
      jr nz, ntr2_ocOn
      ld a, (__nt2Paused)
      or a
      jp nz, ntr2_ocAmp

    ntr2_ocOn:

      ld e, (iy + NT2_V_AMP)
      ld a, b
      or a
      jr nz, ntr2_ocMix       ; sound effects are not attenuated
      ld a, (__nt2Atten)
      or a
      jr z, ntr2_ocMix
      ld d, a
      ld a, e
      cp 0x10
      jr nz, ntr2_ocAttVol

      ld a, d                 ; envelope: on until fully attenuated
      cp 15
      jr c, ntr2_ocMix
      ld e, 0
      jr ntr2_ocMix

    ntr2_ocAttVol:

      sub d
      jr nc, ntr2_ocAttOk
      xor a

    ntr2_ocAttOk:

      ld e, a

    ntr2_ocMix:

      bit 6, (iy + NT2_V_MODE)
      jr z, ntr2_ocNoTone
      ld a, c
      call ntr2_bit
      cpl
      ld hl, __nt2Mixer
      and (hl)
      ld (hl), a

    ntr2_ocNoTone:

      ld a, (iy + NT2_V_MODE)
      bit 5, a
      jr z, ntr2_ocNoNoise
      and 0x1F
      ld (__nt2Noise), a
      ld a, c
      call ntr2_bit
      add a, a
      add a, a
      add a, a
      cpl
      ld hl, __nt2Mixer
      and (hl)
      ld (hl), a

    ntr2_ocNoNoise:

      ld a, (iy + NT2_V_ENV)
      bit 7, a
      jr z, ntr2_ocAmp

      rrca
      rrca
      rrca
      rrca
      and 7                   ; shift
      ld l, (iy + NT2_V_PERIOD)
      ld h, (iy + NT2_V_PERIOD + 1)
      or a
      jr z, ntr2_ocEp

    ntr2_ocShift:

      srl h
      rr l
      dec a
      jr nz, ntr2_ocShift

    ntr2_ocEp:

      ld a, 11
      out (_IO_AYLATCH), a
      ld a, l
      out (_IO_AYDATA), a
      ld a, 12
      out (_IO_AYLATCH), a
      ld a, h
      out (_IO_AYDATA), a

      ld a, (iy + NT2_V_ENVTRIG)
      or a
      jr z, ntr2_ocAmp
      ld a, 13
      out (_IO_AYLATCH), a
      ld a, (iy + NT2_V_ENV)
      and 15
      out (_IO_AYDATA), a

    ntr2_ocAmp:

      ld a, c
      add a, 8
      out (_IO_AYLATCH), a
      ld a, e
      out (_IO_AYDATA), a
      ret

    ; ------------------------------------------------------------------------
    ; ntr2_output: write all 3 channels, then the noise period and the mixer
    ;   out: everything but ix changed
    ; ------------------------------------------------------------------------
    ntr2_output:

      ld a, 0x7F
      ld (__nt2Mixer), a

      ld c, 0

    ntr2_opLoop:

      push bc
      call ntr2_outCh
      pop bc
      inc c
      ld a, c
      cp 3
      jr c, ntr2_opLoop

      ; the envelope restart is used up, played or not
      xor a
      ld hl, __nt2Voices + NT2_V_ENVTRIG
      ld de, NT2_V_SIZE
      ld b, 6

    ntr2_opTrig:

      ld (hl), a
      add hl, de
      djnz ntr2_opTrig

      ld a, 6
      out (_IO_AYLATCH), a
      ld a, (__nt2Noise)
      out (_IO_AYDATA), a
      ld a, 7
      out (_IO_AYLATCH), a
      ld a, (__nt2Mixer)
      out (_IO_AYDATA), a
      ret

    ; ------------------------------------------------------------------------
    ; Tone periods for notes 0 (C1) - 95 (B8), same table as the editor
    ; ------------------------------------------------------------------------
    ntr2_notes:

      defw 3420, 3229, 3047, 2876, 2715, 2562, 2419, 2283, 2155, 2034, 1920, 1812
      defw 1710, 1614, 1524, 1438, 1357, 1281, 1209, 1141, 1077, 1017, 960, 906
      defw 855, 807, 762, 719, 679, 641, 605, 571, 539, 508, 480, 453
      defw 428, 404, 381, 360, 339, 320, 302, 285, 269, 254, 240, 226
      defw 214, 202, 190, 180, 170, 160, 151, 143, 135, 127, 120, 113
      defw 107, 101, 95, 90, 85, 80, 76, 71, 67, 64, 60, 57
      defw 53, 50, 48, 45, 42, 40, 38, 36, 34, 32, 30, 28
      defw 27, 25, 24, 22, 21, 20, 19, 18, 17, 16, 15, 14

  __endasm;
}

// ***************************************************************************
// Public functions (documented in NabuTrackerV2.h)
// ***************************************************************************

static void nt2_init(const uint8_t *song) __z88dk_fastcall __naked {

  // see NOTE ON (void) PARAMETERS in NABU-LIB.c
  (void)song;

  // _nt2Song    = song;
  // _nt2Speed   = song[3];
  // _nt2TickCtr = 0;
  // _nt2Paused  = 0;
  //
  // for each music voice: active = 0, chanVol = 15, instrument = 0, slide = 0
  //
  // load order 0;
  //
  // _nt2Playing = 1;
  __asm

      ld (__nt2Song), hl
      inc hl
      inc hl
      inc hl
      ld a, (hl)
      ld (__nt2Speed), a

      xor a
      ld (__nt2TickCtr), a
      ld (__nt2Paused), a

      ld iy, __nt2Voices
      ld de, NT2_V_SIZE
      ld b, 3

    ntr2_inLoop:

      ld (iy + NT2_V_ACTIVE), a
      ld (iy + NT2_V_CHANVOL), 15
      ld (iy + NT2_V_INST), a
      ld (iy + NT2_V_SLIDE), a
      add iy, de
      djnz ntr2_inLoop

      call ntr2_loadOrder     ; a = 0

      ld a, 1
      ld (__nt2Playing), a
      ret

  __endasm;
}

static void nt2_tick(void) __naked {

  // if (_nt2Playing && !_nt2Paused) {
  //
  //   if (_nt2TickCtr == 0)
  //     play a row;
  //
  //   if (++_nt2TickCtr >= _nt2Speed)
  //     _nt2TickCtr = 0;
  // }
  //
  // if (!_nt2Paused)
  //   step the 3 music voices;
  //
  // step the sound effect voices that are playing;
  //
  // write the AY;
  //
  // count the sound effects down;
  __asm

      ld a, (__nt2Playing)
      or a
      jr z, ntr2_tkVoices
      ld a, (__nt2Paused)
      or a
      jr nz, ntr2_tkVoices

      ld a, (__nt2TickCtr)
      or a
      call z, ntr2_row

      ld hl, __nt2TickCtr
      inc (hl)
      ld a, (hl)
      ld hl, __nt2Speed
      cp (hl)
      jr c, ntr2_tkVoices
      xor a
      ld (__nt2TickCtr), a

    ntr2_tkVoices:

      ld a, (__nt2Paused)
      or a
      jr nz, ntr2_tkSfx

      xor a
      call ntr2_voice
      call ntr2_update
      ld a, 1
      call ntr2_voice
      call ntr2_update
      ld a, 2
      call ntr2_voice
      call ntr2_update

    ntr2_tkSfx:

      ld c, 0

    ntr2_tkSfxLoop:

      ld hl, __nt2SfxTicks
      ld b, 0
      add hl, bc
      ld a, (hl)
      or a
      jr z, ntr2_tkSfxNext

      push bc
      ld a, c
      add a, 3
      call ntr2_voice
      call ntr2_update
      pop bc

    ntr2_tkSfxNext:

      inc c
      ld a, c
      cp 3
      jr c, ntr2_tkSfxLoop

      call ntr2_output

      ld hl, __nt2SfxTicks
      ld b, 3

    ntr2_tkCount:

      ld a, (hl)
      or a
      jr z, ntr2_tkCountNext
      dec (hl)

    ntr2_tkCountNext:

      inc hl
      djnz ntr2_tkCount

      ret

  __endasm;
}

static void nt2_stop(void) __naked {

  // _nt2Playing = 0;
  //
  // every voice off, every sound effect off, AY amplitudes 8 - 10 = 0
  __asm

      xor a
      ld (__nt2Playing), a

      ld hl, __nt2Voices + NT2_V_ACTIVE
      ld de, NT2_V_SIZE
      ld b, 6

    ntr2_spVoice:

      ld (hl), a
      add hl, de
      djnz ntr2_spVoice

      ld hl, __nt2SfxTicks
      ld (hl), a
      inc hl
      ld (hl), a
      inc hl
      ld (hl), a

      ld c, 8

    ntr2_spAmp:

      ld a, c
      out (_IO_AYLATCH), a
      xor a
      out (_IO_AYDATA), a
      inc c
      ld a, c
      cp 11
      jr nz, ntr2_spAmp

      ret

  __endasm;
}

static void nt2_pause(void) __naked {

  // _nt2Paused = 1;
  __asm

      ld a, 1
      ld (__nt2Paused), a
      ret

  __endasm;
}

static void nt2_resume(void) __naked {

  // _nt2Paused = 0;
  __asm

      xor a
      ld (__nt2Paused), a
      ret

  __endasm;
}

static void nt2_setAttenuation(uint8_t attenuation) __z88dk_fastcall __naked {

  // see NOTE ON (void) PARAMETERS in NABU-LIB.c
  (void)attenuation;

  // _nt2Atten = attenuation > 15 ? 15 : attenuation;
  __asm

      ld a, l
      cp 16
      jr c, ntr2_saStore
      ld a, 15

    ntr2_saStore:

      ld (__nt2Atten), a
      ret

  __endasm;
}

static void nt2_sfx(const uint8_t *bank, uint8_t sfx, uint8_t channel) __z88dk_callee __naked {

  // see NOTE ON (void) PARAMETERS in NABU-LIB.c
  (void)bank;
  (void)sfx;
  (void)channel;

  // table = bank sfx offset; if none or sfx >= count, return
  //
  // NT2_SFX_AUTO: the first channel without a sound effect, from 2 down to 0, else 2
  // if (channel > 2) return
  //
  // entry = table + 1 + sfx * 3 (instrument, note, ticks)
  // sound effect voice of the channel: chanVol = 15, slide = 0, start the note
  // _nt2SfxTicks[channel] = ticks ? ticks : 1;
  __asm

      pop hl
      pop bc                  ; bc = bank
      pop de                  ; e = sfx, d = channel
      push hl

      ld hl, 12
      add hl, bc
      ld a, (hl)
      inc hl
      or (hl)
      ret z                   ; no sound effects in this bank

      push de
      ld a, 12
      call ntr2_field         ; hl = sfx table
      pop de

      ld a, e
      cp (hl)
      ret nc                  ; no such sound effect

      ld a, d
      cp NT2_SFX_AUTO
      jr nz, ntr2_sxCh

      ld d, 2
      ld a, (__nt2SfxTicks + 2)
      or a
      jr z, ntr2_sxCh
      dec d
      ld a, (__nt2SfxTicks + 1)
      or a
      jr z, ntr2_sxCh
      dec d
      ld a, (__nt2SfxTicks)
      or a
      jr z, ntr2_sxCh
      ld d, 2

    ntr2_sxCh:

      ld a, d
      cp 3
      ret nc

      push af                 ; channel
      inc hl
      ld d, 0
      add hl, de
      add hl, de
      add hl, de              ; hl = entry
      ld e, (hl)              ; instrument
      inc hl
      ld d, (hl)              ; note
      inc hl
      ld a, (hl)              ; ticks
      or a
      jr nz, ntr2_sxTicks
      inc a

    ntr2_sxTicks:

      ld l, a
      pop af
      ld h, a                 ; l = ticks, h = channel
      push hl

      add a, 3
      push de
      call ntr2_voice
      pop hl                  ; l = instrument, h = note
      ld (iy + NT2_V_CHANVOL), 15
      ld (iy + NT2_V_SLIDE), 0
      call ntr2_noteOn        ; bc = bank

      pop hl
      ld e, h
      ld d, 0
      ld a, l
      ld hl, __nt2SfxTicks
      add hl, de
      ld (hl), a
      ret

  __endasm;
}

static void nt2_sfxStop(uint8_t channel) __z88dk_fastcall __naked {

  // see NOTE ON (void) PARAMETERS in NABU-LIB.c
  (void)channel;

  // if (channel < 3)
  //   _nt2SfxTicks[channel] = 0;
  __asm

      ld a, l
      cp 3
      ret nc
      ld e, a
      ld d, 0
      ld hl, __nt2SfxTicks
      add hl, de
      ld (hl), 0
      ret

  __endasm;
}
