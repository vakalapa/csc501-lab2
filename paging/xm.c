/* xm.c = xmmap xmunmap */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>


/*-------------------------------------------------------------------------
 * xmmap - xmmap
 *-------------------------------------------------------------------------
 */
SYSCALL xmmap(int virtpage, bsd_t source, int npages)
{
  /* sanity check ! */

  if ( (virtpage < 4096) || ( source < 0 ) || ( source > MAX_ID) ||(npages < 1) || ( npages >200)){
	kprintf("xmmap call error: parameter error! \n");
	return SYSERR;
  }

  bs_map_t *bsm_entry;
  bsm_entry = &bsm_tab[source];

  if(bsm_entry->bs_sem == 1 || bsm_entry->bs_ispriv == 1){	
  	kprintf("xmmap call error: source being used by other process! \n");	
	return SYSERR;	
  }	

  if(bsm_entry->bs_status == BSM_UNMAPPED){	
  	kprintf("xmmap call error: source %d not mapped! \n",source);
	return SYSERR;	
  }		

  bsm_entry->bs_pid = currpid;
  bsm_entry->bs_ispriv = 0;	
  bsm_entry->bs_npages = npages;
  bsm_entry->bs_vpno = virtpage;
  
  return OK;
}



/*-------------------------------------------------------------------------
 * xmunmap - xmunmap
 *-------------------------------------------------------------------------
 */
SYSCALL xmunmap(int virtpage )
{
  /* sanity check ! */
  if ( (virtpage < 4096) ){ 
	kprintf("xmummap call error: virtpage (%d) invalid! \n", virtpage);
	return SYSERR;
  }

  int i,j; 
  int frm_vpno,bs_vpno,bs_npages;

  for(i=0;i<NUM_PAGES_PER_BS;i++){
	bs_vpno=bsm_tab[i].bs_vpno;	
	bs_npages = bsm_tab[i].bs_npages;	
	
	for(j=0;j<NFRAMES;j++)	{		
		if(frm_tab[j].fr_pid==currpid){		
			frm_vpno = frm_tab[j].fr_vpno;	
			if(frm_vpno>=bs_vpno && frm_vpno<bs_vpno+bs_npages)	{	
				write_bs((j+FRAME0)*NBPG,i,frm_vpno-bs_vpno);
			}		
		}		
	}  		

	bsm_tab[i].bs_sem = 0;	
	bsm_tab[i].bs_pid = -1;		
	bsm_tab[i].bs_vpno = BASE_VPAGE_NUM;	
	bsm_tab[i].bs_npages = 0;	
	break;
  }

  
  return OK;
}

