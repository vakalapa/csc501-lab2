/* frame.c - manage physical frames */
#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>

extern int page_replace_policy;
/* FIFO queue */
int frm_fifo_hd;
int frm_fifo_tl;

/* Time count for LRU */
unsigned long int timeCount;


/*-------------------------------------------------------------------------
 * init_frm - initialize frm_tab
 *-------------------------------------------------------------------------
 */
SYSCALL init_frm()
{

	STATWORD ps;
	disable(ps);
	int i = 0;

	frm_fifo_hd = INVALID_FRM;
	frm_fifo_tl = INVALID_FRM;
	for(i=0;i<NFRAMES;i++)
	{
		frm_tab[i].fr_status = FRM_UNMAPPED;
		frm_tab[i].fr_type = INVALID_FRM;
		frm_tab[i].fr_dirty = 0;
		frm_tab[i].fr_loadtime = 0;
		frm_tab[i].fr_pid = -1;
		frm_tab[i].fr_refcnt = 0;
		frm_tab[i].fr_vpno = BASE_VPAGE_NUM;
	}
	
	restore(ps);
	return OK;
}

/*-------------------------------------------------------------------------
 * get_frm - get a free frame according page replacement policy
 *-------------------------------------------------------------------------
 */
SYSCALL get_frm(int* avail)
{
	int i;

	for(i=0;i<NFRAMES;i++)
	{
		if(frm_tab[i].fr_status == FRM_UNMAPPED)
		{

			*avail = i;
			return OK;
		}
	}
	if(page_replace_policy == FIFO)
	{
		i = get_frm_FIFO();
		kprintf("\nReplacing Frame %d\n",i);
		free_frm(i);
		kprintf("\nReplacing the Frame Number %d\n",i+FRAME0);
		*avail = i;
		return OK;
	}

	else if(page_replace_policy == LRU)
	{
		i = get_frm_LRU();
		if(i==SYSERR)
			return SYSERR;
		
		kprintf("\nReplacing Frame %d\n",i);

		*avail = i;
		free_frm(i);
		return OK;
	}
	return SYSERR;
}

/*-------------------------------------------------------------------------
 * free_frm - free a frame 
 *-------------------------------------------------------------------------
 */
SYSCALL free_frm(int frm_num)
{
	int i = frm_num;
	int vp;
	unsigned long addr_value;
	virt_addr_t *a;
	unsigned int p,q;/* p is the pd offset while q is the pt offset */
	int pid;
	unsigned long pd;/* pdbr of the process holding this frame */
	pd_t *pd_p;
	pt_t *pt;
	int bs=0,bs_pgoffset=0; /* To write back the frame to bs if dirty */

	/* Using the inverted page table, get vp, 
	the virtual page number of the page to be replaced */
	vp = frm_tab[i].fr_vpno;

	/* Let a be vp*4096 (the first virtual address on page vp).*/
	addr_value = (vp*NBPG);
	a = &addr_value;
#if DEBUG_PAGING
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
#if DEBUG_PAGING
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
	if(frm_tab[pd_p->pd_base-FRAME0].fr_refcnt==0)
	{
		pd_p->pd_pres = 0;
	}

	/* If the dirty bit for page vp was set in its page table */
	if(pt->pt_dirty == 1)
	{
		/* Using the backing store map find the store and page offset within store given pid and a. */
		if(bsm_lookup(pid,(vp*NBPG),&bs,&bs_pgoffset)==SYSERR)
		{
			kill(pid);
			return SYSERR;
		}

		/* Write the page back to the backing store. */
		write_bs((i+FRAME0)*NBPG,bs,bs_pgoffset);
	}
		

  	return OK;
}

