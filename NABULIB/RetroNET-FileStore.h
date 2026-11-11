// ****************************************************************************************
// NABU-LIB C Library - MAKE SOMETHING
// DJ Sures (c) 2024
// https://nabu.ca
// 
// Last updated on 2024.03.31
// 
// Get latest copy from: https://github.com/DJSures/NABU-LIB
// 
// This is a z88dk C library for the NABU Personal Computer. This is a large library
// for remote filesystem, TCP and IA Control functions. For example, this is how Cloud CP/M talks to the
// cloud disk images or local file system on the IA.
// 
// Read the summaries below for details of each function in this file.
// 
// Make something,
// DJ Sures
// **********************************************************************************************

// **********************************************************************************************
// FUNCTION INDEX
// --------------
// A quick reference for every function in this file. Scroll down to a function for its
// full description.
//
// Conventions used by most functions:
//   - Strings are passed as a length plus a pointer (filenameLen, filename). They do not
//     need a NUL terminator.
//   - bufferOffset / dataOffset are offsets into YOUR buffer, not into the file.
//   - Files live in the RetroNET Storage folder set in the adapter's settings, and their
//     names are stored in UPPERCASE. Backslash (\) separates directories.
//   - Handles (files, TCP, shell) are released when the adapter receives an INIT, e.g.
//     after the NABU resets, but you should still close them.
//
// FILES BY NAME
//   uint8_t  rn_fileOpen(uint8_t filenameLen, uint8_t *filename, uint16_t fileFlag,
//                        uint8_t fileHandle)
//              Opens a local file (created empty if missing) or an ftp:// http:// https://
//              URL. fileFlag: OPEN_FILE_FLAG_READONLY or OPEN_FILE_FLAG_READWRITE.
//              fileHandle: your own handle, or 0xff to have one assigned. Returns the handle.
//
//   int32_t  rn_fileSize(uint8_t filenameLen, uint8_t *filename)
//              File size, or -1 when it doesn't exist. Does NOT create the file.
//
//   void     rn_fileDetails(uint8_t filenameLen, uint8_t *filename, FileDetailsStruct *s)
//              s (out): size, created / modified date and time, name, IsFile, Exists.
//
//   uint16_t rn_FileRead(uint8_t filenameLen, uint8_t *filename, uint8_t *buffer,
//                        uint16_t bufferOffset, uint32_t readOffset, uint16_t readLength)
//              Reads readLength bytes from file offset readOffset without a handle.
//              Returns bytes read, 0 on error or end of file.
//
//   void     rn_FileReplace(uint8_t filenameLen, uint8_t *filename, uint32_t fileOffset,
//                           uint16_t dataOffset, uint16_t dataLen, int8_t *data)
//              Overwrites dataLen bytes at fileOffset without a handle.
//
//   void     rn_fileDelete(uint8_t filenameLen, uint8_t *filename)
//              Deletes the file. Closes its handle if it has one.
//
//   void     rn_fileHandleCopy(uint8_t srcFilenameLen, uint8_t *srcFilename,
//                              uint8_t destFilenameLen, uint8_t *destFilename,
//                              uint8_t copyMoveFlag)
//              Copies a file. The source may be a URL, the destination must be a local
//              file. copyMoveFlag: COPY_MOVE_FLAG_NO_REPLACE or COPY_MOVE_FLAG_YES_REPLACE.
//
//   void     rn_fileHandleMove(uint8_t srcFilenameLen, uint8_t *srcFilename,
//                              uint8_t destFilenameLen, uint8_t *destFilename,
//                              uint8_t copyMoveFlag)
//              Moves / renames a file. Close its handle first. copyMoveFlag as above.
//
// FILES BY HANDLE (handle from rn_fileOpen())
//   void     rn_fileHandleClose(uint8_t fileHandle)
//              Closes the file and frees the handle.
//
//   int32_t  rn_fileHandleSize(uint8_t fileHandle)
//              File size. -1 only for a URL that could not be downloaded.
//
//   void     rn_fileHandleDetails(int8_t fileHandle, FileDetailsStruct *s)
//              s (out): same details as rn_fileDetails().
//
//   uint16_t rn_fileHandleRead(uint8_t fileHandle, uint8_t *buffer, uint16_t bufferOffset,
//                              uint32_t readOffset, uint16_t readLength)
//              Reads readLength bytes from file offset readOffset.
//              Returns bytes read, 0 on error or end of file.
//
//   uint16_t rn_fileHandleReadSeq(uint8_t fileHandle, uint8_t *buffer, uint16_t bufferOffset,
//                                 uint16_t readLength)
//              Reads from the current position and moves it forward. 0 at end of file.
//
//   int32_t  rn_fileHandleSeek(uint8_t fileHandle, int32_t offset, uint8_t seekOption)
//              Moves the rn_fileHandleReadSeq() position. seekOption: RN_SEEK_SET,
//              RN_SEEK_CUR or RN_SEEK_END. Returns the new position, kept inside the file.
//
//   void     rn_fileHandleAppend(uint8_t fileHandle, uint16_t dataOffset, uint16_t dataLen,
//                                uint8_t *data)
//              Adds dataLen bytes to the end of the file.
//
//   void     rn_fileHandleInsert(uint8_t fileHandle, uint32_t fileOffset, uint16_t dataOffset,
//                                uint16_t dataLen, uint8_t *data)
//              Inserts dataLen bytes at fileOffset, shifting the rest of the file along.
//
//   void     rn_fileHandleReplace(uint8_t fileHandle, uint32_t fileOffset, uint16_t dataOffset,
//                                 uint16_t dataLen, uint8_t *data)
//              Overwrites dataLen bytes at fileOffset.
//
//   void     rn_fileHandleDeleteRange(uint8_t fileHandle, uint32_t fileOffset, uint16_t deleteLen)
//              Removes deleteLen bytes at fileOffset.
//
//   void     rn_fileHandleEmptyFile(uint8_t fileHandle)
//              Removes all content, leaving a 0 byte file.
//
//   uint16_t rn_fileHandleTruncate(uint8_t fileHandle, uint16_t truncateLength,
//                                  uint8_t truncateFlag)
//              Resizes the file. truncateFlag: TRUNCATE_FLAG_SET_LENGTH (new size) or
//              TRUNCATE_FLAG_REMOVE_FROM_END (bytes to cut). Returns the new length
//              modulo 64K, 0 on error.
//
//   uint16_t rn_fileHandleLineCount(uint8_t fileHandle)
//              Number of text lines in the file, for rn_fileHandleGetLine().
//
//   uint16_t rn_fileHandleGetLine(uint8_t fileHandle, uint16_t lineNumber, uint8_t *buffer)
//              Copies one text line into buffer. Returns its length. The line is not NUL
//              terminated and buffer must be big enough for the longest line.
//
// DIRECTORIES AND LISTING
//   uint16_t rn_fileList(uint8_t pathLen, uint8_t *path, uint8_t wildcardLen,
//                        uint8_t *wildcard, uint8_t fileListFlags)
//              Lists path matching wildcard (e.g. "*.*") into a list on the adapter.
//              fileListFlags: FILE_LIST_FLAG_INCLUDE_FILES and / or _DIRECTORIES.
//              Returns the count, 0 when nothing matched.
//
//   void     rn_fileListItem(uint16_t fileItemIndex, FileDetailsStruct *s)
//              s (out): entry 0..count-1 from the last rn_fileList().
//
//   uint8_t  rn_DirectoryCreate(uint8_t directoryNameLen, uint8_t *directoryName)
//              Creates a directory (subdirectories allowed). Returns the adapter's result.
//
//   uint8_t  rn_DirectoryDelete(uint8_t directoryNameLen, uint8_t *directoryName)
//              Deletes a directory. Files and directories inside it are not deleted.
//
//   uint8_t  rn_directoryExists(uint8_t directoryNameLen, uint8_t *directoryName)
//              1 when the directory exists, 0 when it doesn't.
//
// TCP CLIENT
//   uint8_t  rn_TCPOpen(uint8_t hostnameLen, uint8_t *hostname, uint16_t port,
//                       uint8_t tcpHandle)
//              Connects to hostname:port. tcpHandle: your own handle, or 0xff to have one
//              assigned. Returns the handle, or 0xff when the connection failed.
//
//   void     rn_TCPHandleClose(uint8_t tcpHandle)
//              Closes the connection and frees the handle.
//
//   int32_t  rn_TCPHandleSize(uint8_t tcpHandle)
//              Bytes waiting to be read, -1 when not connected.
//
//   int32_t  rn_TCPHandleRead(uint8_t tcpHandle, uint8_t *buffer, uint16_t bufferOffset,
//                             uint16_t readLength)
//              Reads up to readLength bytes. Returns bytes read, -1 when disconnected.
//
//   int32_t  rn_TCPHandleWrite(uint8_t tcpHandle, uint16_t dataOffset, uint16_t dataLen,
//                              uint8_t *data)
//              Sends dataLen bytes. Returns bytes written, -1 when disconnected.
//
// TCP SERVER (the adapter's TCP server, set up in its settings)
//   uint8_t  rn_TCPServerClientCnt()
//              Number of clients connected.
//
//   uint8_t  rn_TCPServerAvailable()
//              Bytes waiting to be read.
//
//   uint8_t  rn_TCPServerRead(uint8_t *buffer, uint16_t bufferOffset, uint8_t readLength)
//              Reads up to readLength bytes. Returns bytes read.
//
//   void     rn_TCPServerWrite(uint16_t dataOffset, uint8_t dataLen, uint8_t *data)
//              Sends dataLen bytes to every connected client.
//
// HOST SHELL (needs "Enable RetroNET host shell" turned on in the adapter's settings)
//   uint8_t  rn_ShellOpen()
//              Starts a shell on the adapter's computer. Returns a handle, or 0xff when
//              refused (setting off, or no free handles).
//
//   void     rn_ShellClose(uint8_t shellHandle)
//              Ends the shell.
//
//   int32_t  rn_ShellAvailable(uint8_t shellHandle)
//              Output bytes waiting, -1 when the handle is unknown or the shell exited
//              and all its output was read.
//
//   int32_t  rn_ShellRead(uint8_t shellHandle, uint8_t *buffer, uint16_t bufferOffset,
//                         uint16_t readLength)
//              Reads shell output. Returns bytes read, 0 when none yet, -1 unknown handle.
//
//   int32_t  rn_ShellWrite(uint8_t shellHandle, uint16_t dataOffset, uint16_t dataLen,
//                          uint8_t *data)
//              Types into the shell. End a command with "\r\n" to run it.
//              Returns bytes written, -1 when the handle is unknown or the shell exited.
//
// NETWORK TOOLS
//   int32_t  rn_PingIP(uint8_t ipLen, uint8_t *ipStr)
//              Pings an IP or hostname. Returns the round-trip time in milliseconds,
//              or a negative PING_ERR_* code.
//
//   uint8_t  rn_DnsLookup(uint8_t hostLen, uint8_t *host, uint8_t *outBuffer)
//              outBuffer (256 bytes): the IPv4 address as text, or a readable error.
//              NUL terminated. Returns its length.
//
// CP/M DISK SPACE
//   int64_t  rn_GetFreeDiskSpace()
//              Bytes free on the host drive holding the RetroNET storage folder,
//              -1 on error.
//
//   int64_t  rn_GetUsedDiskSpace()
//              Bytes used by the CP/M drives (the storage folder's CPM folder),
//              -1 on error.
//
// PRINTER
//   void     rn_Printer(uint8_t c)
//              Writes one character to LST.TXT in the adapter's storage folder.
//
//   void     rn_PunchOut(uint8_t c)
//              Writes one character to PUNCH.TXT in the adapter's storage folder.
// **********************************************************************************************


