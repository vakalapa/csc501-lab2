/* vgetmem.c - vgetmem */

#include <conf.h>
#include <kernel.h>
#include <mem.h>
#include <proc.h>
#include <paging.h>

extern struct pentry proctab[];
/*------------------------------------------------------------------------
 * vgetmem  --  allocate virtual heap storage, returning lowest WORD address
 *------------------------------------------------------------------------
 */
WORD	*vgetmem(unsigned int nbytes)
{
	STATWORD ps;
	struct pentry *pptr;
	struct mblock *vmlist,*prev,*nxt,*new_rem;
	

	disable(ps);
	nbytes = (unsigned int)roundmb(nbytes);

	pptr = &proctab[currpid];
	vmlist = &pptr->vmemlist;

	/* If the list is empty, return SYSERR */
	if(vmlist->mnext == NULL || nbytes == 0)
	{
		goto end;
	}

	prev = vmlist;
	nxt = vmlist->mnext;

	while(nxt!=NULL)
	{
		if(nxt->mlen == nbytes)
		{
			prev->mnext = nxt->mnext;
			restore(ps);
			return nxt;
		}
		else if(nxt->mlen > nbytes)
		{
			/* Use the new pointer to create a new mem chunk smaller than nxt */
			new_rem = nxt+nbytes;
			new_rem->mnext = nxt->mnext;
			new_rem->mlen = nxt->mlen - nbytes;
			
			/* Point the next of prev to this newly created chunk */
			prev->mnext = new_rem;

			restore(ps);
			return nxt;
		}

		prev = nxt;
		nxt = nxt->mnext;
	}

end:
	restore(ps);
	return (WORD*)( SYSERR );
}


