# GMU OS 64 - Modern Edition: Architectural Baseline & Status

**Project Name:** GMU OS 64 (Modern Edition)  
**Target Architecture:** x86_64 (64-bit Long Mode, Freestanding)  
**Boot Specification:** UEFI via Limine Boot Protocol (v3)  
**Academic Affiliation:** GM University, Davanagere, Karnataka, India  
**Course Context:** UE25CS2304 — Operating System with Practice  
**Build Status:** Cleanly Compiled & Verified in QEMU UEFI (Zero Warnings, Zero Errors)  
**Status Date:** November 2026  

---

## 1. Project Overview & Modernization Summary

GMU OS 64 has been fully modernized into a state-of-the-art, visually polished, flicker-free 64-bit desktop operating system with an aesthetic inspired by macOS Sonoma and Windows 11 Fluent Design, backed by robust PS/2 input drivers and a 13-application ecosystem.

### Core Architecture Highlights
- **Bare-metal x86_64 ELF Toolchain**: Built natively using `x86_64-elf-gcc` and `x86_64-elf-ld` with automated UEFI ISO packaging via `xorriso`.
- **Rock-Solid PS/2 Input Subsystem**:
  - **High-Capacity Ring Buffers (512 Bytes)**: Expanded controller queues from 64 to 512 bytes, preventing packet overflow and byte drops during rapid mouse swipes or high-speed typing.
  - **Smooth Mouse Ballistics & 200 Hz Sampling**: Configured PS/2 sample rate to 200 Hz with 8 counts/mm resolution. Dynamic quadratic acceleration curve ensures pixel-accurate 1:1 targeting for small UI buttons while enabling swift single-swipe screen traversal.
  - **Universal Mouse Polling**: Every single application (Finder, Notepad, Paint, Calculator, Calendar, Hex Viewer, Activity Monitor, Settings, Arcade Hub, Snake, Minesweeper, Breakout, GMU Strike, and Terminal Shell) polls mouse packets in its loop and supports red chiclet close clicks, eliminating queue desynchronization upon returning to the desktop.
  - **True 9-Bit Signed Mouse Packet Decoding**: Proper sign-extension using Packet 0 Bit 4 (X sign) and Bit 5 (Y sign), eliminating jumping, jerkiness, and direction inversion.
  - **Packet Resynchronization Engine**: Bit 3 validation (`data & 0x08`) on packet byte 0 ensures orphan bytes are instantly discarded, maintaining 3-byte packet alignment.
  - **Full Keyboard Scancode Translation Tables**:
    - Complete support for normal (`a-z`, `0-9`, `- = [ ] \ ; ' , . / \``) and shifted (`A-Z`, `! @ # $ % ^ & * ( ) _ + { } | : " < > ? ~`) characters.
    - Full Shift, CapsLock, Ctrl, and Alt state tracking.
    - Extended key support: Arrow keys (Up/Down/Left/Right), Enter, Escape, Tab, Backspace, Delete, Home, End, PageUp, PageDown, and Function keys (F1–F12).
    - Keypad number and arithmetic key support.
- **Flicker-Free Anti-Tear Graphics Architecture**:
  - Eliminated full-framebuffer (786,432 pixels) VRAM repainting on timer ticks.
  - Cursor redraws strictly gated on coordinate deltas (`mouse.x != last_x || mouse.y != last_y`).
  - Localized dirty-rectangle clock/status tray updates (420x24 px).
  - Isolated floating dock rendering upon selection changes.
  - CPU power management with `pause` idling.
