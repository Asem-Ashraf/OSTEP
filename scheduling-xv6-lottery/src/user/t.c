#include "kernel/types.h"
#include "user.h"

int main(int argc, char *argv[]) {
  int rc = settickets(atoi(argv[1]));
  if (rc < 0) {
    printf(2, "settickets failed\n");
    exit();
  }
  while(1);
  exit();
}