#ifndef RETRONET_FILE_STORE_H
#define RETRONET_FILE_STORE_H

#include <stdbool.h>
#include <stdlib.h>
#include "NABU-LIB.h"

// **************************************************************************
// RetroNET
// ------------
// 
// These are methods specific to RetroNet and therefore require the IA 
// (internet adapter) to be connected to the NABU.
// 
// The Internet Adapter will hold the data and the NABU can request it. The
// data is stored in the IA under specified Request Store IDs. That way, a
// program can have many requests stored in the IA as resources. The program
// can store items from the web on the IA, such as images, music arrangements,
// or even code. Code can be copied from the IA request store and written
// to RAM to be executed.
// 
// This allows a NABU program the ability to have a practically unlimited 
// amount of RAM or storage by using the cloud and Internet Adapter.
// 
// **************************************************************************

// **************************************************************************
// These are options for the rn_fileOpen() fileFlag function
// **************************************************************************
#define OPEN_FILE_FLAG_READONLY 0b00000000
#define OPEN_FILE_FLAG_READWRITE 0b00000001

// **************************************************************************
// These are options for the rn_fileHandleCopy() and rn_fileHandleMove()
// **************************************************************************
#define COPY_MOVE_FLAG_NO_REPLACE 0b00000000
#define COPY_MOVE_FLAG_YES_REPLACE 0b00000001

