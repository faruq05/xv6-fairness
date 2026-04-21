#ifndef _BOOSTSTAT_H_
#define _BOOSTSTAT_H_

// Maximum number of process slots — matches NPROC in param.h
#define NPROC_BOOST 64

// Per-process boost statistics.
// Kept separate from pstat.h intentionally —
// pstat.h is already 3840 bytes which is near the 4096-byte
// kalloc() limit. Adding boost fields there would overflow it.
// This struct is only 512 bytes and fits safely on its own.
struct booststat {
  int boost[NPROC_BOOST];    // remaining extra turns for each process
  int boosted[NPROC_BOOST];  // 1 = was ever boosted, 0 = never
};

#endif // _BOOSTSTAT_H_