#include "user/user.h"

#define NULL (void*) (0)
#define CYCLES 1000


void add(void *arg1 , void *arg2) {
  void * mem = malloc(200);
  mem = arg2;
  sleep(30);
  printf(mem);
  printf("\n");
  free(mem);
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

  printf("\n============== start parallel threads ==============\n");
  int pid;
  int j;
  char ar[] = { 'T', 'h', 'i', 's', ' ', 'i', 'm', 'p', 'l', 'e', 'm', 'e', 'n', 't', 'a', 't', 'i', 'o', 'n', ' ', 'm', 'a', 'y', ' ', 'n', 'o', 't', ' ', 'b', 'e', ' ', 't', 'h', 'e', ' ', 'm', 'o', 's', 't', ' ', 'e', 'f', 'f', 'i', 'c', 'i', 'e', 'n', 't', ',', ' ', 'a', 's', ' ', 'i', 't', ' ', 'r', 'e', 'q', 'u', 'e', 's', 't', 's', ' ', 'e', 'x', 't', 'r', 'a', ' ', 'm', 'e', 'm', 'o', 'r', 'y', ' ', 'f', 'o', 'r', ' ', 'a', 'l', 'i', 'g', 'n', 'm', 'e', 'n', 't', ' ', 'p', 'u', 'r', 'p', 'o', 's', 'e', 's', ' ', 'a', 'n', 'd', ' ', 'r', 'e', 'l', 'i', 'e', 's', ' ', 'o', 'n', ' ', 't', 'h', 'e', '\0'
};

  for(j = 0; j < i; j++) {
    pid = thread_create(add, NULL, &ar[j]);
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

  exit();
}
