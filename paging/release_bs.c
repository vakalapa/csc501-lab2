#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>

SYSCALL release_bs(bsd_t bs_id) {

  /* release the backing store with ID bs_id */
  if(bs_id<0|| bs_id == NULL)
  	return SYSERR;
  bs_map_t *bsm_entry;	  
  bsm_entry = &bsm_tab[bs_id];	 
  
  bsm_entry->bs_npages = 0;   
  bsm_entry->bs_ispriv = 0;   
  bsm_entry->bs_status = BSM_UNMAPPED;	  
  bsm_entry->bs_sem = 0;
  
  return OK;
}

