#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>

int get_bs(bsd_t bs_id, unsigned int npages) {

  /* requests a new mapping of npages with ID map_id */
	STATWORD ps;
    disable(ps);

	bs_map_t *bsMap;

	bsMap=bsm_tab[bs_id];

	if(npages<=0 || npages>128 || bs_id < 0 || bs_id >= MAX_ID){
		restore(ps);
		return SYSERR;
	}

	if(bsMap.bs_status== BSM_MAPPED){
		if(bsMap.bs_ispriv==1|| bsMap.bs_sem==1){
			printf("**** Accessing private heap\n*****");
			restore(ps);
			return SYSERR;
		}else{
			bsMap->bs_pid = currpid;	
			bsMap->bs_npages = npages;	
			bsMap->bs_ispriv = 0;		
			bsMap->bs_status = BSM_MAPPED;		
			restore(ps);		
			return bsMap->bs_npages;

		}
	}
	else{
		bsMap->bs_pid = currpid;	
		bsMap->bs_npages = npages;	
		bsMap->bs_ispriv = 0;		
		bsMap->bs_status = BSM_MAPPED;		
		restore(ps);		
		return bsMap->bs_npages;

	}
	
    return npages;

}


