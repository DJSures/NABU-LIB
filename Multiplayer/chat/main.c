// ****************************************************************************************
// NABU MULTIPLAYER CHAT
//
// The smallest useful multiplayer program: joins (or creates) a chat room in the
// chat program's own registered game and lets everyone in it talk. Lines typed here go out with
// mp_chatSend(). Lines from mp_chatRecv() and any raw packets that arrive are shown in
// the scrolling area above the input line.
//
// Run it on two NABUs (or a NABU and MAME) connected to Internet Adapters and they will
// land in the same room.
//
// DJ Sures (c) 2026
// https://nabu.ca
// ****************************************************************************************

#define BIN_TYPE BIN_HOMEBREW
#define FONT_STANDARD
#define DISABLE_CURSOR

#include "../../NABULIB/NABU-LIB.h"
#include "../../NABULIB/patterns.h"
#include "../../NABULIB/RetroNET-Multiplayer.h"

// ---------------------------------------------------------------------------
// Multiplayer calls used by this program (all from RetroNET-Multiplayer.h)
// ---------------------------------------------------------------------------
//
// Order of calls:
//
//   mp_isSupported()     Is the Internet Adapter new enough? ALWAYS call first.
//   mp_init()            Connect the adapter to nabu.ca for this game id.
//   mp_getPlayerName()   Read the player name saved in the adapter's settings.
//   mp_quickJoin()       Join an open room, or create one if none is open.
//   loop:
//     mp_status()        Cheap poll: state, player count, what is waiting.
//     mp_chatSend()      Send a line of chat to everyone in the room.
//     mp_chatRecv()      Read the next waiting chat line.
//     mp_recv()          Read the next waiting raw packet.
//   mp_leave()           Leave the room.
//   mp_shutdown()        Disconnect the adapter from the server.
//
// Error helpers:
//
//   mp_resultName()      Short text for a result code ("TIMEOUT", "OFFLINE"...).
//   mp_getLastError()    The adapter's longer explanation of the last error.
//
// Most mp_ functions return a result code: MP_OK (0) on success, otherwise one of
// the MP_ERR_* values. Most calls are answered instantly from the adapter's memory.
// Only the ones marked "NETWORK CALL" below wait on the internet, and those give up
// after 5 seconds with MP_ERR_TIMEOUT, so don't call them every frame.

// The sandbox works without registering. Swap in your own game id from
// nabu.ca/my/multiplayer when you make something real.
// (The shared sandbox id was removed: every program needs its own registered id.)
// Register this program at nabu.ca/my/multiplayer and put its game id here.
#define CHAT_GAME_ID    1

#if CHAT_GAME_ID == 0
  #error Register the chat program at nabu.ca/my/multiplayer and set CHAT_GAME_ID to its game id
#endif
#define CHAT_MAX_PLAYERS 16

// Screen layout (40 x 24 text mode)
#define ROW_TITLE       0
#define ROW_MSG_TOP     2
#define ROW_MSG_BOTTOM  21
#define ROW_DIVIDER     22
#define ROW_INPUT       23
#define INPUT_MAX       37

// Keys
#define KEY_ENTER       13
#define KEY_BACKSPACE   127

// Poll the adapter every POLL_FRAMES video frames (60 Hz / 6 = 10 times a second).
// Each poll masks the keyboard interrupt for a moment, so polling every frame would
// make fast typing drop keys.
#define POLL_FRAMES     6

// Filled by mp_status(). The fields this program uses:
//   state        MP_STATE_LOBBY / MP_STATE_IN_GAME while the room is alive,
//                MP_STATE_ENDED / MP_STATE_CLOSED / MP_STATE_NONE once it is gone
//   playerCount  how many players are in the room right now (including me)
//   chatWaiting  chat lines queued in the adapter for mp_chatRecv()
//   rxWaiting    raw packets queued in the adapter for mp_recv() (capped at 255)
// Others: flags (MP_STATUS_*), myId, hostId, playerMask, rosterRev, dataRev, endReason.
MpStatus _status;

