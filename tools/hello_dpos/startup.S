.section .text.start
.global _start
_start:
    /* NAJPRV nastav stack, AŽ POTOM push */
    ldr sp, =0x40020000
    push {lr}               /* Ulož FEL návratovú adresu */

    /* Zakáž IRQ a FIQ */
    mrs r0, cpsr
    orr r0, r0, #0xC0
    msr cpsr_c, r0

    /* Nuluj BSS */
    ldr r0, =__bss_start
    ldr r1, =__bss_end
    mov r2, #0
bss_loop:
    cmp r0, r1
    strlt r2, [r0], #4
    blt bss_loop

    bl main

    /* Vráť sa do FEL */
    pop {pc}

.section .bss
__bss_start:
__bss_end:
