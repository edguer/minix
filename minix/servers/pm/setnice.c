#include "pm.h"
#include <minix/callnr.h>
#include <minix/endpoint.h>
#include <limits.h>
#include <minix/com.h>
#include <signal.h>
#include "mproc.h"

int do_setnice(void)
{
    int r;
    register struct mproc *pproc = mp;
    register struct mproc *cproc;
    pid_t pid = m_in.m_lc_pm_setnice.pid;
    int nice = m_in.m_lc_pm_setnice.nice;

    // Finds the child process
    if ((cproc = find_proc(pid)) == NULL)
        return(ESRCH);

    // Fails if the process is not really the child of the calling process
    if (mproc[cproc->mp_parent].mp_pid != pproc->mp_pid)
        return(EACCES);

    // Code extracted from misc.c#270
    // Only root can reduce priority
	if (cproc->mp_nice > nice && mp->mp_effuid != SUPER_USER)
		return(EACCES);

	if ((r = sched_nice(cproc, nice)) != OK) {
		return r;
	}

	cproc->mp_nice = nice;
	return(OK);
}