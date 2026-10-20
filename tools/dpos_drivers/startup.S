.syntax unified
.cpu cortex-a7
.arm

.section .text.start, "ax", %progbits
.global _start
.type _start, %function

.extern main
.extern __bss_start
.extern __bss_end
.extern boot_cpsr
.extern boot_sctlr
.extern boot_ctr
.extern boot_ttbr0
.extern boot_dacr
.extern boot_mpidr

_start:
    /* Program is linked at 0x40000000; stack grows down from 0x40020000. */
    ldr sp, =0x40020000
    push {lr}

    /* Disable IRQ and FIQ while preserving the current processor mode. */
    mrs r0, cpsr
    orr r1, r0, #0xC0
    msr cpsr_c, r1

    /* Clear linker-defined BSS. */
    ldr r1, =__bss_start
    ldr r2, =__bss_end
    mov r3, #0
1:
    cmp r1, r2
    bhs 2f
    str r3, [r1], #4
    b 1b

2:
    dsb sy
    isb

    /* Capture inherited FEL/boot state after BSS has been cleared. */
    ldr r1, =boot_cpsr
    str r0, [r1]

    mrc p15, 0, r0, c1, c0, 0      /* SCTLR */
    ldr r1, =boot_sctlr
    str r0, [r1]

    mrc p15, 0, r0, c0, c0, 1      /* CTR */
    ldr r1, =boot_ctr
    str r0, [r1]

    mrc p15, 0, r0, c2, c0, 0      /* TTBR0 */
    ldr r1, =boot_ttbr0
    str r0, [r1]

    mrc p15, 0, r0, c3, c0, 0      /* DACR */
    ldr r1, =boot_dacr
    str r0, [r1]

    mrc p15, 0, r0, c0, c0, 5      /* MPIDR */
    ldr r1, =boot_mpidr
    str r0, [r1]

    dsb sy
    isb
    bl main

    /* main normally never returns. */
    dsb sy
    isb
    pop {pc}

.size _start, . - _start