int get_frm_FIFO()
{
	int frm;	
	frm = frm_fifo_hd;

	if(frm_fifo_hd == -1)
	{
		kprintf("\n ERROR: FIFO Queue is Empty\n");
		return SYSERR;
	}
	
	frm_fifo_hd = frm_tab[frm].nxt_frm;

	if(frm_fifo_hd == -1)
		frm_fifo_tl = -1;
	else
		frm_tab[frm_fifo_hd].prev_frm = INVALID_FRM;

//	kprintf("\nReplacing Frame %d: H=%d T=%d\n",frm,frm_fifo_hd,frm_fifo_tl);

	return frm;
}
void insert_frm_fifo(int frm_num)
{
	int temp = frm_fifo_tl;
	
#if DEBUG_PAGING
	kprintf("\n\n\t[%s:%d]Inserting Frame %d into FIFO queue\n\n",__FILE__,__LINE__,frm_num);
#endif

	frm_fifo_tl = frm_num;
	frm_tab[frm_fifo_tl].prev_frm = temp;
	frm_tab[frm_fifo_tl].nxt_frm = INVALID_FRM;

	frm_tab[temp].nxt_frm = frm_fifo_tl;

	if(frm_fifo_hd==-1)
		frm_fifo_hd = frm_fifo_tl;
#if DEBUG_PAGING
	kprintf("\n\n\t[%s:%d]Inserting Frame %d into FIFO queue: H = %d T=%d\n\n",__FILE__,__LINE__,frm_num,frm_fifo_hd,frm_fifo_tl);
#endif

}

SYSCALL evict_process_frames(int pid)
{
	STATWORD ps;
	int i =0;
	int bs,vpno;

	if(pid <= 0)
		return SYSERR;
	
	disable(ps);
	for(i=0;i<NFRAMES;i++)
	{
		if(frm_tab[i].fr_pid == pid)
		{
			if(frm_tab[i].fr_type == FR_PAGE)
			{
				bs = proctab[pid].store;
				vpno = frm_tab[i].fr_vpno-bsm_tab[bs].bs_vpno;
				write_bs((FRAME0+i)*NBPG,bs,vpno);
			}
			frm_tab[i].fr_pid = -1;
			frm_tab[i].fr_status = FRM_UNMAPPED;
			frm_tab[i].fr_type = INVALID_FRM;
			frm_tab[i].fr_refcnt = 0;
			frm_tab[i].fr_loadtime = 0;
			frm_tab[i].fr_dirty = 0;
			frm_tab[i].fr_vpno = 0;
		}
	}

	restore(ps);
	return OK;
}

int get_frm_LRU()
{
	STATWORD ps;
	unsigned long int min_load_time=99999999;
	int i;
	int final_frm=0;
	fr_map_t *frm;

	for(i=0;i<NFRAMES;i++)
	{
		frm = &frm_tab[i];
		if(frm->fr_type==FR_PAGE)
		{
			if(min_load_time > frm->fr_loadtime)
			{
				min_load_time = frm->fr_loadtime;
				final_frm = i;
			}

			else if(min_load_time == frm->fr_loadtime)
			{
				if(frm_tab[final_frm].fr_vpno < frm->fr_vpno)
					final_frm = i;
			}
		}
	}

	restore(ps);
	return final_frm;
}

void update_frms_LRU()
{
	STATWORD ps;
	int i,j,pid;
	struct pentry *pptr;
	unsigned long pdbr;
	pd_t *pd_p;
	pt_t *pt_p;
	int frm_num;
	
	disable(ps);
	timeCount++;

	for(pid = 1; pid<NPROC; pid++)
	{
		pptr = &proctab[pid];
		if(pptr->pstate != PRFREE)
		{
			pdbr = pptr->pdbr;
			pd_p = pdbr;

			for(i=0;i<1024;i++)
			{
				if(pd_p->pd_pres == 1)
				{
					pt_p = (pd_p->pd_base)*NBPG;
					for(j=0;j<1024;j++)
					{
						if(pt_p->pt_pres == 1 && pt_p->pt_acc==1)
						{
							frm_num = pt_p->pt_base-FRAME0;
							frm_tab[frm_num].fr_loadtime = timeCount;
							pt_p->pt_acc=0;
						}
						pt_p++;						
					}
				}

				pd_p++;
			}
		}
	}

	restore(ps);
	return OK;
}

void frm_invalidate_TLB(int frm_num)
{
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

