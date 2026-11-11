// ****************************************************************************************
// NABU MULTIPLAYER TIC-TAC-TOE
//
// Two-player example of the "ordered moves" pattern:
//
//   - Every move is sent with MP_TARGET_ALL on the reliable channel 0, which means the
//     server echoes it back to the sender too, in one order shared by both machines.
//   - Nobody applies their own move when the key is pressed. Both machines apply moves
//     only when they come back from the server, so the two boards can never disagree.
//   - The shared seed from mp_getStartInfo() decides who goes first, so both machines
//     agree without sending anything.
//
// The host creates a session and waits in the lobby; the other player lists sessions
// and joins one. 
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
//   mp_isSupported()       Is the Internet Adapter new enough? ALWAYS call first.
//   mp_init()              Connect the adapter to nabu.ca for this game id.
//   mp_getPlayerName()     Read the player name saved in the adapter's settings.
//
//   Hosting:
//     mp_create()          Create a session and become its host.
//     mp_setSessionData()  Store a few bytes anyone browsing can see (our tag).
//   Joining:
//     mp_listSessions()    Ask the server for this game's open sessions.
//     mp_getSessionInfo()  Read one session from that list.
//     mp_join()            Join the session picked.
//
//   Lobby:
//     mp_status()          Cheap poll: state, players, host flag, what is waiting.
//     mp_start()           Host only: start the game for everyone.
//     mp_getStartInfo()    Shared seed and start time, same on both machines.
//   Game:
//     mp_send()            Send a move packet.
//     mp_recv()            Read the next waiting move packet.
//     mp_end()             Host only: end the game, here back to the lobby.
//
//   mp_leave()             Leave the session.
//
// Error helpers:
//
//   mp_resultName()        Short text for a result code ("TIMEOUT", "OFFLINE"...).
//   mp_getLastError()      The adapter's longer explanation of the last error.
//
// Most mp_ functions return a result code: MP_OK (0) on success, otherwise one of
// the MP_ERR_* values. Most calls are answered instantly from the adapter's memory.
// Only the ones marked "NETWORK CALL" below wait on the internet, and those give up
// after 5 seconds with MP_ERR_TIMEOUT, so don't call them every frame.

// The sandbox works without registering. Swap in your own game id from
// nabu.ca/my/multiplayer when you make something real (and drop the tag check).
// (The shared sandbox id was removed: every program needs its own registered id.)
// Register this program at nabu.ca/my/multiplayer and put its game id here.
#define TTT_GAME_ID     2

#if TTT_GAME_ID == 0
  #error Register Tic-Tac-Toe at nabu.ca/my/multiplayer and set TTT_GAME_ID to its game id
#endif

// Packet types
#define MSG_MOVE        0x01  // [MSG_MOVE, cell 0-8]

// Keys
#define KEY_ESC         27
#define KEY_SPACE       ' '

// Cell values
#define CELL_EMPTY      0
#define CELL_X          1
#define CELL_O          2

// Poll the adapter every POLL_FRAMES frames (60 Hz / 3 = 20 times a second)
#define POLL_FRAMES     3

// Max sessions shown in the join list (keys 1-9)
#define MAX_LIST        9

// Tag stored in session data so we only list tic-tac-toe sessions
const uint8_t TAG[4] = { 'T', 'T', 'T', '1' };

// Filled by mp_status(). The fields this program uses:
//   state        MP_STATE_LOBBY, MP_STATE_IN_GAME, or MP_STATE_ENDED /
//                MP_STATE_CLOSED / MP_STATE_NONE once the session is gone
//   flags        MP_STATUS_* bits; MP_STATUS_IS_HOST is set on the host
//   hostId       the host's player id
//   playerMask   bit n set = player id n is in the session
//   playerCount  how many players are in the session (including me)
//   rxWaiting    packets queued in the adapter for mp_recv() (capped at 255)
// Others: myId, rosterRev, dataRev, chatWaiting, endReason.
MpStatus      _status;

