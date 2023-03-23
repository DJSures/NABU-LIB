void nt_init(uint16_t *song) {

  _song = song;
  
  _itemCntr = 0;
  _beatCntr = 0;

  // Noise envelope
  ayWrite(6, 0b00000001);

  // Enable only the Tone generators on A B C
  ayWrite(7, 0b01111000);

  // Set the amplitude (volume) to enveoloe
  ayWrite(8,  0);
  ayWrite(9,  0);
  ayWrite(10, 0);
}

void nt_stopSounds() {

  ayWrite(8,  0);
  ayWrite(9,  0);
  ayWrite(10, 0);
}

void nt_handleNote() {

  // time (0)
  while (_song[_itemCntr] == _beatCntr) {
  
    _itemCntr++;

    // command (1)
    switch (_song[_itemCntr]) {

      case NT_NOTE_OFF:

        ayWrite(8 + _song[_itemCntr + 1],  0);

        _itemCntr += 2;

        break;

      case NT_NOTE_ON: {

        uint8_t note = _song[_itemCntr + 2];

        switch (_song[_itemCntr + 1]) {

          case 0:
            ayWrite(0x00, _NOTES_FINE[note]);
            ayWrite(0x01, _NOTES_COURSE[note]);
            ayWrite(8,    _song[_itemCntr + 3]);
            break;
          case 1:
            ayWrite(0x02, _NOTES_FINE[note]);
            ayWrite(0x03, _NOTES_COURSE[note]);
            ayWrite(9,    _song[_itemCntr + 3]);
            break;
          case 2:
            ayWrite(0x04, _NOTES_FINE[note]);
            ayWrite(0x05, _NOTES_COURSE[note]);
            ayWrite(10,   _song[_itemCntr + 3]);
            break;
        }

        _itemCntr += 4;

        break;
      }        
      case NT_SND_CHNG:

          ayWrite(8,  0);
        
          if (_song[_itemCntr + 1]) 
            ayWrite(7, 0b01111000); // note
          else
            ayWrite(7, 0b01110001); // noise

        _itemCntr += 2;
        
        break;
      
      case NT_LOOP:

        _itemCntr = 0;
        
        _beatCntr = 0;        
    }
  }

  _beatCntr++;
}
