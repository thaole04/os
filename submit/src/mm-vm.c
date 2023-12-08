// #ifdef MM_PAGING
/*
 * PAGING based Memory Management
 * Virtual memory module mm/mm-vm.c
 */

#include "mm.h"
#include "string.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

// synchronized for vm
static pthread_mutex_t vm_lock = PTHREAD_MUTEX_INITIALIZER;

/*enlist_vm_freerg_list - add new rg to freerg_list
 *@mm: memory region
 *@rg_elmt: new region
 *
 */
int enlist_vm_freerg_list(struct mm_struct *mm, struct vm_rg_struct *rg_elmt) {
  struct vm_rg_struct *rg_node = mm->mmap->vm_freerg_list;

  if (rg_elmt->rg_start >= rg_elmt->rg_end)
    return -1;

  if (rg_node != NULL)
    rg_elmt->rg_next = rg_node;

  /* Enlist the new region */
  mm->mmap->vm_freerg_list = rg_elmt;

  return 0;
}

/*get_vma_by_num - get vm area by numID
 *@mm: memory region
 *@vmaid: ID vm area to alloc memory region
 *
 */
struct vm_area_struct *get_vma_by_num(struct mm_struct *mm, int vmaid) {
  struct vm_area_struct *pvma = mm->mmap;

  if (mm->mmap == NULL)
    return NULL;

  int vmait = pvma->vm_id;

  while (vmait < vmaid) {
    if (pvma == NULL)
      return NULL;

    pvma = pvma->vm_next;
    vmait = pvma->vm_id;
  }

  return pvma;
}

/*get_symrg_byid - get mem region by region ID
 *@mm: memory region
 *@rgid: region ID act as symbol index of variable
 *
 */
struct vm_rg_struct *get_symrg_byid(struct mm_struct *mm, int rgid) {
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
 
 // alloc : DONE
int __alloc(struct pcb_t *caller, int vmaid, int rgid, int size,
            int *alloc_addr) {
  /*Allocate at the toproof */
  struct vm_rg_struct rgnode;

  size = PAGING_PAGE_ALIGNSZ(size);

  if (size <= 0) {
    return -1;
  }

  pthread_mutex_lock(&vm_lock);

  if (get_free_vmrg_area(caller, vmaid, size, &rgnode) == 0) {

    caller->mm->symrgtbl[rgid].rg_start = rgnode.rg_start;
    caller->mm->symrgtbl[rgid].rg_end = rgnode.rg_end;
    caller->mm->symrgtbl[rgid].is_alloc = 1;

    *alloc_addr = rgnode.rg_start;

    pthread_mutex_unlock(&vm_lock);
    return 0;
  }

  /* TODO get_free_vmrg_area FAILED handle the region management (Fig.6)*/

  /*Attempt to increate limit to get space */
  struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, vmaid);
  int old_sbrk;

  old_sbrk = cur_vma->sbrk;

  /* TODO INCREASE THE LIMIT
   */
  inc_vma_limit(caller, vmaid, size);

  /*Successful increase limit */
  caller->mm->symrgtbl[rgid].rg_start = old_sbrk;
  caller->mm->symrgtbl[rgid].rg_end = old_sbrk + size;
  caller->mm->symrgtbl[rgid].is_alloc = 1;

  *alloc_addr = old_sbrk;

  pthread_mutex_unlock(&vm_lock);
  return 0;
}

/*__free - remove a region memory
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@rgid: memory region ID (used to identify variable in symbole table)
 *@size: allocated size
 *
 */
 
 // __free : DONE
