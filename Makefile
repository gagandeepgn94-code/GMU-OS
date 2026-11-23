CC = x86_64-elf-gcc
LD = x86_64-elf-ld

BUILD = build
KERNEL = $(BUILD)/gmuos64.elf
ISO = $(BUILD)/gmuos64-uefi.iso

CFLAGS = -std=gnu11 -O2 -Wall -Wextra \
         -ffreestanding -fno-stack-protector \
         -fno-pic -fno-pie -m64 \
         -mcmodel=kernel -mno-red-zone \
         -mno-mmx -mno-sse -mno-sse2

LDFLAGS = -m elf_x86_64 -nostdlib -static \
          -z max-page-size=0x1000 \
          -T kernel/linker.ld

OBJECTS = $(BUILD)/main.o \
          $(BUILD)/idt.o \
          $(BUILD)/interrupt.o \
          $(BUILD)/timer.o \
          $(BUILD)/isr_stubs.o \
          $(BUILD)/pmm.o \
          $(BUILD)/heap.o \
          $(BUILD)/app_manager.o \
          $(BUILD)/fs.o \
          $(BUILD)/file_manager.o \
          $(BUILD)/editor.o \
          $(BUILD)/calculator.o \
          $(BUILD)/monitor.o \
          $(BUILD)/settings.o \
          $(BUILD)/paint.o \
          $(BUILD)/calendar.o \
          $(BUILD)/hexview.o \
          $(BUILD)/snake.o \
          $(BUILD)/minesweeper.o \
          $(BUILD)/breakout.o \
          $(BUILD)/game_center.o \
          $(BUILD)/shooter.o \
          $(BUILD)/framebuffer.o \
          $(BUILD)/draw.o \
          $(BUILD)/theme.o \
          $(BUILD)/desktop.o \
          $(BUILD)/font.o \
          $(BUILD)/keyboard.o \
          $(BUILD)/ps2.o \
          $(BUILD)/mouse.o \
          $(BUILD)/shell.o

.PHONY: all clean iso run

all: $(KERNEL) $(ISO)

$(BUILD):
	mkdir -p $(BUILD)

$(BUILD)/main.o: kernel/core/main.c | $(BUILD)
	$(CC) $(CFLAGS) -Iinclude -c $< -o $@

$(BUILD)/pmm.o: kernel/memory/pmm.c | $(BUILD)
	$(CC) $(CFLAGS) -Iinclude -c $< -o $@

$(BUILD)/heap.o: kernel/memory/heap.c | $(BUILD)
	$(CC) $(CFLAGS) -Iinclude -c $< -o $@

$(BUILD)/framebuffer.o: kernel/drivers/video/framebuffer.c | $(BUILD)
	$(CC) $(CFLAGS) -Iinclude -c $< -o $@

$(BUILD)/draw.o: kernel/graphics/draw.c | $(BUILD)
	$(CC) $(CFLAGS) -Iinclude -c $< -o $@

$(BUILD)/theme.o: kernel/graphics/theme.c | $(BUILD)
	$(CC) $(CFLAGS) -Iinclude -c $< -o $@

$(BUILD)/desktop.o: kernel/graphics/desktop.c | $(BUILD)
	$(CC) $(CFLAGS) -Iinclude -c $< -o $@

$(BUILD)/font.o: kernel/graphics/font.c | $(BUILD)
	$(CC) $(CFLAGS) -Iinclude -c $< -o $@

$(BUILD)/keyboard.o: kernel/drivers/input/keyboard.c | $(BUILD)
	$(CC) $(CFLAGS) -Iinclude -c $< -o $@

$(BUILD)/ps2.o: kernel/drivers/input/ps2.c | $(BUILD)
	$(CC) $(CFLAGS) -Iinclude -c $< -o $@

$(BUILD)/mouse.o: kernel/drivers/input/mouse.c | $(BUILD)
	$(CC) $(CFLAGS) -Iinclude -c $< -o $@

