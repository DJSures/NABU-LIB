// ****************************************************************************************
// NABU-LIB RETRONET MULTIPLAYER
//
// DJ Sures (c) 2026
// https://nabu.ca
//
// Last updated on v2026.11.10.00
//
// Get latest copy and examples from: https://github.com/DJSures/NABU-LIB
//
// Host, find and join multiplayer sessions through the NABU Internet Adapter, and
// exchange raw packets with every other NABU in the session. The Internet Adapter
// keeps the connection to nabu.ca and buffers everything, so almost every call here
// is answered immediately from the adapter's memory. Only mp_init, mp_listSessions,
// mp_getSessionInfoById, mp_create, mp_join, mp_quickJoin, mp_start, mp_scoreSubmit,
// mp_scoreCount, mp_storeWrite and mp_storeRead wait on the internet, and the adapter
// gives up on those after 5 seconds with MP_ERR_TIMEOUT.
//
// Register your game at https://nabu.ca/my/multiplayer to get its game id.
// Every program needs its own registered id, even while you experiment. New games
// start in dev mode, which keeps their sessions hidden until you switch it off.
//
// IMPORTANT
// ---------
// Call mp_isSupported() BEFORE any other mp_ function. An Internet Adapter older than
// MP_MIN_IA_VERSION does not answer the multiplayer command (0xEF) and the NABU would
// hang forever waiting for a reply. mp_isSupported() asks the adapter for its version
// through IA Control (0xBA 0x16), which every adapter since 2024 understands.
//
// Typical flow
// ------------
//   if (!mp_isSupported())               -> tell the user to update their adapter
//   mp_init(MY_GAME_ID)
//   mp_create(...) or mp_listSessions() + mp_getSessionInfo() + mp_join()
//   while lobby: mp_status() until enough players, host calls mp_start()
//   game loop:   mp_send() / mp_recv()
//   mp_leave(), mp_shutdown()
//
// Include this after NABU-LIB.h. It can be included together with RetroNET-IAControl.h.
//
// Full wire protocol: retronetMultiplayerManual.md in the Internet Adapter repository.
// **********************************************************************************************