// The buffer sizes marked "must be" are required by the library. The adapter always
// writes that many bytes, so a smaller buffer would be overrun.
uint8_t  _myName[16];         // must be 16 bytes: mp_getPlayerName() (15 chars + NUL)
uint8_t  _input[INPUT_MAX + 1];
uint8_t  _inputLen = 0;
uint8_t  _line[48];
uint8_t  _chatName[16];       // must be 16 bytes: mp_chatRecv() sender name
uint8_t  _chatText[128];      // must be 128 bytes: mp_chatRecv() text (127 chars + NUL)
uint8_t  _packet[128];        // any size: mp_recv() is told the size and truncates
uint8_t  _errText[64];        // must be 64 bytes: mp_getLastError()
uint8_t  _lastPlayerCount = 0;

// ---------------------------------------------------------------------------
// Tiny string helpers (cheaper than pulling in sprintf)
// ---------------------------------------------------------------------------

// Appends src to dst at position pos, never going past max. Returns the new position.
uint8_t appendStr(uint8_t *dst, uint8_t pos, const char *src, uint8_t max) {

  while (*src != 0x00 && pos < max) {

    dst[pos] = (uint8_t)*src;

    pos++;
    src++;
  }

  dst[pos] = 0x00;

  return pos;
}

// Appends a number in decimal.
uint8_t appendUInt8(uint8_t *dst, uint8_t pos, uint8_t v, uint8_t max) {

  uint8_t digits[3];
  uint8_t cnt = 0;

  do {

    digits[cnt] = '0' + (v % 10);

    v /= 10;
    cnt++;
  } while (v != 0);

  while (cnt > 0 && pos < max) {

    cnt--;

    dst[pos] = digits[cnt];

    pos++;
  }

  dst[pos] = 0x00;

  return pos;
}

// Appends a 16-bit value as 4 hex digits (session ids are shown this way).
uint8_t appendHex16(uint8_t *dst, uint8_t pos, uint16_t v, uint8_t max) {

  for (int8_t shift = 12; shift >= 0; shift -= 4) {

    uint8_t nibble = (v >> shift) & 0x0f;

    if (pos >= max)
      break;

    dst[pos] = nibble < 10 ? '0' + nibble : 'A' + nibble - 10;

    pos++;
  }

  dst[pos] = 0x00;

  return pos;
}

// ---------------------------------------------------------------------------
// Screen
// ---------------------------------------------------------------------------

// Scrolls the message area up one row and prints text on the bottom row.
void printMessage(const uint8_t *text) {

  uint8_t len = 0;

  vdp_scrollTextUp(ROW_MSG_TOP, ROW_MSG_BOTTOM);

  vdp_clearRows(ROW_MSG_BOTTOM, ROW_MSG_BOTTOM + 1);

  // Clip to one screen row
  while (len < 40 && text[len] != 0x00)
    len++;

  vdp_setCursor2(0, ROW_MSG_BOTTOM);

  vdp_printPart(0, len, (uint8_t *)text);
}

// Prints a result code with the adapter's detailed reason underneath.
void printError(const char *what, uint8_t result) {

  uint8_t pos = appendStr(_line, 0, what, 40);

  pos = appendStr(_line, pos, ": ", 40);

  // mp_resultName(result)
  //   result   a result code returned by another mp_ function (MP_OK, MP_ERR_*)
  //   returns  a short upper-case name such as "TIMEOUT" or "SESSION FULL", or
  //            "UNKNOWN ERROR" for a code it doesn't know. Never NULL.
  appendStr(_line, pos, mp_resultName(result), 40);

  printMessage(_line);

  // mp_getLastError(text)
  //   text     buffer of at least 64 bytes. Receives the adapter's readable
  //            explanation of the most recent error, always NUL terminated.
  //            More detail than mp_resultName(), e.g. why the server refused.
  mp_getLastError(_errText);

  printMessage(_errText);
}

