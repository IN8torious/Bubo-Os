cmake_minimum_required(VERSION 3.16)
project(RavenAOS VERSION 0.3.0 LANGUAGES C ASM_NASM)

# =============================================================================
# Raven AOS — CMake Build System
# CORVUS is the chef. CMake is the recipe book.
# =============================================================================

# ── Toolchain guard ───────────────────────────────────────────────────────────
# Must be cross-compiled for bare-metal i386
if(NOT CMAKE_CROSSCOMPILING)
    message(STATUS "Note: Building for host. For bare-metal, use the i386 toolchain file.")
endif()

# ── Output names ─────────────────────────────────────────────────────────────
set(KERNEL_NAME   "raven.kernel")
set(ISO_NAME      "RavenOS.iso")
set(ISO_DIR       "${CMAKE_BINARY_DIR}/iso")

# ── Compiler flags (bare-metal i386) ─────────────────────────────────────────
set(BARE_METAL_FLAGS
    -m32
    -ffreestanding
    -fno-stack-protector
    -fno-builtin
    -nostdlib
    -nostdinc
    -O2
)

# ── Include directories ───────────────────────────────────────────────────────
include_directories(${CMAKE_SOURCE_DIR}/include)

# ── ASM sources ───────────────────────────────────────────────────────────────
set(ASM_SOURCES
    boot/boot.asm
    boot/interrupts.asm
    boot/scheduler_asm.asm
)

# ── C sources ─────────────────────────────────────────────────────────────────
set(C_SOURCES
    kernel/kernel.c
    kernel/pmm.c
    kernel/idt.c
    kernel/vmm.c
    kernel/pit.c
    kernel/keyboard.c
    kernel/scheduler.c
    kernel/corvus.c
    kernel/corvus_brain.c
    kernel/shell.c
    drivers/vga.c
    graphics/framebuffer.c
    graphics/font.c
    graphics/gui.c
    engine/raven_engine.c
)

# ── Kernel target ─────────────────────────────────────────────────────────────
add_executable(${KERNEL_NAME} ${C_SOURCES} ${ASM_SOURCES})

target_compile_options(${KERNEL_NAME} PRIVATE ${BARE_METAL_FLAGS})

# Linker script
set_target_properties(${KERNEL_NAME} PROPERTIES
    LINK_FLAGS "-m elf_i386 -T ${CMAKE_SOURCE_DIR}/kernel.ld --oformat elf32-i386 -nostdlib"
)

# ── Install kernel into iso/boot/ ─────────────────────────────────────────────
install(TARGETS ${KERNEL_NAME}
    RUNTIME DESTINATION ${ISO_DIR}/boot
)

install(FILES iso/boot/grub/grub.cfg
    DESTINATION ${ISO_DIR}/boot/grub
)

# ── ISO target (requires grub-mkrescue) ──────────────────────────────────────
find_program(GRUB_MKRESCUE grub-mkrescue)
if(GRUB_MKRESCUE)
    add_custom_target(iso
        COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target install
        COMMAND ${GRUB_MKRESCUE} -o ${CMAKE_BINARY_DIR}/${ISO_NAME} ${ISO_DIR}
        COMMENT "Building ${ISO_NAME}..."
        DEPENDS ${KERNEL_NAME}
    )
endif()

# ── QEMU test target ──────────────────────────────────────────────────────────
find_program(QEMU qemu-system-i386)
if(QEMU AND GRUB_MKRESCUE)
    add_custom_target(qemu-test
        COMMAND timeout 5 ${QEMU}
            -cdrom ${CMAKE_BINARY_DIR}/${ISO_NAME}
            -m 256M
            -nographic
            -serial stdio
            -no-reboot
            || true
        COMMENT "QEMU headless boot test..."
        DEPENDS iso
    )
endif()

# ── cmake.ini support (matches the CI workflow) ───────────────────────────────
# If cmake.ini exists, its [ubuntu] install section is read by CI
# to install extra apt packages before building.
# Example cmake.ini:
#   [ubuntu]
#   install = nasm grub-pc-bin grub-common xorriso qemu-system-x86

# ── Version info ─────────────────────────────────────────────────────────────
configure_file(
    ${CMAKE_SOURCE_DIR}/include/version.h.in
    ${CMAKE_BINARY_DIR}/include/version.h
    @ONLY
)
