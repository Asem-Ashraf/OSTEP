
# Kernel Threads

In this project, you'll be adding real kernel threads to xv6. Sound like fun?
Well, it should. Because you are on your way to becoming a real kernel
hacker. And what could be more fun than that?

Specifically, you'll do three things. First, you'll define a new system call
to create a kernel thread, called `clone()`, as well as one to wait for a
thread called `join()`. Then, you'll use `clone()` to build a little thread
library, with a `thread_create()` call and `lock_acquire()` and
`lock_release()` functions. That's it! And now, for some details.

## Overview

Your new clone system call should look like this: `int clone(void(*fcn)(void
*, void *), void *arg1, void *arg2, void *stack)`. This call creates a new
kernel thread which shares the calling process's address space. File
descriptors are copied as in `fork()`. The new process uses `stack` as its
user stack, which is passed two arguments (`arg1` and `arg2`) and uses a fake
return PC (`0xffffffff`); a proper thread will simply call `exit()` when it is
done (and not `return`). The stack should be one page in size and
page-aligned. The new thread starts executing at the address specified by
`fcn`. As with `fork()`, the PID of the new thread is returned to the parent
(for simplicity, threads each have their own process ID).

The other new system call is `int join(void **stack)`. This call waits for a
child thread that shares the address space with the calling process to
exit. It returns the PID of waited-for child or -1 if none. The location of
the child's user stack is copied into the argument `stack` (which can then be
freed).

You also need to think about the semantics of a couple of existing system
calls. For example, `int wait()` should wait for a child process that does not
share the address space with this process. It should also free the address
space if this is last reference to it. Also, `exit()` should work as before
but for both processes and threads; little change is required here.

Your thread library will be built on top of this, and just have a simple `int
thread_create(void (*start_routine)(void *, void *), void *arg1, void *arg2)`
routine. This routine should call `malloc()` to create a new user stack, use
`clone()` to create the child thread and get it running. It returns the newly
created PID to the parent and 0 to the child (if successful), -1 otherwise.
An `int thread_join()` call should also be created, which calls the underlying
`join()` system call, frees the user stack, and then returns. It returns the
waited-for PID (when successful), -1 otherwise.

