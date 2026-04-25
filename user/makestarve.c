#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/pstat.h"
#include "user/user.h"

#define NUM_WORKERS 3

static void
worker(void)
{
  volatile long x = 1;
  while(1){
    x++;
    if(x > 2000000000L) x = 1;
  }
}

int
main(void)
{
  int pids[NUM_WORKERS];

  printf("\n=== makestarve: Starting Starvation Demo ===\n");
  printf("Forking %d infinite spinners\n\n", NUM_WORKERS);

  for(int i = 0; i < NUM_WORKERS; i++){
    int pid = fork();
    if(pid < 0){ printf("fork failed\n"); exit(1); }
    if(pid == 0) worker();
    pids[i] = pid;
    printf("  worker %d -> pid %d\n", i+1, pid);
  }

  printf("\n*** Workers spinning! Run: starvation ***\n");
  printf("Workers will auto-die in ~30 sec\n\n");

  // Fork a reaper child — IT waits, parent exits immediately
  // so the shell is returned to you right away
  int reaper = fork();
  if(reaper < 0){ printf("fork failed\n"); exit(1); }

  if(reaper == 0){
    // Child: wait 30 seconds then kill all workers
    uint64 start = uptime();
    while(uptime() - start < 3000)
      ;  // busy wait ~30 sec
    for(int i = 0; i < NUM_WORKERS; i++)
      kill(pids[i]);
    for(int i = 0; i < NUM_WORKERS; i++)
      wait(0);
    exit(0);
  }

  // Parent exits immediately — shell prompt returns to YOU
  exit(0);
}