int __free(struct pcb_t *caller, int vmaid, int rgid) {
  if (rgid < 0 || rgid > PAGING_MAX_SYMTBL_SZ) {
    return -1;
  }
  /* TODO: Manage the collect freed region to freerg_list */

  // Get the region need to be freed
  struct vm_rg_struct *dealloc_rg = get_symrg_byid(caller->mm, rgid);

  if (dealloc_rg->is_alloc != 1) {
    printf("Unable to delocated memory region %d\n", rgid);
    printf("This memory region has not been allocated yet !!\n"); 
    return -1;
  }

  if (dealloc_rg->rg_start == dealloc_rg->rg_end) {
    return -1;
  }

  pthread_mutex_lock(&vm_lock);

  struct vm_rg_struct *rgnode = malloc(sizeof(struct vm_rg_struct));

  // Assign the rgnode with appropriate values
  rgnode->rg_start = dealloc_rg->rg_start;
  rgnode->rg_end = dealloc_rg->rg_end;
  rgnode->rg_next = NULL;
  
  // free region
  dealloc_rg->rg_start = 0;
  dealloc_rg->rg_end = 0;
  dealloc_rg->is_alloc = 0;

  /*enlist the obsoleted memory region */
  enlist_vm_freerg_list(caller->mm, rgnode);

  pthread_mutex_unlock(&vm_lock);

  return 0;
}

/*pgalloc - PAGING-based allocate a region memory
 *@proc:  Process executing the instruction
 *@size: allocated size
 *@reg_index: memory region ID (used to identify variable in symbole table)
 */
int pgalloc(struct pcb_t *proc, uint32_t size, uint32_t reg_index) {
  int addr;

  /* By default using vmaid = 0 */
  return __alloc(proc, 0, reg_index, size, &addr);
}

/*pgfree - PAGING-based free a region memory
 *@proc: Process executing the instruction
 *@size: allocated size
 *@reg_index: memory region ID (used to identify variable in symbole table)
 */

int pgfree_data(struct pcb_t *proc, uint32_t reg_index) {
  return __free(proc, 0, reg_index);
}

/*pg_getpage - get the page in ram
 *@mm: memory region
 *@pagenum: PGN
 *@framenum: return FPN
 *@caller: caller
 *
 */
 // pg_getpage : done ---> go to find_victim_page function
int pg_getpage(struct mm_struct *mm, int pgn, int *fpn, struct pcb_t *caller) {
  uint32_t pte = mm->pgd[pgn];

  if (!PAGING_PAGE_PRESENT(pte)) 
  { /* Page is not online, make it actively living */
    int vicpgn, swpfpn;
    int vicfpn;
    //uint32_t vicpte;

    int tgtfpn = PAGING_SWP(pte); // the target frame storing our variable

    /* TODO: Play with your paging theory here */
    /* Find victim page */
    struct framephy_struct *vicfp =
        (struct framephy_struct *)malloc(sizeof(struct framephy_struct));
    
    // return -1 in case we don't have victim page
    if (find_victim_page(caller, &vicfp) == -1)
      return -1;

    /*
    vicpgn = victim_fp->pte_id;
    vicpte = victim_fp->owner->pgd[vicpgn];
    vicfpn = PAGING_FPN(vicpte);
    */
    
    vicpgn = vicfp -> pte_id; 
    vicfpn = vicfp -> fpn; 
    /* Get free frame in MEMSWP */
    if (MEMPHY_get_freefp(caller->active_mswp, &swpfpn) == -1)
      return -1;

    /* Do swap frame from MEMRAM to MEMSWP and vice versa*/
    /* Copy victim frame to swap */
    __swap_cp_page(vicfp->mapping_process->mram, vicfpn, caller->active_mswp,
                   swpfpn);
    /* Copy target frame from swap to mem */
    __swap_cp_page(caller->active_mswp, tgtfpn, caller->mram, vicfpn);

    /* Update page table */
    // victim page in swap memo, new frame number of victim page is swap's frame number
    pte_set_swap(&vicfp->owner->pgd[vicpgn], 0, swpfpn);

    // TODO: check pte_set_fpn
    /* Update its online status of the target page */
    // frame number of target page is the frame number of victim page
    pte_set_fpn(&mm->pgd[pgn], vicfpn);

    *fpn = PAGING_FPN(pte);

    enlist_pgn_node(&caller->mm->fifo_pgn, pgn);

    enlist_fpn_node(&caller->mram->used_fp_list, *fpn, caller->mm, pgn, caller);
  }

  // get frame number to read or write value
  *fpn = PAGING_FPN(pte);

  return 0;
}