// Filled by mp_getSessionInfo() when browsing. Fields used: sessionId (0 = empty
// slot), name (session name, 24 bytes) and sessionData (the 16 bytes the host
// wrote with mp_setSessionData()).
MpSessionInfo _info;

// Filled by mp_getStartInfo() once the game starts. Fields used: seed (identical
// on every machine) and startTime (different for every start, so it tells one game
// from the next). All zero before the first start.
MpStartInfo   _start;

// Start time of the last game played, so the lobby can spot a rematch. Starts at a
// value no real start time has.
uint32_t      _lastStartTime = 0xFFFFFFFF;

// The buffer sizes marked "must be" are required by the library. The adapter always
// writes that many bytes, so a smaller buffer would be overrun.
uint8_t       _myName[16];       // must be 16 bytes: mp_getPlayerName() (15 chars + NUL)
uint8_t       _errText[64];      // must be 64 bytes: mp_getLastError()
uint8_t       _line[48];
uint8_t       _packet[16];       // any size: mp_recv() is told the size and truncates
uint16_t      _listIds[MAX_LIST];
uint8_t       _board[9];
uint8_t       _myId      = 0;
uint8_t       _turnId    = 0;    // player id whose turn it is
uint8_t       _xId       = 0;    // player id playing X (moves first)
uint8_t       _oId       = 0;    // player id playing O
uint8_t       _moveCount = 0;

// ---------------------------------------------------------------------------
// Small helpers
// ---------------------------------------------------------------------------

void printAt(uint8_t x, uint8_t y, const char *text) {

  vdp_setCursor2(x, y);

  vdp_print((uint8_t *)text);
}

void clearRow(uint8_t y) {

  vdp_clearRows(y, y + 1);
}

// Waits about one video frame.
void waitFrame() {

  vdp_waitVDPReadyInt();
}

// Returns the next key, or 0 when none is waiting.
uint8_t readKey() {

  if (!isKeyPressed())
    return 0;

  return getChar();
}

// Writes a 16-bit value as 4 hex digits into dst (5 bytes with the NUL).
void toHex16(uint8_t *dst, uint16_t v) {

  for (uint8_t i = 0; i < 4; i++) {

    uint8_t nibble = (v >> (12 - i * 4)) & 0x0f;

    dst[i] = nibble < 10 ? '0' + nibble : 'A' + nibble - 10;
  }

  dst[4] = 0x00;
}

// Shows a failed call on row 22/23 and waits for a key.
void showError(const char *what, uint8_t result) {

  clearRow(21);
  clearRow(22);
  clearRow(23);

  printAt(0, 21, what);
  vdp_print((uint8_t *)": ");

  // mp_resultName(result)
  //   result   a result code returned by another mp_ function (MP_OK, MP_ERR_*)
  //   returns  a short upper-case name such as "TIMEOUT" or "SESSION FULL", or
  //            "UNKNOWN ERROR" for a code it doesn't know. Never NULL.
  vdp_print((uint8_t *)mp_resultName(result));

  // mp_getLastError(text)
  //   text     buffer of at least 64 bytes. Receives the adapter's readable
  //            explanation of the most recent error, always NUL terminated.
  //            More detail than mp_resultName(), e.g. why the server refused.
  mp_getLastError(_errText);

  _errText[39] = 0x00;

  printAt(0, 22, (const char *)_errText);

  printAt(0, 23, "Press a key");

  getChar();
}

// The id of the other player, from the roster bitmask.
uint8_t otherPlayerId() {

  for (uint8_t i = 0; i < MP_MAX_PLAYERS; i++) {

    if (i == _myId)
      continue;

    if (_status.playerMask & (1 << i))
      return i;
  }

  return _myId;
}

// ---------------------------------------------------------------------------
// Board
// ---------------------------------------------------------------------------

// Board drawn at column 14, row 6. Cells are numbered 1-9:
//   1 | 2 | 3
//   4 | 5 | 6
//   7 | 8 | 9
#define BOARD_X 14
#define BOARD_Y 6

