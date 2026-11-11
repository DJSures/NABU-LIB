# NABU-LIB

### MAKE SOMETHING :D

A fast, friendly **z88dk C library for the NABU Personal Computer**. Write homebrew programs and **Cloud CP/M** apps in C, with video, audio, keyboard, joysticks, the HCCA network port, RetroNET internet features, and now **online multiplayer**.

It's built for the [NABU Personal Computer Preservation Project](https://nabu.ca) and the NABU Internet Adapter (IA). It's a work in progress, so **watch/star this repo** to keep up with new features and fixes.

---

## Contents

- [Why NABU-LIB?](#why-nabu-lib)
- [Quick Start (5 minutes)](#quick-start-5-minutes)
- [Your First Program](#your-first-program)
- [Homebrew or Cloud CP/M?](#homebrew-or-cloud-cpm)
- [Building Homebrew Apps (.NABU)](#building-homebrew-apps-nabu)
- [Building Cloud CP/M Apps (.COM)](#building-cloud-cpm-apps-com)
- [Configuration Switches](#configuration-switches)
- [What's in the Library](#whats-in-the-library)
- [Examples](#examples)
- [Troubleshooting](#troubleshooting)
- [License](#license)
- [Version Notes](#version-notes)

---

## Why NABU-LIB?

- **Fast.** Hand-tuned for the Z80: a double-buffered text mode for smooth scrolling, special function registers, inlining where it helps, and interrupt handlers that only save the registers they use.
- **Complete.** VDP graphics (text, 80-column text, G2, sprites, bitmaps), AY-3-8910 sound and a music tracker, keyboard, up to 4 joysticks, HCCA serial, and vsync timing for games.
- **Online.** RetroNET lets your NABU fetch HTTP/HTTPS/FTP files, read and write files on the Internet Adapter, open TCP connections, control the IA, and host or join multiplayer games.
- **One include.** Add `NABU-LIB.h` and you're done. You don't need to link anything else.

---

## Quick Start (5 minutes)

### 1. Install z88dk

NABU-LIB is developed and tested with **z88dk v2.3 (the 23.07.2022 build)**.

- **Windows:** unzip z88dk to **`C:\z88dk`**. Every `build.bat` in this repo expects it there, so the examples build without any edits.
- **Linux / macOS:** install z88dk your usual way and run the same `zcc` commands shown below. The batch files are Windows-only, so you're on your own for scripts (sorry! :D).

> Newer z88dk versions may work, but they changed a lot in the compiler. I'm happy with how v2.3 works, so that's what NABULIB is built against. Nothing against their changes, and I really appreciate their NABU support <3

### 2. Get the code

Clone or download this repo. Keep the folder layout as it is, because the examples include the library with relative paths like `../NABULIB/NABU-LIB.h`.

### 3. Build the Skeleton Project

The **Skeleton Project** folder is a ready-to-go template with build scripts and a `main.c` that has every option already in place.

```
cd "Skeleton Project"
build.bat         <- builds a homebrew 000001.nabu
build-cpm.bat     <- builds a Cloud CP/M MYAPP.COM
```

### 4. Run it

- **Homebrew (.NABU):** point your Internet Adapter at the `.nabu` file and boot the NABU (or MAME).
- **Cloud CP/M (.COM):** copy the `.COM` onto a Cloud CP/M disk image and run it from the CP/M prompt. See [Building Cloud CP/M Apps](#building-cloud-cpm-apps-com) for a script that does this for you.

That's it. You're making something!

---

## Your First Program

Here's a complete text-mode "hello world". Save it as `main.c` in a new folder next to `NABULIB` (for example `MyApp/main.c`):

```c
#define BIN_TYPE BIN_HOMEBREW   // or BIN_CPM for a Cloud CP/M .COM
#define FONT_STANDARD           // pick a font: FONT_AMIGA, FONT_SET1, FONT_STANDARD, FONT_LM80C
#define DISABLE_HCCA_RX_INT     // not using the network, so save some space

#include "../NABULIB/NABU-LIB.h"
#include "../NABULIB/patterns.h"

void main() {

  // Set up audio, interrupts and the random number generator
  initNABULib();

  // 40-column white-on-blue text mode with auto scrolling
  vdp_initTextMode(VDP_WHITE, VDP_DARK_BLUE, true);
  vdp_loadASCIIFont(ASCII);

  vdp_print("Hello from the NABU!\r\n");

  // Wait for any key
  while (!isKeyPressed());
}
```

Build it:

```
zcc +nabu -vn --list -m -create-app -compiler=sdcc -O3 --opt-code-speed main.c -o "000001.nabu"
```

> **Important:** `NABU-LIB.h` pulls in `NABU-LIB.c` itself. Include it from **exactly one** `.c` file (normally `main.c`), and **don't** put `NABU-LIB.c` on the `zcc` command line.

---

## Homebrew or Cloud CP/M?

| | **Homebrew** | **Cloud CP/M** |
|---|---|---|
| Output | `000001.nabu` | `MYAPP.COM` |
| `BIN_TYPE` | `BIN_HOMEBREW` | `BIN_CPM` |
| `zcc` target | `+nabu` | `+cpm` |
| How it runs | Loads from the NABU ROM at boot, through the Internet Adapter or as a NABU Channel | Runs from the Cloud CP/M prompt, like any CP/M program |
| Rebuild/test loop | Reboot the NABU | Just run it again. No reboot needed |
| Best for | Full-screen games and demos that own the whole machine | Apps and tools you want to share easily |

**My tip:** I prefer Cloud CP/M builds. They're easy to share, and the test loop is quick. If you disable the keyboard, HCCA, and VDP (see [Configuration Switches](#configuration-switches)) and stick to `printf`/stdio and the VT52 commands, your program will run on **any Z80 CP/M computer**. Once you use the VDP or the NABULIB keyboard interrupt, the program is NABU-only, and that's totally fine too.

> Make sure `BIN_TYPE` in `main.c` matches the build you run. `BIN_HOMEBREW` goes with `+nabu`, and `BIN_CPM` goes with `+cpm`.

---

## Building Homebrew Apps (.NABU)

A `.nabu` file is a standalone binary that the NABU ROM loads at boot. The z88dk team added a `+nabu` target that creates it for you. (My early NABU Hacking videos on YouTube explain how the format works.)

**`build.bat`**

```bat
@echo off

SET Z88DK_DIR=c:\z88dk\
SET ZCCCFG=%Z88DK_DIR%lib\config\
SET PATH=%Z88DK_DIR%bin;%PATH%

echo.
echo ****************************************************************************

zcc +nabu -vn --list -m -create-app -compiler=sdcc -O3 --opt-code-speed main.c -o "000001.nabu"

echo ****************************************************************************

pause
```

---

## Building Cloud CP/M Apps (.COM)

Use the plain **`+cpm`** target. **Don't** use `-subtype=nabu` or `-subtype=nabudos`. Those take over the VDP interrupt, create an HCCA buffer you don't want, and stop stdin from going through BDOS.

```
zcc +cpm -vn --list -m -create-app -compiler=sdcc -O3 --opt-code-speed main.c -o "MYAPP"
```

That creates `MYAPP.COM`. Copy it onto a Cloud CP/M disk image and run it.

### The fast test loop

My `build.bat` builds the app, creates a fresh disk image straight in the **Internet Adapter's Store folder**, and copies the `.COM` onto it. I use drive **B:**, so all I do is switch to `B:` in Cloud CP/M and run the program.

You **don't need to reboot** after each build. CP/M has no disk cache (it only caches the directory), so running the same program name again loads the new binary. If your program crashes the NABU, though, a reboot is on you :)

The CP/M disk tools (`mkfs.cpm.exe`, `cpmcp.exe`, `diskdefs`) are in the **Skeleton Project** folder. Copy them into your project folder.

**`build.bat`** (change `YOURAPP` and the Store path to match your setup)

```bat
@echo off

SET Z88DK_DIR=c:\z88dk\
SET ZCCCFG=%Z88DK_DIR%lib\config\
SET PATH=%Z88DK_DIR%bin;%PATH%

zcc +cpm -vn --list -m -create-app -compiler=sdcc -O3 --opt-code-speed main.c -o "YOURAPP"

REM Make a fresh Cloud CP/M disk image in the Internet Adapter's Store folder
mkfs.cpm -f naburn "C:\My Documents\NABU Internet Adapter\Store\b.dsk"

REM Copy the app onto the disk image (user area 0)
cpmcp -f naburn "C:\My Documents\NABU Internet Adapter\Store\b.dsk" YOURAPP.com 0:

REM Show the file size, which is handy when optimizing
dir *.com

pause
```

Every build also writes a `.lis` listing and a `.map` file, which help when you're tuning for size or speed.

### Optional CP/M size savers

Add any of these to the top of `main.c` to strip out z88dk features you don't use:

```c
#pragma output nostreams       // no stdio (only if you're NOT using DISABLE_VDP)
#pragma output nofileio        // no CP/M file read/write
#pragma output noprotectmsdos  // strip the MS-DOS protection header (recommended)
#pragma output noredir         // no command-line redirection (useless with nostreams)
#pragma output nogfxglobals    // no z88dk graphics globals (recommended, NABULIB does graphics)
```

---

## Configuration Switches

Put these `#define`s **above** `#include "NABU-LIB.h"` in `main.c`. The Skeleton Project already has them all, commented out.

| Define | What it does |
|---|---|
| `BIN_TYPE BIN_HOMEBREW` / `BIN_TYPE BIN_CPM` | **Required.** Which kind of binary you're building. |
| `FONT_AMIGA`, `FONT_SET1`, `FONT_STANDARD`, `FONT_LM80C` | Picks a built-in font from `patterns.h`. Load it with `vdp_loadASCIIFont(ASCII)`. |
| `DISABLE_KEYBOARD_INT` | Turns off the NABULIB keyboard (`isKeyPressed()`, `getChar()`, `readLine()`). **Required** if you read input with CP/M stdio/conio (`gets()`, etc.). |
| `DISABLE_HCCA_RX_INT` | Turns off the HCCA receive interrupt. Use it when you don't need RetroNET, the file store, or IA control. Saves space. |
| `DISABLE_VDP` | Removes the VDP functions. For CP/M console apps that use `printf`/`puts` and the VT52 `vt_` commands. |
| `DISABLE_CURSOR` | Hides the NABULIB text cursor (it doesn't affect the CP/M cursor). |
| `DEBUG_VDP_INT` | Flashes the ALERT LED if your frame takes longer than one vsync before `vdp_waitVDPReadyInt()`. Use it while debugging, then remove it for release. |

Want to use your own font? Define `const uint8_t ASCII[768] = { ... };` and pass it to `vdp_loadASCIIFont(ASCII)`.

---

## What's in the Library

Everything lives in the **`NABULIB`** folder. The **only required file is `NABU-LIB.h`**. Add the others when you need them, and include them **after** `NABU-LIB.h`.

| File | What it's for |
|---|---|
| `NABU-LIB.h` / `.c` | The core: VDP graphics and text, sprites, sound, keyboard, joysticks, HCCA, interrupts, VT52, vsync timing. Include only the `.h`. |
| `patterns.h` | Built-in fonts and pattern tables. |
| `NabuTracker.h` / `.c` | A lightweight music tracker for the AY-3-8910. |
| `RetroNET-FileStore.h` / `.c` | Read and write files on the Internet Adapter, including HTTP/HTTPS/FTP downloads, directory listings, seeking, and TCP client/server sockets. |
| `RetroNET-IAControl.h` | Control Internet Adapter settings and browse NABU Channels from the NABU. |
| `RetroNET-CPMDrive.h` | Ask the IA to build or extract Cloud CP/M drive images. |
| `RetroNET-Multiplayer.h` | **New!** Host, browse, and join online multiplayer sessions, and send packets between NABUs. |
| `DirEntryReader.c` | CP/M BDOS directory walking and FCB helpers for CP/M utilities. |

Every header starts with a **function index** and full descriptions, so the headers are the reference manual.

---

## Examples

There are lots of real programs to learn from. Each folder has its own `build.bat`.

| Folder | What you'll find |
|---|---|
| **Skeleton Project** | Start here. A template with build scripts and CP/M disk tools. |
| **Demos** | Hello World, Bad Apple, Doom, Mandelbrot, JukeBox, NetMuse, SlideShow, NABUCAM, an `.sc2` image viewer, and more. |
| **Games** | Brick Battle, 8-Bit Abyss, Battleship, NADoom, NetHack, CPM Life, and a **Game Dev 1-5** tutorial series. |
| **Multiplayer** | Online **Chat**, **Tic-Tac-Toe**, and **Hockey** using the new multiplayer library. |
| **RetroNET** | Telnet (VT100), IRC, chat, ping, nslookup, IA Control, and drive tools. |
| **Utilities** | CP/M tools: `nano`, `vim`, `grep`, `more`, `wget`, `copy`, `move`, `ren`, `chkdsk`, `tnfs`, `playmidi`, and more. |
| **Tests** | Small focused tests: G2, bitmap graphics, scrolling, 80 columns, HTTP Get, TCP Server, VT52, digital audio, music, input, and more. |

If an example you need is missing or doesn't build, ask me and I'll make one for you.

---

## Troubleshooting

**"Duplicate symbol" errors when linking**
`NABU-LIB.h` contains the whole library. Include it from only one `.c` file, and don't pass `NABU-LIB.c` to `zcc`.

**Keyboard input doesn't work in my CP/M app**
If you use stdio (`gets()`, `getchar()`, etc.), add `#define DISABLE_KEYBOARD_INT`. Otherwise, NABULIB's keyboard interrupt grabs the keys before CP/M sees them.

**My CP/M app acts strangely or stdin is broken**
Build with plain `+cpm`, not `-subtype=nabu` or `-subtype=nabudos`.

**The NABU hangs when I start multiplayer**
Call `mp_isSupported()` before any other `mp_` function. Older Internet Adapters don't answer multiplayer commands, so update your IA from [nabu.ca](https://nabu.ca).

**My game stutters or flickers**
Update the VDP right after `vdp_waitVDPReadyInt()`, and use `DEBUG_VDP_INT` to check whether your frame takes longer than one vsync.

**Network features don't work**
Make sure you didn't `#define DISABLE_HCCA_RX_INT`, and that your Internet Adapter is up to date.

**MAME misses incoming characters**
MAME needs a TX delay set in the Internet Adapter. A real NABU receives at the full 111 kb/s with no delay.

---

## License

This is a library for a 40-year-old computer, so there's no license. All I ask is **fair acknowledgment and credit** for the effort that got you this far. If you use it, please give me credit.

If you change NABULIB or my programs, please send the changes my way so I can add them to this repo. You can find me on the NABU forum at **[forums.nabu.ca](https://forums.nabu.ca)**.

This is a hobby for all of us, not a business, so share and enjoy!

**Make something,**
**DJ Sures**

