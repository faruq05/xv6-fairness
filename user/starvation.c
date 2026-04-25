#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/pstat.h"
#include "user/user.h"
// Starvation severity thresholds
// ratio = wait_ticks / (cpu_ticks + 1)
// ratio < 1   → HEALTHY  — normal amount of waiting
// ratio 3  → STARVING — waiting much more than running
// ratio > 10  → CRITICAL — severely starved of CPU
#define RATIO_HEALTHY   1
#define RATIO_STARVING  3

// calculate starvation ratio for a process
// Returns wait / (cpu + 1) as a plain integer
static uint64
calc_ratio(uint64 wait, uint64 cpu)
{
  return wait / (cpu + 1);
}

// Returns 0 = healthy, 1 = starving, 2 = critical
static int
severity(uint64 wait, uint64 cpu)
{
  uint64 ratio = calc_ratio(wait, cpu);
  if(ratio < RATIO_HEALTHY)  return 0;
  if(ratio < RATIO_STARVING) return 1;
  return 2;
}

// convert state integer to readable string
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

// print string left-aligned to width w
static void
psl(char *s, int w)
{
  int l = 0;
  char *t = s;
  while(*t++) l++;
  printf("%s", s);
  for(int i = l; i < w; i++) printf(" ");
}

// print int left-aligned to width w
static void
pil(int n, int w)
{
  int t = n, d = 0;
  if(t == 0) d = 1;
  while(t > 0){ d++; t /= 10; }
  printf("%d", n);
  for(int i = d; i < w; i++) printf(" ");
}

// print uint64 right-aligned to width w
static void
pur(uint64 n, int w)
{
  uint64 t = n;
  int d = 0;
  if(t == 0) d = 1;
  while(t > 0){ d++; t /= 10; }
  for(int i = d; i < w; i++) printf(" ");
  printf("%lu", n);
}

// print divider line of dashes
static void
divider(int n)
{
  for(int i = 0; i < n; i++) printf("-");
  printf("\n");
}

// Jain's Fairness Index scaled by 1000
// Same implementation as fairness.c — integer math only.
// Returns 0-1000 where 1000 = perfectly fair (F=1.000)
//
// Formula: F = (sum of x)^2 / (n * sum of x^2)
// where x = cpu_ticks of each active process
static int
jain_index(struct pstat *st)
{
  uint64 sum    = 0;
  uint64 sum_sq = 0;
  int    n      = 0;

  for(int i = 0; i < NPROC_STAT; i++){
    if(!st->inuse[i] || st->state[i] == 0) continue;
    if(st->cpu_ticks[i] == 0) continue;
    sum    += st->cpu_ticks[i];
    sum_sq += st->cpu_ticks[i] * st->cpu_ticks[i];
    n++;
  }

  // 0 or 1 active process = perfectly fair by definition
  if(n <= 1 || sum_sq == 0)
    return 1000;

  // F * 1000 = (sum^2 * 1000) / (n * sum_sq)
  uint64 numerator   = sum * sum * 1000;
  uint64 denominator = (uint64)n * sum_sq;

  return (int)(numerator / denominator);
}

// print Jain index as decimal
// e.g. jain_scaled=980 prints "0.980"
//      jain_scaled=1000 prints "1.000"
static void
print_jain(int jain_scaled)
{
  int whole = jain_scaled / 1000;
  int frac  = jain_scaled % 1000;
  printf("%d.%d%d%d",
    whole,
    (frac / 100) % 10,
    (frac / 10)  % 10,
     frac        % 10);
}

// find_name — given a pid, find its name in pstat
// returns pointer to the name string or "unknown"
static char*
find_name(struct pstat *st, int pid)
{
  for(int i = 0; i < NPROC_STAT; i++){
    if(st->inuse[i] && st->pid[i] == pid)
      return st->name[i];
  }
  return "unknown";
}

// print_report — prints the full starvation report.
// Called when user types: starvation
int strncmp(const char *s1, const char *s2, int n) {
  for(int i = 0; i < n; i++) {
    if(s1[i] != s2[i] || s1[i] == '\0' || s2[i] == '\0')
      return s1[i] - s2[i];
  }
  return 0;
}