// **************************************************************************
// These are options for the rn_fileList()
// **************************************************************************
#define FILE_LIST_FLAG_INCLUDE_FILES 0b00000001 
#define FILE_LIST_FLAG_INCLUDE_DIRECTORIES 0b00000010 

// **************************************************************************
// These are options for the rn_fileHandleSeed()
// RN_SEEK_SET seek from the beginning of the file
// RN_SEEK_CUR offset from the current position
// RN_SEEK_END offset from the end of the file
// **************************************************************************
#define RN_SEEK_SET 1
#define RN_SEEK_CUR 2
#define RN_SEEK_END 3

// **************************************************************************
// These are options for the rn_fileHandleTruncate()
// TRUNCATE_FLAG_SET_LENGTH      truncateLength is the new absolute file size (extending zero-fills)
// TRUNCATE_FLAG_REMOVE_FROM_END remove truncateLength bytes from the end
// **************************************************************************
#define TRUNCATE_FLAG_SET_LENGTH 0
#define TRUNCATE_FLAG_REMOVE_FROM_END 1


// **************************************************************************
// Ping Error Respones
// (parenthesized and without the trailing semicolon they used to have, so they
// can be used in comparisons and as case labels)
// **************************************************************************
// catch-all / unknown
#define PING_ERR_GENERIC  (-1)
// IPStatus.TimedOut
#define PING_ERR_TIMEOUT  (-2)
// hostname could not be resolved
#define PING_ERR_DNS  (-3)
// malformed ip / empty string
#define PING_ERR_BAD_ADDRESS  (-4)
// DestinationHostUnreachable / NetworkUnreachable / etc.
#define PING_ERR_DEST_UNREACHABLE  (-5)
// TtlExpiredTransit / TtlReassemblyTimeExceeded
#define PING_ERR_TTL_EXPIRED  (-6)
// null reply object
#define PING_ERR_NO_REPLY  (-7)
// local socket / network down
#define PING_ERR_NO_NETWORK  (-8)

// **************************************************************************
// The details of each file
// **************************************************************************
typedef struct {

  int32_t FileSize;

  uint16_t CreatedYear;
  uint8_t CreatedMonth;
  uint8_t CreatedDay;
  uint8_t CreatedHour;
  uint8_t CreatedMinute;
  uint8_t CreatedSecond;

  uint16_t ModifiedYear;
  uint8_t ModifiedMonth;
  uint8_t ModifiedDay;
  uint8_t ModifiedHour;
  uint8_t ModifiedMinute;
  uint8_t ModifiedSecond;

  uint8_t FilenameLen;
  uint8_t Filename[64];

  bool IsFile;
  bool Exists;

} FileDetailsStruct;


// **************************************************************************************************************
//
// FILES & DIRECTORIES
// -------------------
//
// *NOTE: All files stored on the IA Server will be in UPPERCASE
//
// **************************************************************************************************************


