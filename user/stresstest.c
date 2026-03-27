#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

// -------------------------------------------------------
// Number of worker processes to fork.
// Keep it at 4 — xv6 only has 3 CPUs in QEMU by default
// so 4 workers gives us meaningful contention and waiting.
// -------------------------------------------------------
#define NUM_WORKERS 4

// -------------------------------------------------------
// How many iterations each worker runs before exiting.
// Each iteration does a small amount of arithmetic work
// so the process actually consumes CPU ticks.
// 5000000 = roughly a few seconds per worker in xv6 QEMU.
// -------------------------------------------------------
#define WORK_ITERS 5000000

// -------------------------------------------------------
// worker — the function each child process runs.
// It does a busy arithmetic loop to consume CPU time.
// id = which worker number (1,2,3,4) for reporting.
// -------------------------------------------------------
static void
worker(int id)
{
  // Volatile prevents the compiler from optimizing away the loop.
  // Without volatile, a smart compiler might see that 'x' is
  // never used outside the loop and delete the entire thing.
  volatile int x = 0;

  printf("worker %d started (pid %d)\n", id, getpid());

  for(int i = 0; i < WORK_ITERS; i++){
    // Simple arithmetic — just enough to burn CPU ticks
    x = x + i;
    x = x * 2;
    x = x / 2;
    x = x - i;
    // x is always 0 after each iteration but the CPU
    // still has to execute every instruction
  }

  printf("worker %d done    (pid %d) — x=%d\n", id, getpid(), x);
  exit(0);
}

// -------------------------------------------------------
// main
// -------------------------------------------------------
int
main(void)
{
  printf("\n=== Stress Test — Process Fairness ===\n");
  printf("Forking %d worker processes...\n\n", NUM_WORKERS);

  // Fork all workers
  for(int i = 0; i < NUM_WORKERS; i++){

    int pid = fork();

    if(pid < 0){
      // fork() failed — print error and stop
      printf("stresstest: fork failed at worker %d\n", i+1);
      exit(1);
    }

    if(pid == 0){
      // We are the child process — run worker and never return
      worker(i + 1);
      // worker() calls exit(0) so we never reach here
    }

    // We are the parent — record child's PID
    printf("forked worker %d with pid %d\n", i+1, pid);
  }

  // Parent prints a reminder to run fairness in another window
  printf("\nAll workers running.\n");
  printf(">>> In this shell, type: fairness\n");
  printf(">>> to see live scheduling stats.\n\n");

  // Parent waits for ALL children to finish before exiting.
  // wait() returns the pid of whichever child finished.
  // We call it NUM_WORKERS times to collect all children.
  for(int i = 0; i < NUM_WORKERS; i++){
    int pid = wait(0);   // 0 means we don't care about exit status
    printf("worker pid %d finished\n", pid);
  }

  printf("\nAll workers done. Run 'fairness' to see final stats.\n\n");
  exit(0);
}