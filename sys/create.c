/* create.c - create, newpid */
    
#include <conf.h>
#include <i386.h>
#include <kernel.h>
#include <proc.h>
#include <sem.h>
#include <mem.h>
#include <io.h>
#include <paging.h>

LOCAL int newpid();
int allocate_pdbr(int pid);

/*------------------------------------------------------------------------
 *  create  -  create a process to start running a procedure
 *------------------------------------------------------------------------
 */
SYSCALL create(procaddr,ssize,priority,name,nargs,args)
	int	*procaddr;		/* procedure address		*/
	int	ssize;			/* stack size in words		*/
	int	priority;		/* process priority > 0		*/
	char	*name;			/* name (for debugging)		*/
	int	nargs;			/* number of args that follow	*/
	long	args;			/* arguments (treated like an	*/
					/* array in the code)		*/
{
	unsigned long	savsp, *pushsp;
	STATWORD 	ps;    
	int		pid;		/* stores new process id	*/
	struct	pentry	*pptr;		/* pointer to proc. table entry */
	int		i;
	unsigned long	*a;		/* points to list of args	*/
	unsigned long	*saddr;		/* stack address		*/
	int		INITRET();

	disable(ps);
	if (ssize < MINSTK)
		ssize = MINSTK;
	ssize = (int) roundew(ssize);
	if (((saddr = (unsigned long *)getstk(ssize)) ==
	    (unsigned long *)SYSERR ) ||
	    (pid=newpid()) == SYSERR || priority < 1 ) {
		restore(ps);
		return(SYSERR);
	}

	numproc++;
	pptr = &proctab[pid];

	pptr->fildes[0] = 0;	/* stdin set to console */
	pptr->fildes[1] = 0;	/* stdout set to console */
	pptr->fildes[2] = 0;	/* stderr set to console */

	for (i=3; i < _NFILE; i++)	/* others set to unused */
		pptr->fildes[i] = FDFREE;

	pptr->pstate = PRSUSP;
	for (i=0 ; i<PNMLEN && (int)(pptr->pname[i]=name[i])!=0 ; i++)
		;
	pptr->pprio = priority;
	pptr->pbase = (long) saddr;
	pptr->pstklen = ssize;
	pptr->psem = 0;
	pptr->phasmsg = FALSE;
	pptr->plimit = pptr->pbase - ssize + sizeof (long);	
	pptr->pirmask[0] = 0;
	pptr->pnxtkin = BADPID;
	pptr->pdevs[0] = pptr->pdevs[1] = pptr->ppagedev = BADDEV;

		/* Bottom of stack */
	*saddr = MAGIC;
	savsp = (unsigned long)saddr;

	/* push arguments */
	pptr->pargs = nargs;
	a = (unsigned long *)(&args) + (nargs-1); /* last argument	*/
	for ( ; nargs > 0 ; nargs--)	/* machine dependent; copy args	*/
		*--saddr = *a--;	/* onto created process' stack	*/
	*--saddr = (long)INITRET;	/* push on return address	*/

	*--saddr = pptr->paddr = (long)procaddr; /* where we "ret" to	*/
	*--saddr = savsp;		/* fake frame ptr for procaddr	*/
	savsp = (unsigned long) saddr;

/* this must match what ctxsw expects: flags, regs, old SP */
/* emulate 386 "pushal" instruction */
	*--saddr = 0;
	*--saddr = 0;	/* %eax */
	*--saddr = 0;	/* %ecx */
	*--saddr = 0;	/* %edx */
	*--saddr = 0;	/* %ebx */
	*--saddr = 0;	/* %esp; fill in below */
	pushsp = saddr;
	*--saddr = savsp;	/* %ebp */
	*--saddr = 0;		/* %esi */
	*--saddr = 0;		/* %edi */
	*pushsp = pptr->pesp = (unsigned long)saddr;

	/************************/
	/* Added for Demand Paging */
	/************************/

	/* Create a page directory for this process */

	if(allocate_pdbr(pid)==SYSERR)
	{
		restore(ps);
		return SYSERR;		
	}

	/* Map the first 4 entries of the directory to global page tables */

	restore(ps);

	return(pid);
}

/*------------------------------------------------------------------------
 * newpid  --  obtain a new (free) process id
 *------------------------------------------------------------------------
 */
LOCAL int newpid()
{
	int	pid;			/* process id to return		*/
	int	i;

	for (i=0 ; i<NPROC ; i++) {	/* check all NPROC slots	*/
		if ( (pid=nextproc--) <= 0)
			nextproc = NPROC-1;
		if (proctab[pid].pstate == PRFREE)
			return(pid);
	}
	return(SYSERR);
}
int allocate_pdbr(int pid)
{
	int i;
	int new_frame;
	pd_t *pd_p;
	if(get_frm(&new_frame)==SYSERR)
	{
		kprintf("\nCreate Error: Frame not found for allocating PDBR\n");
		return SYSERR;
	}
	proctab[pid].pdbr = (unsigned long)((FRAME0+new_frame)*NBPG);
#if 0
	kprintf("\n\n\t[%s:%d] New Process's PDBR = %u\n\n",__FILE__,__LINE__,proctab[pid].pdbr);
#endif
	frm_tab[new_frame].fr_status = FRM_MAPPED;
	frm_tab[new_frame].fr_type = FR_DIR;
	frm_tab[new_frame].fr_pid = pid;
	pd_p = (pd_t*)((FRAME0+new_frame)*NBPG);
	for(i=0;i<4;i++)
	{
		pd_p->pd_pres = 1;
		pd_p->pd_write = 1;
		pd_p->pd_user = 0;
		pd_p->pd_pwt = 0;
		pd_p->pd_pcd = 0;
		pd_p->pd_acc = 0;
		pd_p->pd_mbz = 0;
		pd_p->pd_fmb = 0;
		pd_p->pd_global = 0;
		pd_p->pd_avail = 0;
		pd_p->pd_base = FRAME0+i;
		pd_p++;
	}
	for(i=4;i<1024;i++)
	{
		pd_p->pd_write = 1;
		pd_p++;
	}
	return OK;
}