// **************************************************************************
// Opens the file and returns a file handle that will be used for all file functions.
// Files are stored in the RetroNET Storage folder that is defined in the Internet Adapter settings. 
// 
// The fileFlag is managed on the IA and if set for readonly, any write commands will
// be ignored. This ensures the nabulib has not been tampered with to accidentally
// overwrite readonly files.
// 
// *Note: that FTP, HTTP, and HTTPS write functions are not persistent. If a WRITE function is 
// called on a file handle of one of these file types, the file will be altered, but the changes
// will not be persistent the next time the URL is requested. This because we cannot change files
// on remote servers - they won't give us the key to unlock the universe, yet.
// 
// Multiple file types are available...
// 
// - FTP:   ftp://<host>/<path>/<file>
//          (i.e. ftp://ftp.cdrom.com/pub/filelist.txt)
// 
// - HTTP:  http://<host>/<path>/<file>?<url parameters>
//          (Example: http://cloud.nabu.ca/httpGetQueryResponse.txt?someparam=somevalue)
// 
// - HTTPS: https://<host>/<path>/<file>
//          (Example: https://cloud.nabu.ca/httpGetQueryResponse.txt)
// 
// - File:
//          The "File" type has many options. Directory seperator is backslash (\)
// 
// 1) Specify a filename with no drive or directory. For example...
//    uint8_t fileHandle = rn_requestStoreOpenFile(10, "myFile.txt", OPEN_FILE_FLAG_READWRITE, 0xff);
//   
// 2) Specify a filename with drive or directory. For example...
//    uint8_t fileHandle = rn_requestStoreOpenFile(22, "a:\\personal\\myFile.txt", OPEN_FILE_FLAG_READWRITE, 0xff);
// 
// 3) Specify a filename with directory. For example...
//    uint8_t fileHandle = rn_requestStoreOpenFile(20, "personal\\myFile.txt", OPEN_FILE_FLAG_READWRITE, 0xff);
// 
// *Note: If the file does not exist, it is created with 0 bytes.
// 
// When you are done with the file, you must Close the file to release the handle with with rn_requestStoreCloseFile();
// If the NABU is reset and the program therefore cannot close the file, the Internet Adapter will close all files
// when an INIT command is received.
// 
// - filenameLen is the length of the filename to open
// - filename is a pointer to the filename. All files stored on the IA Server will be in UPPERCASE
// - fileFlag is one of #define OPEN_FILE_FLAG_* types
// - fileHandle can be a file handle that you specify or 0xff (255) for the
//   server to assign one for you
// 
// Returns a handle to the file for other file functions. If fileHandle is 255 (0xff), a file handle is
// automatically returned to you. Otherwise, this function will return the fileHandle parameter that you
// passed to it. If the fileHandle that you passed is already in use, a new file handle will be assigned
// to you and returned. 
// **************************************************************************
uint8_t rn_fileOpen(uint8_t filenameLen, uint8_t* filename, uint16_t fileFlag, uint8_t fileHandle);

// **************************************************************************
// Closes and releases the file with the specified fileHandle. The handle can be used again for another
// file once it has been released.
// 
// When you are done with the file, you must Close the file to release the handle with with rn_requestStoreCloseFile();
// If the NABU is reset and the program therefore cannot close the file, the Internet Adapter will close all files
// when an INIT command is received.
// **************************************************************************
void rn_fileHandleClose(uint8_t fileHandle);

// **************************************************************************
// Get the file size of the specified file, or returns -1 if file does not exist.
// This is different than using rn_fileHandleSize() because that will create an empty file when
// a file handle is assigned. If you want to see if a file exists without creating it first, this
// is the function you would use. 
//
// - filenameLen is the length of the filename to open
// - filename is a pointer to the filename. All files stored on the IA Server will be in UPPERCASE
//
// Note: All files stored on the IA Server will be in UPPERCASE
// **************************************************************************
int32_t rn_fileSize(uint8_t filenameLen, uint8_t* filename);

// **************************************************************************
// Read data from the specified filename without using a file handle.
// 
// - filenameLen is the length of the filename to open
// - filename is a pointer to the filename. All files stored on the IA Server will be in UPPERCASE
// - buffer is a pointer to a buffer that the data will be written to.
// - bufferOffset is the offset within the buffer where the data will be written. Use 0 if you're
//   writing to the beginning of the buffer, for example.
// - readOffset is the offset of the file that you will be reading from. 
// - readLength is the amount of data that you will be reading.
//
// Returns the number of bytes read or 0 if there was an error or reached EOF
// **************************************************************************
uint16_t rn_FileRead(uint8_t filenameLen, uint8_t* filename, uint8_t* buffer, uint16_t bufferOffset, uint32_t readOffset, uint16_t readLength);

// **************************************************************************
// Replace data in a file by overwriting bytes with the data
// Files are stored in the RetroNET Storage folder that is defined in the Internet Adapter settings. 
// The file can contain slashes (\) or (/) to specify directory and drive (i.e. A:)
// 
// - filenameLen is the length of the filename to open
// - filename is a pointer to the filename. All files stored on the IA Server will be in UPPERCASE
// - fileOffset is the offset of the file where the data will be overwritten
// - dataOffset is the offset of the data that will be written
// - dataLen is the length of data that will be written 
// - data is a pointer to the data
// **************************************************************************
void rn_FileReplace(uint8_t filenameLen, uint8_t* filename, uint32_t fileOffset, uint16_t dataOffset, uint16_t dataLen, int8_t* data);

// **************************************************************************
// Get the file size of the specified file handle.
// A file (not url) that is opened with rn_FileOpen() will always have 0 length because the file is
// created with 0 bytes when rn_fileOpen() is called. To check if a file exists, use the rn_fileSize()
// function, as it expects a filename (not a file handle) and therefore does not create the file.
// 
// If a URL is used and the URL was not downloaded, the file size will be -1. You will only ever get
// a -1 from a URL, not a file.
// **************************************************************************
int32_t rn_fileHandleSize(uint8_t fileHandle);

