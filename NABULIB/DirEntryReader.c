// ****************************************************************************************
// NABU-LIB DirEntryReader - CP/M Directory Entry Reader
// DJ Sures (c) 2024
// https://nabu.ca
//
// Last updated on 2026.04.27.00
//
// Get latest copy and examples from: https://github.com/DJSures/NABU-LIB
//
// Use z88dk 23.07.2022 (v2.3)
//
// This module provides a thin C wrapper around the CP/M 2.2 BDOS file/directory
// services so a NABU CP/M program can enumerate disk directory entries, build a
// File Control Block (FCB) from a filename string, and pull a clean
// "NAME.EXT" string out of a matched directory record.
//
// What's in this file
// -------------------
//   - BDOS function number defines (CPM_RCON .. CPM_GSX)
//   - struct FCB definition matching the CP/M 2.2 layout
//   - Module-level srcFCB / destFCB used by the directory walk and copy ops
//   - BDOS()                            Inline-asm wrapper that calls 0x0005
//   - ResetTFCB()                       Resets the default FCBs at 0x5C / 0x6C
//   - CreateFcb()                       Builds an FCB from drive + filename
//   - DirMoveFirst() / DirMoveNext()    Walk a wildcard directory search
//   - DirGetEntryName()                 Pulls raw 8.3 fields from current entry
//   - DirGetEntryNameWithExtension()    Builds a trimmed "NAME.EXT" string
//
// File system helpers
// -------------------
//   - SetDMAAddress()                   Set BDOS DMA buffer address
//   - SelectDisk() / GetCurrentDisk()   Default-drive control
//   - ResetDiskSystem()                 Flush buffers, re-select A:
//   - GetLoggedDrives()                 Bitmap of mounted drives
//   - GetUserArea() / SetUserArea()     User-area control (0..15)
//   - FileExists()                      Test for existence (no wildcards)
//   - OpenFile() / CloseFile()          Open/close an FCB
//   - CreateFile()                      Create or truncate a file
//   - DeleteFile()                      Delete (wildcards allowed)
//   - RenameFile()                      Rename a file
//   - ReadRecord() / WriteRecord()      Sequential 128-byte record I/O
//   - ReadRandomRecord() / WriteRandomRecord()
//                                       Indexed 128-byte record I/O
//   - ComputeFileSize()                 File size in 128-byte records
//   - GetFileSizeBytes()                File size in bytes (convenience)
//
// Dependencies
// ------------
// This file expects the following to already be defined by the including
// translation unit (typically through NABU-LIB / a host project header):
//   - copyBuf             : a uint8_t buffer used as the BDOS DMA target
//   - COPY_BUF_LEN        : its length (must be at least 128 bytes for one
//                           directory record set; larger is fine)
//   - UpperStr()          : in-place uppercase helper
//   - memset / memcpy     : standard C library
//
// Typical usage
// -------------
//   if (DirMoveFirst("*.COM")) {
//
//     do {
//
//       uint8_t name[13];
//       DirGetEntryNameWithExtension(name);
//       // ... do something with name ...
//
//     } while (DirMoveNext());
//   }
//
// IMPORTANT
// ---------
// Like NABU-LIB.h, this file is intended to be included from EXACTLY ONE .c
// translation unit because it defines module-level state (srcFCB, destFCB,
// _TEMP* scratch vars, _directoryRecordIndex). Including it twice will cause
// duplicate symbol errors at link time.
//
// Make something,
// DJ Sures
// ****************************************************************************************

/* BDOS calls */
#define CPM_RCON 1               /* read console */
#define CPM_WCON 2               /* write console */
#define CPM_RRDR 3               /* read reader */
#define CPM_WPUN 4               /* write punch */
#define CPM_WLST 5               /* write list */
#define CPM_DCIO 6               /* direct console I/O */
#define CPM_GIOB 7               /* get I/O byte */
#define CPM_SIOB 8               /* set I/O byte */
#define CPM_RCOB 10              /* read console buffered */
#define CPM_ICON 11              /* interrogate console ready */
#define CPM_VERS 12              /* return version number */
#define CPM_RDS  13              /* reset disk system */
#define CPM_LGIN 14              /* log in and select disk */
#define CPM_OPN  15              /* open file */
#define CPM_CLS  16              /* close file */
#define CPM_FFST 17              /* find first */
#define CPM_FNXT 18              /* find next */
#define CPM_DEL  19              /* delete file */
#define CPM_READ 20              /* read next record */
#define CPM_WRIT 21              /* write next record */
#define CPM_MAKE 22              /* create file */
#define CPM_REN  23              /* rename file */
#define CPM_ILOG 24              /* get bit map of logged in disks */
#define CPM_IDRV 25              /* interrogate drive number */
#define CPM_SDMA 26              /* set DMA address for i/o */
#define CPM_SUID 32              /* set/get user id */
#define CPM_RRAN 33              /* read random record */
#define CPM_WRAN 34              /* write random record */
#define CPM_CFS  35              /* compute file size */
#define CPM_DSEG 51              /* set DMA segment */
#define CPM_GSX 115              /* enter GSX */