// **********************************************************************************************
// FUNCTION INDEX
// --------------
// A quick reference for every function in this file. Scroll down to a function for its
// full description.
//
// [NET]    waits on the internet: up to 5 seconds, then MP_ERR_TIMEOUT. Don't call these
//          every frame. Everything else is answered at once from the adapter's memory.
// uint8_t  functions return a result code, MP_OK (0) or one of MP_ERR_*, unless noted.
// "out"    parameters are written by the function. Most may be NULL if you don't need them.
//
// SETUP
//   bool     mp_isSupported()
//              True when the adapter is at least MP_MIN_IA_VERSION. Call it FIRST: an older
//              adapter never answers the other mp_ functions and the NABU would hang.
//
//   uint8_t  mp_init(uint16_t gameId)                                                  [NET]
//              Connects the adapter to the server for your game id (register it at
//              nabu.ca/my/multiplayer). Fills mp_protocolVersion, mp_gameMaxPlayers and
//              mp_maxPacket.
//
//   uint8_t  mp_shutdown()
//              Leaves any session and disconnects from the server.
//
//   void     mp_getPlayerName(uint8_t *name)
//              name (16 bytes, out): default player name from the adapter's settings.
//              Empty string when none is set.
//
//   void     mp_getLastError(uint8_t *text)
//              text (64 bytes, out): readable reason for the last error.
//
// BROWSING
//   uint8_t  mp_listSessions(uint8_t filter, uint8_t *count)                           [NET]
//              Fetches this game's sessions into a snapshot on the adapter.
//              filter: MP_LIST_ALL or MP_LIST_JOINABLE (skips full, locked, started).
//              count (out): number of sessions, for mp_getSessionInfo().
//
//   void     mp_getSessionInfo(uint8_t index, MpSessionInfo *info)
//              info (out): session index 0..count-1 from the last mp_listSessions().
//              info->sessionId is 0 when index is out of range.
//
//   void     mp_getSessionInfoById(uint16_t sessionId, MpSessionInfo *info)            [NET]
//              Looks up one session by id, e.g. a private one a friend told you about.
//              info->sessionId is 0 when not found.
//
// CREATE, JOIN, LEAVE
//   uint8_t  mp_create(uint8_t maxPlayers, uint8_t flags, const char *code,
//                      const char *playerName, const char *sessionName,
//                      const char *description, uint16_t *sessionId, uint8_t *myId)   [NET]
//              Creates a session and makes you its host.
//              maxPlayers: 2-16, clamped to the game's limit. flags: MP_FLAG_* OR'd.
//              code: 5 digits such as "12345", or NULL for none.
//              Names clip to 15 / 23 / 63 characters. sessionId, myId (out).
//
//   uint8_t  mp_join(uint16_t sessionId, const char *code, const char *playerName,
//                    uint8_t *myId)                                                    [NET]
//              Joins a session by id. code: 5 digits, or NULL when none is needed.
//              myId (out): your player id 0-15.
//
//   uint8_t  mp_quickJoin(uint8_t maxPlayers, uint8_t flags, const char *playerName,
//                         uint16_t *sessionId, uint8_t *myId, bool *created)          [NET]
//              Joins an open public session, or creates one with maxPlayers / flags when
//              none is open. created (out): true when a new one was made (you are host).
//
//   uint8_t  mp_leave()
//              Leaves the current session. The server connection stays open.
//
// SESSION MANAGEMENT
//   void     mp_status(MpStatus *status)
//              status (out): state, flags, ids, player count, rx / chat waiting, etc.
//              Cheap: poll it every frame or two.
//
//   void     mp_getPlayer(uint8_t playerId, MpPlayerInfo *player)
//              player (out): one roster slot. MP_PLAYER_PRESENT is clear when empty.
//              Only re-read when MpStatus.rosterRev changes.
//
//   uint8_t  mp_setReady(bool ready)
//              Sets your ready flag in the lobby.
//
//   uint8_t  mp_setPlayerData(const uint8_t *data)
//              Sets your 8 bytes of player data (colour, character, team...).
//
//   uint8_t  mp_setSessionData(uint8_t offset, uint8_t len, const uint8_t *data)
//              Writes len bytes of the 16 bytes of session data, starting at offset.
//              Visible when browsing. Host only, unless MP_FLAG_SHARED_DATA.
//
//   void     mp_getSessionData(uint8_t *buf)
//              buf (16 bytes, out): the session data.
//
//   uint8_t  mp_start()                                                                [NET]
//              Host only. Starts the game: everyone sees MP_STATE_IN_GAME.
//
//   void     mp_getStartInfo(MpStartInfo *info)
//              info (out): shared seed, start time, players. All zero before the start.
//
//   uint8_t  mp_end(uint16_t winnerMask, uint8_t flags)
//              Host only. winnerMask: bit n = player n won (match history).
//              flags: MP_END_FLAG_TO_LOBBY for a rematch, else the session ENDS.
//
//   uint8_t  mp_kick(uint8_t playerId)
//              Host only. Removes a player.
//
//   uint8_t  mp_setLocked(bool locked)
//              Host only. Locks or unlocks the session to new joins.
//
//   uint8_t  mp_setHost(uint8_t playerId)
//              Host only. Hands the host role to another player.
//
// PACKETS
//   uint8_t  mp_send(uint8_t target, uint8_t channel, uint16_t len, const uint8_t *data)
//              target: player id 0-15, MP_TARGET_OTHERS, MP_TARGET_ALL or MP_TARGET_HOST.
//              channel: 0 reliable and ordered, 1-255 latest-only. len: 1..mp_maxPacket.
//              MP_ERR_RATE_LIMIT / MP_ERR_TOO_LARGE mean the packet was dropped.
//
//   uint16_t mp_recv(uint8_t *buf, uint16_t maxLen, uint8_t *from, uint8_t *channel)
//              Takes the next packet into buf. Returns its length, 0 when none waiting.
//              Longer packets are truncated to maxLen. from, channel (out).
//
//   uint16_t mp_peek(uint8_t *from, uint8_t *channel)
//              Length of the next packet without removing it, 0 when none.
//
//   uint8_t  mp_flush()
//              Throws away every waiting packet.
//
//   uint16_t mp_exchange(uint8_t target, uint8_t channel, uint16_t len,
//                        const uint8_t *data, uint8_t *buf, uint16_t maxLen,
//                        uint8_t *from, uint8_t *rxChannel, uint8_t *sendResult)
//              mp_send() and mp_recv() in one round trip. sendResult (out): send result.
//              Returns the received length, 0 when nothing was waiting.
//
// CHAT
//   uint8_t  mp_chatSend(const char *text)
//              Sends a chat line to the session, clipped to 127 characters.
//
//   uint8_t  mp_chatRecv(uint8_t *from, uint8_t *name, uint8_t *text)
//              Takes the next chat line. Returns its length, 0 when none.
//              from (out): player id, MP_CHAT_FROM_WEB or MP_CHAT_FROM_SYSTEM.
//              name (16 bytes, out), text (128 bytes, out), both NUL terminated.
//
// TIME
//   uint32_t mp_time()
//              Milliseconds since the session was created, the same on every player.
//
//   uint16_t mp_ping()
//              Round trip from the adapter to the server, in milliseconds.
//
// SCORES AND STATUS TEXT
//   uint8_t  mp_setScore(int32_t score)
//              Sets your live score, shown in the roster and on nabu.ca.
//
//   uint8_t  mp_setStatusText(const char *text)
//              Host only. Session status line on nabu.ca, e.g. "Round 3 of 5" (63 chars).
//
//   uint8_t  mp_scoreSubmit(uint8_t board, int32_t score, const char *name,
//                           uint16_t *rank)                                            [NET]
//              Submits to a leaderboard defined for the game on nabu.ca.
//              rank (out): the placing.
//
//   uint8_t  mp_scoreCount(uint8_t board, uint8_t *count)                              [NET]
//              Fetches a leaderboard (top 100) into the adapter.
//              count (out): number of rows, for mp_scoreGet().
//
//   void     mp_scoreGet(uint8_t board, uint8_t index, MpScoreEntry *entry)
//              entry (out): one row from the last mp_scoreCount(). rank 0 = out of range.
//
// CLOUD SAVES (per adapter, per game: 4 slots of up to 1024 bytes)
//   uint8_t  mp_storeWrite(uint8_t slot, uint16_t len, const uint8_t *data)            [NET]
//              Saves len bytes into slot 0-3.
//
//   uint16_t mp_storeRead(uint8_t slot, uint8_t *buf, uint16_t maxLen)                 [NET]
//              Loads slot 0-3 into buf. Returns bytes read, 0 when the slot is empty.
//
// CONVENIENCE
//   const char *mp_resultName(uint8_t result)
//              Short name for a result code ("TIMEOUT", "SESSION FULL"...), never NULL.
//
// LOW LEVEL (every mp_ function already does this; you rarely need them)
//   void     mp_focusInterrupts()
//   void     mp_restoreInterrupts()
//              Mask every interrupt except HCCA receive around an adapter exchange, then
//              put the previous mask back. Functions starting with _mp_ are internal.
// **********************************************************************************************

#ifndef RETRONET_MULTIPLAYER_H
#define RETRONET_MULTIPLAYER_H

#include <stdbool.h>
#include <stdlib.h>
#include "NABU-LIB.h"

// **************************************************************************
// Configuration
// **************************************************************************

// The minimum Internet Adapter version that understands the multiplayer command.
// Define your own value before including this file to override it.
#ifndef MP_MIN_IA_VERSION
#define MP_MIN_IA_VERSION "2026.11.10.00"
#endif

// The version of this library, sent to the adapter by mp_init()
#define MP_LIB_VERSION 1

// (Was) Use this game id while developing. It works without registering a game and its
// sessions are never shown on nabu.ca or in Discord.
//
// RETIRED: MP_SANDBOX_GAME_ID (0xFFFF) has been removed. With every developer sharing
// one id, unrelated programs landed in each other's sessions. The server now rejects
// 0xFFFF with MP_ERR_UNKNOWN_GAME. Register your game at nabu.ca/my/multiplayer and
// pass its own id to mp_init().

// **************************************************************************
// Limits
// **************************************************************************