- **macOS / Windows 11 Fluent Desktop Interface**:
  - **High-Resolution Wallpaper**: Atmospheric dusk/aurora gradient with twinkling ambient constellation stars and subtle horizon waves.
  - **High-Visibility Arrow Cursor**: 16x16 pixel-accurate OS cursor with jet-black border and pure white interior.
  - **Top Glass Menu Bar (30px)**: Translucent bar featuring ` GMU OS` logo pill, interactive Apple Dropdown Menu (<kbd>M</kbd>), classic system menus (`File`, `Edit`, `View`, `Go`, `Window`, `Help`), and live status tray with Wi-Fi, Volume (80%), Battery (100% AC), dynamic RAM meter, 100Hz CPU indicator, and digital clock.
  - **Desktop Icons with Custom 32x32 Graphics**:
    - 🖥️ `GMU Macintosh HD` (Silver chassis, NVMe slot, blue LED) -> Launches Finder.
    - 📁 `User Documents` (Golden Manila folder with documents) -> Launches Finder.
    - 🎮 `Arcade Games` (Retro purple gamepad) -> Launches Arcade Hub.
    - 📄 `README.TXT` (Document sheet with corner fold) -> Launches GUI Notepad.
    - 🗑️ `Trash` (Silver mesh wastebasket on bottom right).
  - **macOS "About This Mac" Modal Window**:
    - Accessible via <kbd>A</kbd> or Apple Menu -> *About GMU OS 64...*
    - Presents full hardware specifications, memory statistics, architecture, and University affiliation.
  - **Frosted Squircle Dock**: 10-item macOS-style launcher with custom 2D vector pixel art icons and active glowing emerald indicator pills.
  - **Interactive Dropdown System**: Menu bar dropdown with About GMU OS, System Settings, Activity Monitor, Reboot, and Shutdown.
- **4 Real-Time Workstation Themes**:
  1. *macOS Sonoma (Dark)*: Apple Indigo, Deep Slate, Frosted Glass, Accent Cyan.
  2. *Windows 11 Bloom*: Microsoft Azure, Mica Acrylic, Cobalt Glow.
  3. *Cyber Glow (Neon)*: Futuristic Neon Purple, Pink, and Vibrant Cyan.
  4. *macOS Tahoe (Light)*: High-contrast Frost Silver, Polar White, and Sky Blue.
  Switchable live anytime using <kbd>Tab</kbd> on the desktop, via Settings, or using the shell `theme` command.
- **Physical Memory Manager (PMM)**: Dynamic bitmap page frame allocator parsing Limine's `struct limine_memmap_request` (up to 1 GB addressable physical memory).
- **Kernel Dynamic Heap**: 2 MB dynamic heap with `kmalloc()`, `kfree()`, `kcalloc()`, and `krealloc()` with block splitting and coalescing.
- **Hierarchical VFS / RAMFS with Intelligent Node Sorting**: 128 nodes with 8 KB max file size, canonical path resolution (`/`, `..`, `.`, subpaths), append/truncate, CRUD operations, preloaded documentation, and automated sorting engine prioritizing directories over files alphabetically (A–Z case-insensitive).
- **Hardware Timer & Interrupts**: 8254 PIT calibrated at 100 Hz, 8259 PIC remapped, IDT loaded with 256 gates, live uptime tracking, and safe `gmu_sleep_ms()`.

---

## 2. Complete Application & Game Suite (13 Applications)