// FCB structure definition (may be duplicated from header)
struct FCB {
  uint8_t drive;          // drive code (drive | user area)
  uint8_t name[8];        // file name 
  uint8_t ext[3];         // file type 
  uint8_t extent;         // file extent 
  uint8_t reserved[2];    // reserved
  uint8_t recordCnt;      // number of records in present extent 
  uint8_t discmap[16];    // CP/M disc map (AL)
  uint8_t currentRecord;  // next record to read or write 
  uint8_t ranrec[3];      // random record number (24 bit no.)
};

struct FCB srcFCB  = { 0 }; // srcFCB is used as the source for copying files, launching programs, and for obtaining the directory entry.
struct FCB destFCB = { 0 };

volatile uint8_t _TEMP8_1 = 0;
volatile uint8_t _TEMP8_2 = 0;
volatile uint16_t _TEMP16_1 = 0;
volatile uint16_t _TEMP16_2 = 0;

uint8_t _directoryRecordIndex = 0;


// BDOS function wrapper using inline assembly.
// This function calls the CP/M BDOS routine at address 0x05.
//
// Parameters:
//   func - BDOS function number (loaded into register C).
//   arg  - 16-bit argument (loaded into register pair DE); commonly an
//          address (FCB pointer, DMA buffer) or a packed value depending
//          on the BDOS call.
//
// Returns:
//   The 8-bit value returned by BDOS in register A. Meaning is call
//   specific (status code, character, directory record index, etc.).
//
// Notes:
//   BC, DE, AF are preserved across the call by push/pop. The temp
//   variables _TEMP8_1 and _TEMP16_2 are used to marshal arguments
//   from C into the inline asm block.
uint8_t BDOS(uint8_t func, uint16_t arg) {

  // store the arguments in the temporary variables that can be accessed by the inline assembly code.
  _TEMP8_1 = func;
  _TEMP16_2 = arg;

  __asm
    push bc
    push de
    push af

    ld a, (__TEMP8_1)
    ld c, a
    ld b, 0x00

    ld de, (__TEMP16_2)

    call 0x05

    ld (__TEMP8_1), a // store the value in A into the location of temp8_1

    pop af
    pop de
    pop bc
  __endasm;

  return _TEMP8_1;
}

// ResetTFCB: Resets the default FCB (File Control Block) and fills it with the
// command-line argument if provided. The FCB for CP/M 2.2 is located in zero page
// at address 0x5C (FCB 1) and 0x6C (FCB 2).
//
// Parameters:
//   args - Null-terminated string containing a filename in "NAME.EXT"
//          form. Pass an empty string (first byte 0x00) to leave the
//          name/ext fields blank-padded with spaces. The pointer is
//          advanced internally; the caller's pointer is not modified.
//
// Behavior:
//   - Zeroes both default FCBs (16 bytes each).
//   - Pads the 11-byte name+ext region of FCB1 with spaces (0x20).
//   - If args is non-empty, copies up to 8 chars of name (stopping at
//     '.' or NUL) and up to 3 chars of extension into FCB1.
//   - Pads the name+ext region of FCB2 with spaces.
//
// Notes:
//   This is the standard "reset transient FCB" used before invoking
//   another transient program or BDOS file call.
void ResetTFCB(uint8_t *args) {

  // Define pointers to the default FCB locations.
  uint8_t *fcb1 = (uint8_t *)0x5c;
  uint8_t *fcb2 = (uint8_t *)0x6c;

  // Reset both FCB values to 0 (each FCB is 16 bytes)
  memset(fcb1, 0, 16);
  memset(fcb2, 0, 16);

  // FCB 1:
  // Set filename and extension (11 bytes) to 0x20 (space)
  memset(fcb1 + 1, 0x20, 11);

  if (*args != 0x00) {

    // Fill the default FCB with the argument if provided.
    // The first 8 characters are for the filename (padded with spaces)
    // and the next 3 are for the extension.

    // Copy filename (up to 8 characters or until a '.' is encountered)
    uint8_t i = 0;
    while (i < 8 && *args != 0x00 && *args != '.') {

      // Place the character in the FCB filename area.
      fcb1[1 + i] = *args;  
      i++;
      args++;
    }

    // Skip over the '.' if there was one.
    if (*args == '.')
      args++;

    // Copy extension (up to 3 characters)
    uint8_t j = 0;
    while (j < 3 && *args != 0x00) {

      // Place the character in the FCB extension area.
      fcb1[9 + j] = *args;  
      j++;
      args++;
    }
  }

  // FCB 2:
  // Set filename and extension (11 bytes) to 0x20 (space)
  memset(fcb2 + 1, 0x20, 11);
}