void drawCell(uint8_t cell) {

  uint8_t x = BOARD_X + (cell % 3) * 4 + 1;
  uint8_t y = BOARD_Y + (cell / 3) * 2;
  uint8_t c;

  if (_board[cell] == CELL_X)
    c = 'X';
  else if (_board[cell] == CELL_O)
    c = 'O';
  else
    c = '1' + cell;

  vdp_writeCharAtLocation(x, y, c);
}

void drawBoard() {

  vdp_clearScreen();

  printAt(10, 1, "NABU TIC-TAC-TOE");

  // Grid lines
  for (uint8_t row = 0; row < 3; row++) {

    printAt(BOARD_X, BOARD_Y + row * 2, "   |   |   ");

    if (row < 2)
      printAt(BOARD_X, BOARD_Y + row * 2 + 1, "---+---+---");
  }

  for (uint8_t cell = 0; cell < 9; cell++)
    drawCell(cell);

  printAt(4, 3, _myId == _xId ? "You are X" : "You are O");
}

void drawTurn() {

  clearRow(14);

  if (_turnId == _myId)
    printAt(4, 14, "Your move: press 1-9");
  else
    printAt(4, 14, "Waiting for the other player");
}

// The eight winning lines: rows, columns, diagonals
const uint8_t LINES[8][3] = {
  { 0, 1, 2 }, { 3, 4, 5 }, { 6, 7, 8 },
  { 0, 3, 6 }, { 1, 4, 7 }, { 2, 5, 8 },
  { 0, 4, 8 }, { 2, 4, 6 }
};

// Returns CELL_X / CELL_O for a winner, 0 for none.
uint8_t winner() {

  for (uint8_t i = 0; i < 8; i++) {

    uint8_t a = _board[LINES[i][0]];

    if (a != CELL_EMPTY && a == _board[LINES[i][1]] && a == _board[LINES[i][2]])
      return a;
  }

  return 0;
}

// Applies a move that came back from the server. Moves from the wrong player or onto
// a taken cell are ignored, which keeps both boards identical even with a buggy peer.
// Returns true when the game is over.
bool applyMove(uint8_t from, uint8_t cell) {

  uint8_t w;

  if (from != _turnId || cell > 8 || _board[cell] != CELL_EMPTY)
    return false;

  _board[cell] = from == _xId ? CELL_X : CELL_O;

  _moveCount++;

  drawCell(cell);

  w = winner();

  if (w != 0) {

    clearRow(14);

    if ((w == CELL_X && _myId == _xId) || (w == CELL_O && _myId == _oId))
      printAt(4, 14, "YOU WIN!");
    else
      printAt(4, 14, "You lose.");

    return true;
  }

  if (_moveCount == 9) {

    clearRow(14);

    printAt(4, 14, "Draw.");

    return true;
  }

  _turnId = _turnId == _xId ? _oId : _xId;

  drawTurn();

  return false;
}

// ---------------------------------------------------------------------------
// Game
// ---------------------------------------------------------------------------

