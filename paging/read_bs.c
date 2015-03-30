#include <conf.h>
#include <kernel.h>
#include <mark.h>
#include <bufpool.h>
#include <proc.h>
#include <paging.h>

SYSCALL read_bs(char *dst, bsd_t bs_id, int page) {

  /* fetch page page from map map_id
     and write beginning at dst.
  */
   void * phy_addr = BACKING_STORE_BASE + bs_id<<20 + page*NBPG;
  // kprintf("\n\t[READ_BS.C:14] Contents of Backing Store %d (Address %u) are %s\n",bs_id,tep_add,*tep_add);
   bcopy(phy_addr, (void*)dst, NBPG);
}


