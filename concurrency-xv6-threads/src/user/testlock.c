#include "user/user.h"

#define NULL (void*) (0)
#define CYCLES 1000

int count = 0;
lock_t lock ;

void add(void *arg1 , void *arg2) {
  int i;
  for(i = 0; i < CYCLES; i++) {
    lock_acquire(&lock);
    count++;
    lock_release(&lock);
  }
  exit();
}

int main(int argc, char *argv[])
{
  if (argc==1) {
    printf("Usage: %s <no_of_threads>\n",argv[0]);
    printf("example: %s 5\n",argv[0]);
    exit();
  }
  printf("%s: starting with pid %d\n",argv[0], getpid());
  int i = atoi(argv[1]);
  lock_init(&lock);

  printf("\n============== start parallel threads ==============\n");
  int pid;

  int j;
  for(j = 0; j < i; j++) {
    pid = thread_create(add, NULL, NULL);
    if(pid == -1)
      printf("%d: error in opening\n",j);
    else
      printf("%d: opened pid %d\n",j, pid);

  }
  printf("created %d threads\n",i);

  for(j = i; j > 0; j--) {
    pid = thread_join();
    if(pid == -1)
      printf("%d: error in closed\n",j);
    else
      printf("%d: closed pid %d\n",j, pid);
  }
  printf("closed %d threads\n",i);

  printf("final count = %d\n", count);

  if(count!=i*CYCLES)
    printf("======================== fail ========================\n");
  else
    printf("======================== success ========================\n");

  exit();
}