// Redraws the input line with a cursor block at the end.
void drawInput() {

  vdp_clearRows(ROW_INPUT, ROW_INPUT + 1);

  vdp_setCursor2(0, ROW_INPUT);

  vdp_write('>');

  vdp_printPart(0, _inputLen, _input);

  vdp_write('_');
}

void drawFrame(uint16_t sessionId) {

  uint8_t pos;

  vdp_clearScreen();

  pos = appendStr(_line, 0, "NABU CHAT  ROOM ", 40);
  appendHex16(_line, pos, sessionId, 40);

  vdp_setCursor2(0, ROW_TITLE);
  vdp_print(_line);

  for (uint8_t x = 0; x < 40; x++) {

    vdp_writeCharAtLocation(x, ROW_TITLE + 1, '-');
    vdp_writeCharAtLocation(x, ROW_DIVIDER, '-');
  }

  drawInput();
}

// ---------------------------------------------------------------------------
// Input
// ---------------------------------------------------------------------------

// Handles any waiting keys. Enter sends the line as chat.
void handleKeys() {

  while (isKeyPressed()) {

    uint8_t c = getChar();

    if (c == KEY_ENTER) {

      if (_inputLen > 0) {

        uint8_t result;

        _input[_inputLen] = 0x00;

        // mp_chatSend(text)
        //   text     NUL-terminated line to send to everyone in the room. Clipped
        //            to MP_CHAT_MAX (127) characters.
        //   returns  MP_OK, or e.g. MP_ERR_RATE_LIMIT (sending too fast, line
        //            dropped) or MP_ERR_NOT_IN_SESSION.
        // The line isn't printed here: this program expects it to come back
        // through mp_chatRecv() along with everyone else's.
        result = mp_chatSend((const char *)_input);

        if (result != MP_OK)
          printError("SEND", result);

        _inputLen = 0;
      }
    } else if (c == KEY_BACKSPACE) {

      if (_inputLen > 0)
        _inputLen--;
    } else if (c >= 0x20 && c <= 0x7e && _inputLen < INPUT_MAX) {

      _input[_inputLen] = c;

      _inputLen++;
    }

    drawInput();
  }
}

// ---------------------------------------------------------------------------
// Network
// ---------------------------------------------------------------------------

// Shows every waiting chat line.
void drainChat() {

  uint8_t from;

  // mp_chatRecv(from, name, text)
  //   from     receives who sent the line: a player id (0 - 15),
  //            MP_CHAT_FROM_WEB (someone chatting on nabu.ca) or
  //            MP_CHAT_FROM_SYSTEM (a server notice such as "LEO joined").
  //            May be NULL if you don't care.
  //   name     buffer of at least 16 bytes. Receives the sender's name, NUL terminated.
  //   text     buffer of at least 128 bytes. Receives the line, NUL terminated.
  //   returns  the length of the line, or 0 when no chat is waiting (name and
  //            text are left untouched in that case).
  while (mp_chatRecv(&from, _chatName, _chatText) != 0) {

    uint8_t pos;

    if (from == MP_CHAT_FROM_SYSTEM) {

      pos = appendStr(_line, 0, "* ", 40);
    } else if (from == MP_CHAT_FROM_WEB) {

      pos = appendStr(_line, 0, "[web] ", 40);
      pos = appendStr(_line, pos, (const char *)_chatName, 40);
      pos = appendStr(_line, pos, ": ", 40);
    } else {

      pos = appendStr(_line, 0, (const char *)_chatName, 40);
      pos = appendStr(_line, pos, ": ", 40);
    }

    appendStr(_line, pos, (const char *)_chatText, 40);

    printMessage(_line);
  }
}