static void
print_report(struct pstat *st)
{
  // Count by category
  int total    = 0;
  int healthy  = 0;
  int starving = 0;
  int critical = 0;

  for(int i = 0; i < NPROC_STAT; i++){
    if(!st->inuse[i] || st->state[i] == 0) continue;
    total++;
    int s = severity(st->wait_ticks[i], st->cpu_ticks[i]);
    if(strncmp(st->name[i], "starvation", 10)==0){
      healthy++;
    }else{
      if(s == 0)      healthy++;
      else if(s == 1) starving++;
      else            critical++;
    }
  }

  // Header
  printf("\n=== Starvation Report ===\n");
  printf("[KERNEL PROTECTION ACTIVE], Process starvation won't starve\n");
  // Summary counts
  printf("Total processes  : %d\n", total);
  printf("Healthy          : %d\n", healthy);
  printf("Starving         : %d\n", starving);
  printf("Critical         : %d\n", critical);
  printf("\n");

  // Per-process table header
  printf("PID    ");
  printf("NAME             ");
  printf("STATE       ");
  printf("CPU_TICKS  ");
  printf("WAIT_TICKS  ");
  printf("RATIO   ");
  printf("STATUS\n");
  divider(72);

  // One row per active process
  for(int i = 0; i < NPROC_STAT; i++){
    if(!st->inuse[i] || st->state[i] == 0) continue;

    int s = severity(st->wait_ticks[i], st->cpu_ticks[i]);

    // PID and NAME
    pil(st->pid[i], 6);
    printf("  ");
    psl(st->name[i], 15);
    printf("  ");

    // STATE
    psl(state_str(st->state[i]), 10);
    printf("  ");

    // CPU_TICKS right-aligned
    pur(st->cpu_ticks[i], 9);
    printf("  ");

    // WAIT_TICKS right-aligned
    pur(st->wait_ticks[i], 10);
    printf("  ");

    // RATIO right-aligned
    pur(calc_ratio(st->wait_ticks[i], st->cpu_ticks[i]), 5);
    printf("   ");

    // STATUS
    if(strncmp(st->name[i], "starvation", 10) == 0){
      if(s == 0) printf("HEALTHY [PROTECTED]\n");
      else       printf("PROTECTED\n");
    } else {
      if(s == 0)      printf("HEALTHY\n");
      else if(s == 1) printf("STARVING\n");
      else            printf("CRITICAL\n");
    }
  }

  divider(72);

  // Jain Fairness Index
  int jain = jain_index(st);
  printf("\nJain Fairness Index : ");
  print_jain(jain);
  printf("  (1.000 = perfectly fair)\n");

  // Footer — instruct user how to boost
  if(starving > 0 || critical > 0){
    printf("\nStarving processes detected.\n");
    printf("To boost a process     : starvation boost <pid>\n");
    // printf("To boost with amount   : starvation boost <pid> <amount>\n");
  } else {
    printf("\nAll processes are healthy.\n");
  }
  printf("\n");
}

// do_boost — boosts a specific PID.
// Called when user types: starvation boost <pid>
//                     or: starvation boost <pid> <amount>
static void
do_boost(struct pstat *st, int pid, int amount)
{
  // Validate PID
  if(pid <= 0){
    fprintf(2, "starvation: invalid pid %d\n", pid);
    exit(1);
  }

  // Default amount if user did not specify
  if(amount <= 0)
    amount = 10;

  // Cap at 20 — matches the cap inside kernel setboost()
  if(amount > 20)
    amount = 20;

  // Get the process name for a friendly message
  char *name = find_name(st, pid);

  printf("Boosting PID %d (%s) with %d extra turns...\n",
    pid, name, amount);

  // Call the setboost system call
  int result = setboost(pid, amount);

  if(result == 0){
    printf("Done. Run starvation to check improvement.\n\n");
  } else {
    fprintf(2, "starvation: could not boost PID %d\n", pid);
    fprintf(2, "  process may not exist or may have already exited.\n\n");
  }
}

// convert string to integer (atoi equivalent)
// xv6 user.h has atoi() available — we use it directly

// Global pstat — declared globally to avoid placing
// 3840 bytes on the user stack which would overflow it
struct pstat st;

// main — entry point
//
// Usage:
//   starvation                     — print full report
//   starvation boost <pid>         — boost PID with 10 turns
//   starvation boost <pid> <amt>   — boost PID with amt turns
int
main(int argc, char *argv[])
{
  // Always call schedstat first — we need the data
  // for both the report and for looking up names when boosting
  if(schedstat(&st) < 0){
    fprintf(2, "starvation: schedstat() failed\n");
    exit(1);
  }

  // --- Mode 1: no arguments — print the full report ---
  if(argc == 1){
    print_report(&st);
    exit(0);
  }

  // --- Mode 2 and 3: "starvation boost <pid> [amount]" ---
  // Check first argument is the word "boost"
  if(argc >= 3 && strcmp(argv[1], "boost") == 0){

    // Parse PID from argv[2]
    int pid = atoi(argv[2]);

    // Parse optional amount from argv[3]
    int amount = 10;  // default
    if(argc >= 4){
      amount = atoi(argv[3]);
    }

    do_boost(&st, pid, amount);
    exit(0);
  }

  // --- Unknown usage ---
  fprintf(2, "Usage:\n");
  fprintf(2, "  starvation                   - show starvation report\n");
  fprintf(2, "  starvation boost <pid>        - boost a process\n");
  // fprintf(2, "  starvation boost <pid> <amt>  - boost with amount\n");
  exit(1);
}