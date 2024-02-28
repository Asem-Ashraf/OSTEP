#include "user/user.h"

#define NULL (void*) (0)

void hello(void* arg1, void* arg2) {
  printf("intentional delay\n");
  sleep(200);
  printf("hello from child. pid %d\n", getpid());
  exit();
}
void echo(void* count, void* string) {
  printf("test arguments in child: pid %d\n", getpid());
  printf("got %d %s\n", *(int*)count, (char*) string);
  exit();
}

int main(int argc, char* argv[]) {
  if (argc==1) {
    printf("Usage: %s <no_of_threads> <test_string>\n",argv[0]);
    printf("example: %s 5 apples\n",argv[0]);
    exit();
  }
  printf("test1: starting with pid %d\n", getpid());
  int i = atoi(argv[1]);
  int ar =i;
  char* str = argv[2];

  printf("\n============== start parallel threads ==============\n");
  int pid = thread_create(echo, (void*) &ar, (void*) str);
  if(pid == -1)
    printf("error in first");

  int j;
  for(j = 0; j < i; j++) {
    pid = thread_create(hello, NULL, NULL);
    if(pid == -1)
      printf("error in opening %d\n",j);
  }
  printf("done creating threads\n");

  for(j = i; j >= 0; j--) {
    pid = thread_join();
    if(pid == -1)
      printf("error in close %d\n",j);
    printf("%d closed pid %d\n",j, pid);
  }
  printf("done closing threads\n");

  printf("\n============== start sequentional threads ==============\n");

  for(j = 0; j < i; j++) {
    pid = thread_create(hello, NULL, NULL);
    if(pid == -1)
      printf("error in opening %d\n",j);
    printf("%d opened pid %d\n",j, pid);
    pid = thread_join();
    if(pid == -1)
      printf("error in close %d\n",j);
    printf("%d closed pid %d\n",j, pid);
  printf("---------------------\n");
  }
  printf("\n============== done testing ==============\n");
  exit();
}