// Shows every waiting raw packet as text. Another program in the same sandbox may
// send packets instead of chat; non-printable bytes are shown as dots.
// (With its own registered game id only chat clients share these rooms, but another
// version of the program could still send raw packets.)
void drainPackets() {

  uint8_t  from;
  uint16_t len;

  // mp_recv(buf, maxLen, from, channel)
  //   buf      where the packet's bytes are copied. Not NUL terminated: packets
  //            are raw binary.
  //   maxLen   the most bytes to copy into buf. A longer packet is truncated and
  //            the rest is thrown away.
  //   from     receives the sender's player id (0 - 15). May be NULL.
  //   channel  receives the channel it was sent on: 0 (MP_CHANNEL_RELIABLE) is
  //            reliable and in order, 1 - 255 keep only the newest packet per
  //            sender. May be NULL, as it is here.
  //   returns  the number of bytes copied into buf, or 0 when nothing is waiting.
  while ((len = mp_recv(_packet, sizeof(_packet) - 1, &from, NULL)) != 0) {

    uint8_t pos = appendStr(_line, 0, "P", 40);

    pos = appendUInt8(_line, pos, from, 40);
    pos = appendStr(_line, pos, "> ", 40);

    for (uint16_t i = 0; i < len && pos < 40; i++) {

      uint8_t c = _packet[i];

      _line[pos] = (c >= 0x20 && c <= 0x7e) ? c : '.';

      pos++;
    }

    _line[pos] = 0x00;

    printMessage(_line);
  }
}

// One poll of the adapter. Returns false when the session is over.
bool pollNetwork() {

  // mp_status(status)
  //   status   an MpStatus struct to fill (see _status above for the fields).
  //   returns  nothing. Answered from the adapter's memory, so it is cheap
  //            enough to call every frame or two. Use chatWaiting / rxWaiting to
  //            decide whether mp_chatRecv() / mp_recv() are worth calling.
  mp_status(&_status);

  if (_status.state != MP_STATE_LOBBY && _status.state != MP_STATE_IN_GAME) {

    printMessage((const uint8_t *)"* The room closed.");

    return false;
  }

  if (_status.playerCount != _lastPlayerCount) {

    uint8_t pos = appendStr(_line, 0, "* ", 40);

    pos = appendUInt8(_line, pos, _status.playerCount, 40);
    appendStr(_line, pos, " in the room", 40);

    printMessage(_line);

    _lastPlayerCount = _status.playerCount;
  }

  if (_status.chatWaiting != 0)
    drainChat();

  if (_status.rxWaiting != 0)
    drainPackets();

  return true;
}

// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------