#define MP_MAX_PLAYERS          16
#define MP_MAX_PACKET           1024
#define MP_PLAYER_NAME_MAX      15
#define MP_SESSION_NAME_MAX     23
#define MP_DESCRIPTION_MAX      63
#define MP_CHAT_MAX             127
#define MP_STATUS_TEXT_MAX      63
#define MP_SESSION_DATA_SIZE    16
#define MP_PLAYER_DATA_SIZE     8
#define MP_ACCESS_CODE_SIZE     5

// **************************************************************************
// Result codes (returned by most functions)
// **************************************************************************

#define MP_OK                   0x00
#define MP_ERR_NOT_INIT         0x01  // mp_init not called
#define MP_ERR_OFFLINE          0x02  // adapter cannot reach nabu.ca
#define MP_ERR_UNKNOWN_GAME     0x03  // game id not registered or disabled
#define MP_ERR_NOT_FOUND        0x04  // no such session
#define MP_ERR_FULL             0x05  // session full
#define MP_ERR_BAD_CODE         0x06  // wrong access code
#define MP_ERR_STARTED          0x07  // already started and late join is off
#define MP_ERR_NOT_HOST         0x08  // host-only command
#define MP_ERR_NOT_IN_SESSION   0x09
#define MP_ERR_BAD_ARG          0x0A
#define MP_ERR_RATE_LIMIT       0x0B  // sending too fast; packet dropped
#define MP_ERR_TOO_LARGE        0x0C  // packet over the size limit
#define MP_ERR_KICKED           0x0D  // removed or banned
#define MP_ERR_OUTDATED         0x0E  // adapter must be updated for this server
#define MP_ERR_NAME             0x0F  // name rejected by the filter
#define MP_ERR_BUSY             0x10  // server or per-user session limit reached
#define MP_ERR_TIMEOUT          0x11  // server did not answer in time
#define MP_ERR_LOCKED           0x12  // host locked the session

// **************************************************************************
// Session states (MpStatus.state, MpSessionInfo.state)
// **************************************************************************

#define MP_STATE_NONE           0     // not in a session
#define MP_STATE_LOBBY          1
#define MP_STATE_IN_GAME        2
#define MP_STATE_ENDED          3
#define MP_STATE_CLOSED         4

// **************************************************************************
// MpStatus.flags
// **************************************************************************

#define MP_STATUS_IS_HOST       0x01  // I am the host
#define MP_STATUS_RX_OVERFLOW   0x02  // the adapter's receive queue overflowed; resync your game
#define MP_STATUS_RECONNECTING  0x04  // the adapter's link to nabu.ca is reconnecting
#define MP_STATUS_LOCKED        0x08  // the host locked the session

// **************************************************************************
// MpStatus.endReason (valid when state is ENDED or CLOSED)
// **************************************************************************

#define MP_END_NONE             0
#define MP_END_HOST_ENDED       1
#define MP_END_HOST_LEFT        2
#define MP_END_KICKED           3
#define MP_END_TIMED_OUT        4
#define MP_END_SERVER_SHUTDOWN  5
#define MP_END_ADMIN_CLOSED     6     // a nabu.ca administrator closed the session

// **************************************************************************
// MpSessionInfo.flags
// **************************************************************************

#define MP_INFO_NEEDS_CODE      0x01  // an access code is required to join
#define MP_INFO_LATE_JOIN       0x02  // players may join after the game started
#define MP_INFO_LOCKED          0x04  // the host locked the session

// **************************************************************************
// MpPlayerInfo.flags
// **************************************************************************

#define MP_PLAYER_PRESENT       0x01
#define MP_PLAYER_HOST          0x02
#define MP_PLAYER_READY         0x04
#define MP_PLAYER_IS_ME         0x08
#define MP_PLAYER_RECONNECTING  0x10

// **************************************************************************
// mp_create / mp_quickJoin flags
// **************************************************************************

#define MP_FLAG_NONE            0x00
#define MP_FLAG_PRIVATE         0x01  // unlisted: not in mp_listSessions, the website or Discord
#define MP_FLAG_LATE_JOIN       0x02  // allow joining after the game started
#define MP_FLAG_HOST_MIGRATION  0x04  // if the host leaves, the lowest slot becomes host
#define MP_FLAG_AUTO_START      0x08  // start automatically when the session is full
#define MP_FLAG_NO_DISCORD      0x10  // do not announce this session in Discord
#define MP_FLAG_WEB_CHAT        0x20  // people on nabu.ca may chat into this session
#define MP_FLAG_SHARED_DATA     0x40  // any player may write session data, not only the host

// **************************************************************************
// mp_listSessions filter
// **************************************************************************

#define MP_LIST_ALL             0x00
#define MP_LIST_JOINABLE        0x01  // skip full, locked and already started sessions

// **************************************************************************
// mp_end flags
// **************************************************************************

#define MP_END_FLAG_NONE        0x00
#define MP_END_FLAG_TO_LOBBY    0x01  // return to the lobby with the same players (rematch)

// **************************************************************************
// Packet targets
// **************************************************************************

// 0x00 - 0x0F send to that single player id
#define MP_TARGET_OTHERS        0xFF  // everyone except me
#define MP_TARGET_ALL           0xFE  // everyone including me, in one server-wide order
#define MP_TARGET_HOST          0xFD  // the host only

// Packet channels: 0 is reliable and ordered. 1 - 255 are "latest only": a new packet
// replaces any unread packet from the same sender on the same channel.
#define MP_CHANNEL_RELIABLE     0

// **************************************************************************
// Chat "from" values (besides a player id)
// **************************************************************************

#define MP_CHAT_FROM_WEB        0xFE  // someone chatting from nabu.ca
#define MP_CHAT_FROM_SYSTEM     0xFF  // system line, e.g. "LEO joined"

// **************************************************************************
// Structs. All are packed (SDCC does not pad) and little-endian, exactly as the
// adapter sends them.
// **************************************************************************

// 12 bytes. The one call a game makes every frame or two.
typedef struct {

  uint8_t  state;        // MP_STATE_*
  uint8_t  flags;        // MP_STATUS_*
  uint8_t  myId;
  uint8_t  hostId;
  uint16_t playerMask;   // bit n set = slot n occupied
  uint8_t  playerCount;
  uint8_t  rosterRev;    // changes on any join, leave, ready, name or player data change
  uint8_t  dataRev;      // changes when session data changes
  uint8_t  rxWaiting;    // packets queued (capped at 255)
  uint8_t  chatWaiting;  // chat lines queued
  uint8_t  endReason;    // MP_END_*
} MpStatus;

