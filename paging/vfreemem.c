/* vfreemem.c - vfreemem */

#include <conf.h>
#include <kernel.h>
#include <mem.h>
#include <proc.h>
#include <paging.h>
extern struct pentry proctab[];
/*------------------------------------------------------------------------
 *  vfreemem  --  free a virtual memory block, returning it to vmemlist
 *------------------------------------------------------------------------
 */
SYSCALL	vfreemem(struct mblock *block,	unsigned size)
{
	STATWORD ps;
	struct mblock *prev,*nxt,*vmlist;
	struct pentry *pptr;

	disable(ps);

	if(size == 0 || block < BASE_VPAGE_NUM*NBPG)
		return SYSERR;

	pptr = &proctab[currpid];

	size = roundmb(size);

	vmlist = &pptr->vmemlist;
	prev = vmlist;
	nxt = prev->mnext;

	/* Go to the given block */
	while(nxt!=NULL && nxt < block)
	{
		prev = nxt;
		nxt = nxt->mnext;
	}

	/* block's next points to the next block that is above this */
	block->mnext = nxt;
	block->mlen = size;

	/* the previous blocks next would point to the block */
	prev->mnext = block;
	
	
	return(OK);
}