// Create an FCB from the drive, userNo, and name parameters for searching or opening file.
//
// Parameters:
//   fc    - Pointer to a caller-allocated FCB struct. The struct is
//           fully zeroed before being populated.
//   drive - Drive code byte (0 = default/current, 1 = A:, 2 = B:, ...).
//           This byte may be ORed with the user area number depending
//           on the BDOS convention used by the caller.
//   name  - Null-terminated filename, e.g. "FOO.TXT" or "FOO*.*".
//           The string is uppercased in place. Wildcards:
//             '*' expands to '?' fill characters in the remaining slots.
//             ' '/control chars terminate the field.
//
// Behavior:
//   - Zeros the entire FCB.
//   - Sets fc->drive.
//   - Copies up to 8 name characters and up to 3 extension characters
//     into the FCB, padding the rest with spaces (or '?' if a '*' was
//     seen in that field).
//   - Resets fc->extent and fc->currentRecord to 0.
//
// Notes:
//   Side effect: the caller's name buffer is uppercased via UpperStr.
void CreateFcb(struct FCB *fc, uint8_t drive, uint8_t *name) {

  // Destination pointers for copying into the FCB's name and extension fields.
  uint8_t *dest_name = fc->name; // Points to the beginning of the name field.
  uint8_t *dest_ext  = fc->ext;  // Points to the beginning of the extension field.
  uint8_t fillChar;              // Character used to fill unused spaces.
  
  memset((void*)fc, 0x00, sizeof(*fc));

  // Set drive and user number in the FCB.
  fc->drive = drive;
 
  // Convert the entire input string to uppercase.
  UpperStr(name);
  
  // Copy the name up to the '.' or a '*'.
  // Copy characters into fc->name until a dot, asterisk, or whitespace is encountered,
  // or until 8 characters have been copied.
  while (*name != '.' && *name != '*' && *name > ' ' && dest_name < fc->name + 8) 
      *dest_name++ = *name++;  
  
  // Determine the fill character: if '*' encountered then use '?' otherwise a space.
  if (*name == '*')
      fillChar = '?';
  else
      fillChar = ' ';
  
  // Fill the remainder of the 8-character file name field.
  while (dest_name < fc->name + 8) 
      *dest_name++ = fillChar;  
  
  // Loop till we get to the extension: skip characters until a dot is found.
  while (*name && *name != '.') 
      name++;  
  
  // If a dot is found, skip it.
  if (*name == '.')
      name++;
  
  // Now fill in the extension.
  // Copy up to 3 characters for the extension until whitespace or '*' is encountered.
  while (*name > ' ' && *name != '*' && dest_ext < fc->ext + 3) 
      *dest_ext++ = *name++;  
  
  // Determine the fill character for the extension.
  if (*name == '*')
      fillChar = '?';
  else
      fillChar = ' ';
  
  // Fill the remainder of the 3-character extension field.
  while (dest_ext < fc->ext + 3) 
      *dest_ext++ = fillChar;  
  
  // Initialize extent and currentRecord fields to 0.
  fc->extent = 0;
  fc->currentRecord = 0;
}

