# Kernel Threads
This repository contains my solution to the Concurrency-xv6-threads assignment
from the Operating Systems: Three Easy Pieces (OSTEP) book.

## Solution Overview

My solution provides an implementation of the required functionalities in the
xv6 operating system. Here's an overview of the key components:

### System Calls

1. `clone(void(*fcn)(void *, void *), void *arg1, void *arg2, void *stack)`:
This system call creates a new kernel thread that shares the calling process's
address space. It copies file descriptors similar to `fork()`. The new thread
uses the specified `stack` as its user stack and starts executing at the address
specified by `fcn`. The function arguments `arg1` and `arg2` are passed to the
thread. The parent process receives the PID of the new thread.

2. `join(void **stack)`: This system call waits for a child thread that shares
the address space with the calling process to exit. It returns the PID of the
waited-for child thread or -1 if none. The location of the child's user stack is
copied into the `stack` argument.

### Thread Library

I have implemented a simple thread library on top of the system calls. The
library provides the following functions:

1. `int thread_create(void (*start_routine)(void *, void *), void *arg1, void
*arg2)`: This function creates a new user stack using `malloc()`, uses `clone()`
to create a child thread, and gets it running. It returns the newly created
thread's PID to the parent process and 0 to the child. If the creation fails, it
returns -1.

2. `int thread_join()`: This function calls the underlying `join()` system call,
frees the user stack, and returns the waited-for PID (when successful).
Otherwise, it returns -1.

### Ticket Lock

I have implemented a ticket lock using the `lock_t` type. The lock can be
acquired using `lock_acquire()` and released using `lock_release()`. The spin
lock is built using the x86 atomic fetch-and-add operation (`xaddl`
instruction).

## Programs

I have provided three additional programs to test the functionality of the
implemented thread library and lock APIs:
- `testml.c`: which tests the multi-threaded functionality of a program along with the argument passing to a thread function.
- `testlock.c`: which tests the `lock_t`.
- `testmalloc.c`: which tries to test using `malloc()` multiple times.


## Getting Started

To use my solution and test the programs, follow these steps:

1. Clone this repository: `git clone https://github.com/Asem-Ashraf/OSTEP.git`
2. Navigate to the `concurrency-xv6-threads/src` directory: `cd OSTEP/concurrency-xv6-threads/src`
3. Build the xv6 operating system by running the command: `make`
4. Start xv6 using QEMU by running the command: `make qemu`

## Additional Resources

For more information on the assignment and the concepts involved, refer to the
following resources:

