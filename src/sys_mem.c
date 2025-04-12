
/*
 * Copyright (C) 2025 pdnguyen of HCMC University of Technology VNU-HCM
 */

/* Sierra release
 * Source Code License Grant: The authors hereby grant to Licensee
 * personal permission to use and modify the Licensed Source Code
 * for the sole purpose of studying while attending the course CO2018.
 */

 #include "syscall.h"
 #include "libmem.h"
 #include "mm.h"
 
 //typedef char BYTE;
 
 int __sys_memmap(struct pcb_t *caller, struct sc_regs* regs)
 {
    int memop = regs->a1;
    BYTE value;
 
    switch (memop) {
    case SYSMEM_MAP_OP:
             /* Reserved process case*/
             break;
    case SYSMEM_INC_OP:
             if (inc_vma_limit(caller, regs->a2, regs->a3) != 0) {
                return -1; 
             }
             break;
    case SYSMEM_SWP_OP:
             if (__mm_swap_page(caller, regs->a2, regs->a3) != 0) {
                return -1; 
             }
             break;
    case SYSMEM_IO_READ:
             if (MEMPHY_read(caller->mram, regs->a2, &value) != 0) {
                return -1; 
             }
             regs->a3 = value;
             break;
    case SYSMEM_IO_WRITE:
             if (MEMPHY_write(caller->mram, regs->a2, regs->a3) != 0) {
                return -1; 
             }
             break;
    default:
             printf("Memop code: %d\n", memop);
             break;
    }
    
    return 0;
 }
 