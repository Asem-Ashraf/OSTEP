#include "kernel/mmu.h"
#include "user.h"

char* strcpy(char* s, char* t) {
  char* os;

  os = s;
  while((*s++ = *t++) != 0) {}
  return os;
}

int strcmp(const char* p, const char* q) {
  while(*p && *p == *q)
    p++, q++;
  return (uchar) *p - (uchar) *q;
}

uint strlen(char* s) {
  int n;

  for(n = 0; s[n]; n++) {}
  return n;
}

void* memset(void* dst, int c, uint n) {
  stosb(dst, c, n);
  return dst;
}

char* strchr(const char* s, char c) {
  for(; *s; s++)
    if(*s == c)
      return (char*) s;
  return 0;
}

char* gets(char* buf, int max) {
  int i, cc;
  char c;

  for(i = 0; i + 1 < max;) {
    cc = read(0, &c, 1);
    if(cc < 1)
      break;
    buf[i++] = c;
    if(c == '\n' || c == '\r')
      break;
  }
  buf[i] = '\0';
  return buf;
}

int stat(char* n, struct stat* st) {
  int fd;
  int r;

  fd = open(n, O_RDONLY);
  if(fd < 0)
    return -1;
  r = fstat(fd, st);
  close(fd);
  return r;
}

int atoi(const char* s) {
  int n;

  n = 0;
  while('0' <= *s && *s <= '9')
    n = n * 10 + *s++ - '0';
  return n;
}

void* memmove(void* vdst, void* vsrc, int n) {
  char *dst, *src;

  dst = vdst;
  src = vsrc;
  while(n-- > 0)
    *dst++ = *src++;
  return vdst;
}

// static inline int fetch_and_replace(int* var, int newVal) {
//   __asm__ volatile(
//       "lock; xchgl %0, %1"
//       : "+r"(
//             newVal), // value to replace with (this value probably in a register
//                      // and that register will be returned after the swap)
//       "+m"(*var)     // Memory location to swap with newVal
//       :              // No input operands
//       : "memory");
//   return newVal; // Now contains the old value of *var
// }

static inline int fetch_and_add(int* var, int val) {
  __asm__ volatile("lock; xaddl %0, %1"
                   : "+r"(val), "+m"(*var) // input + output
                   :                       // No input
                   : "memory");
  return val;
}

void lock_init(lock_t* lock) {
  lock->ticket = 0;
  lock->turn = 0;
}

void lock_acquire(lock_t* lock) {
  int myturn = fetch_and_add(&lock->ticket, 1);
  while(lock->turn != myturn)
    ; // spin
}

void lock_release(lock_t* lock) {
  lock->turn = lock->turn + 1;
}

int thread_create(void (*start_routine)(void*, void*), void* arg1, void* arg2) {
  // TODO: palloc is a page-aligned page allocator
  // currently using malloc to get a page without any alignment guarantee
  void* stack = malloc(PGSIZE);
  return clone(start_routine, arg1, arg2, stack);
}

int thread_join() {
  // a stack passed by reference to join. Join assigns it to the correct stack.
  void* stack;
  int rc = join(&stack);
  free(stack);
  return rc;
}
