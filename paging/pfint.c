/* pfint.c - pfint */

#include <conf.h>
#include <kernel.h>
#include <paging.h>
#include <proc.h>

extern int page_replace_policy;
int create_page_table();

/*-------------------------------------------------------------------------
 * pfint - paging fault ISR
 *-------------------------------------------------------------------------
 */
SYSCALL pfint()
{
	STATWORD ps;
	unsigned long v_addr;
	virt_addr_t *virt_addr; /* Virtual address of the page that gave page fault */
	unsigned int pg_offset;
	unsigned int pt_offset;
	unsigned int pd_offset;
	unsigned long pdbr; /* page directory of this process */
	pd_t *pd_entry; /* entry of the page directory */
	pt_t *pt_entry; /* entry of the page table */
	int new_pt; /* page number of the page table newly created */
	int new_frame; /* new frame to store the page */
	int bs; /* backing store */
	int pg_offset_bs; /* offset of the page in the bs */
	unsigned long *temp_bs_content;
	unsigned long *temp_fr_content;
	disable(ps);

	v_addr = read_cr2();

	virt_addr = (virt_addr_t*)&v_addr;

	pg_offset = virt_addr->pg_offset;
	pt_offset = virt_addr->pt_offset;
	pd_offset = virt_addr->pd_offset;


	pdbr = proctab[currpid].pdbr;

	pd_entry = pdbr+pd_offset*sizeof(pd_t); /* This points to the directory entry given by the virtual address */

	/* If the page fault was because a page table was not found,
	create a new page table and update the directory entry */
if(pd_entry->pd_pres ==0)
	{
		new_pt = create_page_table();
		if(new_pt == -1)
		{
			kill(currpid);
			restore(ps);
			return SYSERR;
		}
		pd_entry->pd_pres = 1;
		pd_entry->pd_write = 1;
		pd_entry->pd_user = 0;
		pd_entry->pd_pwt = 0;
		pd_entry->pd_pcd = 0;
		pd_entry->pd_acc = 0;
		pd_entry->pd_mbz = 0;
		pd_entry->pd_fmb = 0;
		pd_entry->pd_global = 0;
		pd_entry->pd_avail = 0;
		pd_entry->pd_base = new_pt+FRAME0;

		frm_tab[new_pt].fr_status = FRM_MAPPED;
		frm_tab[new_pt].fr_type = FR_TBL;
		frm_tab[new_pt].fr_pid = currpid;

	}
	/* If the page table entry is present */
/*	else
	{*/
		pt_entry = (pt_t*)(pd_entry->pd_base*NBPG+pt_offset*sizeof(pt_t));
		
		/* Page fault case b */
		if(pt_entry->pt_pres != 1)
		{
			if(bsm_lookup(currpid,v_addr,&bs,&pg_offset_bs)==SYSERR)
			{
				kill(currpid);
				return SYSERR;
			}
			/*bs = 0;
			pg_offset_bs = 0;*/
			temp_bs_content = BACKING_STORE_BASE+bs*NBPG;
			#if DEBUG_PAGING
				kprintf("\n\n\t[%s:%d] Trying to get a frame \n\n",__FILE__,__LINE__);
			#endif
			if(get_frm(&new_frame)==SYSERR)
			{
				kill(currpid);
				return SYSERR;
			}

			temp_fr_content =(FRAME0+new_frame)*NBPG; 

			pt_entry->pt_pres = 1;
			pt_entry->pt_write = 1;
			pt_entry->pt_base = (FRAME0+new_frame);


			/* Increment the refcount of the page table as there is a new page entry */
			frm_tab[pd_entry->pd_base-FRAME0].fr_refcnt++;

			frm_tab[new_frame].fr_status = FRM_MAPPED;
			frm_tab[new_frame].fr_type = FR_PAGE;
			frm_tab[new_frame].fr_pid = currpid;
			frm_tab[new_frame].fr_vpno = v_addr/NBPG;

			read_bs((char*)((FRAME0+new_frame)*NBPG),bs,pg_offset_bs);

			/* Insert frame into the fifo queue if it is a fifo policy */
			if(page_replace_policy == FIFO)
				insert_frm_fifo(new_frame);
			else if(page_replace_policy == LRU)
			{
//				 new_frame = get_frm_LRU();
//				kprintf(" \n\tPFINT: updating LRUs\n ");
				 update_frms_LRU();
			}
			

		}
//	}


	write_cr3(pdbr);
	restore(ps);
  	return OK;
}

int create_page_table()
{
	int i;
	int frame_number;
	unsigned int frame_addr;
	pt_t *page_table;

	if(get_frm(&frame_number)==SYSERR)
	{
		return -1;/* Frame could not be found */
	}

	frame_addr = get_frame_address(frame_number);

	page_table = (pt_t*)frame_addr;

	/* Fill the page table entries - each page table has 1024 entries */
	for(i=0;i<1024;i++)
	{
		page_table[i].pt_acc = 0;
		page_table[i].pt_avail = 0;
		page_table[i].pt_base = 0;
		page_table[i].pt_dirty = 0;
		page_table[i].pt_global = 0;
		page_table[i].pt_mbz = 0;
		page_table[i].pt_pcd = 0;
		page_table[i].pt_pres = 0;
		page_table[i].pt_pwt = 0;
		page_table[i].pt_user = 0;
		page_table[i].pt_write = 0;
	}
#if DEBUG_PAGING
	kprintf("\n\t[PFINT.C:136] Allocated frame %d for the page table\n",frame_number);
#endif
	return frame_number;
	
}

int get_frame_address(int frame_number){
	return (FRAME0 + frame_number)*NBPG;
}
