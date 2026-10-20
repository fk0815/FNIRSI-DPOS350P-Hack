#ifndef BOOT_STATE_H
#define BOOT_STATE_H

#include <stdint.h>

extern volatile uint32_t boot_cpsr;
extern volatile uint32_t boot_sctlr;
extern volatile uint32_t boot_ctr;
extern volatile uint32_t boot_ttbr0;
extern volatile uint32_t boot_dacr;
extern volatile uint32_t boot_mpidr;

#endif