// 128 bytes. Returned when browsing sessions.
typedef struct {

  uint16_t sessionId;    // 0 = empty / not found
  uint8_t  state;        // MP_STATE_LOBBY or MP_STATE_IN_GAME
  uint8_t  flags;        // MP_INFO_*
  uint8_t  maxPlayers;
  uint8_t  playerCount;
  uint16_t ageSeconds;   // since creation, capped at 65535
  uint8_t  sessionData[MP_SESSION_DATA_SIZE];
  uint8_t  hostName[16];
  uint8_t  name[24];
  uint8_t  description[64];
} MpSessionInfo;

// 32 bytes. One player in the roster.
typedef struct {

  uint8_t  playerId;
  uint8_t  flags;        // MP_PLAYER_*
  uint16_t pingMs;
  int32_t  score;
  uint8_t  playerData[MP_PLAYER_DATA_SIZE];
  uint8_t  name[16];
} MpPlayerInfo;

// 12 bytes. Shared values every player receives when the host starts the game.
typedef struct {

  uint32_t seed;         // same on every machine; seed your random generator with it
  uint32_t startTime;    // mp_time() value at which the game started
  uint16_t playerMask;
  uint8_t  playerCount;
  uint8_t  reserved;
} MpStartInfo;

// 24 bytes. One leaderboard row.
typedef struct {

  uint16_t rank;         // 0 = no entry at that index
  int32_t  score;
  uint8_t  name[16];
  uint16_t reserved;
} MpScoreEntry;

// Compile-time size checks. If one of these fails the compiler padded a struct and the
// adapter's bytes would land in the wrong fields.
typedef char _mp_check_status [(sizeof(MpStatus)      == 12)  ? 1 : -1];
typedef char _mp_check_session[(sizeof(MpSessionInfo) == 128) ? 1 : -1];
typedef char _mp_check_player [(sizeof(MpPlayerInfo)  == 32)  ? 1 : -1];
typedef char _mp_check_start  [(sizeof(MpStartInfo)   == 12)  ? 1 : -1];
typedef char _mp_check_score  [(sizeof(MpScoreEntry)  == 24)  ? 1 : -1];

// **************************************************************************
// Values returned by mp_init()
// **************************************************************************

uint8_t  mp_protocolVersion = 0;    // protocol version the adapter speaks
uint8_t  mp_gameMaxPlayers  = 0;    // max players allowed for this game
uint16_t mp_maxPacket       = 0;    // largest packet the server accepts

// **************************************************************************
// Internal helpers
// **************************************************************************

uint8_t _mp_TmpOriginalInterrupt = 0;

// Temporarily allow only the HCCA receive interrupt while we talk to the adapter.
// hcca_writeByte sets the transmit interrupt itself.
void mp_focusInterrupts() {

  NABU_DisableInterrupts();

  _mp_TmpOriginalInterrupt = ayRead(IOPORTA);

  ayWrite(IOPORTA, INT_MASK_HCCARX);
}

// Put back whatever interrupts were enabled before mp_focusInterrupts().
void mp_restoreInterrupts() {

  ayWrite(IOPORTA, _mp_TmpOriginalInterrupt);

  NABU_EnableInterrupts();
}

// Sends the multiplayer opcode and sub-command.
void _mp_cmd(uint8_t subCmd) {

  hcca_writeByte(0xef);

  hcca_writeByte(subCmd);
}

// Length of a NUL-terminated string, clipped to maxLen. NULL counts as empty.
uint8_t _mp_strLen(const char *str, uint8_t maxLen) {

  uint8_t len = 0;

  if (str == NULL)
    return 0;

  while (len < maxLen && str[len] != 0x00)
    len++;

  return len;
}

// Writes a length-prefixed string (u8 length, then ASCII), clipped to maxLen.
void _mp_writeStr(const char *str, uint8_t maxLen) {

  uint8_t len = _mp_strLen(str, maxLen);

  hcca_writeByte(len);

  // Pointer loop inside hcca_writeBytes instead of an indexed loop here
  hcca_writeBytes(0, len, (uint8_t *)str);
}

// Writes the 5-byte access code. NULL or an empty string means "no code" and sends
// five 0x00 bytes. A code shorter than 5 characters is padded with 0x00.
void _mp_writeCode(const char *code) {

  uint8_t len = _mp_strLen(code, MP_ACCESS_CODE_SIZE);

  // Send the code characters, then pad with 0x00 up to 5 bytes
  hcca_writeBytes(0, len, (uint8_t *)code);

  for (; len < MP_ACCESS_CODE_SIZE; len++)
    hcca_writeByte(0x00);
}

// Reads len bytes into buf. Safe for len == 0 (hcca_readBytes is not).
// (hcca_readBytes is now also safe for 0; the guard is kept as cheap insurance.)
void _mp_readBytes(uint8_t *buf, uint16_t len) {

  if (len == 0)
    return;

  hcca_readBytes(0, len, buf);
}

// Reads and throws away len bytes.
void _mp_skipBytes(uint16_t len) {

  while (len > 0) {

    hcca_readByte();

    len--;
  }
}

// Reads the "len u16, [from u8, channel u8, data[len]]" reply shared by mp_recv and
// mp_exchange. Any bytes beyond maxLen are read and discarded so the stream stays aligned.
uint16_t _mp_readPacket(uint8_t *buf, uint16_t maxLen, uint8_t *from, uint8_t *channel) {

  uint16_t len = hcca_readUInt16();

  if (len == 0)
    return 0;

  uint8_t f = hcca_readByte();
  uint8_t c = hcca_readByte();

  if (from != NULL)
    *from = f;

  if (channel != NULL)
    *channel = c;

  // The adapter already truncates to maxLen; this is a belt-and-braces guard so a
  // misbehaving adapter can never overrun the caller's buffer.
  if (len > maxLen) {

    _mp_readBytes(buf, maxLen);

    _mp_skipBytes(len - maxLen);

    return maxLen;
  }

  _mp_readBytes(buf, len);

  return len;
}

