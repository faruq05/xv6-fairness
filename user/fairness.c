#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/pstat.h"
#include "user/user.h"

// -------------------------------------------------------
// Helper: print string s, then pad with spaces to width w
// Used for left-aligned columns
// -------------------------------------------------------
static void
print_str_left(char *s, int width)
{
  int len = 0;
  char *tmp = s;
  while(*tmp++) len++;          // measure string length manually
  printf("%s", s);
  for(int i = len; i < width; i++)
    printf(" ");
}

// -------------------------------------------------------
// Helper: print integer n, then pad with spaces to width w
// Used for left-aligned integer columns (PID)
// -------------------------------------------------------
static void
print_int_left(int n, int width)
{
  // count digits
  int tmp = n, digits = 0;
  if(tmp == 0) digits = 1;
  while(tmp > 0){ digits++; tmp /= 10; }
  printf("%d", n);
  for(int i = digits; i < width; i++)
    printf(" ");
}

// -------------------------------------------------------
// Helper: print uint64 right-aligned in a field of width w
// Used for CPU_TICKS, WAIT_TICKS, SCHED_COUNT columns
// -------------------------------------------------------
static void
print_uint64_right(uint64 n, int width)
{
  // count digits
  uint64 tmp = n;
  int digits = 0;
  if(tmp == 0) digits = 1;
  while(tmp > 0){ digits++; tmp /= 10; }
  // print leading spaces
  for(int i = digits; i < width; i++)
    printf(" ");
  printf("%lu", n);
}

// -------------------------------------------------------
// Helper: convert integer state to readable string
// 0=UNUSED 1=USED 2=SLEEPING 3=RUNNABLE 4=RUNNING 5=ZOMBIE
// -------------------------------------------------------
static char*
state_name(int state)
{
  switch(state){
    case 0: return "UNUSED";
    case 1: return "USED";
    case 2: return "SLEEPING";
    case 3: return "RUNNABLE";
    case 4: return "RUNNING";
    case 5: return "ZOMBIE";
    default: return "UNKNOWN";
  }
}

// -------------------------------------------------------
// Helper: print a divider line of dashes
// -------------------------------------------------------
static void
print_divider(int len)
{
  for(int i = 0; i < len; i++)
    printf("-");
  printf("\n");
}

// -------------------------------------------------------
// Helper: Jain's Fairness Index scaled by 1000
// Returns 0-1000 (1000 = perfectly fair)
// -------------------------------------------------------
static int
jain_index(struct pstat *st)
{
  uint64 sum    = 0;
  uint64 sum_sq = 0;
  int    n      = 0;

  for(int i = 0; i < NPROC_STAT; i++){
    if(st->inuse[i] && st->cpu_ticks[i] > 0){
      sum    += st->cpu_ticks[i];
      sum_sq += st->cpu_ticks[i] * st->cpu_ticks[i];
      n++;
    }
  }

  if(n <= 1 || sum_sq == 0)
    return 1000;

  uint64 numerator   = sum * sum * 1000;
  uint64 denominator = (uint64)n * sum_sq;

  return (int)(numerator / denominator);
}

// -------------------------------------------------------
// Helper: print Jain index as decimal e.g. 980 → "0.980"
// -------------------------------------------------------
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

// -------------------------------------------------------
// Global pstat — avoids placing 3840 bytes on user stack
// -------------------------------------------------------
struct pstat st;

// -------------------------------------------------------
// main
// -------------------------------------------------------
int
main(void)
{
  if(schedstat(&st) < 0){
    fprintf(2, "fairness: schedstat() failed\n");
    exit(1);
  }

  // ---- Header ----
  printf("\n=== Process-Thread Fairness Analyser ===\n");
  printf("    CSE323 Group 03 - xv6-riscv\n\n");

  // ---- Column headers (manually padded) ----
  print_str_left("PID",         6);
  printf("  ");
  print_str_left("NAME",        15);
  printf("  ");
  print_str_left("STATE",       10);
  printf("  ");
  print_uint64_right(0,          9); // placeholder to print header right-aligned
  // We can't use the uint helper for header strings, so do it manually:
  // Reprint the header line entirely as plain strings with manual spacing
  printf("\n");

  // Reprint header properly — plain strings, manually spaced
  // PID(6) + 2 + NAME(15) + 2 + STATE(10) + 2 + CPU_TICKS(9) + 2 + WAIT_TICKS(10) + 2 + SCHED_COUNT(11)
  printf("PID    ");             // 6+2 = 8 wide including separator
  printf("NAME             ");   // 15+2 = 17
  printf("STATE       ");        // 10+2 = 12
  printf("CPU_TICKS  ");         // 9+2 = 11
  printf("WAIT_TICKS  ");        // 10+2 = 12
  printf("SCHED_COUNT\n");       // 11

  print_divider(70);

  // ---- Data rows ----
  int active           = 0;
  int starvation       = 0;
  int starvation_threshold = 500;

  for(int i = 0; i < NPROC_STAT; i++){
    if(!st.inuse[i])   continue;
    if(st.state[i] == 0) continue;

    print_int_left(st.pid[i], 6);
    printf("  ");
    print_str_left(st.name[i], 15);
    printf("  ");
    print_str_left(state_name(st.state[i]), 10);
    printf("  ");
    print_uint64_right(st.cpu_ticks[i],   9);
    printf("  ");
    print_uint64_right(st.wait_ticks[i],  10);
    printf("  ");
    print_uint64_right(st.sched_count[i], 11);
    printf("\n");

    if(st.wait_ticks[i] > (uint64)starvation_threshold &&
       st.cpu_ticks[i]  < st.wait_ticks[i] / 10){
      starvation = 1;
    }

    active++;
  }

  // ---- Footer ----
  print_divider(70);

  printf("\nTotal active processes : %d\n", active);

  printf("Jain Fairness Index    : ");
  print_jain(jain_index(&st));
  printf("  (1.000 = perfectly fair)\n");

  if(starvation){
    printf("Starvation warning     : YES - one or more processes\n");
    printf("                         have high wait_ticks vs cpu_ticks\n");
  } else {
    printf("Starvation warning     : none\n");
  }

  printf("\n");
  exit(0);
}