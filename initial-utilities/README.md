# Unix Utilities

In this project, a few different UNIX utilities will be built, simple versions
of commonly used commands like **cat**, **ls**, etc. We'll call each of them a
slightly different name to avoid confusion; for example, instead of **cat**,
you'll be implementing **wcat** (i.e., "wisconsin" cat).

Objectives:

* Re-familiarize yourself with the C programming language
* Re-familiarize yourself with a shell / terminal / command-line of UNIX
* Learn (as a side effect) how to use a proper code editor such as emacs
* Learn a little about how UNIX utilities are implemented

While the project focuses upon writing simple C programs, it can be seen from the
above that even that requires a bunch of other previous knowledge, including a
basic idea of what a shell is and how to use the command line on some
UNIX-based systems (e.g., Linux or macOS), how to use an editor such as emacs,

Summary of what gets turned in:

* A bunch of single .c files for each of the utilities below: **wcat.c**,
  **wgrep.c**, **wzip.c**, and **wunzip.c**.
* Each should compile successfully when compiled with the **-Wall** and
  **-Werror** flags.
* Each should pass the tests.

## wcat

The program **wcat** is a simple program. Generally, it reads a file as
specified by the user and prints its contents. A typical usage is as follows,
in which the user wants to see the contents of main.c, and thus types:

```
prompt> ./wcat main.c
#include <stdio.h>
...
```

As shown, **wcat** reads the file **main.c** and prints out its contents.
The "**./**" before the **wcat** above is a UNIX thing; it just tells the
system which directory to find **wcat** in (in this case, in the "." (dot)
directory, which means the current working directory).

## wgrep

The second utility that is built is called **wgrep**, a variant of the UNIX
tool **grep**. This tool looks through a file, line by line, trying to find a
user-specified search term in the line. If a line has the word within it, the
line is printed out, otherwise it is not.

Here is how a user would look for the term **foo** in the file **bar.txt**:

```
prompt> ./wgrep foo bar.txt
this line has foo in it
so does this foolish line; do you see where?
even this line, which has barfood in it, will be printed.
```

## wzip and wunzip

The next tools that will be built come in a pair, because one (**wzip**) is a
file compression tool, and the other (**wunzip**) is a file decompression
tool.

The type of compression used here is a simple form of compression called
*run-length encoding* (*RLE*). RLE is quite simple: when encountering **n**
characters of the same type in a row, the compression tool (**wzip**) will
turn that into the number **n** and a single instance of the character.

Thus, if we had a file with the following contents:

```
aaaaaaaaaabbbb
```

the tool would turn it (logically) into:

```
10a4b
```

However, the exact format of the compressed file is quite important; here,
you will write out a 4-byte integer in binary format followed by the single
character in ASCII. Thus, a compressed file will consist of some number of
5-byte entries, each of which is comprised of a 4-byte integer (the run length) and the single character.

The **wunzip** tool simply does the reverse of the **wzip** tool, taking

in a compressed file and writing (to standard output again) the uncompressed
results. For example, to see the contents of **file.txt**, you would type:

```
prompt> ./wunzip file.z
```

**wunzip** should read in the compressed file (likely using **fread()**)
and print out the uncompressed output to standard output using **printf()**.
