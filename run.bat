@echo off
title GMU OS 64 Runner
echo ========================================================
echo       Launching GMU OS 64 (Modern UEFI Edition)        
echo ========================================================
"C:\Program Files\qemu\qemu-system-x86_64.exe" -M q35 -m 1024M -drive if=pflash,format=raw,readonly=on,file="C:\Program Files\qemu\share\edk2-x86_64-code.fd" -cdrom "%~dp0build\gmuos64-uefi.iso" -serial stdio
pause