// Plays one game. Returns false when the other player left or the session closed.
bool playGame() {

  uint8_t frame = 0;
  bool    over  = false;

  // mp_getStartInfo(info)
  //   info     an MpStartInfo struct to fill:
  //              seed         random number from the server, the same on every
  //                           machine. Use it for any random choice both
  //                           machines must agree on (here: who plays X).
  //              startTime    mp_time() value when the game started. New for
  //                           every start, including rematches.
  //              playerMask   players in the game when it started
  //              playerCount  how many players that is
  //            All zero before the game has started.
  //   returns  nothing. Answered from the adapter's memory.
  mp_getStartInfo(&_start);

  // Remember which start this is so the lobby can tell a rematch from this game.
  _lastStartTime = _start.startTime;

  for (uint8_t i = 0; i < 9; i++)
    _board[i] = CELL_EMPTY;

  _moveCount = 0;

  // The shared seed picks who plays X, identically on both machines
  if (_start.seed & 1) {

    _xId = _status.hostId;
    _oId = otherPlayerId() == _status.hostId ? _myId : otherPlayerId();
  } else {

    _oId = _status.hostId;
    _xId = otherPlayerId() == _status.hostId ? _myId : otherPlayerId();
  }

  _turnId = _xId;

  drawBoard();

  drawTurn();

  while (!over) {

    uint8_t key = readKey();

    // Send my move. It is drawn when the server echoes it back, not now.
    if (_turnId == _myId && key >= '1' && key <= '9') {

      uint8_t cell = key - '1';

      if (_board[cell] == CELL_EMPTY) {

        uint8_t result;

        _packet[0] = MSG_MOVE;
        _packet[1] = cell;

        // mp_send(target, channel, len, data)
        //   target   who receives it: a player id (0 - 15), MP_TARGET_OTHERS
        //            (everyone but me), MP_TARGET_ALL (everyone including me, in
        //            one order the server shares with every player) or
        //            MP_TARGET_HOST. MP_TARGET_ALL is what makes the "apply moves
        //            only when they come back" pattern work.
        //   channel  MP_CHANNEL_RELIABLE (0): every packet arrives, in order.
        //            1 - 255: "latest only", a new packet replaces an unread one
        //            from the same sender on that channel (good for positions,
        //            never for moves).
        //   len      number of bytes in data, 1 up to mp_maxPacket.
        //   data     the bytes to send. Any format you like; here [MSG_MOVE, cell].
        //   returns  MP_OK, or MP_ERR_RATE_LIMIT / MP_ERR_TOO_LARGE /
        //            MP_ERR_NOT_IN_SESSION, which all mean the packet was dropped.
        result = mp_send(MP_TARGET_ALL, MP_CHANNEL_RELIABLE, 2, _packet);

        if (result != MP_OK)
          showError("SEND", result);
      }
    }

    if (key == KEY_ESC)
      return false;

    waitFrame();

    frame++;

    if (frame < POLL_FRAMES)
      continue;

    frame = 0;

    // mp_status(status)
    //   status   an MpStatus struct to fill (see _status above for the fields).
    //   returns  nothing. Answered from the adapter's memory, so it is cheap
    //            enough to call every frame or two. Use rxWaiting to decide
    //            whether mp_recv() is worth calling.
    mp_status(&_status);

    if (_status.state != MP_STATE_IN_GAME || _status.playerCount < 2) {

      clearRow(14);

      printAt(4, 14, "The other player left.");

      printAt(4, 16, "Press a key");

      getChar();

      return false;
    }

    // Apply every move waiting, in server order
    while (!over && _status.rxWaiting != 0) {

      uint8_t  from;

      // mp_recv(buf, maxLen, from, channel)
      //   buf      where the packet's bytes are copied (raw binary, no NUL).
      //   maxLen   the most bytes to copy into buf. A longer packet is truncated
      //            and the rest is thrown away.
      //   from     receives the sender's player id (0 - 15). Our own moves come
      //            back with our own id because they were sent to MP_TARGET_ALL.
      //            May be NULL.
      //   channel  receives the channel it was sent on. May be NULL, as here.
      //   returns  the number of bytes copied into buf, or 0 when nothing is waiting.
      uint16_t len = mp_recv(_packet, sizeof(_packet), &from, NULL);

      if (len == 0)
        break;

      if (len == 2 && _packet[0] == MSG_MOVE)
        over = applyMove(from, _packet[1]);
    }
  }

  return true;
}

// ---------------------------------------------------------------------------
// Lobby
// ---------------------------------------------------------------------------

