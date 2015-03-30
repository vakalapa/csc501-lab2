#include <conf.h>
#include <i386.h>
#include <kernel.h>
#include <proc.h>
#include <sem.h>
#include <mem.h>
#include <io.h>
#include <paging.h>


int get_bs(bsd_t bs_id, unsigned int npages) {

	STATWORD ps;
	disable(ps);

	bs_map_t *bsm_entry;
 	bsm_entry = &bsm_tab[bs_id];

	if(npages<=0 || npages>128 || bs_id < 0 || bs_id >= MAX_ID)
	{
		restore(ps);
		return SYSERR;
	}

  /* requests a new mapping of npages with ID map_id */
	if(bsm_entry->bs_status == BSM_MAPPED)
	{
		if(bsm_entry->bs_ispriv == 1 || bsm_entry->bs_sem == 1)
		{
			restore(ps);
			return SYSERR;
		}

		else
		{
//		kprintf("\n\n\t[GET_BS]IF-ELSE BSM = %d\n\n",bs_id);
			bsm_entry->bs_pid = currpid;
			bsm_entry->bs_npages = npages;
			bsm_entry->bs_ispriv = 0;
			bsm_entry->bs_status = BSM_MAPPED;
			restore(ps);
			return bsm_entry->bs_npages;
		}
	}

	else
	{
//	kprintf("\n\n\t[GET_BS]ELSE BSM = %d\n\n",bs_id);
		bsm_entry->bs_pid = currpid;
		bsm_entry->bs_npages = npages;
		bsm_entry->bs_ispriv = 0;
		bsm_entry->bs_status = BSM_MAPPED;
		restore(ps);
		return bsm_entry->bs_npages;
	}
  
}