| # | Application | ID | Category | Description & Shortcuts |
| :-: | :--- | :--- | :---: | :--- |
| 1 | **Finder / File Manager** | `GMU_APP_FILE_MANAGER` | Productivity | Dual-pane file browser with auto-sorted directories & files (Folders First, A–Z), multi-mode sorting (<kbd>S</kbd> or column header click for Name ASC/DESC, Size, Type), smooth page scrolling & scrollbar thumb, alphabetical quick locations (<kbd>1</kbd>–<kbd>4</kbd>), integrated text preview (<kbd>V</kbd>/<kbd>Enter</kbd>), direct Notepad editing (<kbd>E</kbd>). |
| 2 | **GMU Terminal** | `GMU_APP_TERMINAL` | System / CLI | Line-buffered interactive shell, 16-command history (<kbd>↑</kbd>/<kbd>↓</kbd>), 25+ built-in commands, dynamic prompt (`GMU [/Documents]> `). |
| 3 | **GUI Notepad** | `GMU_APP_EDITOR` | Productivity | Multi-line text editor with line number gutter, text buffer, cursor editing, blank document creation (<kbd>F1</kbd>), and direct RAMFS saving (<kbd>F2</kbd>). |
| 4 | **Paint & Pixel Art Studio** | `GMU_APP_PAINT` | Creative / Graphics | 10-color artist palette (<kbd>0</kbd>–<kbd>9</kbd>), adjustable brush size (<kbd>+</kbd>/<kbd>-</kbd>), canvas clear (<kbd>C</kbd>), smooth mouse canvas drawing with keyboard fallback. |
| 5 | **GUI Calculator** | `GMU_APP_CALCULATOR` | Utility | 4x5 button grid, live expression history, 2x scaled digital readout, arithmetic evaluation (`+ - * / % ()`), keyboard and mouse input. |
| 6 | **Calendar & World Clock** | `GMU_APP_CALENDAR` | Productivity | Interactive monthly calendar grid (<kbd>P</kbd>/<kbd>N</kbd>), live 3x digital clock, real-time stopwatch (<kbd>S</kbd> start/stop, <kbd>R</kbd> reset), and world time zones. |
| 7 | **Activity & Health Monitor** | `GMU_APP_MONITOR` | System | Real-time visual progress bars for Physical RAM (PMM), Kernel Heap (2 MB pool), and RAMFS Inodes, plus driver/subsystem status table. |
| 8 | **Hex & Memory Inspector** | `GMU_APP_HEXVIEW` | Developer Tool | Dual hexadecimal and ASCII columns, 16-byte alignment, offset tracking, smooth scrolling (<kbd>↑</kbd>/<kbd>↓</kbd>), and document quick-load (<kbd>1</kbd>–<kbd>4</kbd>). |
| 9 | **Arcade Hub** | `GMU_APP_ARCADE` | Games & Entertainment | Unified game launcher presenting all 4 arcade games for one-click selection. |
| 10 | **GMU Snake** | `GMU_APP_SNAKE` | Games & Entertainment | Retro arcade snake game with arrow key navigation, fruit spawning, score and high score tracking, and game over overlay. |
| 11 | **Minesweeper Classic** | `GMU_APP_MINESWEEPER` | Games & Entertainment | 10x10 minefield with 12 hidden mines, flood-fill algorithm, flag marking (<kbd>F</kbd>), click reveal (<kbd>Space</kbd>/<kbd>Enter</kbd>), and win detection. |
| 12 | **Breakout Brick Breaker** | `GMU_APP_BREAKOUT` | Games & Entertainment | Classic arcade brick breaker with paddle physics, 4 rows of colored bricks, ball reflection, score multiplier, and lives tracker. |
| 13 | **System Settings** | `GMU_APP_SETTINGS` | Configuration | 5 interactive categories (Themes, Display, Storage, Input, About OS), mouse click & arrow navigation, live hardware specs, real-time modifier monitor, and 4-theme switcher. |
| * | **GMU Strike** | `GMU_APP_SHOOTER` | Games & Entertainment | 2D space shooter with lasers, enemy waves, collision detection, and score system. |

---

## 3. Verification & Testing

### Running in QEMU UEFI
To run the operating system in QEMU UEFI:
```powershell
cd "D:\GMU-OS-Transfer\GMU-OS-64\GMU-OS-64"
& "C:\Program Files\qemu\qemu-system-x86_64.exe" `
    -M q35 -m 1024M `
    -drive if=pflash,format=raw,readonly=on,file="C:\Program Files\qemu\share\edk2-x86_64-code.fd" `
    -cdrom "build\gmuos64-uefi.iso" `
    -serial stdio
```
Or simply double-click **`run.bat`** in `GMU-OS-64/`.

### Verified Serial Boot Output
```
========================================
  GMU OS 64 - Modern Edition (x86_64)   
========================================
[OK] IDT initialized (256 vectors)
[OK] Limine protocol handshake: v3
[OK] 8254 PIT Timer initialized (100 Hz)
[OK] Physical Memory Manager active
[OK] Kernel Dynamic Heap initialized (2 MB pool)
[OK] RAM filesystem self-test passed
[OK] Default documents & filesystem tree loaded
[OK] Application Manager initialized (13 GUI Apps & Games active)
[OK] UEFI GOP Framebuffer acquired
[OK] Framebuffer driver initialized
[OK] PS/2 Input Subsystem initialized
----------------------------------------
  System Ready. Starting GMU Desktop... 
----------------------------------------
```
