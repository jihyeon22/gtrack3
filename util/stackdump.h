#ifndef __UTIL_STACKDUMP_H__
#define __UTIL_STACKDUMP_H__

#include <execinfo.h>
#include <signal.h>

#define CALLSTACK_LOG_PATH "/data/mds/log/abort.log"
#define SIZE_STACKDUMP 100*1024

typedef struct _sig_ucontext {
	unsigned long     uc_flags;
	struct ucontext   *uc_link;
	stack_t           uc_stack;
	struct sigcontext uc_mcontext;
	sigset_t          uc_sigmask;
} sig_ucontext_t;

extern void (*stackdump_abort_callback)(void);
extern void (*stackdump_abort_base_callback)(void);

void stackdump_init(void);
void stackdump_abortHandler(int signum, siginfo_t* si, void * ucontext);
int stackdump_read_callstack_log(char *buff, const int buff_len);

#endif /* __STACKDUMP_H__ */
