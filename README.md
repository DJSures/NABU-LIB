# NABU-LIB

For HomeBrew HTTPS://NABU.CA RetroNet of the NABU Persoal Computer Preservation Project. This repo is a z88dk C library for the NABU PC (personal computer). It includes efficient C functions for video, audio, hcca, keyboard, and more for the NABU Computer. This is an ongoing project, so follow this repo for additions, changes and bug fixes

This library is optimized to be as efficient as possible, including text mode double buffer, special function registers, inlining, and fastcall where appropriate. This project aims to build a library for homebrew development of the NABU PC with the easiest and fastest toolset. 

To use this library, you can follow the tutorial here: https://nabu.ca/homebrew-c-tutorial

## v2023.01.23.00
Includes VT52 commands for the CPM VT52 terminal emulation. There's also a VT52 and CPM Cursor project added.

## v2023.01.21.00
Update includes the check in the HCCA TX for the interrupt flag. Also a few other fixes and restructures. A fix for not disabling the COAX output on the control port in initNABULib()

## v2023.01.18.00
Includes options for enabling and disabling VDP, HCCA, Keyboard, and others. There is also a way to specify if the program you are creating is CPM or HOMEBREW. Read the NABU-LIB.h file for description of settings.

## v2023.01.15.00
Updated NABU-LIB to have a initNABULib() function that will be called at the start of the program. It does a bunch of configuring, such as disabling the ROM, enabling HCCA and Keyboard interrupts, and audio.

## v2023.01.09.00
Minor change in that the NABU-LIB and Retro File System stuff has been seperated into files. This is because I'm working on a new NABU-LIB-CPM, that will include the remote file system. There is also a chnage to the rn_handleFileRead() because it now returns the number of bytes read. That makes it operate more like standard C streams. In addition, I have added rn_handleFileReadSeq() for sequential reading, and a related rn_handleSeek() to seek through the file. These changes require the latest Internet Adapter 2023.01.09.00

## v2023.01.04.00
Ability to choose your own font per application, rather than being LIB specific. Before the #include "nabu-lib.h" in your program, #define the font. Look at any of the examples in this github repo to see how it's done. And if you want to load a font programmatically while the program is running, there is a vdp_setTextModeFont() as well.

## v2023.01.03.00
Updated with additional file functions for listing directories, files, searching with wildcards and more. Use the HTTP Get Test project to see it in action.

## v2023.01.01.00
Happy New Year! The update to nabu-lib includes a much faster interrupt for handling RX buffer. There is also a larger RX buffer, which the size can be changed if you don't need it. With this RX interrupt change, the real NABU PC can receice data at full 111kb/s speed. That means the Internet Adapter no longer has a TX delay value for Serial connections. However, MAME still requires a TX delay because it will miss characters if they are transmitted too quickly. 

To see the new RX interrupt in action, use the RetroNET Telnet Client from the NABU Channels, or compile it from source here.

## v2022.12.31.00
Added support for Internet Adapter (2022.12.31.00) remote file storage functions. This allows HomeBrew apps to read/write files from the Internet Adapter. It includes HTTP, FTP, HTTPS, and any file name with drives/directories. Check the NABU-LIB.h file for details about each function. There is also an example project named "HTTP Get" that you can try out. It demonstrates HTTP get, clearing file contents, writing files, inserting into files, replacing contents in a file, and more. 

## v2022.12.26.01
Added a new function for keyboard input called readLine(). Also added an example on how to use it (Example - User Input and Print)

## v2022.12.26.00

This update includes http get features and requires the most recent Internet Adapter from https://nabu.ca. Now NABU HomeBrew applications can query data on the internet and the Internet Adapter will cache it. When the NABU application needs the data, it can request it from the Internet Adapter. Up to 256 queries per application can be stored in the IA for each NABU HomeBrew program. This can include additional code, music, images, etc.. Essentially, this gives a NABU a practically unlimited amount of storage space because it can use the cloud for obtaining resources. 