// **************************************************************************
// Setup
// **************************************************************************

// -----------------------------------------------------------
// Returns true when the Internet Adapter supports multiplayer.
// MUST be called before any other mp_ function: an older adapter
// does not answer 0xEF and the NABU would hang waiting for it.
// -----------------------------------------------------------
bool mp_isSupported() {

  uint8_t version[14];
  const char *minVersion = MP_MIN_IA_VERSION;

  memset(version, 0x00, sizeof(version));

  mp_focusInterrupts();

  // IA Control: get adapter version
  hcca_writeByte(0xba);

  hcca_writeByte(0x16);

  uint8_t len = hcca_readByte();

  // The adapter clips the version to 13 characters; read anything extra and drop it
  if (len > 13) {

    _mp_readBytes(version, 13);

    _mp_skipBytes(len - 13);
  } else {

    _mp_readBytes(version, len);
  }

  mp_restoreInterrupts();

  // Plain string compare of up to 13 characters. Versions are zero-padded dates
  // (yyyy.mm.dd.bb) so a string compare is also a version compare.
  for (uint8_t i = 0; i < 13; i++) {

    if (minVersion[i] == 0x00)
      return true;

    if (version[i] > (uint8_t)minVersion[i])
      return true;

    if (version[i] < (uint8_t)minVersion[i])
      return false;
  }

  return true;
}

// -----------------------------------------------------------
// Connects the adapter to the multiplayer server for this game.
// Fills mp_protocolVersion, mp_gameMaxPlayers and mp_maxPacket.
// Network call (up to 5 seconds).
// -----------------------------------------------------------
uint8_t mp_init(uint16_t gameId) {

  mp_focusInterrupts();

  _mp_cmd(0x00);

  hcca_writeUInt16(gameId);

  hcca_writeByte(MP_LIB_VERSION);

  uint8_t result = hcca_readByte();

  mp_protocolVersion = hcca_readByte();
  mp_gameMaxPlayers  = hcca_readByte();
  mp_maxPacket       = hcca_readUInt16();

  mp_restoreInterrupts();

  return result;
}

// -----------------------------------------------------------
// Leaves any session and disconnects the adapter from the server.
// -----------------------------------------------------------
uint8_t mp_shutdown() {

  mp_focusInterrupts();

  _mp_cmd(0x01);

  uint8_t result = hcca_readByte();

  mp_restoreInterrupts();

  return result;
}

// -----------------------------------------------------------
// Gets the default player name the user set in the adapter's
// settings. name must be 16 bytes. Empty when none is set.
// -----------------------------------------------------------
void mp_getPlayerName(uint8_t *name) {

  mp_focusInterrupts();

  _mp_cmd(0x02);

  _mp_readBytes(name, 16);

  mp_restoreInterrupts();

  // Guarantee termination even if the adapter filled all 16 bytes
  name[15] = 0x00;
}

// -----------------------------------------------------------
// Gets readable text for the last error. text must be 64 bytes.
// -----------------------------------------------------------
void mp_getLastError(uint8_t *text) {

  mp_focusInterrupts();

  _mp_cmd(0x03);

  _mp_readBytes(text, 64);

  mp_restoreInterrupts();

  text[63] = 0x00;
}

// **************************************************************************
// Browsing
// **************************************************************************

// -----------------------------------------------------------
// Asks the server for this game's sessions. The adapter keeps a
// snapshot so the indexes stay valid until the next call.
// filter: MP_LIST_ALL or MP_LIST_JOINABLE.
// Network call (up to 5 seconds).
// -----------------------------------------------------------
uint8_t mp_listSessions(uint8_t filter, uint8_t *count) {

  mp_focusInterrupts();

  _mp_cmd(0x10);

  hcca_writeByte(filter);

  uint8_t result = hcca_readByte();
  uint8_t cnt    = hcca_readByte();

  mp_restoreInterrupts();

  if (count != NULL)
    *count = cnt;

  return result;
}

// -----------------------------------------------------------
// Gets one session from the last mp_listSessions() snapshot.
// info->sessionId is 0 when the index is out of range.
// -----------------------------------------------------------
void mp_getSessionInfo(uint8_t index, MpSessionInfo *info) {

  mp_focusInterrupts();

  _mp_cmd(0x11);

  hcca_writeByte(index);

  _mp_readBytes((uint8_t *)info, sizeof(MpSessionInfo));

  mp_restoreInterrupts();
}

// -----------------------------------------------------------
// Looks up one session by id, e.g. a private session a friend
// told you about. info->sessionId is 0 when not found.
// Network call (up to 5 seconds).
// -----------------------------------------------------------
void mp_getSessionInfoById(uint16_t sessionId, MpSessionInfo *info) {

  mp_focusInterrupts();

  _mp_cmd(0x12);

  hcca_writeUInt16(sessionId);

  _mp_readBytes((uint8_t *)info, sizeof(MpSessionInfo));

  mp_restoreInterrupts();
}

// **************************************************************************
// Create, join, leave
// **************************************************************************

// -----------------------------------------------------------
// Creates a session and makes you its host.
// code: 5 digits such as "12345", or NULL for no access code.
// Strings are clipped to 15 / 23 / 63 characters.
// Network call (up to 5 seconds).
// -----------------------------------------------------------
uint8_t mp_create(uint8_t maxPlayers, uint8_t flags, const char *code, const char *playerName, const char *sessionName, const char *description, uint16_t *sessionId, uint8_t *myId) {

  mp_focusInterrupts();

  _mp_cmd(0x20);

  hcca_writeByte(maxPlayers);

  hcca_writeByte(flags);

  _mp_writeCode(code);

  _mp_writeStr(playerName, MP_PLAYER_NAME_MAX);

  _mp_writeStr(sessionName, MP_SESSION_NAME_MAX);

  _mp_writeStr(description, MP_DESCRIPTION_MAX);

  uint8_t  result = hcca_readByte();
  uint16_t sid    = hcca_readUInt16();
  uint8_t  id     = hcca_readByte();

  mp_restoreInterrupts();

  if (sessionId != NULL)
    *sessionId = sid;

  if (myId != NULL)
    *myId = id;

  return result;
}

