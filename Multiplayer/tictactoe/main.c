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

MpStatus      _status;
MpSessionInfo _info;
MpStartInfo   _start;

// Start time of the last game played, so the lobby can spot a rematch. Starts at a
// value no real start time has.
uint32_t      _lastStartTime = 0xFFFFFFFF;

uint8_t       _myName[16];
uint8_t       _errText[64];
uint8_t       _line[48];
uint8_t       _packet[16];
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
  vdp_print((uint8_t *)mp_resultName(result));

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

  result = mp_create(2, MP_FLAG_NONE, NULL, (const char *)_myName, (const char *)_line, "Tic-tac-toe. First to three in a row.", &sessionId, &_myId);

  if (result != MP_OK) {

    showError("CREATE", result);

    return 0;
  }

  // Tag the session so the join list can skip other sandbox programs
  // (now a sanity check that the session came from this program)
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

    result = mp_listSessions(MP_LIST_JOINABLE, &count);

    if (result != MP_OK) {

      showError("LIST", result);

      return 0;
    }

    clearRow(3);

    printAt(2, 3, "Pick a game:");

    for (uint8_t i = 0; i < count && shown < MAX_LIST; i++) {

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
  if (!mp_isSupported()) {

    printAt(2, 4, "Your Internet Adapter is too old.");
    printAt(2, 5, "Update it from nabu.ca to play.");

    while (true);
  }

  printAt(2, 4, "Connecting...");

  result = mp_init(TTT_GAME_ID);

  if (result != MP_OK) {

    showError("CONNECT", result);

    while (true);
  }

  // Use the name from the adapter settings, or ask for one
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

        mp_end(0, MP_END_FLAG_TO_LOBBY);
      }
    }

    mp_leave();
  }
}