$(BUILD)/app_manager.o: applications/app_manager.c | $(BUILD)
	$(CC) $(CFLAGS) -Iinclude -c $< -o $@

$(BUILD)/editor.o: applications/editor/editor.c | $(BUILD)
	$(CC) $(CFLAGS) -Iinclude -c $< -o $@

$(BUILD)/calculator.o: applications/calculator/calculator.c | $(BUILD)
	$(CC) $(CFLAGS) -Iinclude -c $< -o $@

$(BUILD)/monitor.o: applications/monitor/monitor.c | $(BUILD)
	$(CC) $(CFLAGS) -Iinclude -c $< -o $@

$(BUILD)/settings.o: applications/settings/settings.c | $(BUILD)
	$(CC) $(CFLAGS) -Iinclude -c $< -o $@

$(BUILD)/paint.o: applications/paint/paint.c | $(BUILD)
	$(CC) $(CFLAGS) -Iinclude -c $< -o $@

$(BUILD)/calendar.o: applications/calendar/calendar.c | $(BUILD)
	$(CC) $(CFLAGS) -Iinclude -c $< -o $@

$(BUILD)/hexview.o: applications/hexview/hexview.c | $(BUILD)
	$(CC) $(CFLAGS) -Iinclude -c $< -o $@

$(BUILD)/snake.o: applications/games/snake/snake.c | $(BUILD)
	$(CC) $(CFLAGS) -Iinclude -c $< -o $@

$(BUILD)/minesweeper.o: applications/games/minesweeper/minesweeper.c | $(BUILD)
	$(CC) $(CFLAGS) -Iinclude -c $< -o $@

$(BUILD)/breakout.o: applications/games/breakout/breakout.c | $(BUILD)
	$(CC) $(CFLAGS) -Iinclude -c $< -o $@

$(BUILD)/game_center.o: applications/games/game_center.c | $(BUILD)
	$(CC) $(CFLAGS) -Iinclude -c $< -o $@

$(BUILD)/fs.o: kernel/fs/fs.c | $(BUILD)
	$(CC) $(CFLAGS) -Iinclude -c $< -o $@

$(BUILD)/shell.o: shell/shell.c | $(BUILD)
	$(CC) $(CFLAGS) -Iinclude -c $< -o $@

$(BUILD)/file_manager.o: applications/file_manager/file_manager.c | $(BUILD)
	$(CC) $(CFLAGS) -Iinclude -c $< -o $@

$(BUILD)/idt.o: kernel/arch/x86_64/interrupts/idt.c | $(BUILD)
	$(CC) $(CFLAGS) -Iinclude -c $< -o $@

$(BUILD)/isr_stubs.o: kernel/arch/x86_64/interrupts/isr_stubs.S | $(BUILD)
	$(CC) -m64 -ffreestanding -fno-pic -fno-pie -c $< -o $@

$(BUILD)/interrupt.o: kernel/arch/x86_64/interrupts/interrupt.c | $(BUILD)
	$(CC) $(CFLAGS) -Iinclude -c $< -o $@

$(BUILD)/timer.o: kernel/arch/x86_64/interrupts/timer.c | $(BUILD)
	$(CC) $(CFLAGS) -Iinclude -c $< -o $@

$(BUILD)/shooter.o: applications/games/shooter/shooter.c | $(BUILD)
	$(CC) $(CFLAGS) -Iinclude -c $< -o $@

$(KERNEL): $(OBJECTS) kernel/linker.ld
	$(LD) $(LDFLAGS) -o $@ $(OBJECTS)

iso: $(ISO)

$(ISO): $(KERNEL)
	cp $(KERNEL) iso_root/boot/gmuos64.elf
	xorriso -as mkisofs -R -r -J \
		-hfsplus -apm-block-size 2048 \
		--efi-boot boot/limine-uefi-cd.bin \
		-efi-boot-part --efi-boot-image --protective-msdos-label \
		iso_root -o $@

clean:
	rm -rf $(BUILD)
