/* frame.c - manage physical frames */
#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>

/*-------------------------------------------------------------------------
 * init_frm - initialize frm_tab
 *-------------------------------------------------------------------------
 */

SYSCALL init_frm()
{
/*
Initializing all the required frame tabs. Called in initialize
*/
	STATWORD ps;	
    disable(ps);

  int i=0;

  for(i=0;i<NBPG;i++){
	frm_tab[i].fr_status= FRM_UNMAPPED;
	frm_tab[i].fr_pid= -1;
	frm_tab[i].fr_vpno= VPNO_BASE;
	frm_tab[i].fr_refcnt= 0;
	frm_tab[i].fr_type= INVALID;
	frm_tab[i].fr_dirty= 0;
	frm_tab[i].fr_loadtime= 0;
	
  }
  kprintf("in init frame!\n");

  enable(ps);
  return OK;
}

/*-------------------------------------------------------------------------
 * get_frm - get a free frame according page replacement policy
 *-------------------------------------------------------------------------
 */
SYSCALL get_frm(int* avail)
{
  int i;

  for(i=0;i<NBPG;i++){
	if(frm_tab[i].fr_status == FRM_UNMAPPED){
		*avail=i;
		return OK;
	}
  }

  if(page_replace_policy == FIFO){
	printf("** get_frm FIFO coming soon**\n");
	return SYSERR;
  }else if(page_replace_policy == LRU){
	  printf("** get_frm LRU coming soon**\n");
	return SYSERR;
  }
  return OK;
}

/*-------------------------------------------------------------------------
 * free_frm - free a frame 
 *-------------------------------------------------------------------------
 */
SYSCALL free_frm(int i)
{

  kprintf("To be implemented!\n");
  return OK;
}



