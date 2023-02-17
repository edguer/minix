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
    printf("\n ---- Version modified by edguer.\n");
```

### Chapter 2 - Processes

#### *44.  Add code to the MINIX 3 kernel to keep track of the number of messages sent from process (or task) i to process (or task) j. Print this matrix when the F4 key is hit.*

After some changes, I came to the conclusion it would be easier and cooler to change the `proc` struct and add a counter for messages sent per process at [proc.h](../minix/kernel/proc.h#L137).
1. Changed [proc.h](../minix/kernel/proc.h#L137) to add a new field.
2. Changed [proc.c](../minix/kernel/proc.c#L896) to increment the field every time a message is sent.
3. Changed [dmp_kernel.c](../minix/servers/is/dmp_kernel.c#L298) to show the field in F4 screen.

#### *45. Modify the MINIX 3 scheduler to keep track of how much CPU time each user process has had recently. When no task or server wants to run, pick the user process that has had the smallest share of the CPU.*

For part one, CPU time is already being stored at `proc`'s `p_user_time` and `p_sys_time`, and being displayed at F1 screen, at [dmp_kernel.c](../minix/servers/is/dmp_kernel.c#L348). The math is being done inside the [clock's interrupt handling process](../minix/kernel/clock.c#L116). Both fields are filled with ticks: every clock tick they are incremented. We did a change to store the time in milliseconds - MIB server has such an example [here](../minix/servers/mib/proc.c#L164). Since this could be an expensive call, we only made the math when the process is unscheduled at the [dequeue proc function](../minix/kernel/proc.c#L1800), the only difference to what MIB does is the `sys_hz()` call: apparently it can only be called from the user space, since we get segfaults if we try doing it from the kernel, so we just used the default hertz value, which is 1000 for x86.

For the second part, we tried to change the [pick_proc](../minix/kernel/proc.c#L1823) returning the `proc` found in the `proc` table, but it failed with no errors - Minix just hanged. Same happened if we tried to do it at the `switch_to_user`, and even after enqueuing the process instead of just returning it, which makes sense since the queues are cpu variables, but that did not work - Minix hangs.

To pick up a user process we have to understand how to find them, and where processes reside. First of all, there is a global process table, the `proc` array, used across the board, and there are per-cpu queues for ready processes. So when the scheduler or the kernel decides a process is ready to run, its address is added to the ready queue and then picked up later. So we need to find the processes in the global table and enqueue it, so it gets to a per-process queue.

To traverse the global process table, one can use the `BEG_PROC_ADDR`, `BEG_USER_ADDR`, and `END_PROC_ADDR` macros and read each memory location to get the `proc` structures in the list. Later, we need to make sure that the process we pick up is ready to be executed, otherwise the kernel will crash. For that, we use the `proc_is_runnable` macro, which essentially checks if the process flags are empty. If it is runnable, in theory we can enqueue it.

Even after doing all the checks, Minix hangs if we keep booking those processes. The problem might be that if we keep booking and switching processes like this, the machine will keep busy and hang?

#### *46. Modify MINIX 3 so that each process can explicitly set the scheduling priority of its children using a new system call setpriority with parameters pid and priority.*

Setting up a new system call is simple, but requires a few steps and changes in a lot of files. We opted to call it `setnice`, since `setpriority` was already being used. The draw below shows the basic flow that a syscall takes when in process manager server:

![image](syscall.png)

1. We must change the user library (libc) to add the function
2. The libc function calls `_syscall` function (providing the system call number), which essentially sends the message to the IPC server with `sendrec`, do we wait a reply
3. The IPC server dispatches the message to the PM server
4. The PM server reads the message number and call the appropriate function, in this case it is do_setnice.

Let's drill down on the changes...

1. The first thing we had to do was to add our function prototype to the libc header file: [unistd.h](include/unistd.h#112)
2. Then, we had to add a system call number at [callnr.h](minix/include/minix/callnr.h#40), notice that because we are adding one more syscall to the PM, the `NR_PM_CALLS` definition was increased by one
3. To be able to communicate with the PM, we must add our data parameters to the message. For that, we had to add a additional struct field called `m_lc_pm_setnice` containing our parameters to the `message` struct at [ipc.h](minix/include/minix/ipc.h#2411) header. Of course we had to create a struct for our case, located at that same file at [line 532](minix/include/minix/ipc.h#532).
4. With all that setup, we were good to create the implementation. First one was the libc function implementation, and for that we opted to create a new source file called [setnice.c](minix/lib/libc/sys/setnice.c). We simply create the message and fill our parameters, then send it using `_syscall`, passing the PM process number and our system call number as arguments. Notice the source file must me added to the [Makefile.inc](minix/lib/libc/sys/Makefile.inc)
5. Next step is to create the prototype for the `do_setnice` function for the PM, and this was done inside the [proto.h](minix/servers/pm/proto.h#26) file
6. We also need to associate our system call number to the `do_setnice` function at [table.c](minix/servers/pm/table.c#62)
7. Now we can write the `do_setnice` implementation at the PM. We did that at the [setnice.c](minix/servers/pm/setnice.c) file. In summary, we look for that process, check if it is really child of the calling process and then just use the `sched_nice` function to change the priority and update the process entry with the new value at the PM processes' list