/* vcreate.c - vcreate */
    
#include <conf.h>
#include <i386.h>
#include <kernel.h>
#include <proc.h>
#include <sem.h>
#include <mem.h>
#include <io.h>
#include <paging.h>

/*
static unsigned long esp;
*/

LOCAL	newpid();
/*------------------------------------------------------------------------
 *  create  -  create a process to start running a procedure
 *------------------------------------------------------------------------
 */
SYSCALL vcreate(procaddr,ssize,hsize,priority,name,nargs,args)
	int	*procaddr;		/* procedure address		*/
	int	ssize;			/* stack size in words		*/
	int	hsize;			/* virtual heap size in pages	*/
	int	priority;		/* process priority > 0		*/
	char	*name;			/* name (for debugging)		*/
	int	nargs;			/* number of args that follow	*/
	long	args;			/* arguments (treated like an	*/
					/* array in the code)		*/
{
//	kprintf("To be implemented!\n");
	int pid;
	bsd_t bs;
	STATWORD ps;
	disable(ps);
	if(hsize > NUM_PAGES_PER_BS)
	{
		kprintf("\nVCREATE Failed! hsize exceeds the limit\n");
		restore(ps);
		return SYSERR;
	}
	pid = create(procaddr,ssize,priority,name,nargs,args);
	if(pid == SYSERR)
	{
		kprintf("\nVCREATE Failed! Create Failed!\n");
		restore(ps);
		return SYSERR;
	}
	if(get_bsm(&bs)==SYSERR)
	{
	kprintf("\n\n\t[%s:%d] Get BSM Failed!\n\n",__FILE__,__LINE__);
		restore(ps);
		return SYSERR;
	}
	
	proctab[pid].store = bs;
	proctab[pid].vhpnpages = hsize;
	proctab[pid].vhpno = 0;
	proctab[pid].vmemlist.mnext= BASE_VPAGE_NUM*NBPG;
	struct mblock *bs_base;
	bs_base = BACKING_STORE_BASE+(bs*BACKING_STORE_UNIT_SIZE);
	bs_base->mlen = hsize*NBPG;
	bs_base->mnext = NULL;
	
	bsm_tab[bs].bs_pid = pid;
	bsm_tab[bs].bs_status = BSM_MAPPED;
	bsm_tab[bs].bs_vpno = BASE_VPAGE_NUM;
	bsm_tab[bs].bs_npages = hsize;
	bsm_tab[bs].bs_ispriv = 1;
	restore(ps);
	return pid;
}

/*------------------------------------------------------------------------
 * newpid  --  obtain a new (free) process id
 *------------------------------------------------------------------------
 */
LOCAL	newpid()
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
