#  GMU OS 64 — Modern Desktop Edition

[![Target](https://img.shields.io/badge/Architecture-x86__64%20(Long%20Mode)-blue?style=flat-square&logo=cpu)](https://github.com)
[![Boot Protocol](https://img.shields.io/badge/Boot-UEFI%20(Limine%20v3)-purple?style=flat-square&logo=gnu)](https://github.com)
[![Toolchain](https://img.shields.io/badge/Toolchain-x86__64--elf--gcc%20%7C%20ld-informational?style=flat-square)](https://gcc.gnu.org)
[![Build Status](https://img.shields.io/badge/Build-Passing%20(0%20Errors%2C%200%20Warnings)-brightgreen?style=flat-square)](https://github.com)
[![License](https://img.shields.io/badge/Academic%20Project-UE25CS2304-orange?style=flat-square)](https://gmu.ac.in)

> **GM University (GMU), Davanagere, Karnataka, India**  
> **Department of Computer Science & Engineering**  
> **Course:** UE25CS2304 — Operating System with Practice  
> **Edition:** Modern Desktop Edition (UEFI 64-bit Freestanding)  
> **Release Date:** November 2026

---

## 📖 Table of Contents
1. [Project Overview](#-project-overview)
2. [Key Architectural Highlights](#-key-architectural-highlights)
3. [Complete Application Suite (13 Applications)](#-complete-application-suite-13-applications)
4. [Desktop Interface & Theming](#-desktop-interface--theming)
5. [Filesystem & Intelligent Sorting](#-filesystem--intelligent-sorting)
6. [Repository & Directory Structure](#-repository--directory-structure)
7. [Prerequisites & Toolchain Setup](#-prerequisites--toolchain-setup)
8. [Build & Execution Guide](#-build--execution-guide)
9. [Terminal Shell Command Reference](#-terminal-shell-command-reference)
10. [Global Keyboard & Mouse Shortcuts](#-global-keyboard--mouse-shortcuts)
11. [Academic Attribution](#-academic-attribution)

---

## 🌟 Project Overview

**GMU OS 64 (Modern Edition)** is a bare-metal, freestanding 64-bit operating system engineered from the ground up for x86_64 machines. Booting natively through the **Limine Boot Protocol (v3)** under UEFI firmware, GMU OS 64 delivers a high-performance graphical workstation environment inspired by **macOS Sonoma** and **Windows 11 Fluent Design**.

Unlike traditional toy kernels that operate in text mode VGA, GMU OS 64 boots directly into a 32-bit truecolor framebuffer (`1024x768` or higher) with custom typography, hardware-accurate mouse ballistics, dynamic memory management, a hierarchical RAM filesystem, and a complete suite of **13 built-in GUI productivity applications and retro arcade games**.

```
+--------------------------------------------------------------------------+
|  GMU OS  File  Edit  View  Go  Window  Help  [WI-FI] VOL 80% RAM 24% 22:30|
+--------------------------------------------------------------------------+
|  [🖥️ Macintosh HD]                                                      |
|  [📁 Documents]                                                          |
|  [🎮 Arcade Games]             ✨ Atmospheric Gradient Wallpaper          |
|  [📄 README.TXT]                                                         |
|                                                                          |
|                                                                          |
|                   +------------------------------------+                 |
|                   |  [📁] [💻] [📝] [🎨] [🧮] [📅] ... |   (macOS Dock)  |
|                   +------------------------------------+                 |
+--------------------------------------------------------------------------+
```

---

## ⚡ Key Architectural Highlights

### 1. Bare-Metal x86_64 Long Mode & UEFI Boot
- **Limine Boot Protocol (v3)**: Parses UEFI GOP framebuffer, memory map requests, and kernel entry structures.
- **Freestanding Toolchain**: Native compilation with `x86_64-elf-gcc` and `x86_64-elf-ld` without libc dependencies.
- **Interrupt Infrastructure**: Complete 256-entry Interrupt Descriptor Table (IDT), 8259 Programmable Interrupt Controller (PIC) remapping (`0x20`–`0x2F`), and 8254 Programmable Interval Timer (PIT) calibrated at 100 Hz.

### 2. High-Precision PS/2 Input Subsystem
- **512-Byte High-Capacity Queues**: Expanded controller FIFO ring buffers prevent byte dropping during aggressive mouse movement or fast typing.
- **Smooth Mouse Ballistics (200 Hz Sampling)**: Configured at 200 Hz with 8 counts/mm resolution. Quadratic acceleration curve enables pixel-accurate button targeting while maintaining single-swipe screen traversal.
- **Signed 9-Bit Packet Decoding**: Packet 0 Bit 4 (X sign) and Bit 5 (Y sign) sign-extension eliminates jitter, coordinate overflow, and directional inversion.
- **Universal Application Polling**: All 13 applications service the mouse ring buffer in their event loops and support red chiclet window closing, ensuring zero queue desynchronization upon returning to the desktop.
- **Full Keyboard Scancode Translation**: Full support for normal and shifted characters, Shift/CapsLock/Ctrl/Alt states, arrow keys, function keys (<kbd>F1</kbd>–<kbd>F12</kbd>), keypad, and editing keys (<kbd>Home</kbd>, <kbd>End</kbd>, <kbd>PageUp</kbd>, <kbd>PageDown</kbd>).

### 3. Anti-Tear Flicker-Free Graphics Architecture
- **Eliminated Full-Screen Redraws**: Zero full-framebuffer (786,432 pixels) VRAM repainting on timer ticks.
- **Delta-Gated Cursor Redraw**: Cursor save/restore logic only triggers when mouse coordinates change (`mouse.x != last_x || mouse.y != last_y`).
- **Localized Dirty Rectangles**: Live clock and status tray updates isolated to an exact `420x24` px bounding box.
- **CPU Power Conservation**: Event-driven architecture with `pause` idling minimizes CPU power draw.

### 4. Dynamic Memory Management
- **Physical Memory Manager (PMM)**: Bitmap page frame allocator parsing Limine physical memory maps, managing up to 1 GB of physical RAM in 4 KB frames.
- **Kernel Dynamic Heap**: 2 MB heap pool implementing `kmalloc()`, `kfree()`, `kcalloc()`, and `krealloc()` with block header metadata, free-list splitting, and immediate block coalescing.

### 5. Hierarchical VFS / RAMFS
- In-memory filesystem supporting 128 nodes and up to 8 KB file sizes.
- Canonical path resolution (`/`, `..`, `.`, nested subpaths).
- CRUD operations: create, read, write, append, truncate, rename, and delete.
- **Intelligent Sorting Engine**: Directory nodes are automatically prioritized over files, and entries are sorted alphabetically (<kbd>A–Z</kbd>, case-insensitive) across the Finder and terminal `ls`.

---

## 🖥️ Complete Application Suite (13 Applications)

GMU OS 64 includes a rich set of 13 built-in GUI productivity tools, system monitors, developer utilities, and arcade games:

| # | Application | Identifier | Category | Key Features & Shortcuts |
| :-: | :--- | :--- | :---: | :--- |
| **1** | **Finder / File Manager** | `GMU_APP_FILE_MANAGER` | Productivity | Dual-pane file browser with auto-sorted directories & files (Folders First, A–Z), multi-mode sorting (<kbd>S</kbd> or column header click for Name ASC/DESC, Size, Type), smooth page scrolling & scrollbar thumb, alphabetical quick locations (<kbd>1</kbd>–<kbd>4</kbd>), integrated text preview (<kbd>V</kbd>/<kbd>Enter</kbd>), direct Notepad editing (<kbd>E</kbd>). |
| **2** | **GMU Terminal** | `GMU_APP_TERMINAL` | System / CLI | Line-buffered interactive shell, 16-command history (<kbd>↑</kbd>/<kbd>↓</kbd>), 25+ built-in commands, dynamic prompt (`GMU [/Documents]> `). |
| **3** | **GUI Notepad** | `GMU_APP_EDITOR` | Productivity | Multi-line text editor with line number gutter, text buffer, cursor editing, blank document creation (<kbd>F1</kbd>), and direct RAMFS saving (<kbd>F2</kbd>). |
| **4** | **Paint & Studio** | `GMU_APP_PAINT` | Creative / Graphics | 10-color artist palette (<kbd>0</kbd>–<kbd>9</kbd>), adjustable brush size (<kbd>+</kbd>/<kbd>-</kbd>), canvas clear (<kbd>C</kbd>), smooth mouse canvas drawing with keyboard fallback. |
| **5** | **GUI Calculator** | `GMU_APP_CALCULATOR` | Utility | 4x5 button grid, live expression history, 2x scaled digital readout, arithmetic evaluation (`+ - * / % ()`), keyboard and mouse input. |
| **6** | **Calendar & Clock** | `GMU_APP_CALENDAR` | Productivity | Interactive monthly calendar grid (<kbd>P</kbd>/<kbd>N</kbd>), live 3x digital clock, real-time stopwatch (<kbd>S</kbd> start/stop, <kbd>R</kbd> reset), and world time zones. |
| **7** | **Activity Monitor** | `GMU_APP_MONITOR` | System | Real-time visual progress bars for Physical RAM (PMM), Kernel Heap (2 MB pool), and RAMFS Inodes, plus driver/subsystem status table. |
| **8** | **Hex & Memory Inspector** | `GMU_APP_HEXVIEW` | Developer Tool | Dual hexadecimal and ASCII columns, 16-byte alignment, offset tracking, smooth scrolling (<kbd>↑</kbd>/<kbd>↓</kbd>), and document quick-load (<kbd>1</kbd>–<kbd>4</kbd>). |
| **9** | **Arcade Hub** | `GMU_APP_ARCADE` | Games | Unified game launcher presenting all 4 arcade games for one-click selection. |
| **10** | **GMU Snake** | `GMU_APP_SNAKE` | Games | Retro arcade snake game with arrow key navigation, fruit spawning, score and high score tracking, and game over overlay. |
| **11** | **Minesweeper Classic**| `GMU_APP_MINESWEEPER` | Games | 10x10 minefield with 12 hidden mines, flood-fill algorithm, flag marking (<kbd>F</kbd>), click reveal (<kbd>Space</kbd>/<kbd>Enter</kbd>), and win detection. |
| **12** | **Breakout Brick Breaker** | `GMU_APP_BREAKOUT`| Games | Classic arcade brick breaker with paddle physics, 4 rows of colored bricks, ball reflection, score multiplier, and lives tracker. |
| **13** | **System Settings** | `GMU_APP_SETTINGS` | Configuration | 5 interactive categories (Themes, Display, Storage, Input, About OS), mouse click & arrow navigation, live hardware specs, real-time modifier monitor, and 4-theme switcher. |
| **★** | **GMU Strike** | `GMU_APP_SHOOTER` | Games | 2D space shooter with lasers, enemy waves, collision detection, and score system. |

---

## 🎨 Desktop Interface & Theming

### Desktop Components
- **Top Glass Menu Bar (30px)**: Translucent acrylic bar featuring ` GMU OS` logo pill, interactive Apple Dropdown Menu (<kbd>M</kbd>), standard menus (`File`, `Edit`, `View`, `Go`, `Window`, `Help`), and live status tray (Wi-Fi, Volume, Battery, RAM meter, 100Hz CPU indicator, and clock).
- **macOS "About This Mac" Modal**: Accessible via <kbd>A</kbd> or Apple Menu, displaying system specs, memory stats, and academic affiliation.
- **Frosted Squircle Dock**: 10-item macOS-style launcher with vector pixel art icons and glowing emerald active indicator pills.
- **Desktop Shortcuts**:
  - 🖥️ `GMU Macintosh HD` -> Launches Finder at `/`
  - 📁 `User Documents` -> Launches Finder at `/Documents`
  - 🎮 `Arcade Games` -> Launches Arcade Hub
  - 📄 `README.TXT` -> Launches GUI Notepad with system guide
  - 🗑️ `Trash` -> Desktop trash bin icon

### 4 Workstation Themes
Switchable instantly anytime using <kbd>Tab</kbd> on the desktop, via **Settings**, or using the shell `theme` command:
1. **macOS Sonoma (Dark)**: Apple Indigo, Deep Slate, Frosted Glass, Accent Cyan.
2. **Windows 11 Bloom**: Microsoft Azure, Mica Acrylic, Cobalt Glow.
3. **Cyber Glow (Neon)**: Futuristic Neon Purple, Pink, and Vibrant Cyan.
4. **macOS Tahoe (Light)**: High-contrast Frost Silver, Polar White, Sky Blue.

---

## 📂 Filesystem & Intelligent Sorting

The RAMFS subsystem enforces modern file manager sorting conventions:
1. **Directories Grouped First**: All directory nodes (`GMU_FS_DIRECTORY`) are rendered at the top of the list before any regular files.
2. **Alphabetical Ordering**: Case-insensitive comparison (`a–z` matching `A–Z`) with case-sensitive tie-breaking.
3. **Multi-Mode File Manager Sorting**:
   - `NAME [A-Z]` / `NAME [Z-A]`: Alphabetical ascending / descending.
   - `SIZE [v]` / `SIZE [^]`: File size descending / ascending.
   - `TYPE [*]`: Grouped by file extension / type.
4. **Dynamic Selection Tracking**: Creating (<kbd>N</kbd> / <kbd>F</kbd>) or renaming (<kbd>R</kbd>) items automatically keeps the selection cursor on the affected item in its new sorted position.

---

## 📁 Repository & Directory Structure

```text
GMU-OS-64/
├── applications/             # Userspace GUI application ecosystem
│   ├── app_manager.c         # Central app registry & dispatcher
│   ├── calculator/           # GUI Calculator with expression parser
│   ├── calendar/             # Calendar, Digital Clock & Stopwatch
│   ├── editor/               # Notepad multi-line text editor
│   ├── file_manager/         # Finder dual-pane file manager with sorting
│   ├── games/                # Arcade games suite
│   │   ├── breakout/         # Breakout Brick Breaker game
│   │   ├── game_center.c     # Arcade Hub game launcher
│   │   ├── minesweeper/      # Classic Minesweeper game
│   │   ├── shooter/          # GMU Strike 2D space shooter
│   │   └── snake/            # GMU Snake arcade game
│   ├── hexview/              # Hexadecimal & memory inspector
│   ├── monitor/              # System & memory health monitor
│   ├── paint/                # 10-color Paint & Pixel Art Studio
│   └── settings/             # System settings & theme switcher
├── boot/                     # Bootloader configuration & Limine assets
│   └── limine-src/           # Limine bootloader source files
├── include/                  # Kernel & subsystem C header files
│   └── gmu/                  # GMU OS API headers (fs, draw, font, etc.)
├── iso_root/                 # Limine UEFI ISO layout root
│   ├── boot/                 # Limine EFI binaries & kernel target
│   └── limine.conf           # Limine bootloader configuration script
├── kernel/                   # Core operating system kernel
│   ├── arch/x86_64/          # x86_64 architecture-specific code
│   │   └── interrupts/       # IDT, PIC, PIT timer, ISR stubs assembly
│   ├── core/main.c           # Kernel entry point & initialization
│   ├── drivers/              # Hardware driver implementations
│   │   ├── input/            # PS/2 keyboard, mouse & ballistics
│   │   └── video/            # GOP framebuffer driver
│   ├── fs/                   # Hierarchical RAMFS & sorted child engine
│   ├── graphics/             # 2D rendering, desktop, fonts & themes
│   ├── memory/               # Physical Memory Manager (PMM) & Heap
│   └── linker.ld             # Kernel x86_64 ELF linker script
├── shell/                    # Interactive command line shell
│   └── shell.c               # Terminal implementation with 25+ commands
├── Makefile                  # Automated build & ISO generation script
├── PROJECT_STATUS.md         # Detailed architectural status document
├── README.md                 # Project README documentation
└── run.bat                   # One-click Windows QEMU UEFI runner script
```

---

## 🛠️ Prerequisites & Toolchain Setup

### Windows (Recommended)
1. **MSYS2**: Install from [msys2.org](https://www.msys2.org/).
2. In the MSYS2 UCRT64 terminal, install the cross-compiler and packaging tools:
   ```bash
   pacman -S mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-make xorriso
   ```
3. Ensure `x86_64-elf-gcc` and `x86_64-elf-ld` are available in your `PATH` (e.g. via MSYS2 or prebuilt cross-compiler binaries).
4. **QEMU**: Install [QEMU for Windows](https://www.qemu.org/download/#windows) to `C:\Program Files\qemu\` with EDK2 UEFI firmware (`edk2-x86_64-code.fd`).

### Linux / Ubuntu / Debian
```bash
sudo apt update
sudo apt install build-essential gcc-x86-64-linux-gnu binutils-x86-64-linux-gnu xorriso qemu-system-x86 ovmf
```

---

## 🚀 Build & Execution Guide

### 1. Compile Kernel & Generate UEFI ISO
From the `GMU-OS-64/` directory:
```bash
# Clean previous builds
make clean

# Compile kernel ELF and generate build/gmuos64-uefi.iso
make
```

On Windows PowerShell:
```powershell
$env:PATH = "C:\msys64\ucrt64\bin;C:\msys64\usr\bin;" + $env:PATH
cd GMU-OS-64
mingw32-make clean
mingw32-make
```

### 2. Run in QEMU UEFI
To launch the operating system in QEMU with UEFI firmware:
```powershell
cd GMU-OS-64
& "C:\Program Files\qemu\qemu-system-x86_64.exe" `
    -M q35 -m 1024M `
    -drive if=pflash,format=raw,readonly=on,file="C:\Program Files\qemu\share\edk2-x86_64-code.fd" `
    -cdrom "build\gmuos64-uefi.iso" `
    -serial stdio
```

Or simply double-click **`run.bat`**!

---

## 💻 Terminal Shell Command Reference

The GMU Terminal Shell supports 25+ built-in commands with argument parsing, 16-command history (<kbd>↑</kbd>/<kbd>↓</kbd>), and path resolution:

| Command | Arguments | Description |
| :--- | :--- | :--- |
| `HELP` | — | Display command reference catalog. |
| `LS` / `DIR` | `[dir]` | List files and directories (folders first, A–Z). |
| `CD` | `<dir>` | Change working directory (`..`, `/`, subpaths). |
| `PWD` | — | Print current working directory path. |
| `CAT` / `TYPE`| `<file>` | Output text contents of a file. |
| `TOUCH` | `<file>` | Create a new blank file. |
| `WRITE` | `<file> <text>` | Overwrite file with text string. |
| `APPEND` | `<file> <text>` | Append a line of text to an existing file. |
| `MKDIR` | `<dir>` | Create a new directory node. |
| `RM` / `DEL` | `<name>` | Delete a file or empty directory. |
| `STAT` | `<name>` | Display node metadata (type, size in bytes). |
| `SYSINFO` | — | Display system specs, OS banner, and logo. |
| `MEM` | — | Display physical RAM (PMM) and heap memory statistics. |
| `UPTIME` | — | Show system uptime in seconds and timer ticks. |
| `CALC` | `[expr]` | Inline arithmetic evaluator (`+ - * / % ()`) or GUI calculator. |
| `EDIT` / `NOTEPAD` | `[file]` | Launch Notepad (optionally open specified file). |
| `PAINT` | — | Launch Paint & Pixel Art Studio. |
| `CALENDAR` / `CLOCK` | — | Launch Calendar, World Clock & Stopwatch. |
| `HEXVIEW` | `[file]` | Launch Hexadecimal & Memory Inspector. |
| `MONITOR` / `TOP` | — | Launch Activity & Health Monitor. |
| `SETTINGS` | — | Launch System Settings. |
| `THEME` | `[1-4]` | Cycle themes or switch directly to theme 1–4. |
| `ARCADE` / `GAMES` | — | Launch Arcade Hub unified game launcher. |
| `SNAKE` | — | Launch GMU Snake retro arcade game. |
| `MINES` | — | Launch Minesweeper Classic game. |
| `BREAKOUT` | — | Launch Breakout Brick Breaker game. |
| `STRIKE` | — | Launch GMU Strike 2D space shooter. |
| `CLEAR` / `CLS`| — | Clear the terminal screen buffer. |
| `DESKTOP` / `EXIT` | — | Return to Graphical Desktop. |
| `REBOOT` | — | Perform hardware reset via 8042 keyboard controller. |
| `SHUTDOWN` / `HALT` | — | Halt CPU (`cli; hlt`). |

---

## ⌨️ Global Keyboard & Mouse Shortcuts

### Desktop Shortcuts
- <kbd>Tab</kbd>: Cycle through workstation themes live (Dark, Bloom, Cyber, Light).
- <kbd>M</kbd>: Open / close Apple Menu Bar dropdown.
- <kbd>A</kbd>: Open "About GMU OS" system modal.
- <kbd>←</kbd> / <kbd>→</kbd>: Navigate active dock items.
- <kbd>Enter</kbd> / <kbd>Space</kbd>: Launch selected dock application.
- <kbd>1</kbd>–<kbd>0</kbd>: Fast launch dock applications 1 through 10.
- **Mouse Click**: Click any dock icon, menu item, desktop shortcut, or window control chiclet.

### File Manager Shortcuts
- <kbd>↑</kbd> / <kbd>↓</kbd>: Navigate item list.
- <kbd>PageUp</kbd> / <kbd>PageDown</kbd>: Scroll page up / down.
- <kbd>Enter</kbd> / <kbd>V</kbd>: Open folder or preview file modal.
- <kbd>Backspace</kbd>: Navigate to parent directory (`..`).
- <kbd>S</kbd>: Cycle sorting mode (Name A–Z, Name Z–A, Size, Type).
- <kbd>N</kbd>: Create new folder.
- <kbd>F</kbd>: Create new file.
- <kbd>R</kbd>: Rename selected item.
- <kbd>D</kbd>: Delete selected item (with confirmation).
- <kbd>E</kbd>: Open selected file in GUI Notepad.
- <kbd>1</kbd>–<kbd>4</kbd>: Quick jump to Root (`/`), Config (`/Config`), Documents (`/Documents`), or Games (`/Games`).
- <kbd>Esc</kbd>: Exit File Manager back to Desktop.

---

## 🎓 Academic Attribution

- **University:** GM University, Davanagere, Karnataka, India
- **Department:** Department of Computer Science & Engineering
- **Course Context:** UE25CS2304 — Operating System with Practice
- **Project Title:** GMU OS 64 (Modern Edition)

Developed as a state-of-the-art educational research operating system demonstrating bare-metal x86_64 architecture, UEFI firmware integration, double-buffered graphics engineering, hardware interrupt handling, dynamic memory management, and modern GUI user-experience design.