// DirMoveFirst() initializes the directory search.
// drive is 0 = current, 1 = A:, 2 = B:, 3 = C:, etc...
// It sets up the DMA buffer, initializes the FCB with the search pattern "*.*", and calls CPM_FFST.
// Returns true or false failure
//
// Parameters:
//   filter - Null-terminated wildcard pattern such as "*.*", "*.COM",
//            or "FOO?.TXT". Passed through CreateFcb, which uppercases
//            it and translates '*' into '?' fills.
//
// Returns:
//   true  - A matching directory entry was found. Use the module-level
//           copyBuf and _directoryRecordIndex to read the entry, or
//           call DirGetEntryName / DirGetEntryNameWithExtension.
//   false - No match (BDOS returned 0xFF).
//
// Side effects:
//   - Clears copyBuf (length COPY_BUF_LEN).
//   - Sets the BDOS DMA address to copyBuf.
//   - Re-initializes the module-level srcFCB.
//   - Updates _directoryRecordIndex with the directory record number
//     (0..3) of the matched entry within the 128-byte DMA buffer.
bool DirMoveFirst(uint8_t * filter) {

  // (Optional) Mark the end of the directory buffer with a null terminator.
  memset(copyBuf, 0x00, COPY_BUF_LEN);

  // Set the DMA address for the BDOS call.
  BDOS(CPM_SDMA, (uint16_t)&copyBuf);

  // Initialize the FCB with the search pattern "*.*".
  // Fixed: Use srcFCB (which is already declared) instead of undefined "der_fc_dir".
  CreateFcb(&srcFCB, 0, filter);

  // Call BDOS function CPM_FFST to find the first matching file.
  // Fixed: Pass the address of srcFCB instead of the undefined "der_fc_dir".
  _directoryRecordIndex = BDOS(CPM_FFST, (uint16_t)&srcFCB);

  return _directoryRecordIndex != 0xff;
}

// DirMoveNext() moves the search to the next file entry.
// It does not reinitialize the directory search, so subsequent calls simply advance.
// Returns true or false failure
//
// Parameters:
//   (none) - Reuses the srcFCB and DMA buffer set up by DirMoveFirst.
//
// Returns:
//   true  - Another matching directory entry was found.
//   false - No more matches (BDOS returned 0xFF). The directory walk
//           is complete.
//
// Notes:
//   Must be preceded by a successful DirMoveFirst call. The DMA buffer
//   and srcFCB must not be disturbed between calls, or the search will
//   restart / return garbage.
bool DirMoveNext(void) {

  // Call BDOS function CPM_FNXT to find the next file using srcFCB.
  _directoryRecordIndex = BDOS(CPM_FNXT, (uint16_t)&srcFCB);

  return _directoryRecordIndex != 0xff;
}

// Extracts the filename at the specified index from the current directory entry.
// It assumes the directory entry is 32 bytes long with the filename stored starting at specified offset.
// The filename consists of an 8-character name and a 3-character extension.
// filenamePtr must be 9 characters (filename + null)
// extensionPtr must be 4 characters (ext + null)
//
// Parameters:
//   filenamePtr  - Caller-supplied buffer of at least 9 bytes. Receives
//                  the 8-byte filename (space-padded as stored in the
//                  directory entry) followed by a NUL terminator.
//   extensionPtr - Caller-supplied buffer of at least 4 bytes. Receives
//                  the 3-byte extension (space-padded) followed by a
//                  NUL terminator.
//
// Behavior:
//   - Zeros both output buffers first.
//   - Reads the entry at copyBuf + (_directoryRecordIndex * 32):
//       byte 0      : user number (skipped)
//       bytes 1..8  : filename
//       bytes 9..11 : extension
//
// Notes:
//   Padding spaces are NOT trimmed. Use DirGetEntryNameWithExtension
//   if a "NAME.EXT" form without trailing spaces is required.
void DirGetEntryName(uint8_t *filenamePtr, uint8_t *extensionPtr) {

  memset(filenamePtr, 0, 9);
  memset(extensionPtr, 0, 4);

  // Calculate the pointer to the desired directory entry in copyBuf.
  // Each directory entry is 32 bytes long.
  uint8_t *entry = copyBuf + (_directoryRecordIndex * 32);

  // Copy the 8-byte filename from bytes 1-8.
  // (Byte 0 is the user number, so we start at entry + 1.)
  memcpy(filenamePtr, entry + 1, 8);

  // Copy the 3-byte extension from bytes 9-11.
  memcpy(extensionPtr, entry + 9, 3);
}