// Waits in the lobby until the game starts. The host presses SPACE once the other
// player is here. Returns false on ESC or if the session closed.
bool lobby(uint16_t sessionId) {

  uint8_t hex[5];
  uint8_t frame        = 0;
  uint8_t lastCount    = 0xff;

  vdp_clearScreen();

  printAt(10, 1, "NABU TIC-TAC-TOE");

  toHex16(hex, sessionId);

  printAt(2, 4, "Session ");
  vdp_print(hex);

  printAt(2, 22, "ESC leaves");

  while (true) {

    uint8_t key = readKey();

    if (key == KEY_ESC)
      return false;

    waitFrame();

    frame++;

    if (frame < POLL_FRAMES)
      continue;

    frame = 0;

    // mp_status(): cheap poll, see _status above for the fields
    mp_status(&_status);

    if (_status.state == MP_STATE_CLOSED || _status.state == MP_STATE_ENDED || _status.state == MP_STATE_NONE) {

      printAt(2, 10, "The session closed. Press a key.");

      getChar();

      return false;
    }

    // After a game the joiner may still see the old IN_GAME until the host returns to
    // the lobby, and a quick rematch can skip LOBBY between two polls. Every start
    // gets a new start time, so a start time we have not played yet means a new game.
    if (_status.state == MP_STATE_IN_GAME) {

      // mp_getStartInfo(): see playGame() for the fields
      mp_getStartInfo(&_start);

      if (_start.startTime != _lastStartTime)
        return true;
    }

    if (_status.playerCount != lastCount) {

      lastCount = _status.playerCount;

      clearRow(8);
      clearRow(10);

      if (_status.playerCount < 2) {

        printAt(2, 8, "Waiting for another player...");
      } else if (_status.flags & MP_STATUS_IS_HOST) {

        printAt(2, 8, "Opponent is here!");
        printAt(2, 10, "Press SPACE to start");
      } else {

        printAt(2, 8, "Waiting for the host to start");
      }
    }

    // Only the host starts, and only with two players
    if (key == KEY_SPACE && (_status.flags & MP_STATUS_IS_HOST) && _status.playerCount == 2) {

      // mp_start()                                             NETWORK CALL
      //   Host only. Moves the session to MP_STATE_IN_GAME and has the server
      //   pick the shared seed. Every player sees the new state on their next
      //   mp_status() and can then read the seed with mp_getStartInfo().
      //   returns  MP_OK, or e.g. MP_ERR_NOT_HOST or MP_ERR_TIMEOUT.
      uint8_t result = mp_start();

      if (result != MP_OK) {

        showError("START", result);

        lastCount = 0xff;
      }
    }
  }
}

// Creates a session. Returns its id, or 0 on failure.
uint16_t hostGame() {

  uint8_t  result;
  uint16_t sessionId = 0;

  // Session name like "DJ's board"
  uint8_t pos = 0;

  while (_myName[pos] != 0x00 && pos < 15) {

    _line[pos] = _myName[pos];

    pos++;
  }

  _line[pos] = 0x00;

  // mp_create(maxPlayers, flags, code, playerName, sessionName, description, sessionId, myId)   NETWORK CALL
  //   Creates a session and makes you its host and first player.
  //   maxPlayers   2 - 16, clamped to the game's limit (mp_gameMaxPlayers).
  //   flags        MP_FLAG_* OR'd together, e.g. MP_FLAG_PRIVATE (unlisted),
  //                MP_FLAG_LATE_JOIN (join after the game starts),
  //                MP_FLAG_HOST_MIGRATION (keep the session when the host leaves),
  //                MP_FLAG_AUTO_START (start as soon as it is full),
  //                MP_FLAG_NO_DISCORD, MP_FLAG_WEB_CHAT, MP_FLAG_SHARED_DATA.
  //   code         5-digit access code such as "12345" that joiners must give,
  //                or NULL for none.
  //   playerName   your name as others see it (max 15 characters).
  //   sessionName  shown in session lists (max 23 characters).
  //   description  shown on nabu.ca (max 63 characters).
  //   sessionId    receives the new session's id. May be NULL.
  //   myId         receives your player id (0 - 15). May be NULL.
  //   returns      MP_OK, or e.g. MP_ERR_NAME (name refused by the filter),
  //                MP_ERR_BUSY, MP_ERR_TIMEOUT.
  result = mp_create(2, MP_FLAG_NONE, NULL, (const char *)_myName, (const char *)_line, "Tic-tac-toe. First to three in a row.", &sessionId, &_myId);

  if (result != MP_OK) {

    showError("CREATE", result);

    return 0;
  }

  // Tag the session so the join list can skip other sandbox programs
  // (now a sanity check that the session came from this program)
  //
  // mp_setSessionData(offset, len, data)
  //   Every session has 16 bytes of shared data (MP_SESSION_DATA_SIZE) that all
  //   players can read, and that anyone browsing sees in MpSessionInfo.sessionData
  //   before joining. Host only, unless created with MP_FLAG_SHARED_DATA.
  //   offset   first byte to write, 0 - 15.
  //   len      number of bytes to write. Clipped so offset + len stays within 16.
  //   data     the bytes to write.
  //   returns  MP_OK, MP_ERR_BAD_ARG (offset out of range) or MP_ERR_NOT_HOST.
  mp_setSessionData(0, 4, TAG);

  return sessionId;
}

