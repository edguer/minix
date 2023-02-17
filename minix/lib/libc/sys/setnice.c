#include <sys/cdefs.h>
#include "namespace.h"
#include <lib.h>

#include <string.h>
#include <unistd.h>

#ifdef __weak_alias
__weak_alias(setnice, _setnice)
#endif

int setnice(pid_t pid, int nice)
{
  message m;

  memset(&m, 0, sizeof(m));
  m.m_lc_pm_setnice.pid = pid;
  m.m_lc_pm_setnice.nice = nice;
  
  return _syscall(PM_PROC_NR, PM_SETNICE, &m);
}