// Extracts the filename from the current directory entry and builds a null‐terminated
// string in the format "NAME.EXT" (with no trailing spaces).
// The directory entry is 32 bytes long, with:
//   - Byte 0: user number (ignored)
//   - Bytes 1-8: 8-character filename (padded with spaces)
//   - Bytes 9-11: 3-character extension (padded with spaces)
// The filenamePtr buffer must be at least 13 bytes long.
//
// Parameters:
//   filenamePtr - Caller-supplied buffer of at least 13 bytes
//                 (8 name + 1 dot + 3 ext + 1 NUL). Receives the
//                 trimmed "NAME.EXT" string. If the entry has no
//                 extension, the dot is omitted (just "NAME").
//
// Behavior:
//   - Zeros the output buffer first.
//   - Reads the entry at copyBuf + (_directoryRecordIndex * 32).
//   - Copies filename characters until a space or 8 chars consumed.
//   - Appends '.' + extension only if at least one non-space byte
//     exists in the extension field.
//   - NUL-terminates the result.
//
// Notes:
//   Relies on _directoryRecordIndex set by DirMoveFirst / DirMoveNext.
void DirGetEntryNameWithExtension(uint8_t *filenamePtr) {

  memset(filenamePtr, 0, 13);
 
  // Calculate pointer to the directory entry in copyBuf.
  // Each directory entry is 32 bytes.
  uint8_t *entry = copyBuf + (_directoryRecordIndex * 32);

  // Build the filename into filenamePtr.
  uint8_t *dst = filenamePtr;
  uint8_t *src = entry + 1;  // Bytes 1-8 contain the filename.

  // Copy filename characters until a space is encountered or 8 characters are copied.
  for (uint8_t i = 0; i < 8; i++) {

      if (src[i] == ' ') 
          break;
      
      *dst++ = src[i];
  }

  // Check if there is any non-space character in the extension (bytes 9-11).
  src = entry + 9;
  bool extPresent = false;
  for (uint8_t i = 0; i < 3; i++) {

      if (src[i] != ' ') {

          extPresent = true;

          break;
      }
  }

  // If an extension is present, add a dot and copy the extension characters.
  if (extPresent) {

      *dst++ = '.';

      for (uint8_t i = 0; i < 3; i++) {

          if (src[i] == ' ') 
              break;
          
          *dst++ = src[i];
      }
  }

  // Null-terminate the resulting string.
  *dst = '\0';
}

// SetDMAAddress: Set the BDOS DMA buffer used by subsequent file/directory I/O.
//
// Parameters:
//   addr - Pointer to a buffer (typically 128 bytes) that will receive
//          data from BDOS reads or supply data to BDOS writes.
//
// Notes:
//   The DMA address persists across BDOS calls until explicitly changed
//   or until a transient program is loaded. The directory walk routines
//   (DirMoveFirst/DirMoveNext) reset DMA to copyBuf, so call this again
//   before file I/O if you want data to land somewhere else.
void SetDMAAddress(void *addr) {

  BDOS(CPM_SDMA, (uint16_t)addr);
}

// SelectDisk: Log in and select a drive as the default.
//
// Parameters:
//   drive - 0 = A:, 1 = B:, 2 = C:, ... (BDOS drive code, NOT the FCB
//           form where 1 = A:).
//
// Returns:
//   The BDOS return code from function 14 (typically 0 on success).
uint8_t SelectDisk(uint8_t drive) {

  return BDOS(CPM_LGIN, drive);
}

// GetCurrentDisk: Return the currently selected default drive.
//
// Returns:
//   0 = A:, 1 = B:, 2 = C:, ...
uint8_t GetCurrentDisk(void) {

  return BDOS(CPM_IDRV, 0);
}

// ResetDiskSystem: Reset the disk system, flushing buffers and re-selecting A:.
//
// Notes:
//   Call this after changing media or before walking the filesystem
//   from a known state.
void ResetDiskSystem(void) {

  BDOS(CPM_RDS, 0);
}

// GetLoggedDrives: Return a 16-bit bitmap of currently logged-in drives.
//
// Returns:
//   Bit 0 = A:, bit 1 = B:, bit 2 = C:, ... (1 = logged in).
//
// Notes:
//   BDOS function 24 returns the map in HL, but the BDOS() wrapper only
//   returns the A register. This routine issues the call directly so it
//   can capture the full HL value.
uint16_t GetLoggedDrives(void) {

  _TEMP8_1 = CPM_ILOG;

  __asm
    push bc
    push de
    push hl

    ld a, (__TEMP8_1)
    ld c, a
    ld b, 0x00

    call 0x05

    ld (__TEMP16_1), hl

    pop hl
    pop de
    pop bc
  __endasm;

  return _TEMP16_1;
}