// **************************************************************************
// Get the file details by filename.
// 
// The FileDetailStruct is populated with details about the file. If the file does not exist,
// the FileDetailStruct->Exists will reflect that.
//
// Note: All files stored on the IA Server will be in UPPERCASE
// **************************************************************************
void rn_fileDetails(uint8_t filenameLen, uint8_t* filename, FileDetailsStruct* s);

// **************************************************************************
// Get the file details by file handle.
// 
// The FileDetailStruct is populated with details about the file. If the file does not exist,
// the FileDetailStruct->Exists will reflect that.
// **************************************************************************
void rn_fileHandleDetails(int8_t fileHandle, FileDetailsStruct* s);

// **************************************************************************
// Read data from the specified file handle.
// 
// - fileHandle is the obtained by rn_fileOpen()
// - buffer is a pointer to a buffer that the data will be written to.
// - bufferOffset is the offset within the buffer where the data will be written. Use 0 if you're
//   writing to the beginning of the buffer, for example.
// - readOffset is the offset of the file that you will be reading from. 
// - readLength is the amount of data that you will be reading.
// 
// Returns the number of bytes read or 0 if there was an error or reached EOF
// **************************************************************************
uint16_t rn_fileHandleRead(uint8_t fileHandle, uint8_t* buffer, uint16_t bufferOffset, uint32_t readOffset, uint16_t readLength);

// **************************************************************************
// Append data to the end of the specified file in the filestore. If the file does not exist, 
// the file is created. Files are stored in the RetroNET Storage folder that is defined in 
// the Internet Adapter settings. 
// 
// - fileHandle is the obtained by rn_fileOpen()
// - dataOffset is the offset of the data that will be written
// - dataLen is the length of data that will be written 
// - data is a pointer to the data
// **************************************************************************
void rn_fileHandleAppend(uint8_t fileHandle, uint16_t dataOffset, uint16_t dataLen, uint8_t* data);

// **************************************************************************
// Insert data in the file at the specified offset. This function will shift all data following the
// fileOffset to insert the data.
// 
// - fileHandle is the obtained by rn_fileOpen()
// - fileOffset is the offset of the file where the data will be inserted
// - dataOffset is the offset of the data that will be written
// - dataLen is the length of data that will be written 
// - data is a pointer to the data
// **************************************************************************
void rn_fileHandleInsert(uint8_t fileHandle, uint32_t fileOffset, uint16_t dataOffset, uint16_t dataLen, uint8_t* data);

// **************************************************************************
// Delete range of bytes from within file handle
// 
// - fileHandle is the obtained by rn_fileOpen()
// - fileOffset is the offset of the file where the data will be removed
// - deleteLen is the length of data that will be removed
// **************************************************************************
void rn_fileHandleDeleteRange(uint8_t fileHandle, uint32_t fileOffset, uint16_t deleteLen);

// **************************************************************************
// Delete all the content of the file and leave the file as a 0 byte length
// 
// - fileHandle is the obtained by rn_fileOpen()
// **************************************************************************
void rn_fileHandleEmptyFile(uint8_t fileHandle);

// **************************************************************************
// Replace data in a file by overwriting bytes with the data
// Files are stored in the RetroNET Storage folder that is defined in the Internet Adapter settings. 
// The file can contain slashes (\) or (/) to specify directory and drive (i.e. A:)
// 
// - fileHandle is the obtained by rn_fileOpen()
// - fileOffset is the offset of the file where the data will be overwritten
// - dataOffset is the offset of the data that will be written
// - dataLen is the length of data that will be written 
// - data is a pointer to the data
// **************************************************************************
void rn_fileHandleReplace(uint8_t fileHandle, uint32_t fileOffset, uint16_t dataOffset, uint16_t dataLen, uint8_t* data);

// **************************************************************************
// Delete the physical file from the store. If the file has a handle, it is closed
// 
// - filenameLen is the length of the filename. All files stored on the IA Server will be in UPPERCASE
// - filename is the filename string
// **************************************************************************
void rn_fileDelete(uint8_t filenameLen, uint8_t* filename);

// **************************************************************************
// Copy the src file to the dest file. The source file can be of any type (http, ftp, file) but the
// dest file must be a regular file. This allows copying files from the cloud to the local
// file store.
// 
// - srcFilenameLen is the length of the source file to copy
// - srcFilename is a pointer to the filename of the source file
// - destFilenameLen is the length of the destination filename
// - destFilename is a pointer to the filename of the destionation file
// - copyMoveFlag is one of #define COPY_MOVE_FLAG_* 
// **************************************************************************
void rn_fileHandleCopy(uint8_t srcFilenameLen, uint8_t* srcFilename, uint8_t destFilenameLen, uint8_t* destFilename, uint8_t copyMoveFlag);

// **************************************************************************
// Move the src file to the dest file.
// 
// *Note: if the source file has an open file handle, it is not closed and will continue
//        working with the old filename. It's expected that a friendly
//        programmer would close the file first before moving/renaming it. 
// 
// *Note: All files stored on the IA Server will be in UPPERCASE
//
// - srcFilenameLen is the length of the source file to move
// - srcFilename is a pointer to the filename of the source file
// - destFilenameLen is the length of the destination filename
// - destFilename is a pointer to the filename of the destionation file
// - copyMoveFlag is one of #define COPY_MOVE_FLAG_* 
// **************************************************************************
void rn_fileHandleMove(uint8_t srcFilenameLen, uint8_t* srcFilename, uint8_t destFilenameLen, uint8_t* destFilename, uint8_t copyMoveFlag);

