#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/pstat.h"
#include "user/user.h"

#define WORK_ITERS 3000000

static void
heavy_worker(void)
{
  volatile int x = 0;
  for(int i = 0; i < WORK_ITERS * 2; i++){
    x = x + i; x = x * 2; x = x / 2; x = x - i;
  }
  exit(0);
}

static void
light_worker(int id)
{
  volatile int x = 0;
  for(int i = 0; i < WORK_ITERS; i++){
    x = x + i; x = x * 2; x = x / 2; x = x - i;
    // pause very frequently — barely gets CPU
    if(i % 30000 == 0)
      pause(2);
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

struct pstat st;

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

  int starvation = 0;

  for(int i=0;i<NPROC_STAT;i++){
    if(!st.inuse[i] || st.state[i]==0) continue;
    pil(st.pid[i], 6);   printf("  ");
    psl(st.name[i], 15); printf("  ");
    psl(state_str(st.state[i]), 10); printf("  ");
    pur(st.cpu_ticks[i],   9); printf("  ");
    pur(st.wait_ticks[i], 10); printf("  ");
    pur(st.sched_count[i],11); printf("\n");

    // Check starvation: wait_ticks > 100 AND cpu < wait/10
    if(st.wait_ticks[i] > 100 &&
       st.cpu_ticks[i] < st.wait_ticks[i] / 10){
      starvation = 1;
    }
  }

  for(int i=0;i<70;i++) printf("-");
  printf("\n");

  if(starvation)
    printf("\nStarvation warning: YES — processes starved of CPU!\n");
  else
    printf("\nStarvation warning: none\n");
}

int
main(void)
{
  printf("\n=== Starvation Demo ===\n");
  printf("1 heavy worker (no sleep) + 3 light workers (frequent sleep)\n\n");

  // Fork heavy worker
  int pid = fork();
  if(pid < 0){ printf("fork failed\n"); exit(1); }
  if(pid == 0) heavy_worker();

  // Fork 3 light workers
  for(int i = 0; i < 3; i++){
    pid = fork();
    if(pid < 0){ printf("fork failed\n"); exit(1); }
    if(pid == 0) light_worker(i+1);
  }

  // Wait for all 4 children
  for(int i = 0; i < 4; i++)
    wait(0);

  // Print stats while zombies are still in proc[]
  printf("All workers done. Printing stats:\n");
  print_fairness();

  exit(0);
}