void main() {

  uint8_t  result;
  uint16_t sessionId = 0;
  uint8_t  myId      = 0;
  bool     created   = false;
  uint8_t  frame     = 0;

  initNABULib();

  vdp_initTextMode(VDP_WHITE, VDP_DARK_BLUE, false);

  vdp_loadASCIIFont(ASCII);

  vdp_print("NABU CHAT");
  vdp_newLine();
  vdp_newLine();

  // Must come first: an older adapter would hang on any other mp_ call
  //
  // mp_isSupported()
  //   returns  true when the Internet Adapter's version is at least
  //            MP_MIN_IA_VERSION. Uses an older adapter command that every
  //            adapter answers, so it can never hang.
  if (!mp_isSupported()) {

    vdp_print("Your Internet Adapter is too old.");
    vdp_newLine();
    vdp_print("Update it from nabu.ca to play.");

    while (true);
  }

  vdp_print("Connecting...");
  vdp_newLine();

  // mp_init(gameId)                                          NETWORK CALL
  //   gameId   your game's id from nabu.ca/my/multiplayer. Rooms are separated
  //            by game id, so only programs using the same id meet each other.
  //   returns  MP_OK, or e.g. MP_ERR_OFFLINE (adapter can't reach nabu.ca),
  //            MP_ERR_UNKNOWN_GAME (id not registered or disabled),
  //            MP_ERR_OUTDATED (adapter needs an update) or MP_ERR_TIMEOUT.
  //   On success it also fills the globals mp_protocolVersion,
  //   mp_gameMaxPlayers and mp_maxPacket (largest packet the server accepts).
  result = mp_init(CHAT_GAME_ID);

  if (result != MP_OK) {

    vdp_print("Connect failed: ");
    vdp_print((uint8_t *)mp_resultName(result));
    vdp_newLine();

    mp_getLastError(_errText);
    vdp_print(_errText);

    while (true);
  }

  // Use the name from the adapter settings, or ask for one
  //
  // mp_getPlayerName(name)
  //   name     buffer of at least 16 bytes. Receives the default player name
  //            the user saved in the Internet Adapter's settings, NUL
  //            terminated. name[0] is 0 when no name has been set.
  mp_getPlayerName(_myName);

  if (_myName[0] == 0x00) {

    uint8_t len;

    vdp_print("Your name: ");

    len = readLine(_myName, MP_PLAYER_NAME_MAX);

    _myName[len] = 0x00;

    vdp_newLine();
  }

  // mp_quickJoin(maxPlayers, flags, playerName, sessionId, myId, created)   NETWORK CALL
  //   Joins an open public room of this game (no access code, not full, not
  //   locked), or creates a new one when none is open.
  //   maxPlayers  room size to use IF a new room is created: 2 - 16, clamped to
  //               the game's limit (mp_gameMaxPlayers, filled by mp_init()).
  //               Ignored when joining an existing room.
  //   flags       options IF a new room is created, MP_FLAG_* OR'd together, e.g.
  //               MP_FLAG_PRIVATE (unlisted), MP_FLAG_LATE_JOIN (join after the
  //               game starts), MP_FLAG_HOST_MIGRATION (keep the room alive when
  //               the host leaves), MP_FLAG_WEB_CHAT (let people on nabu.ca chat in).
  //   playerName  your name as others will see it. Clipped to 15 characters and
  //               may be refused by the name filter (MP_ERR_NAME).
  //   sessionId   receives the room's id. May be NULL.
  //   myId        receives your player id (0 - 15) in the room. This is the
  //               "from" value others see on your packets. May be NULL.
  //   created     receives true when a new room was created, which makes you
  //               the host. May be NULL.
  //   returns     MP_OK, or e.g. MP_ERR_FULL, MP_ERR_NAME, MP_ERR_BUSY,
  //               MP_ERR_TIMEOUT.
  result = mp_quickJoin(CHAT_MAX_PLAYERS, MP_FLAG_NONE, (const char *)_myName, &sessionId, &myId, &created);

  if (result != MP_OK) {

    vdp_print("Join failed: ");
    vdp_print((uint8_t *)mp_resultName(result));
    vdp_newLine();

    mp_getLastError(_errText);
    vdp_print(_errText);

    while (true);
  }

  drawFrame(sessionId);

  printMessage(created ? (const uint8_t *)"* You opened a new room." : (const uint8_t *)"* You joined a room.");

  printMessage((const uint8_t *)"* Type and press RETURN to talk.");

  vdp_enableVDPReadyInt();

  while (true) {

    vdp_waitVDPReadyInt();

    handleKeys();

    frame++;

    if (frame < POLL_FRAMES)
      continue;

    frame = 0;

    if (!pollNetwork())
      break;
  }

  // mp_leave()
  //   Leaves the current room; the others see their playerCount drop. Returns
  //   a result code. The adapter also leaves on its own if the NABU resets,
  //   disconnects, or stops talking to it for 30 seconds while in a room.
  mp_leave();

  // mp_shutdown()
  //   Leaves any room still joined and disconnects the adapter from the
  //   multiplayer server. Call mp_init() again before using mp_ functions again.
  //   Returns a result code.
  mp_shutdown();

  while (true);
}
