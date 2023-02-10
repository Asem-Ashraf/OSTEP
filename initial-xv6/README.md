
# Intro To Kernel Hacking

The kernel is a port of the original Unix (version 6), and is runnable on modern x86 processors. It was developed at MIT and is a small and relatively understandable OS and thus an excellent focus for simple projects.

This first project is just a warmup, and thus relatively light on work. The
goal of the project is simple: to add a system call to xv6. Your system call,
**getreadcount()**, simply returns how many times that the **read()** system
call has been called by user processes since the time that the kernel was
booted. 

## The System Call

The new system call should look have the following return codes and
parameters: 

```c
int getreadcount(void)
```

The system call returns the value of a counter (perhaps called **readcount**
or something like that) which is incremented every time any process calls the
**read()** system call.

# Running Tests

```sh
$ ./test-getreadcounts.sh
```