// -----------------------------------------------------------
// Joins a session by id.
// code: the 5-digit access code, or NULL when none is needed.
// Network call (up to 5 seconds).
// -----------------------------------------------------------
uint8_t mp_join(uint16_t sessionId, const char *code, const char *playerName, uint8_t *myId) {

  mp_focusInterrupts();

  _mp_cmd(0x21);

  hcca_writeUInt16(sessionId);

  _mp_writeCode(code);

  _mp_writeStr(playerName, MP_PLAYER_NAME_MAX);

  uint8_t result = hcca_readByte();
  uint8_t id     = hcca_readByte();

  mp_restoreInterrupts();

  if (myId != NULL)
    *myId = id;

  return result;
}

// -----------------------------------------------------------
// "Just put me in a game": joins an open session of this game,
// or creates one with maxPlayers/flags when none is open.
// created is set true when a new session was created (you are host).
// Network call (up to 5 seconds).
// -----------------------------------------------------------
uint8_t mp_quickJoin(uint8_t maxPlayers, uint8_t flags, const char *playerName, uint16_t *sessionId, uint8_t *myId, bool *created) {

  mp_focusInterrupts();

  _mp_cmd(0x22);

  hcca_writeByte(maxPlayers);

  hcca_writeByte(flags);

  _mp_writeStr(playerName, MP_PLAYER_NAME_MAX);

  uint8_t  result = hcca_readByte();
  uint16_t sid    = hcca_readUInt16();
  uint8_t  id     = hcca_readByte();
  uint8_t  isNew  = hcca_readByte();

  mp_restoreInterrupts();

  if (sessionId != NULL)
    *sessionId = sid;

  if (myId != NULL)
    *myId = id;

  if (created != NULL)
    *created = isNew != 0;

  return result;
}

// -----------------------------------------------------------
// Leaves the current session.
// -----------------------------------------------------------
uint8_t mp_leave() {

  mp_focusInterrupts();

  _mp_cmd(0x23);

  uint8_t result = hcca_readByte();

  mp_restoreInterrupts();

  return result;
}

// **************************************************************************
// Session management
// **************************************************************************

// -----------------------------------------------------------
// Gets the session status. Cheap (14 bytes on the wire); poll it
// every frame or two to see joins, the game starting, packets
// waiting, and the session ending.
// -----------------------------------------------------------
void mp_status(MpStatus *status) {

  mp_focusInterrupts();

  _mp_cmd(0x30);

  _mp_readBytes((uint8_t *)status, sizeof(MpStatus));

  mp_restoreInterrupts();
}

// -----------------------------------------------------------
// Gets one player from the roster. player->flags has
// MP_PLAYER_PRESENT clear when the slot is empty. Only re-read the
// roster when MpStatus.rosterRev changes.
// -----------------------------------------------------------
void mp_getPlayer(uint8_t playerId, MpPlayerInfo *player) {

  mp_focusInterrupts();

  _mp_cmd(0x31);

  hcca_writeByte(playerId);

  _mp_readBytes((uint8_t *)player, sizeof(MpPlayerInfo));

  mp_restoreInterrupts();
}

// -----------------------------------------------------------
// Sets your ready flag in the lobby.
// -----------------------------------------------------------
uint8_t mp_setReady(bool ready) {

  mp_focusInterrupts();

  _mp_cmd(0x32);

  hcca_writeByte(ready ? 1 : 0);

  uint8_t result = hcca_readByte();

  mp_restoreInterrupts();

  return result;
}

// -----------------------------------------------------------
// Sets your 8 bytes of player data (colour, character, team...).
// data must be 8 bytes.
// -----------------------------------------------------------
uint8_t mp_setPlayerData(const uint8_t *data) {

  mp_focusInterrupts();

  _mp_cmd(0x33);

  hcca_writeBytes(0, MP_PLAYER_DATA_SIZE, (uint8_t *)data);

  uint8_t result = hcca_readByte();

  mp_restoreInterrupts();

  return result;
}

// -----------------------------------------------------------
// Writes len bytes of the 16 bytes of session data, starting at
// offset. Host only, unless the session has MP_FLAG_SHARED_DATA.
// Session data is visible in MpSessionInfo before joining.
// -----------------------------------------------------------
uint8_t mp_setSessionData(uint8_t offset, uint8_t len, const uint8_t *data) {

  // Clip locally so we never send more than the adapter will accept
  if (offset >= MP_SESSION_DATA_SIZE)
    return MP_ERR_BAD_ARG;

  if (len > MP_SESSION_DATA_SIZE - offset)
    len = MP_SESSION_DATA_SIZE - offset;

  mp_focusInterrupts();

  _mp_cmd(0x34);

  hcca_writeByte(offset);

  hcca_writeByte(len);

  hcca_writeBytes(0, len, (uint8_t *)data);

  uint8_t result = hcca_readByte();

  mp_restoreInterrupts();

  return result;
}

// -----------------------------------------------------------
// Gets the 16 bytes of session data. buf must be 16 bytes.
// -----------------------------------------------------------
void mp_getSessionData(uint8_t *buf) {

  mp_focusInterrupts();

  _mp_cmd(0x35);

  _mp_readBytes(buf, MP_SESSION_DATA_SIZE);

  mp_restoreInterrupts();
}

// -----------------------------------------------------------
// Starts the game (host only). Everyone polling mp_status() sees
// MP_STATE_IN_GAME. Network call (up to 5 seconds).
// -----------------------------------------------------------
uint8_t mp_start() {

  mp_focusInterrupts();

  _mp_cmd(0x36);

  uint8_t result = hcca_readByte();

  mp_restoreInterrupts();

  return result;
}

// -----------------------------------------------------------
// Gets the shared seed and start time once the game started.
// -----------------------------------------------------------
void mp_getStartInfo(MpStartInfo *info) {

  mp_focusInterrupts();

  _mp_cmd(0x37);

  _mp_readBytes((uint8_t *)info, sizeof(MpStartInfo));

  mp_restoreInterrupts();
}

// -----------------------------------------------------------
// Ends the game (host only). winnerMask: bit n = player n won.
// flags: MP_END_FLAG_TO_LOBBY to go back to the lobby for a rematch.
// -----------------------------------------------------------
uint8_t mp_end(uint16_t winnerMask, uint8_t flags) {

  mp_focusInterrupts();

  _mp_cmd(0x38);

  hcca_writeUInt16(winnerMask);

  hcca_writeByte(flags);

  uint8_t result = hcca_readByte();

  mp_restoreInterrupts();

  return result;
}

