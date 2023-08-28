#include "kernel/types.h"
#include "user.h"

int main(int argc, char *argv[]) {
  struct pstat pstat;
    int rc = getpinfo(&pstat);
    if (rc < 0) {
      printf(2,"getpinfo() failed\n");
      exit();
    }
    printf(0,"PID\tUSED\tTICKETS\t\tTICKS\n");
    for (int i = 0; i < NPROC; i++) {
      if (pstat.inuse[i]) {
        printf(1,"%d\t%d\t%d\t\t%d\n", pstat.pid[i], pstat.inuse[i], 
               pstat.tickets[i], pstat.ticks[i]);
      }
    }
  exit();
}
