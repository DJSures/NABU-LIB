// ****************************************************************************************
// NABU-LIB C Library
// DJ Sures (c) 2023
// https://nabu.ca
// 
// Last updated on Feb 11, 2023 (v2023.02.11.00)
// 
// Get latest copy from: https://github.com/DJSures/NABU-LIB
// 
// This is a z88dk C library for the NABU Personal Computer. This is a large library
// for remote filesystem functions. For example, this is how Cloud CP/M talks to the
// cloud disk images or local file system on the IA.
// 
// Read the summaries below for details of each function in this file.
// 
// **********************************************************************************************


#ifndef RETRONET_TCP_CLIENT_H
#define RETRONET_TCP_CLIENT_H

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

// Stuff
// -----
uint8_t _rn_TCPTmpOriginalInterrupt = 0;

/// <summary>
/// Connects to the host and returns a file handle that will be used for all file functions.
/// 
/// 1) Specify a hostname and receive an automatic file handle. For example...
///    uint8_t fileHandle = rn_requestStoreOpenFile(10, "myFile.txt", OPEN_FILE_FLAG_READWRITE, 0xff);
///   
/// 2) Specify a hostname and specify your a handle. For example...
///    uint8_t fileHandle = rn_requestStoreOpenFile(22, "a:\\personal\\myFile.txt", OPEN_FILE_FLAG_READWRITE, 0xff);
/// 
/// When you are done with the connection, you must Close the connection to release the handle with with rn_tcpClose();
/// If the NABU is reset and the program therefore cannot close the connection, the Internet Adapter will close all connections
/// when an INIT command is received.
/// 
/// - hostnameLen is the length of the filename to open
/// - hostname is a pointer to the filename. All files stored on the IA Server will be in UPPERCASE
/// - port is the TCP port on the host to connect to
/// - fileHandle can be a handle that you specify or 0xff (255) for the server to assign one for you
/// 
/// Returns a handle to the connection for other TCP functions -or- returns 0xff if connection could not be established
/// </summary>
uint8_t rn_TCPOpen(uint8_t hostnameLen, uint8_t* hostname, uint16_t port, uint8_t tcpHandle);

/// <summary>
/// Closes and releases the connection with the specified Handle. The handle can be used again for another
/// connection once it has been released.
/// 
/// When you are done with the connection, you must Close the handle to release the handle with with rn_TCPCloseFile();
/// If the NABU is reset and the program therefore cannot close the connection, the Internet Adapter will close all connections
/// when an INIT command is received.
/// </summary>
void rn_TCPHandleClose(uint8_t tcpHandle);

/// <summary>
/// Get the length of readable data for the TCP handle.
/// A -1 will be returned if there is no connection
/// </summary>
int32_t rn_TCPHandleSize(uint8_t tcpHandle);

/// <summary>
/// Read data from the specified tcp client
/// 
/// - Handle is the obtained by rn_TCPOpen()
/// - buffer is a pointer to a buffer that the data will be written to.
/// - bufferOffset is the offset within the buffer where the data will be written. Use 0 if you're
///   writing to the beginning of the buffer, for example.
/// - readLength is the amount of data that you will be reading.
/// 
/// Returns: the number of bytes read or -1 if client is disconnected
/// </summary>
int32_t rn_TCPHandleRead(uint8_t tcpHandle, uint8_t* buffer, uint16_t bufferOffset, uint16_t readLength);

/// <summary>
/// Write data to the host with the handle. Returns the number of bytes written, or 0 if disconnected.
/// 
/// - tcpHandle is the obtained by rn_TCPOpen()
/// - dataOffset is the offset of the data that will be written
/// - dataLen is the length of data that will be written 
/// - data is a pointer to the data
///
/// Returns: the number of bytes written or -1 if client is disconnected
/// </summary>
int32_t rn_TCPHandleWrite(uint8_t fileHandle, uint16_t dataOffset, uint16_t dataLen, uint8_t* data);

#include "RetroNET-TCPClient.c"

#endif
