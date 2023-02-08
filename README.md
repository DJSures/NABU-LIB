# NABU-LIB

For HomeBrew HTTPS://NABU.CA RetroNet of the NABU Persoal Computer Preservation Project. This repo is a z88dk C library for the NABU PC (personal computer). It includes efficient C functions for video, audio, hcca, keyboard, and more for the NABU Computer. This is an ongoing project, so follow this repo for additions, changes and bug fixes

This library is optimized to be as efficient as possible, including text mode double buffer for scrolling, special function registers, and inlining where appropriate. This project aims to build a library for Cloud CP/M and homebrew development of the NABU PC with the easiest and fastest toolset. 

To use this library, you can follow the tutorial here: https://nabu.ca/homebrew-c-tutorial

# The NABU-LIB Library
The main library in the directories above is NABULIB. Within that folder you will find the main NABULIB libraries, which is the only stuff that need to be included in your NABU program. I have included fonts and utilities for graphics, sound, joystick, CPM, and more. If an example is missing or not working, ask me and I'll create it for you.

# Tips
I'm not using homebrew much, because I prefer Cloud CP/M builds. So you'll see my examples are leaning toward Cloud CP/M. It'll make your program friendly to distribute and will easily work on other CP/M systems if it isn't using hardware specific stuff. So if you disable keyboard, disable hcca, disable vdp, and use fprint/stdout and vt52 commands, it'll work on any z80 cpm computer. Now, if you use VDP and maintain interrupts for keyboard, then you're program is stuck with NABU but I still prefer CPM builds. Either way, the instructions for both are provided below.

# Cloud CP/M Apps
When building for Cloud CP/M, I recommend using the +cpm target with z88dk. While they've been really great adding support for NABU, they have aligned their development efforts at NABU CP/M for MAME and not Cloud CP/M. The +cpm with -subtype=nabu or -subtype=nabudos will take the vdp interrupt, create a unwanted hcca buffer and a few other things. Since no one has storage on their NABU, it isn't possible for 99.9% of the metal out there to use NABU CP/M. That is why I created Cloud CP/M, so we don't need floppy's (Don't copy that floppy! xD). 

For building apps for Cloud CP/M, this is the suggested command-line (replace the MYAPP with your app title)..

``
   zcc +cpm -vn --list -m -create-app -compiler=sdcc -O3 --opt-code-speed main.c -o "MYAPP"
``

That command-line will create a MYAPP.COM file, which you can simply copy to a Cloud CP/M disk image and run it. I have a BUILD.BAT for every project, and in the project folder I include the CPMTOOLS (mkfs.cpm, cpmcp, diskdefs) files. In my BUILD.BAT, I create a Cloud CP/M disk image with the MYAPP.com and copy it to my Internet Adapter Storage Folder. I usually use B: and that way you can just switch to B: in Cloud CP/M and test the program. You don't need to reboot the NABU every time you create a new build/disk image. Just run the program again after a rebuild and the new binary will be read - that's what's great about CP/M not having disk cache. It really only caches the directory contents, so the same program name can be ran over and over. Now, if your program crashes the NABU, you'll need to reboot because that's your problem :)

# Sample Cloud CP/M BUILD.BAT
CPM apps have a .COM extension. Because NABULIB is created for z88dk and I use Windows, this is a batch file. I always put the latest (nightly build) of z88dk in C:\Z88DK folder so it's easy to find and works with all my BUILD.BAT files. Another thing is that I name all my programs main.c, so that's a thing you'll notice. I put a pause at the end of the batch file so I can see if there were any errors. I also have a DIR *.COM because I like to see the file size of my program when optimizing. The parameters that are used for z88dk will create a LST and MAP file for you, as well.

```
@echo off

SET Z88DK_DIR=c:\z88dk\
SET ZCCCFG=%Z88DK_DIR%lib\config\
SET PATH=%Z88DK_DIR%bin;%PATH%

zcc +cpm -vn --list -m -create-app -compiler=sdcc -O3 --opt-code-speed main.c -o "YOURAPP"

mkfs.cpm -f naburn "C:\My Documents\NABU Internet Adapter\Store\b.dsk"

cpmcp -f naburn    "C:\My Documents\NABU Internet Adapter\Store\b.dsk" YOURAPP.com 0:

dir *.com

pause
```
# Home Brew Apps
Building a homebrew app is just as easy as a CPM App. It will generate a .NABU file, which is an executable binary that will load from the ROM when the NABU is first booted up. You can find more details about how that works and how I created the .NABU file format on my early YouTube NABU Hacking videos. But, the great folks at z88dk have created a +NABU targer for the compiler, which will generate a .NABU file. Here is an example z88dk commandline for compiling a .NABU file.
```
@echo off

SET Z88DK_DIR=c:\z88dk\
SET ZCCCFG=%Z88DK_DIR%lib\config\
SET PATH=%Z88DK_DIR%bin;%PATH%

echo.
echo ****************************************************************************

zcc +nabu -vn --list -m -create-app -compiler=sdcc main.c -o "000001.nabu"

echo ****************************************************************************

pause
```


# Version Notes

## v2023.02.03.00
Added new vdp functions for syncing game speed with vsync. Check the nabulib.h header in the VDP section for instructions and sample code. I'll add more examples and fix the build of other examples as time permits. Right now I'm breaking things while I continue to build GameManYeah!. I'll update the code on here for it someday when I'm a bit more comfortable with what I've created. There's also the NABU Tracker uploaded in here. But I need to provide examples for that as well. Stay tuned, I guess :)

## v2023.02.04.00
Few fixes for vdp text features in any mode. A few additional commands for changing pattern colors on the fly (useful for multiple character sets). And the IA control library for the new retronet menu. IA is losing it's interface in the next few days and it will entirely be controlled by a menu on the NABU

## v2023.02.01.01
Sorry, I broke your build! This release has a bunch of changes that allows G2 and Text mode to operate the same. You can load ASCII fonts -or- entire patterns. Even performing text print/write functions to G2 mode will work, including scrolling. So there's a number of changes that will break your build - but it's worth it.

## v2023.02.01.00
VDP uses default MSX memory mapping so you can load .sc2 files directly from vdp's 0x0. RetroNET remote filesystem calls that include reading now disable all other interrupts so it can get the job done without being annoyed with keyboard and vdp interrupts.

## v2023.01.30.00
Fixed a bunch of sprite stuff.

## v2023.01.29.00
optimized interrupts by reviewing the LIS and only saving registered that are needed. Added new assignable interrupt for vdp (read documentation) vdp_addISR()

## v2023.01.28.00
Added a G2 graphics mode 2 demo. I'll be rewriting the entire G2 graphics part because the stuff that's in NABULIB is not easy to work with.

## v2023.01.27.00
Added joystick support to nabulib. The keyboard interrupt must be left enabled, and up to 4 joysticks can be used.

## v2023.01.25.00
The Telnet CPM application demonstrates how to build a CPM program without needing the -subtype=nabu on the commandline. This fixes the issue where z88dk was taking the interupts and not using bdos for STDIN. The latest Cloud CP/M BIOS 2.7b with virtual 80 display is using this build.

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

# LICENSE
This is a library for a 40 year old computer - so there's no license. The only thing I ask is if you could give me credit if you use it. If you make changes, send them my way. 