// **************************************************************************
// Returns the number of files within the path, including wildcards.
// Populates an internal list of files on the server that can be viewed with rn_fileListItem().
// 
// *Note: All files and directories stored on the IA Server will be in UPPERCASE
// 
// To get the details of a file, you must do this...
// 
// 1) rn_fileList() will give you the number of files that matches the search criteria. 
//  
// 2) Finally, call rn_fileListItem() to get the details of the file 
//    (including size, created, modified, and filename)
// 
// Examples
// - uint16_t fileCnt =  rn_fileList(1, "\\",  1, "*", FILE_LIST_FLAG_INCLUDE_FILES | FILE_LIST_FLAG_INCLUDE_DIRECTORIES);
// - uint16_t fileCnt =  rn_fileList(1, "\\", 3, "*.*", FILE_LIST_FLAG_INCLUDE_FILES);
// - uint16_t fileCnt =  rn_fileList(8, "c:\\myApp", 3, "*.*", FILE_LIST_FLAG_INCLUDE_FILES);
// - uint16_t fileCnt =  rn_fileList(6, "a:\\cpm", 7, "zo*.cmd", FILE_LIST_FLAG_INCLUDE_FILES);
// 
// - pathLen is the length of the path string
// - path is a pointer to the path string
// - wildcardLen is the length of the wildcard string
// - wildcard is a pointer to the wildcard string
// - fileListFlags is one or many values of #define FILE_LIST_FLAG_*
// 
// Returns the number of matching files. Call rn_fileListItem() from 0 to N
// 0 means no files, no directories
// **************************************************************************
uint16_t rn_fileList(uint8_t pathLen, uint8_t* path, uint8_t wildcardLen, uint8_t* wildcard, uint8_t fileListFlags);

// **************************************************************************
// Populates buffer with the size, created datetime, modified datetime, 
// filename length, and filename of the file at the fileItemIndex.
// **************************************************************************
void rn_fileListItem(uint16_t fileItemIndex, FileDetailsStruct* s);

// **************************************************************************
// Sequentially read data from the specified filename. That means continue reading from the
// last position of the file. The file position can be changed programatically with
// rn_fileHandleSeek() as well.
// 
// - fileHandle is the obtained by rn_fileOpen()
// - buffer is a pointer to a buffer that the data will be written to.
// - bufferOffset is the offset within the buffer where the data will be written. Use 0 if you're
//   writing to the beginning of the buffer, for example.
// - readOffset is the offset of the file that you will be reading from. 
// - readLength is the amount of data that you will be reading.
// 
// Returns the number of bytes read. If this number differs from the readLength parameter, then 
// either an error had occurred or the End Of File was reached. 0 will be returned if the end of
// the file has been reached
// **************************************************************************
uint16_t rn_fileHandleReadSeq(uint8_t fileHandle, uint8_t* buffer, uint16_t bufferOffset, uint16_t readLength);

// **************************************************************************
// Sets the sequential read position within the file for using rn_fileHandleReadSeq()
// 
// - fileHandle is obtained by rn_fileOpen()
// - offset is the positive or negative position to move to. If using RN_SEEK_SET,
//          only positive values are relevant. If using RN_SEEK_CUR, a positive or 
//          negative value can be used to move forward or backward from the current
//          position. If using RN_SEEK_END, only a negatie number can be used because it
//          will be the offset from the end of the file.
// - seekOption is the option to seek. One of these...
//              RN_SEEK_SET seek from the beginning of the file
//              RN_SEEK_CUR offset from the current position
//              RN_SEEK_END offset from the end of the file
// 
// Returns the new position of the file. If the seek is set past the end of the file, the
// end of the file position is returned. The seek function will not let you seek past
// the end of a file, or before a file. That is why we give you this value so you can be sure
// the pointer is always within the file.
// **************************************************************************
int32_t rn_fileHandleSeek(uint8_t fileHandle, int32_t offset, uint8_t seekOption);

// **************************************************************************
// Gets the number of lines in the specified file handle. This is a helper
// function that assists the rn_fileHandleGetLine()
// 
// - fileHandle is obtained by rn_fileOpen()
//
// Returns the number of text lines in the file. Use it as the upper limit for the
// lineNumber you pass to rn_fileHandleGetLine().
// **************************************************************************
uint16_t rn_fileHandleLineCount(uint8_t fileHandle);

// **************************************************************************
// Gets the string at the specified line number of the text file. 
// This assumes you have loaded a text file with the fileHandle. It uses the
// newline character of the of the operating system to split the file into
// lines. If you have a file with 100 text lines, you can
// use this to grab the text on line #10
//
// - fileHandle is obtained by rn_fileOpen()
// 
// Returns the length of the line read
// **************************************************************************
uint16_t rn_fileHandleGetLine(uint8_t fileHandle, uint16_t lineNumber, uint8_t* buffer);

// **************************************************************************
// Resize the file behind a handle.
//
// - fileHandle is obtained by rn_fileOpen()
// - truncateLength is interpreted according to truncateFlag
// - truncateFlag is one of #define TRUNCATE_FLAG_*
//     TRUNCATE_FLAG_SET_LENGTH      file becomes exactly truncateLength bytes
//                                   (extending the file zero-fills the new bytes)
//     TRUNCATE_FLAG_REMOVE_FROM_END new length = max(0, currentLength - truncateLength)
//
// Returns the new file length modulo 64K (cast from int64 on the server). For
// files that may exceed 64KB after truncation, follow up with rn_fileHandleSize()
// to get the true post-truncate size. Returns 0 for unknown handle, READONLY
// handle, or any server-side error.
//
// After truncation the handle's sequential-read cursor is clamped to [0, newLength].
// **************************************************************************
uint16_t rn_fileHandleTruncate(uint8_t fileHandle, uint16_t truncateLength, uint8_t truncateFlag);

