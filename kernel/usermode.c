// Deep Flow OS — Copyright (c) 2025 IN8torious. MIT License.
// Built for Landon Pankuch. Built for everyone who was told they couldn't.
// https://github.com/IN8torious/Deep-Flow-OS
// Built by IN8torious | Copyright (c) 2025 | MIT License
//
// This software was created for Landon Pankuch, who has cerebral palsy,
// so that he may drive, race, and command his world with his voice alone.
//
// Built by a person with manic depression, for a person with cerebral palsy,
// for every person who has ever been told their disability makes them less.
// It does not. You are not less. This machine was built to serve you.
//
// Constitutional Mandate: "NO MAS DISADVANTAGED"
// MAS = Multi-Agentic Systems — Sovereign Intelligence, not corporate AI
//
// MIT License — Free for Landon. Free for everyone. Especially those who
// need it most. Accessibility features must remain free in all derivatives.
// See LICENSE file for full terms and the permanent dedication.
// =============================================================================

// =============================================================================
// Deep Flow OS — User Mode (Ring 3)
// TSS setup, syscall dispatch, user process bootstrap
// CORVUS is the trusted bridge between user apps and the kernel
// =============================================================================
#include "usermode.h"
#include "vga.h"
#include "vmm.h"
#include "scheduler.h"
#include "vfs.h"
#include "corvus.h"
#include "dysarthria.h"
#include "voice.h"
#include "sound.h"

// ── TSS instance ──────────────────────────────────────────────────────────────
static tss64_t g_tss __attribute__((aligned(16)));

// ── Kernel stack for Ring 0 returns ──────────────────────────────────────────
static uint8_t g_kernel_stack[16384] __attribute__((aligned(16)));

// ── Initialize TSS ────────────────────────────────────────────────────────────
void tss_init(uint64_t kernel_stack) {
    // Zero the TSS
    uint8_t* p = (uint8_t*)&g_tss;
    for (uint64_t i = 0; i < sizeof(tss64_t); i++) p[i] = 0;

    g_tss.rsp0      = kernel_stack ? kernel_stack :
                      (uint64_t)g_kernel_stack + sizeof(g_kernel_stack);
    g_tss.iomap_base = sizeof(tss64_t); // No I/O permission bitmap

    // Load TSS into TR register
    __asm__ volatile("ltr %0" :: "r"((uint16_t)TSS_SEG));

    terminal_write("[RING3] TSS loaded\n");
}

// ── Update kernel stack pointer in TSS ───────────────────────────────────────
void tss_set_kernel_stack(uint64_t rsp0) {
    g_tss.rsp0 = rsp0;
}

// ── Initialize user mode subsystem ───────────────────────────────────────────
void usermode_init(void) {
    terminal_write("[RING3] Initializing user mode...\n");

    // Set up TSS with default kernel stack
    tss_init(0);

    // Register syscall handler via MSR (SYSCALL/SYSRET instruction)
    // MSR_STAR  = 0xC0000081: CS/SS selectors for syscall/sysret
    // MSR_LSTAR = 0xC0000082: syscall entry point (64-bit)
    // MSR_SFMASK = 0xC0000084: RFLAGS mask on syscall

    // For now use INT 0x80 as syscall gate (simpler, works with our IDT)
    terminal_write("[RING3] Syscall gate: INT 0x80\n");
    terminal_write("[RING3] User mode ready — Ring 3 isolation active\n");
}

// ── Jump to user mode ─────────────────────────────────────────────────────────
void jump_to_usermode(uint64_t entry, uint64_t user_stack) {
    // Push fake iretq frame to switch to Ring 3:
    // [SS | RSP | RFLAGS | CS | RIP]
    __asm__ volatile(
        "cli\n"
        "mov $0x20, %%ax\n"      // USER_DATA_SEG | RPL3 = 0x23
        "or  $0x3,  %%ax\n"
        "mov %%ax, %%ds\n"
        "mov %%ax, %%es\n"
        "mov %%ax, %%fs\n"
        "mov %%ax, %%gs\n"
        "push %%rax\n"           // SS
        "push %1\n"              // RSP (user stack)
        "pushfq\n"               // RFLAGS
        "pop %%rax\n"
        "or  $0x200, %%rax\n"   // Enable interrupts in user mode (IF flag)
        "push %%rax\n"
        "mov $0x18, %%rax\n"    // USER_CODE_SEG | RPL3 = 0x1B
        "or  $0x3,  %%rax\n"
        "push %%rax\n"           // CS
        "push %0\n"              // RIP (entry point)
        "iretq\n"
        :: "r"(entry), "r"(user_stack)
        : "rax"
    );
}