// -----------------------------------------------------------
// Removes a player from the session (host only).
// -----------------------------------------------------------
uint8_t mp_kick(uint8_t playerId) {

  mp_focusInterrupts();

  _mp_cmd(0x39);

  hcca_writeByte(playerId);

  uint8_t result = hcca_readByte();

  mp_restoreInterrupts();

  return result;
}

// -----------------------------------------------------------
// Locks or unlocks the session to new joins (host only).
// -----------------------------------------------------------
uint8_t mp_setLocked(bool locked) {

  mp_focusInterrupts();

  _mp_cmd(0x3a);

  hcca_writeByte(locked ? 1 : 0);

  uint8_t result = hcca_readByte();

  mp_restoreInterrupts();

  return result;
}

// -----------------------------------------------------------
// Hands the host role to another player (host only).
// -----------------------------------------------------------
uint8_t mp_setHost(uint8_t playerId) {

  mp_focusInterrupts();

  _mp_cmd(0x3b);

  hcca_writeByte(playerId);

  uint8_t result = hcca_readByte();

  mp_restoreInterrupts();

  return result;
}

// **************************************************************************
// Packets
// **************************************************************************

// -----------------------------------------------------------
// Sends a packet of 1 to mp_maxPacket bytes.
// target:  a player id, MP_TARGET_OTHERS, MP_TARGET_ALL or MP_TARGET_HOST.
// channel: 0 reliable & ordered, 1-255 latest-only.
// Returns MP_OK, or MP_ERR_RATE_LIMIT / MP_ERR_TOO_LARGE / ...
// -----------------------------------------------------------
uint8_t mp_send(uint8_t target, uint8_t channel, uint16_t len, const uint8_t *data) {

  mp_focusInterrupts();

  _mp_cmd(0x40);

  hcca_writeByte(target);

  hcca_writeByte(channel);

  hcca_writeUInt16(len);

  hcca_writeBytes(0, len, (uint8_t *)data);

  uint8_t result = hcca_readByte();

  mp_restoreInterrupts();

  return result;
}

// -----------------------------------------------------------
// Receives the next packet into buf (up to maxLen bytes).
// Returns 0 when nothing is waiting, otherwise the packet length.
// from and channel may be NULL. A longer packet is truncated.
// -----------------------------------------------------------
uint16_t mp_recv(uint8_t *buf, uint16_t maxLen, uint8_t *from, uint8_t *channel) {

  mp_focusInterrupts();

  _mp_cmd(0x41);

  hcca_writeUInt16(maxLen);

  uint16_t len = _mp_readPacket(buf, maxLen, from, channel);

  mp_restoreInterrupts();

  return len;
}

// -----------------------------------------------------------
// Looks at the next packet without removing it.
// Returns its length (0 = none). from and channel may be NULL.
// -----------------------------------------------------------
uint16_t mp_peek(uint8_t *from, uint8_t *channel) {

  mp_focusInterrupts();

  _mp_cmd(0x42);

  uint16_t len = hcca_readUInt16();
  uint8_t  f   = hcca_readByte();
  uint8_t  c   = hcca_readByte();

  mp_restoreInterrupts();

  if (from != NULL)
    *from = f;

  if (channel != NULL)
    *channel = c;

  return len;
}

// -----------------------------------------------------------
// Throws away every waiting packet.
// -----------------------------------------------------------
uint8_t mp_flush() {

  mp_focusInterrupts();

  _mp_cmd(0x43);

  uint8_t result = hcca_readByte();

  mp_restoreInterrupts();

  return result;
}

// -----------------------------------------------------------
// Sends a packet and receives the next waiting one in a single
// exchange with the adapter (saves one round trip per frame).
// sendResult (may be NULL) gets the mp_send result.
// Returns the received length, 0 when nothing was waiting.
// -----------------------------------------------------------
uint16_t mp_exchange(uint8_t target, uint8_t channel, uint16_t len, const uint8_t *data, uint8_t *buf, uint16_t maxLen, uint8_t *from, uint8_t *rxChannel, uint8_t *sendResult) {

  mp_focusInterrupts();

  _mp_cmd(0x44);

  hcca_writeByte(target);

  hcca_writeByte(channel);

  hcca_writeUInt16(len);

  hcca_writeBytes(0, len, (uint8_t *)data);

  hcca_writeUInt16(maxLen);

  uint8_t result = hcca_readByte();

  uint16_t rxLen = _mp_readPacket(buf, maxLen, from, rxChannel);

  mp_restoreInterrupts();

  if (sendResult != NULL)
    *sendResult = result;

  return rxLen;
}

// **************************************************************************
// Chat
// **************************************************************************

// -----------------------------------------------------------
// Sends a chat line (clipped to 127 characters) to the session.
// -----------------------------------------------------------
uint8_t mp_chatSend(const char *text) {

  mp_focusInterrupts();

  _mp_cmd(0x50);

  _mp_writeStr(text, MP_CHAT_MAX);

  uint8_t result = hcca_readByte();

  mp_restoreInterrupts();

  return result;
}

// -----------------------------------------------------------
// Gets the next chat line. Returns its length, 0 when none.
// name must be 16 bytes, text must be 128 bytes; both are NUL
// terminated. from is a player id, MP_CHAT_FROM_WEB or
// MP_CHAT_FROM_SYSTEM.
// -----------------------------------------------------------
uint8_t mp_chatRecv(uint8_t *from, uint8_t *name, uint8_t *text) {

  mp_focusInterrupts();

  _mp_cmd(0x51);

  uint8_t len = hcca_readByte();

  if (len != 0) {

    uint8_t f = hcca_readByte();

    if (from != NULL)
      *from = f;

    _mp_readBytes(name, 16);

    // Never write past the 128-byte text buffer
    if (len > MP_CHAT_MAX) {

      _mp_readBytes(text, MP_CHAT_MAX);

      _mp_skipBytes(len - MP_CHAT_MAX);

      len = MP_CHAT_MAX;
    } else {

      _mp_readBytes(text, len);
    }
  }

  mp_restoreInterrupts();

  if (len != 0) {

    name[15]  = 0x00;
    text[len] = 0x00;
  }

  return len;
}