// **************************************************************************
/// Creates a directory in the file store. The directory can contain slashes (\) or (/) to specify subdirectories
// **************************************************************************
uint8_t rn_DirectoryCreate(uint8_t directoryNameLen, uint8_t* directoryName);

// **************************************************************************
/// Deletes a directory in the file store. The directory can contain slashes (\) or (/) to specify subdirectories. If the directory contains files or other directories or files, they will not be deleted.
// **************************************************************************
uint8_t rn_DirectoryDelete(uint8_t directoryNameLen, uint8_t* directoryName);

// **************************************************************************
/// Returns 1 if the directory exists in the file store, or 0 if it does not exist. The directory can contain slashes (\) or (/) to specify subdirectories. If the directory contains files or other directories, it will still return 1 because the directory itself exists.
// **************************************************************************
uint8_t rn_directoryExists(uint8_t directoryNameLen, uint8_t* directoryName);

// **************************************************************************************************************
//
// TCP CLIENT
//
// **************************************************************************************************************

// **************************************************************************
// Connects to the host and returns a file handle that will be used for all file functions.
// 
// 1) Specify a hostname and receive an automatic file handle. For example...
//    uint8_t fileHandle = rn_requestStoreOpenFile(10, "myFile.txt", OPEN_FILE_FLAG_READWRITE, 0xff);
//   
// 2) Specify a hostname and specify your a handle. For example...
//    uint8_t fileHandle = rn_requestStoreOpenFile(22, "a:\\personal\\myFile.txt", OPEN_FILE_FLAG_READWRITE, 0xff);
// 
// When you are done with the connection, you must Close the connection to release the handle with with rn_tcpClose();
// If the NABU is reset and the program therefore cannot close the connection, the Internet Adapter will close all connections
// when an INIT command is received.
// 
// - hostnameLen is the length of the filename to open
// - hostname is a pointer to the filename. All files stored on the IA Server will be in UPPERCASE
// - port is the TCP port on the host to connect to
// - fileHandle can be a handle that you specify or 0xff (255) for the server to assign one for you
// 
// Returns a handle to the connection for other TCP functions -or- returns 0xff if connection could not be established
// **************************************************************************
uint8_t rn_TCPOpen(uint8_t hostnameLen, uint8_t* hostname, uint16_t port, uint8_t tcpHandle);

// **************************************************************************
// Closes and releases the connection with the specified Handle. The handle can be used again for another
// connection once it has been released.
// 
// When you are done with the connection, you must Close the handle to release the handle with with rn_TCPCloseFile();
// If the NABU is reset and the program therefore cannot close the connection, the Internet Adapter will close all connections
// when an INIT command is received.
// **************************************************************************
void rn_TCPHandleClose(uint8_t tcpHandle);

// **************************************************************************
// Get the length of readable data for the TCP handle.
// A -1 will be returned if there is no connection
// **************************************************************************
int32_t rn_TCPHandleSize(uint8_t tcpHandle);

// **************************************************************************
// Read data from the specified tcp client
// 
// - Handle is the obtained by rn_TCPOpen()
// - buffer is a pointer to a buffer that the data will be written to.
// - bufferOffset is the offset within the buffer where the data will be written. Use 0 if you're
//   writing to the beginning of the buffer, for example.
// - readLength is the amount of data that you will be reading.
// 
// Returns: the number of bytes read or -1 if client is disconnected
// **************************************************************************
int32_t rn_TCPHandleRead(uint8_t tcpHandle, uint8_t* buffer, uint16_t bufferOffset, uint16_t readLength);

// **************************************************************************
// Write data to the host with the handle. Returns the number of bytes written, or 0 if disconnected.
// 
// - tcpHandle is the obtained by rn_TCPOpen()
// - dataOffset is the offset of the data that will be written
// - dataLen is the length of data that will be written 
// - data is a pointer to the data
//
// Returns: the number of bytes written or -1 if client is disconnected
// **************************************************************************
int32_t rn_TCPHandleWrite(uint8_t tcpHandle, uint16_t dataOffset, uint16_t dataLen, uint8_t* data);


// **************************************************************************************************************
//
// TCP SERVER
//
// **************************************************************************************************************

// **************************************************************************
// Get the number of clients connected to the IA TCP Server
// **************************************************************************
uint8_t rn_TCPServerClientCnt();


// **************************************************************************
// Get the length of readable data on the TCP Server connection.
// **************************************************************************
uint8_t rn_TCPServerAvailable();

// **************************************************************************
// Read data from the specified tcp server connection
// 
// - buffer is a pointer to a buffer that the data will be written to.
// - bufferOffset is the offset within the buffer where the data will be written. Use 0 if you're
//   writing to the beginning of the buffer, for example.
// - readLength is the amount of data that you will be reading.
// 
// Returns: the number of bytes read
// **************************************************************************
uint8_t rn_TCPServerRead(uint8_t* buffer, uint16_t bufferOffset, uint8_t readLength);

// **************************************************************************
// Write data to all clients that are connected to the TCP Server..
// 
// - dataOffset is the offset of the data that will be written
// - dataLen is the length of data that will be written 
// - data is a pointer to the data
//
// **************************************************************************
void rn_TCPServerWrite(uint16_t dataOffset, uint8_t dataLen, uint8_t* data);



// **************************************************************************************************************
//
// HOST SHELL
// ----------
//
// These functions drop a NABU program to a host command shell on the
// Internet Adapter side (cmd.exe on Windows, $SHELL / /bin/sh on Linux/macOS).
// Stdin/stdout/stderr are buffered: write lines into the shell with
// rn_ShellWrite() (caller appends "\r\n" to execute), then drain output
// with rn_ShellAvailable() / rn_ShellRead().
//
// The IA must have "Enable RetroNET host shell" turned on in Settings;
// otherwise rn_ShellOpen() returns 0xff and no shell is launched.
//
// **************************************************************************************************************

// **************************************************************************
// Open a new host shell session.
//
// Returns a handle (0x00..0xfe) for use with the other rn_Shell* functions,
// or 0xff if the IA refused the request (feature disabled in Settings, or
// no free handle slots).
// **************************************************************************
uint8_t rn_ShellOpen();

// **************************************************************************
// Close a previously-opened shell session and tear down the host process.
//
// - shellHandle is obtained by rn_ShellOpen()
//
// Unknown handles are ignored. Any handles still open at INIT time will
// be closed automatically by the IA.
// **************************************************************************
void rn_ShellClose(uint8_t shellHandle);

// **************************************************************************
// Returns the number of bytes waiting in the shell's stdout/stderr buffer
// for the NABU to read, or -1 if the handle is unknown / the shell has
// exited and its buffer has been fully drained.
//
// - shellHandle is obtained by rn_ShellOpen()
// **************************************************************************
int32_t rn_ShellAvailable(uint8_t shellHandle);

// **************************************************************************
// Read up to readLength bytes off the front of the shell's stdout/stderr
// buffer.
//
// - shellHandle is obtained by rn_ShellOpen()
// - buffer is a pointer to a buffer that the data will be written to
// - bufferOffset is the offset within the buffer where data will be written
// - readLength is the maximum number of bytes to read
//
// Returns the number of bytes actually read (may be less than readLength
// when fewer bytes are buffered), 0 if no output is waiting yet, or -1 if
// the handle is unknown.
// **************************************************************************
int32_t rn_ShellRead(uint8_t shellHandle, uint8_t* buffer, uint16_t bufferOffset, uint16_t readLength);

// **************************************************************************
// Write raw bytes to the shell's stdin. The caller is responsible for
// including a CR / LF when the shell should execute the line — bytes are
// piped through verbatim, exactly as if a human were typing them.
//
// - shellHandle is obtained by rn_ShellOpen()
// - dataOffset is the offset of the data that will be written
// - dataLen is the length of data that will be written
// - data is a pointer to the data
//
// Returns the number of bytes written, or -1 if the handle is unknown or
// the shell has exited.
// **************************************************************************
int32_t rn_ShellWrite(uint8_t shellHandle, uint16_t dataOffset, uint16_t dataLen, uint8_t* data);


// **************************************************************************************************************
//
// Pings a host from the Internet Adapter.
//
// - ipLen is the length of the address string
// - ipStr is a pointer to the address: an IP such as "8.8.8.8" or a hostname
//
// Returns the round-trip time in milliseconds (0 or more) when the host replied, or a
// negative PING_ERR_* code as defined above when it did not.
//
// **************************************************************************************************************
int32_t rn_PingIP(uint8_t ipLen, uint8_t* ipStr);



// **************************************************************************
// Resolves a hostname to an IPv4 address. Writes a NUL-terminated ASCII
// string into outBuffer. On success the string is the resolved IP. On
// failure the string is a human-readable error description ("Host not
// found", "Network unreachable: ...", etc.) so the caller can show
// something useful. The response is always non-empty.
//
// The IA wire response is length-prefixed: a single uint8_t length byte
// followed by that many ASCII bytes. This routine reads both, NUL-terminates
// the buffer, and returns the payload length. outBuffer must have room for
// at least 256 bytes (255 max payload + NUL).
//
// Returns:
//    >0  payload length (success or descriptive failure string)
//     0  empty response (should not happen per protocol)
//
// Note: this is a separate IA sub-command from rn_PingIP; it shares the
// ICMP-resolver path on the IA side which is why it lives next to ping.
// **************************************************************************
uint8_t rn_DnsLookup(uint8_t hostLen, uint8_t* host, uint8_t* outBuffer);

// **************************************************************************
// Returns the available disk space, in bytes, on the host drive that holds the
// Internet Adapter's RetroNET storage folder (where the CPM drives live)
// -1 means error, e.g. the storage folder is missing
// **************************************************************************
int64_t rn_GetFreeDiskSpace();

// **************************************************************************
// Returns the used disk space, in bytes, of the CPM drives: the total size of
// every file under the CPM folder inside the RetroNET storage folder.
// 0 when the CPM folder doesn't exist yet
// -1 means error
// **************************************************************************
int64_t rn_GetUsedDiskSpace();

// **************************************************************************************************************
//
// PRINTER
//
// **************************************************************************************************************

// **************************************************************************
// PRINTER
// -------
//
// These functions are for printing to a file on the host PC
// **************************************************************************

// **************************************************************************
// Write to the LST.TXT file on the internet adapter's storage folder
// **************************************************************************
void rn_Printer(uint8_t c);

// **************************************************************************
// Write to the PUNCH.TXT file on the internet adapter's storage folder
// **************************************************************************
void rn_PunchOut(uint8_t c);


#include "RetroNET-FileStore.c"

#endif
