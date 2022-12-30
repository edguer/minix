# My KB

## Build

Run `build_debug.sh` from source root folder. Here is what i does:
- Set variable JOBS to 8 - change to the number of cores in your machine
- Set BUILDVARS, adding the debug flag
- Set USR_SIZE and ROOT_SIZE so the image accommodates the new sizes after the debug compilation - the build script considers the release size for those sections.

## Run

Just run the `run.sh` command, it will start qemu using the Minix image.

## Problem Solving

## Outside the book

### Change welcome message

Just changed [boot2.c](../sys/arch/i386/stand/boot/boot2.c#L291), adding a single printf:

```c
    `printf("\n ---- Version modified by edguer.\n");
```

### Chapter 2 - Processes

#### 44.  Add code to the MINIX 3 kernel to keep track of the number of messages sent from process (or task) i to process (or task) j. Print this matrix when the F4 key is hit.

After some changes, I came to the conclusion it would be easier and cooler to change the `proc` struct and add a counter for messages sent per process at [proc.h](../minix/kernel/proc.h#L137).
1. Changed [proc.h](../minix/kernel/proc.h#L137) to add a new field.
2. Changed [proc.c](../minix/kernel/proc.c#L896) to increment the field every time a message is sent.
3. Changed [dmp_kernel.c](../minix/servers/is/dmp_kernel.c#L298) to show the field in F4 screen.