/*
 * Copyright (C) 2025 pdnguyen of HCMC University of Technology VNU-HCM
 */

/* Sierra release
 * Source Code License Grant: The authors hereby grant to Licensee
 * personal permission to use and modify the Licensed Source Code
 * for the sole purpose of studying while attending the course CO2018.
 */

// #ifdef MM_PAGING
/*
 * System Library
 * Memory Module Library libmem.c
 */

 #include "string.h"
 #include "mm.h"
 #include "syscall.h"
 #include "libmem.h"
 #include <stdlib.h>
 #include <stdio.h>
 #include <pthread.h>
 
 static pthread_mutex_t mmvm_lock = PTHREAD_MUTEX_INITIALIZER;
 
 /*enlist_vm_freerg_list - add new rg to freerg_list
  *@mm: memory region
  *@rg_elmt: new region
  *
  */
 int enlist_vm_freerg_list(struct mm_struct *mm, struct vm_rg_struct *rg_elmt)
 {
   struct vm_rg_struct *rg_node = mm->mmap->vm_freerg_list;
   struct vm_rg_struct *rgit = mm->mmap->vm_freerg_list;  
 
   if (rg_elmt->rg_start >= rg_elmt->rg_end)
     return -1;
 
   while (rgit) {
     if (rgit->rg_start <= rg_elmt->rg_start && rgit->rg_end >= rg_elmt->rg_end) {
       return -1;
     }
     rgit = rgit->rg_next;
   }
 
   if (rg_node != NULL)
     rg_elmt->rg_next = rg_node;
 
   /* Enlist the new region */
   mm->mmap->vm_freerg_list = rg_elmt;
 
   return 0;
 }
 
 /*delist_vm_freerg_list - remove a region from vm_freerg_list
  *@mm: memory region
  *@rg_elmt: region to be removed
  *
  *Return 0 if successful, -1 otherwise
  */
 int delist_vm_freerg_list(struct mm_struct *mm, struct vm_rg_struct *rg_elmt) {
  //printf("Get in delist\n");
   if (mm == NULL || rg_elmt == NULL || mm->mmap == NULL)
     return -1;
 
   struct vm_rg_struct *prev = NULL;
   struct vm_rg_struct *curr = mm->mmap->vm_freerg_list;
   
   if (curr == NULL)
     return -1;
   
   // Delete at head
   if (curr->rg_start == rg_elmt->rg_start && curr->rg_end == rg_elmt->rg_end) {
     mm->mmap->vm_freerg_list = curr->rg_next;
     //printf("Get out delist 1\n");
     return 0;
   }
   
   while (curr) {
     prev = curr;
     curr = curr->rg_next;
     if (curr->rg_start == rg_elmt->rg_start && curr->rg_end == rg_elmt->rg_end) {
       /* Found the region, remove it from the list */
       prev->rg_next = curr->rg_next;
       //printf("Get out delist 2\n");
       return 0;
     }    
   }
   //printf("Get out delist 3\n");
   /* Region not found */
   return -1;
 }
 
 
 /*get_symrg_byid - get mem region by region ID
  *@mm: memory region
  *@rgid: region ID act as symbol index of variable
  *
  */
 struct vm_rg_struct *get_symrg_byid(struct mm_struct *mm, int rgid)
 {
   if (rgid < 0 || rgid > PAGING_MAX_SYMTBL_SZ)
     return NULL;
 
   return &mm->symrgtbl[rgid];
 }
 
 /*__alloc - allocate a region memory
  *@caller: caller
  *@vmaid: ID vm area to alloc memory region
  *@rgid: memory region ID (used to identify variable in symbole table)
  *@size: allocated size
  *@alloc_addr: address of allocated memory region
  *
  */
 int __alloc(struct pcb_t *caller, int vmaid, int rgid, int size, int *alloc_addr) {
   /* Check the validation */
   if (caller == NULL || alloc_addr == NULL || size <= 0 || vmaid < 0 || rgid < 0 || rgid > PAGING_MAX_SYMTBL_SZ) {
     return -1;
   }
 
   /* Lock */
   pthread_mutex_lock(&mmvm_lock);
 
   /*Allocate at the toproof */
   struct vm_rg_struct rgnode;
 
   /* TODO: commit the vmaid */
   // rgnode.vmaid
   int num_mapped_pages;
   if (get_free_vmrg_area(caller, vmaid, size, &rgnode, &num_mapped_pages) == 0) {
     caller->mm->symrgtbl[rgid].rg_start = rgnode.rg_start;
     caller->mm->symrgtbl[rgid].rg_end = rgnode.rg_end;
 
     *alloc_addr = rgnode.rg_start;

     vm_map_ram(caller, caller->mm->mmap->vm_start, caller->mm->mmap->vm_end, rgnode.rg_start, num_mapped_pages, NULL);
 
     pthread_mutex_unlock(&mmvm_lock);
     return 0;
   }
 
   /* TODO get_free_vmrg_area FAILED handle the region management (Fig.6)*/
 
   /* TODO retrive current vma if needed, current comment out due to compiler redundant warning*/
   /*Attempt to increate limit to get space */
   struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, vmaid);
   if (cur_vma == NULL) {
     pthread_mutex_unlock(&mmvm_lock);
     return -1;
   }
 
   int inc_sz = PAGING_PAGE_ALIGNSZ(size);
   int inc_limit_ret;
 
   /* TODO retrive old_sbrk if needed, current comment out due to compiler redundant warning*/
   int old_sbrk = cur_vma->sbrk;
 
   /* TODO INCREASE THE LIMIT as inovking systemcall
    * sys_memap with SYSMEM_INC_OP
    */
   struct sc_regs regs;
   regs.a1 = SYSMEM_INC_OP;
   regs.a2 = vmaid;
   regs.a3 = inc_sz;
 
   /* SYSCALL 17 sys_memmap */
   inc_limit_ret = syscall(caller, 17, &regs);
   if (inc_limit_ret < 0) {
     pthread_mutex_unlock(&mmvm_lock);
     return -1;
   }
 
   /* TODO: commit the limit increment */
   cur_vma->sbrk = old_sbrk + inc_sz;
 
   /* TODO: commit the allocation address
   // *alloc_addr = ...
   */
   caller->mm->symrgtbl[rgid].rg_start = old_sbrk;
   caller->mm->symrgtbl[rgid].rg_end = old_sbrk + size;
   *alloc_addr = old_sbrk; // same logic as alloc_addr = rgnode[rgid].rg_start
 
   if (inc_sz > size) { // allocate more than need
     struct vm_rg_struct *new_rg = malloc(sizeof(struct vm_rg_struct));
     new_rg->rg_start = old_sbrk + size;
     new_rg->rg_end = old_sbrk + inc_sz;
     new_rg->rg_next = NULL;
     enlist_vm_freerg_list(caller->mm, new_rg); // add the remaining space to free list
   }
 
   /* Unlock */
   pthread_mutex_unlock(&mmvm_lock);
   
   return 0;
 }
 
 /*__free - remove a region memory
  *@caller: caller
  *@vmaid: ID vm area to alloc memory region
  *@rgid: memory region ID (used to identify variable in symbole table)
  *
  */
 int __free(struct pcb_t *caller, int vmaid, int rgid)
 {
   // struct vm_rg_struct rgnode;
 
   // Dummy initialization for avoding compiler dummay warning
   // in incompleted TODO code rgnode will overwrite through implementing
   // the manipulation of rgid later
 
   if (rgid < 0 || rgid > PAGING_MAX_SYMTBL_SZ || caller == NULL || vmaid < 0)
     return -1;
 
   /* Lock */
   pthread_mutex_lock(&mmvm_lock);
 
   /* Check if rgid appear in caller->mm->symrgtbl */
   if (caller->mm->symrgtbl[rgid].rg_start < 0 || caller->mm->symrgtbl[rgid].rg_end < 0) {
     pthread_mutex_unlock(&mmvm_lock);
     return -1;
   }
 
   /* TODO: Manage the collect freed region to freerg_list */
   struct vm_rg_struct *rgnode = &(caller->mm->symrgtbl[rgid]);
   struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, vmaid);
   struct vm_rg_struct *rgit = cur_vma->vm_freerg_list;
   struct vm_rg_struct *temp_next = NULL;

   // Merge adjacent free regions
   while (rgit) {
     temp_next = rgit->rg_next;
     if (rgit->rg_end == rgnode->rg_start) {
       rgnode->rg_start = rgit->rg_start; 
       delist_vm_freerg_list(caller->mm, rgit); 
       //printf("here?\n");
       free(rgit); 
     } else if (rgit->rg_start == rgnode->rg_end) {
       rgnode->rg_end = rgit->rg_end; 
       delist_vm_freerg_list(caller->mm, rgit);
       //printf("or here?\n");
       free(rgit);
     }
     rgit = temp_next;
   }
 
   /*enlist the obsoleted memory region */
   struct vm_rg_struct *free_node = malloc(sizeof(struct vm_rg_struct));
   free_node->rg_start = rgnode->rg_start;
   free_node->rg_end = rgnode->rg_end;
   rgnode->rg_start = rgnode->rg_end = -1;
   enlist_vm_freerg_list(caller->mm, free_node);
   pthread_mutex_unlock(&mmvm_lock);  //add unlock
   return 0;
 }
 
 /*liballoc - PAGING-based allocate a region memory
  *@proc:  Process executing the instruction
  *@size: allocated size
  *@reg_index: memory region ID (used to identify variable in symbole table)
  */
 int liballoc(struct pcb_t *proc, uint32_t size, uint32_t reg_index)
 {
   /* TODO Implement allocation on vm area 0 */
   int addr;
 
   /* By default using vmaid = 0 */
   int stat = __alloc(proc, 0, reg_index, size, &addr);
   printf("Allocated region %u with size %u, return status: %d\n",reg_index,size,stat);
   print_list_rg(proc->mm->mmap->vm_freerg_list);
   print_pgtbl(proc, 0, -1);
   return stat;
 }
 
 /*libfree - PAGING-based free a region memory
  *@proc: Process executing the instruction
  *@size: allocated size
  *@reg_index: memory region ID (used to identify variable in symbole table)
  */
 
 int libfree(struct pcb_t *proc, uint32_t reg_index)
 {
   /* TODO Implement free region */
 
   /* By default using vmaid = 0 */
   int stat = __free(proc, 0, reg_index);
   //printf("Freed region %u, return status: %d\n",reg_index,stat);
   //printf("Free_rg_lst\n");
   print_list_rg(proc->mm->mmap->vm_freerg_list);
   print_pgtbl(proc, 0, -1);
   return stat;
 }
 
 /*pg_getpage - get the page in ram
  *@mm: memory region
  *@pagenum: PGN
  *@framenum: return FPN
  *@caller: caller
  *
  */
 int pg_getpage(struct mm_struct *mm, int pgn, int *fpn, struct pcb_t *caller)
 {
   /* Check the validation */
   if (mm == NULL || pgn < 0 || pgn > PAGING_MAX_PGN || fpn == NULL || caller == NULL)
     return -1;
 
   uint32_t pte = mm->pgd[pgn];
 
   if (!PAGING_PAGE_PRESENT(pte))
   { /* Page is not online, make it actively living */
     int vicpgn, swpfpn;
     int vicfpn;
     // uint32_t vicpte;
 
     int tgtfpn = PAGING_PTE_SWP(pte); // the target frame storing our variable
 
     /* TODO: Play with your paging theory here */
     /* Find victim page */
     if (find_victim_page(caller->mm, &vicpgn) == -1)
       return -1;                          // No victim page found
     vicfpn = PAGING_FPN(mm->pgd[vicpgn]); // we need to swap this frame out
 
     /* Get free frame in MEMSWP */
     if (MEMPHY_get_freefp(caller->active_mswp, &swpfpn) == -1)
       return -1; // No free frame in MEMSWP
 
     /* TODO: Implement swap frame from MEMRAM to MEMSWP and vice versa*/
 
     /* TODO copy victim frame to swap
      * SWP(vicfpn <--> swpfpn)
      * SYSCALL 17 sys_memmap
      * with operation SYSMEM_SWP_OP
      * we do this syscall to copy the content of the victim frame to the swap frame
      */
     struct sc_regs regs;
     regs.a1 = SYSMEM_SWP_OP;
     regs.a2 = vicfpn;
     regs.a3 = swpfpn;
 
     /* SYSCALL 17 sys_memmap */
     if (syscall(caller, 17, &regs) != 0)
     {
       return -1; // syscall failed
     }
 
     /* TODO copy target frame from swap to mem
      * SWP(tgtfpn <--> vicfpn)
      * SYSCALL 17 sys_memmap
      * with operation SYSMEM_SWP_OP
      * we do this syscall to copy the content of the target frame to the victim frame
      */
     // regs.a1 = SYSMEM_SWP_OP;
     // regs.a2 = tgtfpn;
     // regs.a3 = vicfpn;
 
     // /* SYSCALL 17 sys_memmap */
     // if (syscall(caller, 17, &regs) != 0) {
     //   return -1; // syscall failed
     // }
 
     // Don't have any choices
     __swap_cp_page(caller->active_mswp, tgtfpn, caller->mram, vicfpn);
 
     /* Update page table */
     pte_set_swap(&mm->pgd[vicpgn], caller->active_mswp_id, swpfpn); // update the victim page table to swap out
 
     /* Update its online status of the target page */
     pte_set_fpn(&mm->pgd[pgn], vicfpn);
     // mm->pgd[pgn];
     // pte_set_fpn();
 
     enlist_pgn_node(&caller->mm->fifo_pgn, pgn);
   }
 
   *fpn = PAGING_FPN(mm->pgd[pgn]);
 
   return 0;
 }
 
 /*pg_getval - read value at given offset
  *@mm: memory region
  *@addr: virtual address to acess
  *@data: return value
  *@caller: caller
  */
 int pg_getval(struct mm_struct *mm, int addr, BYTE *data, struct pcb_t *caller)
 {
   int pgn = PAGING_PGN(addr);
   int off = PAGING_OFFST(addr);
   int fpn;
 
   /* Get the page to MEMRAM, swap from MEMSWAP if needed */
   if (pg_getpage(mm, pgn, &fpn, caller) != 0)
     return -1; /* invalid page access */
 
   /* TODO
    *  MEMPHY_read(caller->mram, phyaddr, data);
    *  MEMPHY READ
    *  SYSCALL 17 sys_memmap with SYSMEM_IO_READ
    */
   int phyaddr = fpn * PAGING_PAGESZ + off;
   struct sc_regs regs;
   regs.a1 = SYSMEM_IO_READ;
   regs.a2 = phyaddr;
   regs.a3 = -1;
 
   /* SYSCALL 17 sys_memmap */
   if (syscall(caller, 17, &regs) != 0)
   {
     return -1; // syscall failed
   }
 
   // Update data
   *data = (BYTE)regs.a3;
 
   return 0;
 }
 
 /*pg_setval - write value to given offset
  *@mm: memory region
  *@addr: virtual address to acess
  *@value: value
  *
  */
 int pg_setval(struct mm_struct *mm, int addr, BYTE value, struct pcb_t *caller)
 {
   int pgn = PAGING_PGN(addr);
   int off = PAGING_OFFST(addr);
   int fpn;
 
   /* Get the page to MEMRAM, swap from MEMSWAP if needed */
   if (pg_getpage(mm, pgn, &fpn, caller) != 0)
     return -1; /* invalid page access */
 
   /* TODO
    *  MEMPHY_write(caller->mram, phyaddr, value);
    *  MEMPHY WRITE
    *  SYSCALL 17 sys_memmap with SYSMEM_IO_WRITE
    */
   int phyaddr = fpn * PAGING_PAGESZ + off;
   struct sc_regs regs;
   regs.a1 = SYSMEM_IO_WRITE;
   regs.a2 = phyaddr;
   regs.a3 = value;
 
   /* SYSCALL 17 sys_memmap */
   if (syscall(caller, 17, &regs) != 0)
   {
     return -1; // syscall failed
   }
 
   // Update data
   // data = (BYTE)
 
   return 0;
 }
 
 /*__read - read value in region memory
  *@caller: caller
  *@vmaid: ID vm area to alloc memory region
  *@offset: offset to acess in memory region
  *@rgid: memory region ID (used to identify variable in symbole table)
  *@size: allocated size
  *
  */
 int __read(struct pcb_t *caller, int vmaid, int rgid, int offset, BYTE *data)
 {
   /* Lock */
   pthread_mutex_lock(&mmvm_lock);
 
   struct vm_rg_struct *currg = get_symrg_byid(caller->mm, rgid);
   struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, vmaid);
 
   if (currg == NULL || cur_vma == NULL)
   { /* Invalid memory identify */
     pthread_mutex_unlock(&mmvm_lock);
     return -1;
   }
 
   pg_getval(caller->mm, currg->rg_start + offset, data, caller);
   pthread_mutex_unlock(&mmvm_lock);
 
   return 0;
 }
 
 /*libread - PAGING-based read a region memory */
 int libread(
     struct pcb_t *proc, // Process executing the instruction
     uint32_t source,    // Index of source register
     uint32_t offset,    // Source address = [source] + [offset]
     uint32_t *destination)
 {
   BYTE data;
   int val = __read(proc, 0, source, offset, &data);
 
   if (val != 0)
     return -1; // read failed
 
   /* TODO update result of reading action*/
   *destination = (uint32_t)data;
 #ifdef IODUMP
   printf("read region=%d offset=%d value=%d\n", source, offset, data);
 #ifdef PAGETBL_DUMP
   print_pgtbl(proc, 0, -1); // print max TBL
 #endif
   MEMPHY_dump(proc->mram);
 #endif
 
   return val;
 }
 
 /*__write - write a region memory
  *@caller: caller
  *@vmaid: ID vm area to alloc memory region
  *@offset: offset to acess in memory region
  *@rgid: memory region ID (used to identify variable in symbole table)
  *@size: allocated size
  *
  */
 int __write(struct pcb_t *caller, int vmaid, int rgid, int offset, BYTE value)
 {
   /* Lock */
   pthread_mutex_lock(&mmvm_lock);
 
   struct vm_rg_struct *currg = get_symrg_byid(caller->mm, rgid);
   struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, vmaid);
 
   if (currg == NULL || cur_vma == NULL)
   { /* Invalid memory identify */
     pthread_mutex_unlock(&mmvm_lock);
     return -1;
   }
 
   pg_setval(caller->mm, currg->rg_start + offset, value, caller);
   pthread_mutex_unlock(&mmvm_lock);
 
   return 0;
 }
 
 /*libwrite - PAGING-based write a region memory */
 int libwrite(
     struct pcb_t *proc,   // Process executing the instruction
     BYTE data,            // Data to be wrttien into memory
     uint32_t destination, // Index of destination register
     uint32_t offset)
 {
 #ifdef IODUMP
   printf("write region=%d offset=%d value=%d\n", destination, offset, data);
 #ifdef PAGETBL_DUMP
   print_pgtbl(proc, 0, -1); // print max TBL
 #endif
   MEMPHY_dump(proc->mram);
 #endif
 
   return __write(proc, 0, destination, offset, data);
 }

 /*free_pbc_mem - collect all memphy of pcb and free its memvir
  *@caller: caller*/
 void free_pbc_mem(struct pcb_t* proc){
   free_pcb_memph(proc);
   if(proc->page_table) free(proc->page_table);
   if(proc->code){
    if(proc->code->text) free(proc->code->text);
    free(proc->code);
  }
   if(!proc->mm) return;
   if(proc->mm->pgd) free(proc->mm->pgd);
   struct pgn_t *pgn = proc->mm->fifo_pgn;
   while(pgn){
    struct pgn_t *tmp = pgn;
    pgn = pgn->pg_next;
    free(tmp);
   }
   struct vm_area_struct *vma = proc->mm->mmap;
   while(vma){
    struct vm_rg_struct *free_lst = vma->vm_freerg_list;
    while(free_lst){
      struct vm_rg_struct *free_lst_tmp = free_lst;
      free_lst = free_lst->rg_next;
      free(free_lst_tmp);
    }
    struct vm_area_struct *tmp = vma;
    vma = vma->vm_next;
    free(tmp);
   }
   free(proc->mm);
   //anything else to free?
 }

 /*free_pcb_memphy - collect all memphy of pcb
  *@caller: caller
  *@vmaid: ID vm area to alloc memory region
  *@incpgnum: number of page
  */
 int free_pcb_memph(struct pcb_t *caller)
 {
   int pagenum, fpn;
   uint32_t pte;
 
   for (pagenum = 0; pagenum < PAGING_MAX_PGN; pagenum++)
   {
     pte = caller->mm->pgd[pagenum];

     if (PAGING_PAGE_PRESENT(pte))
     {
       fpn = PAGING_PTE_FPN(pte);
       MEMPHY_put_freefp(caller->mram, fpn);
     }
     else
     {
       fpn = PAGING_PTE_SWP(pte);
       if(fpn == 0) continue; //handle uninit pte;
       MEMPHY_put_freefp(caller->active_mswp, fpn);
     }
   }
 
   return 0;
 }
 
 /*find_victim_page - find victim page
  *@caller: caller
  *@pgn: return page number
  *
  */
 int find_victim_page(struct mm_struct *mm, int *retpgn)
 {
   if (mm == NULL || retpgn == NULL)
     return -1;
 
   if (mm->fifo_pgn == NULL)
     return -1;
 
   struct pgn_t *pg = mm->fifo_pgn;
   struct pgn_t *prev = NULL;
 
   /* TODO: Implement the theorical mechanism to find the victim page
    * As we add new pages to the head of the list, the victim page should be at the end of the list.
    */
   while (pg->pg_next) {
     prev = pg;
     pg = pg->pg_next;
   }
 
   *retpgn = pg->pgn;
   if (prev) {
     prev->pg_next = NULL;
   } else {
     mm->fifo_pgn = NULL; // No pages left.
   }
 
   free(pg);
 
   return 0;
 }
 
 /*get_free_vmrg_area - get a free vm region
  *@caller: caller
  *@vmaid: ID vm area to alloc memory region
  *@size: allocated size
  *@newrg: return new region
  */
 int get_free_vmrg_area(struct pcb_t *caller, int vmaid, int size, struct vm_rg_struct *newrg, int *num_mapped_pages)
 {
   struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, vmaid);
 
   struct vm_rg_struct *rgit = cur_vma->vm_freerg_list;
 
   if (rgit == NULL)
     return -1;
 
   /* Probe unintialized newrg */
   newrg->rg_start = newrg->rg_end = -1;
   int rg_size;
 
    int inc_sz = PAGING_PAGE_ALIGNSZ(size);

   /* TODO Traverse on list of free vm region to find a fit space */
   while (rgit) {
     rg_size = rgit->rg_end - rgit->rg_start;
     if (rg_size >= inc_sz) {
       newrg->rg_start = rgit->rg_start;
       newrg->rg_end = rgit->rg_start + inc_sz;
       break;
     }
     rgit = rgit->rg_next;
   }
 
   if (!rgit)
     return -1; // No fit region found
 
   /* Free the region in vm_freerg_list */
   delist_vm_freerg_list(caller->mm, rgit);
   *num_mapped_pages = inc_sz / PAGING_PAGESZ;
   
 
   if (rg_size > inc_sz) { // allocate more than need
     struct vm_rg_struct *newrg2 = malloc(sizeof(struct vm_rg_struct));
     newrg2->rg_start = newrg->rg_end;
     newrg2->rg_end = rgit->rg_end;
     newrg2->rg_next = NULL;
 
     /* Enlist the new region */
     enlist_vm_freerg_list(caller->mm, newrg2);
   }
   free(rgit);
   return 0;
 }
 
 // #endif