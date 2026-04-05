#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/pstat.h"
#include "user/user.h"

#define NUM_WORKERS 4
#define WORK_ITERS  5000000

static void
worker(int id)
{
  volatile int x = 0;
  for(int i = 0; i < WORK_ITERS; i++){
    x = x + i; x = x * 2; x = x / 2; x = x - i;
  }
  exit(0);
}

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

static void psl(char *s, int w){
  int l=0; char *t=s; while(*t++) l++;
  printf("%s",s);
  for(int i=l;i<w;i++) printf(" ");
}
static void pil(int n, int w){
  int t=n,d=0;
  if(t==0)d=1;
  while(t>0){d++;t/=10;}
  printf("%d",n);
  for(int i=d;i<w;i++) printf(" ");
}
static void pur(uint64 n, int w){
  uint64 t=n; int d=0;
  if(t==0)d=1;
  while(t>0){d++;t/=10;}
  for(int i=d;i<w;i++) printf(" ");
  printf("%lu",n);
}

// Global to avoid stack overflow
struct pstat st;

static void
print_table(void)
{
  if(schedstat(&st) < 0){
    printf("schedstat failed\n");
    return;
  }
  printf("\n=== Fairness Report (workers still in proc[]) ===\n");
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
    pil(st.pid[i],6);          printf("  ");
    psl(st.name[i],15);        printf("  ");
    psl(state_str(st.state[i]),10); printf("  ");
    pur(st.cpu_ticks[i],9);    printf("  ");
    pur(st.wait_ticks[i],10);  printf("  ");
    pur(st.sched_count[i],11); printf("\n");
  }
  for(int i=0;i<70;i++) printf("-");
  printf("\n");
}

int
main(void)
{
  printf("\n=== Stress Test — Equal Workload ===\n");
  printf("Forking %d equal CPU-busy workers...\n\n", NUM_WORKERS);

  for(int i = 0; i < NUM_WORKERS; i++){
    int pid = fork();
    if(pid < 0){ printf("fork failed\n"); exit(1); }
    if(pid == 0) worker(i+1);
  }

  // Wait for ALL workers to finish
  for(int i = 0; i < NUM_WORKERS; i++)
    wait(0);

  // Print table NOW — workers are ZOMBIE, still in proc[] with valid stats
  // Parent has not exited yet so they have not been cleaned up
  printf("All workers finished. Reading stats while zombies exist:\n");
  print_table();


  exit(0);
}