/*pg_getval - read value at given offset
 *@mm: memory region
 *@addr: virtual address to acess
 *@value: value
 *
 */
 
 // pg_getval : done ---> go to pg_getpage function
int pg_getval(struct mm_struct *mm, int addr, BYTE *data,
              struct pcb_t *caller) {
  int pgn = PAGING_PGN(addr);
  int off = PAGING_OFFST(addr);
  int fpn;

  /* Get the page to MEMRAM, swap from MEMSWAP if needed */
  if (pg_getpage(mm, pgn, &fpn, caller) != 0)
    return -1; /* invalid page access */

  int phyaddr = (fpn << PAGING_ADDR_FPN_LOBIT) + off;

  MEMPHY_read(caller->mram, phyaddr, data);

  return 0;
}

/*pg_setval - write value to given offset
 *@mm: memory region
 *@addr: virtual address to acess
 *@value: value
 *
 */
 
 // pg_setval : done ---> go to pg_getpage function
int pg_setval(struct mm_struct *mm, int addr, BYTE value,
              struct pcb_t *caller) {
  int pgn = PAGING_PGN(addr);
  int off = PAGING_OFFST(addr);
  int fpn;

  /* Get the page to MEMRAM, swap from MEMSWAP if needed */
  if (pg_getpage(mm, pgn, &fpn, caller) != 0)
    return -1; /* invalid page access */

  int phyaddr = (fpn << PAGING_ADDR_FPN_LOBIT) + off;

  MEMPHY_write(caller->mram, phyaddr, value);

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
 
 // __read :    --> go to pg_getval function
int __read(struct pcb_t *caller, int vmaid, int rgid, int offset, BYTE *data) {

  struct vm_rg_struct *currg = get_symrg_byid(caller->mm, rgid);

  if (!currg->is_alloc) {
    /*printf("process %d access violation reading location: memory region %d\n",
           caller->pid, rgid);*/
    return -1;
  }

  struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, vmaid);

  if (currg == NULL || cur_vma == NULL) /* Invalid memory identify */
  {
    return -1;
  }

  pthread_mutex_lock(&vm_lock);

  pg_getval(caller->mm, currg->rg_start + offset, data, caller);

  pthread_mutex_unlock(&vm_lock);

  return 0;
}

/*pgwrite - PAGING-based read a region memory */
int pgread(struct pcb_t *proc, // Process executing the instruction
           uint32_t source,    // Index of source register
           uint32_t offset,    // Source address = [source] + [offset]
           uint32_t destination) {
  BYTE data;

  int val = __read(proc, 0, source, offset, &data);

  if (val == -1)
    return -1;

  destination = (uint32_t)data;
#ifdef IODUMP
  printf("process %d read region=%d offset=%d value=%d\n\n", proc->pid, source,
         offset, data);
  print_pgtbl(proc, 0, -1); // print max TBL
#ifdef MEMPHYS_DUMP
  MEMPHY_dump(proc->mram);
#endif
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
int __write(struct pcb_t *caller, int vmaid, int rgid, int offset, BYTE value) {

  struct vm_rg_struct *currg = get_symrg_byid(caller->mm, rgid);

  if (!currg->is_alloc) {
    printf("access violation writing location: memory region %d\n", rgid);
    return -1;
  }

  struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, vmaid);

  if (currg == NULL || cur_vma == NULL) /* Invalid memory identify */
  {
    return -1;
  }

  pthread_mutex_lock(&vm_lock);

  pg_setval(caller->mm, currg->rg_start + offset, value, caller);

  pthread_mutex_unlock(&vm_lock);
  return 0;
}

