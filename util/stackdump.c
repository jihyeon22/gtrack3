#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

#include <board/board_system.h>
#include "stackdump.h"

unsigned int get_log_file_size(void);

extern char *strsignal(int __sig) __THROW;

static struct sigaction sa;

void (*stackdump_abort_callback)(void) = NULL;
void (*stackdump_abort_base_callback)(void) = NULL;

void stackdump_init(void)
{
	sa.sa_flags = SA_SIGINFO;
	sa.sa_sigaction = stackdump_abortHandler;
	sigemptyset(&sa.sa_mask);

/*
	sigaction(SIGABRT, &sa, NULL);
	sigaction(SIGSEGV, &sa, NULL);
	sigaction(SIGBUS,  &sa, NULL);
	sigaction(SIGILL,  &sa, NULL);
	sigaction(SIGFPE,  &sa, NULL);
*/

	sigaction(SIGABRT, &sa, NULL);
	sigaction(SIGALRM, &sa, NULL);
	sigaction(SIGBUS, &sa, NULL);
	sigaction(SIGFPE, &sa, NULL);
	//sigaction(SIGHUP, &sa, NULL); //already ignored
	sigaction(SIGILL, &sa, NULL);
	sigaction(SIGINT, &sa, NULL);
	//sigaction(SIGPIPE, &sa, NULL); //already ignored
	sigaction(SIGQUIT, &sa, NULL);
	sigaction(SIGSEGV, &sa, NULL);
	sigaction(SIGUSR1, &sa, NULL);
	sigaction(SIGUSR2, &sa, NULL);
	sigaction(SIGPOLL, &sa, NULL);
	sigaction(SIGPROF, &sa, NULL);
	sigaction(SIGSYS, &sa, NULL);
	sigaction(SIGTRAP, &sa, NULL);
	sigaction(SIGVTALRM, &sa, NULL);
	sigaction(SIGXCPU, &sa, NULL);
	sigaction(SIGXFSZ, &sa, NULL);

}

