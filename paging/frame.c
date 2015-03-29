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
  int i = frm_num;	
  int vp;	
  unsigned long addr_value;	
  virt_addr_t *a;	
  unsigned int p,q;
  
  /* p is the pd offset while q is the pt offset */

  int pid;	
  unsigned long pd;

  /* pdbr of the process holding this frame */	
  pd_t *pd_p;
  pt_t *pt;

  int bs=0,bs_pgoffset=0;

  /* To write back the frame to bs if dirty */	
  /* Using the inverted page table, get vp, 	the virtual page number of the page to be replaced */	
  vp = frm_tab[i].fr_vpno;	

  /* Let a be vp*4096 (the first virtual address on page vp).*/	
  addr_value = (vp*NBPG);	
  a = &addr_value;
  #if 0	
  kprintf("\n\n\t[%s:%d]Virtual Page Number = %d Index = %d ADDR_VALUE=%u A=%u\n\n",__FILE__,__LINE__,vp,i,addr_value,*a);
  #endif	

  /* Let p be the high 10 bits of a. */	
  p = a->pd_offset;	

  /*Let q be bits [21:12] of a. */	
  q = a->pt_offset;

  /* Let pid be the pid of the process owning vp. */	
  pid = frm_tab[i].fr_pid;	

  /* Let pd point to the page directory of process pid. */	
  pd = proctab[pid].pdbr;
  #if 0	
  kprintf("\n\n\t[%s:%d]PID= %d PDBR = %u P=%d\n\n",__FILE__,__LINE__,pid,pd,p);
  #endif	

  /* Let pt point to the pid's pth page table. */	
  pd_p = pd+p*sizeof(pd_t);
  pt = (pd_p->pd_base*NBPG)+q*sizeof(pt_t);

  /* Mark the appropriate entry of pt as not present. */
  pt->pt_pres = 0;

  //	kprintf("\n\n\t Frame %d was previously for Pentry %d and pd_base=%d\n",i,q,pd_p->pd_base);	

  frm_invalidate_TLB(i);	

  /* In the inverted page table decrement the reference count of the frame occupied by pt. */	

  frm_tab[pd_p->pd_base-FRAME0].fr_refcnt--;		
  /* If the reference count has reached zero mark the appropriate entry in pd as being not present. */	
  /* This conserves frames by keeping only page tables which are necessary.*/
  if(frm_tab[pd_p->pd_base-FRAME0].fr_refcnt==0){		
  	pd_p->pd_pres = 0;	
}	
  /* If the dirty bit for page vp was set in its page table */	
  if(pt->pt_dirty == 1)	{	
  	/* Using the backing store map find the store and page offset within store given pid and a. */		
	if(bsm_lookup(pid,(vp*NBPG),&bs,&bs_pgoffset)==SYSERR)		{	
		kill(pid);			
		return SYSERR;		
	}		

	/* Write the page back to the backing store. */		
	write_bs((i+FRAME0)*NBPG,bs,bs_pgoffset);	

  }
  //kprintf("To be implemented!\n");
  return OK;
}

void frm_invalidate_TLB(int frm_num){	
	int pid,i,j;	
	struct pentry *pptr;
	unsigned long pdbr;	
	pd_t *pd_p;	
	pt_t *pt_p;	
	int bs;	
	int frame;	

	pid = frm_tab[frm_num].fr_pid;	
	if(pid == currpid)		
		write_cr3(read_cr3());

}

SYSCALL evict_process_frames(int pid){	
	STATWORD ps;	

	int i =0;	
	int bs,vpno;

	if(pid <= 0)		
		return SYSERR;		

	disable(ps);
	for(i=0;i<NFRAMES;i++)	{	
		if(frm_tab[i].fr_pid == pid){		
			if(frm_tab[i].fr_type == FR_PAGE)	{	
				bs = proctab[pid].store;				
				vpno = frm_tab[i].fr_vpno-bsm_tab[bs].bs_vpno;		
				write_bs((FRAME0+i)*NBPG,bs,vpno);			

			}			

			frm_tab[i].fr_pid = -1;		
			frm_tab[i].fr_status = FRM_UNMAPPED;	
			frm_tab[i].fr_type = INVALID;		
			frm_tab[i].fr_refcnt = 0;		
			frm_tab[i].fr_loadtime = 0;		
			frm_tab[i].fr_dirty = 0;	
			frm_tab[i].fr_vpno = 0;		

		}	

	}	

	restore(ps);	
	return OK;

}





