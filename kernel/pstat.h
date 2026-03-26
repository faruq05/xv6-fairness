#ifndef _PSTAT_H_
#define _PSTAT_H_

// Maximum number of processes in xv6 (defined in kernel/param.h as 64)
// We restate it here so this header is self-contained.
#define NPROC_STAT 64

// Per-process scheduling statistics.
// This struct is filled by the kernel's schedstat() function
// and copied to user space via copyout().
// Arrays are indexed 0..NPROC_STAT-1, one slot per proc[] entry.
struct pstat {
  int    inuse[NPROC_STAT];           // 1 = slot is a live process, 0 = empty
  int    pid[NPROC_STAT];             // process ID
  int    state[NPROC_STAT];           // process state as integer (see below)
  char   name[NPROC_STAT][16];        // process name string, max 16 chars
  uint64 cpu_ticks[NPROC_STAT];       // total ticks this process ran on CPU
  uint64 wait_ticks[NPROC_STAT];      // total ticks spent RUNNABLE but not chosen
  uint64 sched_count[NPROC_STAT];     // how many times scheduler picked this process
  uint64 last_sched_tick[NPROC_STAT]; // tick value when process last ran
};

// State values stored in pstat.state[]:
//   0 = UNUSED
//   1 = USED
//   2 = SLEEPING
//   3 = RUNNABLE
//   4 = RUNNING
//   5 = ZOMBIE

#endif  // _PSTAT_H_