void stackdump_abortHandler(int signum, siginfo_t* si, void * ucontext)
{
	const char* name = NULL;
	sig_ucontext_t *   uc;
	void *             caller_address;
	FILE *log_fd;

	void* addrlist[256];
	int i;
	int addrlen;
	char** symbollist;

	uc = (sig_ucontext_t *)ucontext;

	if(get_log_file_size() > SIZE_STACKDUMP)
	{
		remove(CALLSTACK_LOG_PATH);
	}

	log_fd = fopen(CALLSTACK_LOG_PATH, "a");

#if defined(__i386__) // gcc specific
	caller_address = (void *) uc->uc_mcontext.eip; // EIP: x86 specific
#elif defined(__x86_64__) // gcc specific
	caller_address = (void *) uc->uc_mcontext.rip; // RIP: x86_64 specific
#elif defined(__arm__)
	//caller_address = (void *) uc->uc_mcontext.eip//.gregs[REG_RIP]; // RIP: arm
	caller_address = (void *) uc->uc_mcontext.arm_pc;
	fprintf(stderr, "fault_address = %p\n", (void *) uc->uc_mcontext.fault_address);
	fprintf(stderr, "arm_pc = %p\n", (void *) uc->uc_mcontext.arm_pc);
	fprintf(stderr, "arm_lr = %p\n", (void *) uc->uc_mcontext.arm_lr);
	fprintf(stderr, "arm_sp = %p\n", (void *) uc->uc_mcontext.arm_sp);
	fprintf(stderr, "arm_ip = %p\n", (void *) uc->uc_mcontext.arm_ip);

	if(log_fd !=  NULL) {	
		fprintf(log_fd, "fault_address = %p\n", (void *) uc->uc_mcontext.fault_address);
		fprintf(log_fd, "arm_pc = %p\n", (void *) uc->uc_mcontext.arm_pc);
		fprintf(log_fd, "arm_lr = %p\n", (void *) uc->uc_mcontext.arm_lr);
		fprintf(log_fd, "arm_sp = %p\n", (void *) uc->uc_mcontext.arm_sp);
		fprintf(log_fd, "arm_ip = %p\n", (void *) uc->uc_mcontext.arm_ip);
	}
#else
#error Unsupported architecture. // TODO: Add support for other arch.
#endif

	//delete stack dump file if size of stackdump is lager than SIZE_STACKDUMP
	

	fprintf(stderr, "signal %d (%s), address is %p from %p\n",
	        signum, strsignal(signum), si->si_addr, (void *)caller_address);
	if(log_fd != NULL) {
		fprintf(log_fd, "signal %d (%s), address is %p from %p\n",
		        signum, strsignal(signum), si->si_addr, (void *)caller_address);
	}

	switch(signum)
	{
		case SIGABRT:
			name = "SIGABRT";
			break;
		case SIGSEGV:
			name = "SIGSEGV";
			break;
		case SIGBUS:
			name = "SIGBUS";
			break;
		case SIGILL:
			name = "SIGILL";
			break;
		case SIGFPE:
			name = "SIGFPE";
			break;
		case SIGPIPE:
			name = "SIGPIPE";
			break;
	}

	if(name) {
		fprintf(stderr, "Caught signal %d (%s)\n", signum, name);
		if(log_fd != NULL) {
			fprintf(log_fd, "Caught signal %d (%s)\n", signum, name);
		}
	}
	else {
		fprintf(stderr, "Caught signal %d\n", signum);
		if(log_fd != NULL) {
			fprintf(log_fd, "Caught signal %d\n", signum);
		}
	}

	// retrieve current stack addresses
	printf("sizeof(addrlist) / sizeof(void*) = [%d]\n", sizeof(addrlist) / sizeof(void*));
	addrlen = backtrace(addrlist, sizeof(addrlist) / sizeof(void*));

	if(addrlen == 0)
	{
		fprintf(stderr, "  <empty, possibly corrupt>\n");
		if(log_fd != NULL) {
			fflush(log_fd);
		}
	}

	symbollist = backtrace_symbols(addrlist, addrlen);

	for(i = 0; i < addrlen; i++) {
		if(symbollist[i] == NULL) {
			break;
		}
		fprintf(stderr, "%s\n", symbollist[i]);
		if(log_fd != NULL) {
			fprintf(log_fd, "%s\n", symbollist[i]);
		}
	}

	free(symbollist);
	printf("\n");

	addrlist[1] = caller_address;
	symbollist = backtrace_symbols(addrlist, addrlen);

	for(i = 0; i < addrlen; i++) {
		if(symbollist[i] == NULL) {
			break;
		}
		fprintf(stderr, "%s\n", symbollist[i]);
		if(log_fd != NULL) {
			fprintf(log_fd, "%s\n", symbollist[i]);
		}
	}

	if(log_fd != NULL) {
		printf("======================================================\n");
	printf("4-A. exit abort\n");
	printf("======================================================\n");

		fprintf(log_fd, "\n\n");
		fflush(log_fd);
		fclose(log_fd);
		printf("======================================================\n");
	printf("4-B. exit abort\n");
	printf("======================================================\n");
	}

	if(stackdump_abort_base_callback != NULL)
	{
		fprintf(stderr, "run stackdump_abort_callback\n");
		stackdump_abort_base_callback();
	}
	
	if(stackdump_abort_callback != NULL)
	{
		fprintf(stderr, "run stackdump_abort_callback\n");
		stackdump_abort_callback();
	}

	printf("======================================================\n");
	printf("5. exit abort\n");
	printf("======================================================\n");
	exit(signum);
}

int stackdump_read_callstack_log(char *buff, const int buff_len)
{
	FILE *log_fd;
	int n_try = 5; //wait for 500 mseconds.
	int size = 0;
	while(n_try--)
	{
		log_fd = fopen(CALLSTACK_LOG_PATH, "r");
		if(log_fd != NULL) {
			size = fread(buff, 1, 1024, log_fd);
			fclose(log_fd);
			return size;
		}
		usleep(100000);
	}
	return -1;
}

unsigned int get_log_file_size(void)
{
	FILE *fp;
	unsigned int sz = 0;
	
	fp = fopen(CALLSTACK_LOG_PATH, "r");
	if(fp == NULL)
	{
		return 0;
	}
	
	fseek(fp, 0L, SEEK_END);
	sz = ftell(fp);
	
	printf("log file size : %u\n", sz);
	
	fclose(fp);
	
	return sz;
}