- [Operating Systems: Three Easy Pieces (OSTEP) book](http://pages.cs.wisc.edu/~remzi/OSTEP/)
- [Concurrency-xv6-threads Assignment Description](https://github.com/remzi-arpacidusseau/ostep-projects/blob/master/concurrency-xv6-threads/README.md)

Feel free to explore the code and modify it according to your needs. If you have
any questions or suggestions, please feel free to reach out.


</br>
</br>
</br>
</br>


##### My Initial Questions
- [x] what is `uses a fake return PC (0xffffffff)`?

#### My Steps
- [x] add the syscall
  - [x] edit `usys.S` to add the assembly of the syscall
  - [x] edit `syscall.h` to add the number of the syscall
  - [x] edit `user.h` to add the signature of syscall
  - [x] edit `syscall.c` to direct the syscall to the correct func.
  - [x] add the wrapper functions to `sysproc.c`
- [x] implement `int clone(void(*fcn)(void *, void *), void *arg1, void *arg2,
void *stack)`  
  `fcn`: a pointer, points to function. This function specifically takes 2
arguments.
  Code starts executing here.  
  `arg1`: The first argument of the function.  
  `arg2`: the second argument of the function.  
  `stack`: the independent stack of this thread(function).  
  `return`: the ID of the new thread, as if it is a process.
  - [x] copy `fork()` in `clone()` as a start
  - ~~Implement `allocthread()`~~ NOT NEEDED. `allocproc()` works fine. ~~Does a
  thread need its own kernel stack? I say yes. Because if it wants to trap into
  the kernel it needs its own stack with its own stack frame?~~
    - ~~Would it be a problem if all threads used the same kernel stack? IDK,
    yet.~~
  - [x] Child thread got the same physical address.
  - [x] do we need to copy the trap frame? Yes, because it contains the
    arguments of the thread function passed by the main thread and the return
    address of the main thread after the trap. The main thread trap frame is
    copied fully to a new location where the child thread is the only process
    that will edit it. But, we dont need the return address of the main thread.
    Yes, we don't. That's why we are replacing it with the thread function.
    The trap frame is copied in whole because the main thread will continue
    execution and will continue to use its stack normally, which could result in
    overwriting what was on it. So, we take a whole copy so that the 2 threads
    are now independent.
  - [x] Put arguments on the stack according to the x86 convention.
  - [x] push a fake address after the arguments.
  - [x] Figure out how to reroute the return address in the trap frame to
    return to the thread function instead of where it the syscall was called
    in main.
    - This is done through the eip register, which is the program counter in x86
- [x] implement `int join(void **stack)`  
  `stack`: a passed-by-reference variable that will contain a pointer to the
stack
  of the thread after it exits.
    - [x] waits for the thread to exit
    - [x] returns the ID of the thread to the calling calling main thread, after
    the
    thread with the stack `stack` exits.
    - [x] wait for a thread if it didnt exit yet
    - ~~a child thread can call a child child thread in the same address space
    of the main thread and not free it. We have to make join detect such case so
    even if the main thread only made one thread and the child thread made other
    threads, join from the main thread should be able to see all those threads
    so that we don't get massive memory leaks. But, the main thread cannot know
    how many child threads are still allocated. Maybe another function to return
    that count or maybe the join function returns a count by a variable passed
    by reference. Or the main thread just keep joining threads till it gets -1
    as the return value, which mean that there are no child thread running and
    also no child stacks allocated.~~  
      - ~~**fix 1**: Implement a mechanism to keep count of all child and
      child's child and give the main thread access to that count along with the
      count of all active threads other than the main thread.~~
      - ~~**fix 2**: disallow a thread to create new threads by requiring a key
      to a lock before allocating more memory in the address space where this
      lock is always held by the main thread.~~
        - ~~**problem with fix 2**: this can create a deadlock by the child
        thread waiting for the lock to be released to continue execution while
        the main thread can call join on that thread.~~
      - This is handled by the `exit()` function where if I exit and I have
      children, they are all classified as zombies and given away to the
      initproc where the initproc kills them everytime it wakes us.
- [ ] edit `exit()` to comply with exiting a thread.
  - [x] `exit()` should signal to the main thread that this thread has exited.
    - Done through `wakeup1()`
  - ~~`exit()` has to know that the caller is a thread of a process not the
  whole process.~~
  - [x] should `exit()` deallocate the stack? Because join might not be called?
    - It is left for the user to manage their stacks by calling `thread_join()`.
    However, if `thread_join()` was never called, the stack will never be
    deallocated.
  - I couldn't figure out what to edit to make it thread compatible. Maybe the
  edit is already made in my version.
    - [ ] compare different version of xv6.
- [ ] create a user library 
  - [x] Where?
    - prototypes in `user/user.h`
    - implementation in `user/ulib.c`
  - [ ] implement 
  `int thread_create(void (*start_routine)(void *, void *), void *arg1, void
  *arg2)`
    `start_routine`:   
    `arg1`: the first argument of the thread function  
    `arg2`: the second argument of the thread function 
    - use `malloc()`to allocate stack
      - [x] add a lock somewhere in `malloc()` to avoid allocating the same
      memory twice in a multi-threaded program.
    - [x] stack should be 1 page
    - [ ] stack should be page aligned
    - [x] stack should be in the same address space
    - [x] stack is allocated before calling clone
    - [x] calls the `clone()` system call to create the child and run it.
    - [x] returns child thread PID returned from `clone()`, -1 if unsuccessful
  - [x] implement `int thread_join()`
    - calls `join()`
    - frees the stack
      - [x] how to get the address of that stack?
        - using a queue.
    - returns waited-for PID, -1 if unsuccessful
      - [x] How to get the PID of that thread.
        - `join()` returns the PID
    - [x] wait for a thread if it didnt exit yet
      - This is done by `join()`
    - should this implementation be non blocking? NO.
  - [x] Implement a simple ticket lock (read [this book
  chapter](http://pages.cs.wisc.edu/~remzi/OSTEP/threads-locks.pdf))
    - [x] implement `void lock_acquire(lock_t *)`
    - [x] implement `void lock_release(lock_t *)`
