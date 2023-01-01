# NABU-LIB

For HomeBrew HTTPS://NABU.CA RetroNet of the NABU Persoal Computer Preservation Project. This repo is a z88dk C library for the NABU PC (personal computer). It includes efficient C functions for video, audio, hcca, keyboard, and more for the NABU Computer. This is an ongoing project, so follow this repo for additions, changes and bug fixes

This library is optimized to be as efficient as possible, including text mode double buffer, special function registers, inlining, and fastcall where appropriate. This project aims to build a library for homebrew development of the NABU PC with the easiest and fastest toolset. 

To use this library, you can follow the tutorial here: https://nabu.ca/homebrew-c-tutorial

## v2023.01.01.00
Happy New Year! The update to nabu-lib includes a much faster interrupt for handling RX buffer. There is also a larger RX buffer, which the size can be changed if you don't need it. With this RX interrupt change, the real NABU PC can receice data at full 111kb/s speed. That means the Internet Adapter no longer has a TX delay value for Serial connections. However, MAME still requires a TX delay because it will miss characters if they are transmitted too quickly. 

## v2022.12.31.00
Added support for Internet Adapter (2022.12.31.00) remote file storage functions. This allows HomeBrew apps to read/write files from the Internet Adapter. It includes HTTP, FTP, HTTPS, and any file name with drives/directories. Check the NABU-LIB.h file for details about each function. There is also an example project named "HTTP Get" that you can try out. It demonstrates HTTP get, clearing file contents, writing files, inserting into files, replacing contents in a file, and more. 

## v2022.12.26.01
Added a new function for keyboard input called readLine(). Also added an example on how to use it (Example - User Input and Print)

## v2022.12.26.00

This update includes http get features and requires the most recent Internet Adapter from https://nabu.ca. Now NABU HomeBrew applications can query data on the internet and the Internet Adapter will cache it. When the NABU application needs the data, it can request it from the Internet Adapter. Up to 256 queries per application can be stored in the IA for each NABU HomeBrew program. This can include additional code, music, images, etc.. Essentially, this gives a NABU a practically unlimited amount of storage space because it can use the cloud for obtaining resources. 
