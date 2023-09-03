# VSCode Integration

## Compiling
If you are using the class vm, to compile, just run `make xv6.img` in the project directory. That will build xv6.img. Assuimging that was successful, you can then run xv6 by running `make qemu`. If you make changes to any xv6, you will likely need to first clean out the "stale" binaries before rebuilding xv6.img. You can clean your build environment with `make clean`.

## Debugging 
You will then want to navigate to
run `make clean && make qemu-vscode` once the gdb server has started, it will wait for connections. You can then 
navigate to your debug console in VSCode and select gdb from the gear icon. You should see an "Attach to QEMU" profile 
available. When you run this, you should connect to the gdb server. Go ahead and try to set up breakpoints and whatnot. 

## Compiling with Docker (if you want to)
If you are already a linux power user, you can compile natively with docker. You can compile xv6 with docker my running `make docker`. That will run an ubuntu 18.04 container that will compile xv6.img for you. I would not recommend relying on this unless you are already profcient with docker.

## Dependencies (if you are not on the VM)
If you are compiling natively, will need to add a plugin to your vscode. To install the necessary dependencies for debugging in vscode, press `ctrl p` then enter `ext install webfreak.debug`. 
That will install [this](https://github.com/WebFreak001/code-debug) vscode module. You will also need to install
gdb and lldb if you haven't already. On debian that is `sudo apt update && sudo apt install -y gdb lldb`, on arch 
(btw) `sudo pacman -S gdb lldb`, and alpine `sudo apk add --update gdb lldb`.

# Testing the Lottery Scheduler
To test the lottery scheduler run `make clean; make qemu-test-scheduler` the kernel will be compiled and will start collecting data about the process. This is done by editing the `user/sh.c` program to run multiple process with different ticket counts and a looped version of the `ps` program to collect data of the running process over time. After qemu is closed, the `user/sh.c` code is returned to its original form.
After about a minute or so of running qemu, manually press `Ctrl-a then x` to close qemu and save the collected data. The more you let qemu run the more data it will collect which will enhance the smoothness of the graph. To use the kernel normally, you will need to run `make clean` to reverse the effects of the test, otherwise, the shell will keep running `ps`.

# Graph of Running Processes
To generate the graph run `make graph`. This will run a python script and will generate a `graph.png` file in the `../graph` directory
