#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"
#include "pstat.h" //include new code
#include "vm.h"

uint64
sys_exit(void)
{
  int n;
  argint(0, &n);
  kexit(n);
  return 0;  // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return kfork();
}

uint64
sys_wait(void)
{
  uint64 p;
  argaddr(0, &p);
  return kwait(p);
}

uint64
sys_sbrk(void)
{
  uint64 addr;
  int t;
  int n;

  argint(0, &n);
  argint(1, &t);
  addr = myproc()->sz;

  if(t == SBRK_EAGER || n < 0) {
    if(growproc(n) < 0) {
      return -1;
    }
  } else {
    // Lazily allocate memory for this process: increase its memory
    // size but don't allocate memory. If the processes uses the
    // memory, vmfault() will allocate it.
    if(addr + n < addr)
      return -1;
    if(addr + n > TRAPFRAME)
      return -1;
    myproc()->sz += n;
  }
  return addr;
}

uint64
sys_pause(void)
{
  int n;
  uint ticks0;

  argint(0, &n);
  if(n < 0)
    n = 0;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(killed(myproc())){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

uint64
sys_kill(void)
{
  int pid;

  argint(0, &pid);
  return kkill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

// new code
// sys_schedstat - system call wrapper for schedstat()
// The user passes a pointer to a struct pstat.
// We extract that pointer, call the real schedstat() function,
// and use copyout() to safely write the data into user memory.
uint64
sys_schedstat(void)
{
  uint64 addr;   // will hold the user-space pointer to struct pstat

  // argaddr(0, &addr) reads the first argument (argument index 0)
  // that the user passed to this syscall.
  // The user called: schedstat(&st)
  // So argument 0 is the address of their struct pstat variable.
  argaddr(0, &addr);
  if(addr == 0)
    return -1;  // if we can't read the argument, return error

  // Declare a local kernel-side copy of struct pstat.
  // schedstat() will fill this in by reading the proc[] table.
  struct pstat st;

  // Call the real kernel function that fills st with data.
  if(schedstat(&st) < 0)
    return -1;

  // copyout() copies bytes FROM kernel memory (our &st)
  // TO user memory (the address the user gave us).
  // Arguments: page table, destination address, source pointer, byte count.
  // We MUST use copyout() and never write to addr directly —
  // addr is a user virtual address, not a kernel address.
  if(copyout(myproc()->pagetable, addr, (char*)&st, sizeof(st)) < 0)
    return -1;

  return 0;  // success
}