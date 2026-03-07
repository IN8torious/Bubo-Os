// =============================================================================
// Raven OS — Kernel Main
// Entry point called from boot.asm after multiboot2 handoff
// =============================================================================

#include "vga.h"
#include <stdint.h>
#include <stddef.h>

// Multiboot2 magic value
#define MULTIBOOT2_BOOTLOADER_MAGIC 0x36d76289

// Simple integer to string (decimal)
static void itoa(uint32_t n, char* buf) {
    if (n == 0) { buf[0] = '0'; buf[1] = '\0'; return; }
    char tmp[12];
    int i = 0;
    while (n > 0) { tmp[i++] = '0' + (n % 10); n /= 10; }
    int j = 0;
    while (i > 0) buf[j++] = tmp[--i];
    buf[j] = '\0';
}

// Simple hex to string
static void itohex(uint32_t n, char* buf) {
    const char* hex = "0123456789ABCDEF";
    buf[0] = '0'; buf[1] = 'x';
    for (int i = 7; i >= 0; i--) {
        buf[2 + (7 - i)] = hex[(n >> (i * 4)) & 0xF];
    }
    buf[10] = '\0';
}

// Draw the Raven OS banner
static void draw_banner(void) {
    uint8_t red_on_black   = vga_entry_color(VGA_LIGHT_RED,  VGA_BLACK);
    uint8_t white_on_black = vga_entry_color(VGA_WHITE,      VGA_BLACK);
    uint8_t grey_on_black  = vga_entry_color(VGA_DARK_GREY,  VGA_BLACK);

    terminal_setcolor(red_on_black);
    terminal_writeline("================================================================================");
    terminal_writeline("                                                                                ");

    terminal_setcolor(white_on_black);
    terminal_writeline("        ██████╗  █████╗ ██╗   ██╗███████╗███╗   ██╗     ██████╗ ███████╗       ");
    terminal_writeline("        ██╔══██╗██╔══██╗██║   ██║██╔════╝████╗  ██║    ██╔═══██╗██╔════╝       ");
    terminal_writeline("        ██████╔╝███████║██║   ██║█████╗  ██╔██╗ ██║    ██║   ██║███████╗       ");
    terminal_writeline("        ██╔══██╗██╔══██║╚██╗ ██╔╝██╔══╝  ██║╚██╗██║    ██║   ██║╚════██║       ");
    terminal_writeline("        ██║  ██║██║  ██║ ╚████╔╝ ███████╗██║ ╚████║    ╚██████╔╝███████║       ");
    terminal_writeline("        ╚═╝  ╚═╝╚═╝  ╚═╝  ╚═══╝  ╚══════╝╚═╝  ╚═══╝     ╚═════╝ ╚══════╝       ");

    terminal_setcolor(red_on_black);
    terminal_writeline("                                                                                ");
    terminal_writeline("                    K E R N E L   P L U T O :   R E D   C L O U D             ");
    terminal_writeline("                         Akatsuki Edition  |  v0.1.0-alpha                    ");
    terminal_writeline("                                                                                ");
    terminal_writeline("================================================================================");
    terminal_writeline("");

    terminal_setcolor(grey_on_black);
    terminal_writeline("  Kernel compiled with GCC x86 cross-compiler");
    terminal_writeline("  Engines: GamePlay3D + Ouzel [pending integration]");
    terminal_writeline("");
}

// Kernel main — called from boot.asm
void kernel_main(uint32_t multiboot_magic, uint32_t multiboot_info_addr) {
    // Initialize VGA terminal
    terminal_init();

    // Draw the banner
    draw_banner();

    // Verify multiboot2 magic
    uint8_t red   = vga_entry_color(VGA_LIGHT_RED, VGA_BLACK);
    uint8_t green = vga_entry_color(VGA_LIGHT_GREEN, VGA_BLACK);
    uint8_t white = vga_entry_color(VGA_WHITE, VGA_BLACK);
    uint8_t grey  = vga_entry_color(VGA_DARK_GREY, VGA_BLACK);

    terminal_setcolor(white);
    terminal_write("  [ BOOT ] Multiboot2 magic: ");

    char hexbuf[12];
    itohex(multiboot_magic, hexbuf);
    if (multiboot_magic == MULTIBOOT2_BOOTLOADER_MAGIC) {
        terminal_setcolor(green);
        terminal_write(hexbuf);
        terminal_writeline("  ✓ VALID");
    } else {
        terminal_setcolor(red);
        terminal_write(hexbuf);
        terminal_writeline("  ✗ INVALID — boot may be unstable");
    }

    terminal_setcolor(white);
    terminal_write("  [ BOOT ] Multiboot info at: ");
    terminal_setcolor(grey);
    itohex(multiboot_info_addr, hexbuf);
    terminal_writeline(hexbuf);

    terminal_setcolor(white);
    terminal_writeline("");
    terminal_writeline("  [ MEM  ] Memory subsystem     ... [TODO]");
    terminal_writeline("  [ INT  ] Interrupt descriptor  ... [TODO]");
    terminal_writeline("  [ FS   ] Virtual filesystem    ... [TODO]");
    terminal_writeline("  [ GFX  ] Framebuffer driver    ... [TODO]");
    terminal_writeline("  [ UI   ] GamePlay3D + Ouzel    ... [TODO]");
    terminal_writeline("  [ AI   ] Agent subsystem       ... [TODO]");
    terminal_writeline("");

    terminal_setcolor(red);
    terminal_writeline("  ████████████████████████████████████████████████████████████████████████████");
    terminal_setcolor(white);
    terminal_writeline("  Raven OS kernel loaded successfully. System halted pending next phase.");
    terminal_setcolor(red);
    terminal_writeline("  ████████████████████████████████████████████████████████████████████████████");

    // Halt — future: jump to scheduler
    for (;;) {
        __asm__ volatile("hlt");
    }
}