// **************************************************************************
// Time
// **************************************************************************

// -----------------------------------------------------------
// Milliseconds since the session was created, synchronized
// across all players.
// -----------------------------------------------------------
uint32_t mp_time() {

  mp_focusInterrupts();

  _mp_cmd(0x60);

  uint32_t t = hcca_readUInt32();

  mp_restoreInterrupts();

  return t;
}

// -----------------------------------------------------------
// Round trip from the adapter to the server, in milliseconds.
// -----------------------------------------------------------
uint16_t mp_ping() {

  mp_focusInterrupts();

  _mp_cmd(0x61);

  uint16_t t = hcca_readUInt16();

  mp_restoreInterrupts();

  return t;
}

// **************************************************************************
// Scores and status text
// **************************************************************************

// -----------------------------------------------------------
// Sets your live score, shown in the roster and on nabu.ca.
// -----------------------------------------------------------
uint8_t mp_setScore(int32_t score) {

  mp_focusInterrupts();

  _mp_cmd(0x70);

  hcca_writeInt32(score);

  uint8_t result = hcca_readByte();

  mp_restoreInterrupts();

  return result;
}

// -----------------------------------------------------------
// Sets the session's status line shown on nabu.ca, e.g.
// "Round 3 of 5" (host only, clipped to 63 characters).
// -----------------------------------------------------------
uint8_t mp_setStatusText(const char *text) {

  mp_focusInterrupts();

  _mp_cmd(0x71);

  _mp_writeStr(text, MP_STATUS_TEXT_MAX);

  uint8_t result = hcca_readByte();

  mp_restoreInterrupts();

  return result;
}

// -----------------------------------------------------------
// Submits a score to a leaderboard defined for the game on
// nabu.ca. rank (may be NULL) gets the placing.
// Network call (up to 5 seconds).
// -----------------------------------------------------------
uint8_t mp_scoreSubmit(uint8_t board, int32_t score, const char *name, uint16_t *rank) {

  mp_focusInterrupts();

  _mp_cmd(0x72);

  hcca_writeByte(board);

  hcca_writeInt32(score);

  _mp_writeStr(name, MP_PLAYER_NAME_MAX);

  uint8_t  result = hcca_readByte();
  uint16_t r      = hcca_readUInt16();

  mp_restoreInterrupts();

  if (rank != NULL)
    *rank = r;

  return result;
}

// -----------------------------------------------------------
// Fetches a leaderboard (top 100) into the adapter.
// count gets the number of entries for mp_scoreGet().
// Network call (up to 5 seconds).
// -----------------------------------------------------------
uint8_t mp_scoreCount(uint8_t board, uint8_t *count) {

  mp_focusInterrupts();

  _mp_cmd(0x73);

  hcca_writeByte(board);

  uint8_t result = hcca_readByte();
  uint8_t cnt    = hcca_readByte();

  mp_restoreInterrupts();

  if (count != NULL)
    *count = cnt;

  return result;
}

// -----------------------------------------------------------
// Gets one row from the last mp_scoreCount() fetch.
// entry->rank is 0 when the index is out of range.
// -----------------------------------------------------------
void mp_scoreGet(uint8_t board, uint8_t index, MpScoreEntry *entry) {

  mp_focusInterrupts();

  _mp_cmd(0x74);

  hcca_writeByte(board);

  hcca_writeByte(index);

  _mp_readBytes((uint8_t *)entry, sizeof(MpScoreEntry));

  mp_restoreInterrupts();
}

// **************************************************************************
// Cloud saves (per adapter, per game: 4 slots of up to 1024 bytes)
// **************************************************************************

// -----------------------------------------------------------
// Saves len bytes into a cloud save slot (0-3).
// Network call (up to 5 seconds).
// -----------------------------------------------------------
uint8_t mp_storeWrite(uint8_t slot, uint16_t len, const uint8_t *data) {

  mp_focusInterrupts();

  _mp_cmd(0x78);

  hcca_writeByte(slot);

  hcca_writeUInt16(len);

  hcca_writeBytes(0, len, (uint8_t *)data);

  uint8_t result = hcca_readByte();

  mp_restoreInterrupts();

  return result;
}

// -----------------------------------------------------------
// Loads a cloud save slot into buf (up to maxLen bytes).
// Returns the number of bytes read, 0 when the slot is empty.
// Network call (up to 5 seconds).
// -----------------------------------------------------------
uint16_t mp_storeRead(uint8_t slot, uint8_t *buf, uint16_t maxLen) {

  mp_focusInterrupts();

  _mp_cmd(0x79);

  hcca_writeByte(slot);

  hcca_writeUInt16(maxLen);

  uint16_t len = hcca_readUInt16();

  // Guard the caller's buffer even if the adapter sends more than asked
  if (len > maxLen) {

    _mp_readBytes(buf, maxLen);

    _mp_skipBytes(len - maxLen);

    len = maxLen;
  } else {

    _mp_readBytes(buf, len);
  }

  mp_restoreInterrupts();

  return len;
}

// **************************************************************************
// Convenience
// **************************************************************************

// -----------------------------------------------------------
// Short readable name for a result code, handy for on-screen
// errors. For the detailed reason use mp_getLastError().
// -----------------------------------------------------------
// Indexed by result code (MP_OK .. MP_ERR_LOCKED)
const char *_mp_resultNames[] = {
  "OK",
  "NOT INITIALIZED",
  "OFFLINE",
  "UNKNOWN GAME",
  "NOT FOUND",
  "SESSION FULL",
  "WRONG CODE",
  "ALREADY STARTED",
  "NOT HOST",
  "NOT IN SESSION",
  "BAD ARGUMENT",
  "RATE LIMIT",
  "TOO LARGE",
  "KICKED",
  "UPDATE ADAPTER",
  "NAME REJECTED",
  "SERVER BUSY",
  "TIMEOUT",
  "LOCKED"
};

const char *mp_resultName(uint8_t result) {

  if (result > MP_ERR_LOCKED)
    return "UNKNOWN ERROR";

  return _mp_resultNames[result];
}

#endif