/*pgwrite - PAGING-based write a region memory */
int pgwrite(struct pcb_t *proc,   // Process executing the instruction
            BYTE data,            // Data to be wrttien into memory
            uint32_t destination, // Index of destination register
            uint32_t offset) {
#ifdef IODUMP
  printf("process %d write region=%d offset=%d value=%d\n\n", proc->pid,
         destination, offset, data);
#endif
  uint32_t max_offset = proc->mm->symrgtbl[destination].rg_end -
                        proc->mm->symrgtbl[destination].rg_start - 1;

  if (offset > max_offset) {
    printf("process %d access violation writing location: memory region %d\n",
           proc->pid, destination);
    return -1;
  }

  int status = __write(proc, 0, destination, offset, data);
  if (status != -1) {
    print_pgtbl(proc, 0, -1); // print max TBL
  }
#ifdef MEMPHYS_DUMP
  MEMPHY_dump(proc->mram);
#endif

  return status;
}

/*free_pcb_memphy - collect all memphy of pcb
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@incpgnum: number of page
 */
int free_pcb_memph(struct pcb_t *caller) {
  int pagenum, fpn;
  uint32_t pte;

  for (pagenum = 0; pagenum < PAGING_MAX_PGN; pagenum++) {
    pte = caller->mm->pgd[pagenum];

    if (!PAGING_PAGE_PRESENT(pte)) {
      fpn = PAGING_FPN(pte);
      MEMPHY_put_freefp(caller->mram, fpn);
    } else {
      fpn = PAGING_SWP(pte);
      MEMPHY_put_freefp(caller->active_mswp, fpn);
    }
  }

  return 0;
}

/*get_vm_area_node - get vm area for a number of pages
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@incpgnum: number of page
 *@vmastart: vma end
 *@vmaend: vma end
 *
 */
struct vm_rg_struct *get_vm_area_node_at_brk(struct pcb_t *caller, int vmaid,
                                             int size, int alignedsz) {
  struct vm_rg_struct *newrg;
  struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, vmaid);

  newrg = malloc(sizeof(struct vm_rg_struct));

  newrg->rg_start = cur_vma->sbrk;
  newrg->rg_end = newrg->rg_start + size;

  return newrg;
}

/*validate_overlap_vm_area
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@vmastart: vma end
 *@vmaend: vma end
 *
 */
 
 // overlap done
int validate_overlap_vm_area(struct pcb_t *caller, int vmaid, int vmastart,
                             int vmaend) {
  struct vm_area_struct *vma = caller->mm->mmap;
  // destination vma to check overlap
  struct vm_area_struct *dest_vma = get_vma_by_num(caller->mm, vmaid);

  /* TODO validate the planned memory area is not overlapped */

  if (vmastart > vmaend)
    return -1;

  while (vma != NULL) {
    // Check for overlap conditions
    if (vma != dest_vma){
    	if((vma->vm_start >= vmastart && vma->vm_end <= vmaend)  // internal overlap
    	|| (vma->vm_start <= vmastart && vma->vm_end >= vmaend)  // external overlap
    	|| (vma->vm_start <= vmastart && vmastart < vma->vm_end) // start address overlap
    	|| (vma->vm_start < vmaend && vmaend <= vma->vm_end)	 // end address overlap
    	){
    		//overlap detected
    		return -1; 
    	}      
    }
    vma = vma->vm_next; // Move to the next memory region
  }

  // No overlap detected, return 0 to indicate success
  return 0;
}

/*inc_vma_limit - increase vm area limits to reserve space for new variable
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@inc_sz: increment size
 *
 */
 
 // inc_vma_limit : done --> go to vm_map_ram in mm.c
 // having some change : old_end -> new sbrk become old_sbrk -> new sbrk
