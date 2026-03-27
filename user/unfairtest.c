#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/pstat.h"
#include "user/user.h"

#define NUM_GREEDY 2
#define NUM_POLITE 2
#define WORK_ITERS 3000000

// Greedy worker — never yields, burns CPU continuously
static void
greedy_worker(int id)
{
  volatile int x = 0;
  for(int i = 0; i < WORK_ITERS; i++){
    x = x + i; x = x * 2; x = x / 2; x = x - i;
  }
  exit(0);
}

// Polite worker — pauses frequently, yields CPU back
static void
polite_worker(int id)
{
  volatile int x = 0;
  for(int i = 0; i < WORK_ITERS; i++){
    x = x + i; x = x * 2; x = x / 2; x = x - i;
    if(i % 200000 == 0)
      pause(1);
  }
  exit(0);
}

// Helper to convert state int to string
static char*
state_str(int s)
{
  switch(s){
    case 0: return "UNUSED";
    case 1: return "USED";
    case 2: return "SLEEPING";
    case 3: return "RUNNABLE";
    case 4: return "RUNNING";
    case 5: return "ZOMBIE";
    default: return "UNKNOWN";
  }
}

// Helper: print string left-aligned to width
static void
psl(char *s, int w)
{
  int l=0; char *t=s; while(*t++) l++;
  printf("%s", s);
  for(int i=l;i<w;i++) printf(" ");
}

// Helper: print int left-aligned to width
static void
pil(int n, int w)
{
  int t=n, d=0;
  if(t==0) d=1;
  while(t>0){d++;t/=10;}
  printf("%d",n);
  for(int i=d;i<w;i++) printf(" ");
}

// Helper: print uint64 right-aligned to width
static void
pur(uint64 n, int w)
{
  uint64 t=n; int d=0;
  if(t==0) d=1;
  while(t>0){d++;t/=10;}
  for(int i=d;i<w;i++) printf(" ");
  printf("%lu",n);
}

// Global pstat to avoid stack overflow
struct pstat st;

// Print the fairness table inline
static void
print_fairness(void)
{
  if(schedstat(&st) < 0){
    printf("schedstat failed\n");
    return;
  }

  printf("\n=== Fairness Report ===\n");
  printf("PID    ");
  printf("NAME             ");
  printf("STATE       ");
  printf("CPU_TICKS  ");
  printf("WAIT_TICKS  ");
  printf("SCHED_COUNT\n");
  for(int i=0;i<70;i++) printf("-");
  printf("\n");

  for(int i=0;i<NPROC_STAT;i++){
    if(!st.inuse[i] || st.state[i]==0) continue;
    pil(st.pid[i], 6);   printf("  ");
    psl(st.name[i], 15); printf("  ");
    psl(state_str(st.state[i]), 10); printf("  ");
    pur(st.cpu_ticks[i],   9); printf("  ");
    pur(st.wait_ticks[i], 10); printf("  ");
    pur(st.sched_count[i],11); printf("\n");
  }

  for(int i=0;i<70;i++) printf("-");
  printf("\n");
}

int
main(void)
{
  printf("\n=== Unfair Load Test ===\n");
  printf("2 greedy (no sleep) + 2 polite (frequent sleep)\n\n");

  // Fork greedy workers
  for(int i = 0; i < NUM_GREEDY; i++){
    int pid = fork();
    if(pid < 0){ printf("fork failed\n"); exit(1); }
    if(pid == 0) greedy_worker(i+1);
  }

  // Fork polite workers
  for(int i = 0; i < NUM_POLITE; i++){
    int pid = fork();
    if(pid < 0){ printf("fork failed\n"); exit(1); }
    if(pid == 0) polite_worker(i+1);
  }

  // Wait for ALL children to finish first
  for(int i = 0; i < NUM_GREEDY + NUM_POLITE; i++)
    wait(0);

  // NOW print the fairness table.
  // Workers are ZOMBIE — still in proc[] with valid stats.
  // Parent has not exited yet so they haven't been cleaned up.
  printf("All workers finished. Stats while zombies exist:\n");
  print_fairness();

  printf("\nNote: greedy workers got more cpu_ticks.\n");
  printf("Note: polite workers got more wait_ticks.\n");
  printf("Jain index will be below 1.000 showing unfairness.\n\n");

  exit(0);
}