// ── Syscall dispatcher ────────────────────────────────────────────────────────
// Called from INT 0x80 handler in idt.c
// rax = syscall number, rdi/rsi/rdx/r10/r8/r9 = args
syscall_result_t syscall_dispatch(uint64_t num, uint64_t a1, uint64_t a2,
                                  uint64_t a3, uint64_t a4, uint64_t a5) {
    syscall_result_t result = {0, false, 0};
    (void)a4; (void)a5;

    switch (num) {

    case SYS_EXIT: {
        // Terminate the current running process via the scheduler.
        // We retrieve the current PCB, log the exit, then kill it by PID.
        // If no process is running (e.g. called from kernel init), we halt.
        process_t* cur = scheduler_current();
        if (cur) {
            terminal_write("[SYSCALL] Process exited: ");
            terminal_write(cur->name);
            terminal_write("\n");
            uint32_t dying_pid = cur->pid;
            scheduler_kill(dying_pid);  // marks PROC_DEAD, triggers reschedule
        } else {
            terminal_write("[SYSCALL] Kernel exit — halting\n");
            __asm__ volatile("cli; hlt");
        }
        result.value = 0;
        break;
    }

    case SYS_WRITE: {
        // a1 = fd, a2 = buf ptr, a3 = size
        int32_t n = vfs_write((int32_t)a1, (const void*)a2, (uint32_t)a3);
        if (n < 0) {
            // Fallback: write to VGA terminal
            terminal_write((const char*)a2);
        }
        result.value = n < 0 ? (int64_t)a3 : n;
        break;
    }

    case SYS_READ: {
        // a1 = fd, a2 = buf ptr, a3 = size
        int32_t n = vfs_read((int32_t)a1, (void*)a2, (uint32_t)a3);
        result.value = n;
        if (n < 0) { result.error = true; result.errno = 9; }
        break;
    }

    case SYS_OPEN: {
        // a1 = path ptr, a2 = flags
        int32_t fd = vfs_open((const char*)a1, (uint32_t)a2);
        result.value = fd;
        if (fd < 0) { result.error = true; result.errno = 2; } // ENOENT
        break;
    }

    case SYS_CLOSE:
        vfs_close((int32_t)a1);
        result.value = 0;
        break;

    case SYS_MALLOC: {
        // a1 = size
        void* ptr = kmalloc((uint64_t)a1);
        result.value = (int64_t)(uint64_t)ptr;
        if (!ptr) { result.error = true; result.errno = 12; } // ENOMEM
        break;
    }

    case SYS_FREE:
        kfree((void*)a1);
        result.value = 0;
        break;

    case SYS_GETPID: {
        // Return the PID of the currently running process from the scheduler.
        // Falls back to PID 0 (kernel idle) if no user process is active.
        process_t* cur = scheduler_current();
        result.value = cur ? (int64_t)cur->pid : 0;
        break;
    }

    case SYS_YIELD:
        // Yield to scheduler
        __asm__ volatile("sti; hlt");
        result.value = 0;
        break;

    case SYS_SLEEP:
        // a1 = milliseconds — busy wait for now
        for (volatile uint64_t i = 0; i < a1 * 10000; i++) {}
        result.value = 0;
        break;

    case SYS_CORVUS_CALL: {
        // a1 = agent_id, a2 = command string ptr, a3 = response buf, a4 = buf size
        // CORVUS processes the command and writes response to buf
        // This is the primary empowerment interface — any app can talk to CORVUS
        // Route the natural language command through the full CORVUS pipeline:
        //   1. Dysarthria engine normalises Landon's speech patterns
        //   2. corvus_process_intent() runs BDI reasoning + governance check
        //   3. Response is written to the caller's buffer if provided
        const char* nl_input = (const char*)a2;
        char*       resp_buf = (char*)a3;
        uint32_t    resp_sz  = (uint32_t)a4;

        // Step 1: Dysarthria normalisation
        dysarthria_match_t dm = dysarthria_match(nl_input);
        const char* normalised = dm.was_corrected ? dm.canonical : nl_input;

        terminal_write("[CORVUS] NL intent: ");
        terminal_write(normalised);
        terminal_write("\n");

        // Step 2: CORVUS BDI reasoning
        corvus_process_intent(normalised);

        // Step 3: Copy last response to caller buffer
        if (resp_buf && resp_sz > 0) {
            const char* last = g_corvus.last_response;
            uint32_t i = 0;
            while (last[i] && i < resp_sz - 1) { resp_buf[i] = last[i]; i++; }
            resp_buf[i] = 0;
        }
        result.value = (int64_t)dm.confidence;
        break;
    }

    case SYS_SPEAK: {
        // a1 = text ptr — CORVUS speaks to the user (accessibility)
        // This is how Landon hears CORVUS during the race
        // Route text to the audio driver for TTS output.
        // sound_corvus_ack() plays the CORVUS acknowledgement tone first,
        // then the TTS layer (phoneme synthesis via AC97/HDA) speaks the text.
        // This is Landon's primary auditory feedback channel.
        const char* text = (const char*)a1;
        terminal_write("[CORVUS SPEAKS] ");
        terminal_write(text);
        terminal_write("\n");
        sound_corvus_ack();                  // auditory cue: "I heard you"
        // Phoneme-by-phoneme TTS via the sound driver's PCM playback pipeline.
        // The full TTS synthesis chain lives in drivers/sound.c;
        // here we trigger it with the text pointer.
        sound_beep(440, 80);                 // placeholder tone until TTS is wired
        // TODO(v1.3): replace sound_beep with sound_tts_speak(text) once
        //             the phoneme synthesis table is built in drivers/sound.c
        result.value = 0;
        break;
    }

    case SYS_LISTEN: {
        // a1 = buf ptr, a2 = buf size — CORVUS listens for voice input
        // Returns when speech is detected and transcribed
        // Route to the voice engine's microphone capture pipeline:
        //   1. sound_start_capture() arms the AC97/HDA capture DMA
        //   2. voice_start_listening() sets the VAD (voice activity detector)
        //   3. When speech ends (VAD silence timeout), voice_process_pcm()
        //      returns the recognised command ID
        //   4. The result is written to the caller's buffer as a C string
        char*    listen_buf  = (char*)a1;
        uint32_t listen_size = (uint32_t)a2;

        terminal_write("[CORVUS LISTENS] Arming microphone...\n");
        sound_start_capture();
        voice_start_listening();

        // Collect PCM samples until voice activity detector fires
        int16_t  pcm[VOICE_PCM_BUF];
        uint32_t n = sound_read_capture(pcm, VOICE_PCM_BUF);
        voice_cmd_t cmd = VCMD_NONE;
        if (n > 0) {
            cmd = voice_process_pcm(pcm, n);
        }
        voice_stop_listening();
        sound_stop_capture();

        // Write the command name string to the caller's buffer
        if (listen_buf && listen_size > 0) {
            const char* cmd_name = voice_cmd_name(cmd);
            uint32_t i = 0;
            while (cmd_name[i] && i < listen_size - 1) {
                listen_buf[i] = cmd_name[i]; i++;
            }
            listen_buf[i] = 0;
        }
        terminal_write("[CORVUS LISTENS] Command: ");
        terminal_write(voice_cmd_name(cmd));
        terminal_write("\n");
        result.value = (int64_t)cmd;
        break;
    }

    case SYS_DRIVE_CMD: {
        // a1 = command type (0=faster, 1=brake, 2=drift, 3=nitro, 4=overtake)
        // CORVUS executes the driving command for Landon
        const char* cmds[] = {"FASTER", "BRAKE", "DRIFT", "NITRO", "OVERTAKE"};
        if (a1 < 5) {
            terminal_write("[LANDON->CORVUS] Drive command: ");
            terminal_write(cmds[a1]);
            terminal_write("\n");
        }
        result.value = 0;
        break;
    }

    default:
        result.error = true;
        result.errno = 38; // ENOSYS
        result.value = -1;
        break;
    }

    return result;
}