int inc_vma_limit(struct pcb_t *caller, int vmaid, int inc_sz) {
  struct vm_rg_struct *newrg = malloc(sizeof(struct vm_rg_struct));
  int inc_amt = PAGING_PAGE_ALIGNSZ(inc_sz);
  int incnumpage = inc_amt / PAGING_PAGESZ;
  struct vm_rg_struct *area =
      get_vm_area_node_at_brk(caller, vmaid, inc_sz, inc_amt);
  struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, vmaid);

  int old_end = cur_vma->vm_end;
  int old_sbrk = cur_vma->sbrk; 
  /*Validate overlap of obtained region */
  if (validate_overlap_vm_area(caller, vmaid, area->rg_start, area->rg_end) < 0) {
    fprintf(stderr, "error: Overlap detected when allocated memory region");
    return -1; /*Overlap and failed allocation */
  }

  /* The obtained vm area (only)
   * now will be alloc real ram region */
  cur_vma->vm_end += inc_sz;
  
  // added code : Update new sbrk follow the rate of vm_end
  cur_vma->sbrk += inc_sz; 
  if (vm_map_ram(caller, area->rg_start, area->rg_end, old_end, incnumpage,
                 newrg) < 0)
    return -1; /* Map the memory to MEMRAM */

  // assign new region to new memory of vm area
  //new_rg->rg_start = old_end;
  newrg->rg_start = old_sbrk;
  newrg->rg_end = cur_vma->sbrk;

  return 0;
}

/*find_victim_page - find victim page
 *@caller: caller
 *@pgn: return physical page
 *
 */
int find_victim_page(struct pcb_t *caller, struct framephy_struct **re_fp) {
  struct framephy_struct *fp_q = caller->mram->used_fp_list;
  struct framephy_struct *prev = NULL;
  struct vm_rg_struct *victim_rg = malloc(sizeof(struct vm_rg_struct));

  /* TODO: Implement the theorical mechanism to find the victim page */

  // check if free region list have frame
  if (!get_free_vmrg_area_online(caller, 0, victim_rg)) {
    int vicpgn = PAGING_PGN(victim_rg->rg_start);
    uint32_t vicpte = caller->mm->pgd[vicpgn];
    int vicfpn = PAGING_FPN(vicpte);

    // TODO: remove the node in used_fp_list match fpn = vicfpn then return that
    // node in re_fp
    while (fp_q != NULL) {
      if (fp_q->fpn == vicfpn) {
        // Remove the node from the list
        if (prev != NULL) {
          prev->fp_next = fp_q->fp_next;
        } else {
          caller->mram->used_fp_list = fp_q->fp_next;
        }

        // Set re_fp to the removed node
        *re_fp = fp_q;

        // Free the victim_rg memory
        free(victim_rg);

        return 0;
      }

      prev = fp_q;
      fp_q = fp_q->fp_next;
    }
  }

  if (fp_q == NULL) {
    // The FIFO queue is empty, which should not happen
    return -1;
  }

  // Traverse to the end of the FIFO queue to find the oldest page
  while (fp_q->fp_next != NULL) {
    prev = fp_q;
    fp_q = fp_q->fp_next;
  }

  // Get the page number of the oldest page
  *re_fp = fp_q;

  // Remove the oldest page from the FIFO queue
  if (prev != NULL) {
    prev->fp_next = NULL;
  } else {
    caller->mram->used_fp_list = NULL;
  }

  return 0;
}

/*get_free_vmrg_area - get a free vm region
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@size: allocated size
 *
 */