// GetUserArea: Return current user area number (0..15).
//
// Notes:
//   BDOS function 32 with E = 0xFF queries the current user area.
uint8_t GetUserArea(void) {

  return BDOS(CPM_SUID, 0x00FF);
}

// SetUserArea: Set current user area (0..15).
//
// Parameters:
//   userNo - User area number (0..15).
void SetUserArea(uint8_t userNo) {

  BDOS(CPM_SUID, (uint16_t)userNo);
}

// FileExists: Test whether a single named file exists.
//
// Parameters:
//   drive    - Drive code in FCB form (0 = current, 1 = A:, 2 = B:, ...).
//   filename - "NAME.EXT" form. Wildcards work but the result is "any
//              match found", not a count.
//
// Returns:
//   true if at least one match exists, false otherwise.
//
// Side effects:
//   Modifies srcFCB and the BDOS DMA buffer (copyBuf is set as DMA).
bool FileExists(uint8_t drive, uint8_t *filename) {

  // Make sure the find-first uses our known DMA buffer.
  BDOS(CPM_SDMA, (uint16_t)&copyBuf);

  CreateFcb(&srcFCB, drive, filename);

  return BDOS(CPM_FFST, (uint16_t)&srcFCB) != 0xff;
}

// OpenFile: Open an existing file for sequential or random access.
//
// Parameters:
//   fc       - Caller-allocated FCB. Will be populated by CreateFcb and
//              must remain valid for the life of the open file.
//   drive    - Drive code (0 = current, 1 = A:, ...).
//   filename - "NAME.EXT" string (uppercased in place by CreateFcb).
//
// Returns:
//   true if opened (BDOS returned anything other than 0xFF),
//   false if file not found.
bool OpenFile(struct FCB *fc, uint8_t drive, uint8_t *filename) {

  CreateFcb(fc, drive, filename);

  return BDOS(CPM_OPN, (uint16_t)fc) != 0xff;
}

// CloseFile: Close a previously opened FCB and flush any directory updates.
//
// Parameters:
//   fc - The FCB previously passed to OpenFile or CreateFile.
//
// Returns:
//   true on success, false on error.
bool CloseFile(struct FCB *fc) {

  return BDOS(CPM_CLS, (uint16_t)fc) != 0xff;
}

// CreateFile: Create a new file (truncating any existing file of the same name
// requires a prior DeleteFile).
//
// Parameters:
//   fc       - Caller-allocated FCB; populated by CreateFcb and must
//              remain valid until CloseFile.
//   drive    - Drive code.
//   filename - "NAME.EXT" string. No wildcards.
//
// Returns:
//   true if file created and ready for writing,
//   false on directory full or other error.
bool CreateFile(struct FCB *fc, uint8_t drive, uint8_t *filename) {

  CreateFcb(fc, drive, filename);

  return BDOS(CPM_MAKE, (uint16_t)fc) != 0xff;
}

// DeleteFile: Delete one or more files. Wildcards in the filename are honored
// by BDOS (e.g. "*.BAK" deletes every .BAK on the drive).
//
// Parameters:
//   drive    - Drive code.
//   filename - "NAME.EXT" or wildcard pattern.
//
// Returns:
//   true if at least one file was deleted, false otherwise.
//
// Side effects:
//   Modifies the module-level srcFCB.
bool DeleteFile(uint8_t drive, uint8_t *filename) {

  CreateFcb(&srcFCB, drive, filename);

  return BDOS(CPM_DEL, (uint16_t)&srcFCB) != 0xff;
}

// RenameFile: Rename a file from oldName to newName on the given drive.
//
// Parameters:
//   drive   - Drive code.
//   oldName - Existing filename ("NAME.EXT").
//   newName - Desired new filename ("NAME.EXT").
//
// Returns:
//   true on success, false if the source was not found or the rename
//   was rejected (e.g. destination already exists).
//
// Notes:
//   BDOS rename uses a 32-byte argument: source FCB at offset 0,
//   destination FCB at offset 16. We populate srcFCB (full 36-byte FCB)
//   and copy the destFCB's drive+name+ext into srcFCB's second half.
bool RenameFile(uint8_t drive, uint8_t *oldName, uint8_t *newName) {

  // Build source FCB.
  CreateFcb(&srcFCB, drive, oldName);

  // Build a temporary FCB for the new name.
  CreateFcb(&destFCB, drive, newName);

  // Copy destFCB's drive + name + ext (16 bytes) into the second half of
  // srcFCB so BDOS sees a contiguous source/destination pair.
  memcpy(((uint8_t *)&srcFCB) + 16, &destFCB, 16);

  return BDOS(CPM_REN, (uint16_t)&srcFCB) != 0xff;
}

