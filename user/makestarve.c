// #include "kernel/types.h"
// #include "kernel/stat.h"
// #include "kernel/pstat.h"
// #include "user/user.h"

// #define NUM_WORKERS 8

// // Spin forever — parent kills us
// static void
// worker(void)
// {
//   volatile long x = 1;
//   while(1){
//     x++;
//     if(x > 2000000000L) x = 1;
//   }
// }

// int
// main(void)
// {
//   int pids[NUM_WORKERS];

//   printf("\n=== makestarve: Starting Starvation Demo ===\n");
//   printf("Forking %d infinite spinners on 3 CPUs\n", NUM_WORKERS);
//   printf("8 workers / 3 CPUs = 5 always waiting\n\n");

//   for(int i = 0; i < NUM_WORKERS; i++){
//     int pid = fork();
//     if(pid < 0){ printf("fork failed\n"); exit(1); }
//     if(pid == 0) worker();
//     pids[i] = pid;
//     printf("  worker %d -> pid %d\n", i+1, pid);
//   }

//   printf("\n*** Workers are spinning NOW ***\n");
//   printf("You have ~30 seconds. Run: starvation\n\n");

//   // Use uptime() to wait ~30 seconds
//   // uptime() returns ticks; xv6 runs ~100 ticks/sec
//   uint64 start = uptime();
//   uint64 wait_ticks = 3000;  // ~30 seconds at 100 ticks/sec

//   // Print countdown every ~5 seconds
//   int last_print = 0;
//   while(1){
//     uint64 now = uptime();
//     uint64 elapsed = now - start;
//     if(elapsed >= wait_ticks) break;

//     int seconds_left = (int)((wait_ticks - elapsed) / 100);
//     int checkpoint = (int)(elapsed / 500);  // every ~5 sec

//     if(checkpoint != last_print){
//       last_print = checkpoint;
//       printf("  [%d sec left] -- run: starvation\n", seconds_left);
//     }
//   }

//   printf("\nTime's up. Killing workers...\n");
//   for(int i = 0; i < NUM_WORKERS; i++)
//     kill(pids[i]);
//   for(int i = 0; i < NUM_WORKERS; i++)
//     wait(0);

//   printf("All workers killed. Done.\n");
//   exit(0);
// }

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
  printf("Forking %d infinite spinners on 3 CPUs\n\n", NUM_WORKERS);

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