int get_free_vmrg_area(struct pcb_t *caller, int vmaid, int size,
                       struct vm_rg_struct *newrg) {
  struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, vmaid);

  struct vm_rg_struct *rgit = cur_vma->vm_freerg_list;

  if (rgit == NULL)
    return -1;

  /* Probe unintialized newrg */
  newrg->rg_start = newrg->rg_end = -1;

  /* Traverse on list of free vm region to find a fit space */
  while (rgit != NULL) {
    if (rgit->rg_start + size <=
        rgit->rg_end) { /* Current region has enough space */
      newrg->rg_start = rgit->rg_start;
      int inc_size = PAGING_PAGE_ALIGNSZ(size);
      newrg->rg_end = rgit->rg_start + inc_size;
      newrg->rg_next = NULL;

      /* Update left space in chosen region */
      if (rgit->rg_start + inc_size < rgit->rg_end) {
        rgit->rg_start = rgit->rg_start + inc_size;
      } else { /*Use up all space, remove current node */
        /*Clone next rg node */
        struct vm_rg_struct *nextrg = rgit->rg_next;

        /*Cloning */
        if (nextrg != NULL) {
          rgit->rg_start = nextrg->rg_start;
          rgit->rg_end = nextrg->rg_end;

          rgit->rg_next = nextrg->rg_next;

          free(nextrg);
        } else {                         /*End of free list */
          rgit->rg_start = rgit->rg_end; // dummy, inc_size 0 region
          rgit->rg_next = NULL;
        }
      }
      break;
    } else {
      rgit = rgit->rg_next; // Traverse next rg
    }
  }

  if (newrg->rg_start == -1) // new region not found
    return -1;

  return 0;
}

/*get_free_vmrg_area - get a free vm region in ram
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 */
int get_free_vmrg_area_online(struct pcb_t *caller, int vmaid,
                              struct vm_rg_struct *newrg) {
  struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, vmaid);

  struct vm_rg_struct *rgit = cur_vma->vm_freerg_list;

  int size = PAGING_PAGESZ;

  if (rgit == NULL)
    return -1;

  /* Probe unintialized newrg */
  newrg->rg_start = newrg->rg_end = -1;

  /* Traverse on list of free vm region to find a fit space */
  while (rgit != NULL) {
    if (rgit->rg_start + size <=
        rgit->rg_end) { /* Current region has enough space */

      if (!PAGING_PAGE_PRESENT(caller->mm->pgd[PAGING_PGN(rgit->rg_start)])) {
        int cur_pg_adr = rgit->rg_start;
        while (cur_pg_adr < rgit->rg_end) {
          cur_pg_adr = rgit->rg_start + size;
          int free_pgn = PAGING_PGN(cur_pg_adr);
          if (PAGING_PAGE_PRESENT(caller->mm->pgd[free_pgn])) {
            // current region have online page, so take frame from this region

            // split the region
            struct vm_rg_struct *split_rg = malloc(sizeof(struct vm_rg_struct));
            split_rg->rg_start = cur_pg_adr;
            split_rg->rg_end = rgit->rg_end;
            rgit->rg_end = cur_pg_adr;
            split_rg->rg_next = rgit->rg_next;
            //rgit->rg_next = split_rg->rg_next;
            rgit->rg_next = split_rg; 
          }
        }

        rgit = rgit->rg_next;
        continue;
      }

      // update new region
      newrg->rg_start = rgit->rg_start;
      int inc_size = PAGING_PAGE_ALIGNSZ(size);
      newrg->rg_end = rgit->rg_start + inc_size;
      newrg->rg_next = NULL;

      /* Update left space in chosen region */
      if (rgit->rg_start + inc_size < rgit->rg_end) {
        rgit->rg_start = rgit->rg_start + inc_size;
      } else { /*Use up all space, remove current node */
        /*Clone next rg node */
        struct vm_rg_struct *nextrg = rgit->rg_next;

        /*Cloning */
        if (nextrg != NULL) {
          rgit->rg_start = nextrg->rg_start;
          rgit->rg_end = nextrg->rg_end;

          rgit->rg_next = nextrg->rg_next;

          free(nextrg);
        } else {                         /*End of free list */
          rgit->rg_start = rgit->rg_end; // dummy, inc_size 0 region
          rgit->rg_next = NULL;
        }
      }
      break;
    } else {
      rgit = rgit->rg_next; // Traverse next rg
    }
  }

  if (newrg->rg_start == -1) // new region not found
    return -1;

  return 0;
}

// #endif
