#include "boot_state.h"

/* Written by startup.S after BSS clear and before main(). */
volatile uint32_t boot_cpsr;
volatile uint32_t boot_sctlr;
volatile uint32_t boot_ctr;
volatile uint32_t boot_ttbr0;
volatile uint32_t boot_dacr;
volatile uint32_t boot_mpidr;
