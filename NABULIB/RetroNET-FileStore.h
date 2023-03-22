// ****************************************************************************************
// NABU-LIB C Library - MAKE SOMETHING
// DJ Sures (c) 2023 
// https://nabu.ca
// 
// Last updated on March 22, 2023 (v2023.03.22.00)
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
// Note: All files stored on the IA Server will be in UPPERCASE
// **************************************************************************
int32_t rn_fileSize(uint8_t filenameLen, uint8_t* filename);

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
// Read data from the specified filename.
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



// **************************************************************************************************************
//
// TCP
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