// ReadRecord: Read the next 128-byte sequential record into the DMA buffer.
//
// Parameters:
//   fc - Open FCB.
//
// Returns:
//   0   = success (DMA buffer filled, FCB advanced).
//   1   = end of file.
//   >1  = read error (see CP/M docs for specific code).
//
// Notes:
//   The caller should set the DMA address (SetDMAAddress) before calling
//   if data should land somewhere other than the last DMA buffer used.
uint8_t ReadRecord(struct FCB *fc) {

  return BDOS(CPM_READ, (uint16_t)fc);
}

// WriteRecord: Write the next 128-byte sequential record from the DMA buffer.
//
// Parameters:
//   fc - Open FCB.
//
// Returns:
//   0   = success.
//   >0  = error (disk full, directory full, etc.).
uint8_t WriteRecord(struct FCB *fc) {

  return BDOS(CPM_WRIT, (uint16_t)fc);
}

// ReadRandomRecord: Read a specific 128-byte record by index.
//
// Parameters:
//   fc       - Open FCB.
//   recordNo - 0-based record number (only the low 24 bits are used).
//
// Returns:
//   0 = success, non-zero = error
//   (1 = unwritten data, 4 = past EOF, 6 = record beyond addressable range).
uint8_t ReadRandomRecord(struct FCB *fc, uint32_t recordNo) {

  // Pack the 24-bit record number into the FCB's random-record field.
  fc->ranrec[0] = (uint8_t)(recordNo & 0xff);
  fc->ranrec[1] = (uint8_t)((recordNo >> 8) & 0xff);
  fc->ranrec[2] = (uint8_t)((recordNo >> 16) & 0xff);

  return BDOS(CPM_RRAN, (uint16_t)fc);
}

// WriteRandomRecord: Write a specific 128-byte record by index.
//
// Parameters:
//   fc       - Open FCB.
//   recordNo - 0-based record number (only the low 24 bits are used).
//
// Returns:
//   0 = success, non-zero = error.
uint8_t WriteRandomRecord(struct FCB *fc, uint32_t recordNo) {

  // Pack the 24-bit record number into the FCB's random-record field.
  fc->ranrec[0] = (uint8_t)(recordNo & 0xff);
  fc->ranrec[1] = (uint8_t)((recordNo >> 8) & 0xff);
  fc->ranrec[2] = (uint8_t)((recordNo >> 16) & 0xff);

  return BDOS(CPM_WRAN, (uint16_t)fc);
}

// ComputeFileSize: Compute the size of a file in 128-byte records.
//
// Parameters:
//   fc - FCB built via CreateFcb. The file does NOT need to be open;
//        BDOS function 35 examines the directory.
//
// Returns:
//   Number of 128-byte records, or 0 on failure.
//
// Notes:
//   BDOS writes the size into fc->ranrec as a 24-bit little-endian
//   record count (the next record number that would be allocated).
uint32_t ComputeFileSize(struct FCB *fc) {

  if (BDOS(CPM_CFS, (uint16_t)fc) == 0xff)
    return 0;

  return ((uint32_t)fc->ranrec[0])
       | (((uint32_t)fc->ranrec[1]) << 8)
       | (((uint32_t)fc->ranrec[2]) << 16);
}

// GetFileSizeBytes: Convenience wrapper - returns file size in bytes.
//
// Parameters:
//   drive    - Drive code (0 = current, 1 = A:, ...).
//   filename - "NAME.EXT" string. No wildcards.
//
// Returns:
//   Size in bytes (record count * 128), or 0 if the file is missing.
//
// Notes:
//   Allocates a temporary FCB on the stack, so the module-level srcFCB
//   is not disturbed.
uint32_t GetFileSizeBytes(uint8_t drive, uint8_t *filename) {

  struct FCB tmp;

  CreateFcb(&tmp, drive, filename);

  return ComputeFileSize(&tmp) * 128UL;
}