Your thread library should also have a simple *ticket lock* (read [this book
chapter](http://pages.cs.wisc.edu/~remzi/OSTEP/threads-locks.pdf) for more
information on this). There should be a type `lock_t` that one uses to declare
a lock, and two routines `void lock_acquire(lock_t *)` and `void
lock_release(lock_t *)`, which acquire and release the lock. The spin lock
should use x86 atomic add to build the lock -- see [this wikipedia
page](https://en.wikipedia.org/wiki/Fetch-and-add) for a way to create an
atomic fetch-and-add routine using the x86 `xaddl` instruction. One last
routine, `void lock_init(lock_t *)`, is used to initialize the lock as need be
(it should only be called by one thread).

The thread library should be available as part of every program that runs in
xv6. Thus, you should add prototypes to `user/user.h` and the actual code to
implement the library routines in `user/ulib.c`.

One thing you need to be careful with is when an address space is grown by a
thread in a multi-threaded process (for example, when `malloc()` is called, it
may call `sbrk` to grow the address space of the process). Trace this code
path carefully and see where a new lock is needed and what else needs to be
updated to grow an address space in a multi-threaded process correctly.

## Building `clone()` from `fork()`

To implement `clone()`, you should study (and mostly copy) the `fork()` system
call. The `fork()` system call will serve as a template for `clone()`, with
some modifications. For example, in `kernel/proc.c`, we see the beginning of
the `fork()` implementation:

```c
int
fork(void)
{
  int i, pid;
  struct proc *np;

  // Allocate process.
  if((np = allocproc()) == 0)
    return -1;

  // Copy process state from p.
  if((np->pgdir = copyuvm(proc->pgdir, proc->sz)) == 0){
    kfree(np->kstack);
    np->kstack = 0;
    np->state = UNUSED;
    return -1;
  }
  np->sz = proc->sz;
  np->parent = proc;
  *np->tf = *proc->tf;
```

This code does some work you need to have done for `clone()`, for example,
calling `allocproc()` to allocate a slot in the process table, creating a
kernel stack for the new thread, etc.

However, as you can see, the next thing `fork()` does is copy the address
space and point the page directory (`np->pgdir`) to a new page table for that
address space. When creating a thread (as `clone()` does), you'll want the 
new child thread to be in the *same* address space as the parent; thus, there
is no need to create a copy of the address space, and the new thread's
`np->pgdir` should be the same as the parent's -- they now share the address
space, and thus have the same page table.

Once that part is complete, there is a little more effort you'll have to apply
inside `clone()` to make it work. Specifically, you'll have to set up the
kernel stack so that when `clone()` returns in the child (i.e., in the newly
created thread), it runs on the user stack passed into clone (`stack`), that
the function `fcn` is the starting point of the child thread, and that the
arguments `arg1` and `arg2` are available to that function. This will be a
little work on your part to figure out; have fun!

## x86 Calling Convention

One other thing you'll have to understand to make this all work is the x86
calling convention, and exactly how the stack works when calling a function.
This is you can read about in [Programming From The Ground
Up](https://download-mirror.savannah.gnu.org/releases/pgubook/ProgrammingGroundUp-1-0-booksize.pdf),
a free online book. Specifically, you should understand Chapter 4 (and maybe
Chapter 3) and the details of call/return. All of this will be useful in
getting `clone()` above to set things up properly on the user stack of the
child thread.




#### Initial Questions
- what is `uses a fake return PC (0xffffffff)`? I suppose as if this is a
function call and there is a return address from the location where the function
was called. This should never be used, so it is a fake return address.

## My Steps
- [x] add the syscall
  - [x] edit `usys.S` to add the assembly of the syscall
  - [x] edit `syscall.h` to add the number of the syscall
  - [x] edit `user.h` to add the signature of syscall
  - [x] edit `syscall.c` to direct the syscall to the correct func.
  - [x] add the wrapper functions to `sysproc.c`
- [x] implement `int clone(void(*fcn)(void *, void *), void *arg1, void *arg2, void *stack)`  
  `fcn`: a pointer, points to function. This function specifically takes 2 arguments.
  Code starts executing here.  
  `arg1`: The first argument of the function.  
  `arg2`: the second argument of the function.  
  `stack`: the independent stack of this thread(function).  
  `return`: the ID of the new thread, as if it is a process.
  - [x] copy `fork()` in `clone()` as a start
  - ~~Implement `allocthread()`~~ NOT NEEDED. `allocproc()` works fine. ~~Does a thread need its own kernel stack? I say yes. Because if it wants to trap into the kernel it needs its own stack with its own stack frame?~~
    - ~~Would it be a problem if all threads used the same kernel stack? IDK, yet.~~
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
  `stack`: a passed-by-reference variable that will contain a pointer to the stack
  of the thread after it exits.
    - [x] waits for the thread to exit
    - [x] returns the ID of the thread to the calling calling main thread, after the
    thread with the stack `stack` exits.
    - [x] wait for a thread if it didnt exit yet
    - ~~a child thread can call a child child thread in the same address space of the main thread and not free it. We have to make join detect such case so even if the main thread only made one thread and the child thread made other threads, join from the main thread should be able to see all those threads so that we don't get massive memory leaks. But, the main thread cannot know how many child threads are still allocated. Maybe another function to return that count or maybe the join function returns a count by a variable passed by reference. Or the main thread just keep joining threads till it gets -1 as the return value, which mean that there are no child thread running and also no child stacks allocated.~~  
      - ~~**fix 1**: Implement a mechanism to keep count of all child and child's child and give the main thread access to that count along with the count of all active threads other than the main thread.~~
      - ~~**fix 2**: disallow a thread to create new threads by requiring a key to a lock before allocating more memory in the address space where this lock is always held by the main thread.~~
        - ~~**problem with fix 2**: this can create a deadlock by the child thread waiting for the lock to be released to continue execution while the main thread can call join on that thread.~~
      - This is handled by the `exit()` function where if I exit and I have
      children, they are all classified as zombies and given away to the
      initproc where the initproc kills them everytime it wakes us.
- [ ] edit `exit()` to comply with exiting a thread.
  - [x] `exit()` should signal to the main thread that this thread has exited.
    - Done through `wakeup1()`
  - ~~`exit()` has to know that the caller is a thread of a process not the whole process.~~
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
  `int thread_create(void (*start_routine)(void *, void *), void *arg1, void *arg2)`
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
  - [x] Implement a simple ticket lock (read [this book chapter](http://pages.cs.wisc.edu/~remzi/OSTEP/threads-locks.pdf))
    - [x] implement `void lock_acquire(lock_t *)`
    - [x] implement `void lock_release(lock_t *)`