// Lists open tic-tac-toe sessions and joins the one picked. Returns its id, or 0.
uint16_t joinGame() {

  while (true) {

    uint8_t result;
    uint8_t count;
    uint8_t shown = 0;
    uint8_t key;

    vdp_clearScreen();

    printAt(10, 1, "NABU TIC-TAC-TOE");

    printAt(2, 3, "Looking for games...");

    // mp_listSessions(filter, count)                          NETWORK CALL
    //   Asks the server for this game's sessions. The adapter keeps the list
    //   as a snapshot, so the indexes stay valid until the next call.
    //   filter   MP_LIST_ALL, or MP_LIST_JOINABLE to skip sessions that are
    //            full, locked or already started.
    //   count    receives how many sessions are in the snapshot. May be NULL.
    //   returns  MP_OK, or e.g. MP_ERR_OFFLINE or MP_ERR_TIMEOUT.
    result = mp_listSessions(MP_LIST_JOINABLE, &count);

    if (result != MP_OK) {

      showError("LIST", result);

      return 0;
    }

    clearRow(3);

    printAt(2, 3, "Pick a game:");

    for (uint8_t i = 0; i < count && shown < MAX_LIST; i++) {

      // mp_getSessionInfo(index, info)
      //   index    0 to count - 1 from the last mp_listSessions() call.
      //   info     an MpSessionInfo struct to fill: sessionId, state, flags
      //            (MP_INFO_*), maxPlayers, playerCount, ageSeconds,
      //            sessionData, hostName, name and description.
      //            info->sessionId is 0 when index is out of range.
      //   returns  nothing. Read from the adapter's snapshot, no network wait.
      mp_getSessionInfo(i, &_info);

      // Skip sessions from other sandbox programs
      // (now: skip anything that was not created by this program)
      if (_info.sessionId == 0 || _info.sessionData[0] != TAG[0] || _info.sessionData[1] != TAG[1] || _info.sessionData[2] != TAG[2] || _info.sessionData[3] != TAG[3])
        continue;

      _listIds[shown] = _info.sessionId;

      _info.name[23] = 0x00;

      vdp_writeCharAtLocation(2, 5 + shown * 2, '1' + shown);

      printAt(4, 5 + shown * 2, (const char *)_info.name);

      shown++;
    }

    if (shown == 0)
      printAt(2, 5, "No games right now.");

    printAt(2, 22, "R refreshes  ESC goes back");

    key = getChar();

    if (key == KEY_ESC)
      return 0;

    if (key >= '1' && key < '1' + shown) {

      uint16_t sessionId = _listIds[key - '1'];

      // mp_join(sessionId, code, playerName, myId)              NETWORK CALL
      //   sessionId   the session to join (MpSessionInfo.sessionId).
      //   code        its 5-digit access code, or NULL when none is needed
      //               (MP_INFO_NEEDS_CODE clear in MpSessionInfo.flags).
      //   playerName  your name as others see it (max 15 characters).
      //   myId        receives your player id (0 - 15). May be NULL.
      //   returns     MP_OK, or e.g. MP_ERR_FULL, MP_ERR_STARTED,
      //               MP_ERR_BAD_CODE, MP_ERR_LOCKED, MP_ERR_NOT_FOUND.
      result = mp_join(sessionId, NULL, (const char *)_myName, &_myId);

      if (result == MP_OK)
        return sessionId;

      showError("JOIN", result);
    }
  }
}

// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------

void main() {

  uint8_t result;

  initNABULib();

  vdp_initTextMode(VDP_WHITE, VDP_DARK_BLUE, false);

  vdp_loadASCIIFont(ASCII);

  vdp_enableVDPReadyInt();

  printAt(10, 1, "NABU TIC-TAC-TOE");

  // Must come first: an older adapter would hang on any other mp_ call
  //
  // mp_isSupported()
  //   returns  true when the Internet Adapter's version is at least
  //            MP_MIN_IA_VERSION. Uses an older adapter command that every
  //            adapter answers, so it can never hang.
  if (!mp_isSupported()) {

    printAt(2, 4, "Your Internet Adapter is too old.");
    printAt(2, 5, "Update it from nabu.ca to play.");

    while (true);
  }

  printAt(2, 4, "Connecting...");

  // mp_init(gameId)                                          NETWORK CALL
  //   gameId   your game's id from nabu.ca/my/multiplayer. Sessions are
  //            separated by game id, so only programs using the same id see
  //            each other.
  //   returns  MP_OK, or e.g. MP_ERR_OFFLINE (adapter can't reach nabu.ca),
  //            MP_ERR_UNKNOWN_GAME (id not registered or disabled),
  //            MP_ERR_OUTDATED (adapter needs an update) or MP_ERR_TIMEOUT.
  //   On success it also fills the globals mp_protocolVersion,
  //   mp_gameMaxPlayers and mp_maxPacket (largest packet the server accepts).
  result = mp_init(TTT_GAME_ID);

  if (result != MP_OK) {

    showError("CONNECT", result);

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

    printAt(2, 6, "Your name: ");

    len = readLine(_myName, MP_PLAYER_NAME_MAX);

    _myName[len] = 0x00;
  }

  while (true) {

    uint8_t  key;
    uint16_t sessionId = 0;

    vdp_clearScreen();

    printAt(10, 1, "NABU TIC-TAC-TOE");

    printAt(4, 6, "1  Host a game");
    printAt(4, 8, "2  Join a game");

    key = getChar();

    if (key == '1')
      sessionId = hostGame();
    else if (key == '2')
      sessionId = joinGame();

    if (sessionId == 0)
      continue;

    // Lobby -> game -> (host offers a rematch) -> lobby ...
    while (lobby(sessionId)) {

      if (!playGame())
        break;

      // The host decides: SPACE returns both players to the lobby for a rematch.
      // The other player goes straight back to the lobby screen and waits there.
      if (_status.flags & MP_STATUS_IS_HOST) {

        uint8_t k;

        printAt(4, 16, "SPACE: rematch  ESC: quit");

        do {

          k = getChar();
        } while (k != KEY_SPACE && k != KEY_ESC);

        if (k == KEY_ESC)
          break;

        // mp_end(winnerMask, flags)
        //   Host only. Ends the current game.
        //   winnerMask  bit n set = player id n won; saved in the match history.
        //               0 here, so no winner is recorded.
        //   flags       MP_END_FLAG_TO_LOBBY returns everyone to MP_STATE_LOBBY
        //               with the same players (a rematch). MP_END_FLAG_NONE
        //               finishes the session as MP_STATE_ENDED instead.
        //   returns     MP_OK, or e.g. MP_ERR_NOT_HOST.
        mp_end(0, MP_END_FLAG_TO_LOBBY);
      }
    }

    // mp_leave()
    //   Leaves the current session; the others see their playerCount drop.
    //   Returns a result code. The adapter also leaves on its own if the NABU
    //   resets, disconnects, or stops talking to it for 30 seconds while in a
    //   session. The connection from mp_init() stays open, so the menu can
    //   host or join again right away.
    mp_leave();
  }
}
