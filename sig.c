/* Adapted from D. J. Bernstein's public domain library. */

#include <signal.h>

void
sig_pause(void)
{
	sigset_t ss;

	sigemptyset(&ss);
	sigsuspend(&ss);
}

void
sig_block(int sig)
{
	sigset_t ss;

	sigemptyset(&ss);
	sigaddset(&ss, sig);
	sigprocmask(SIG_BLOCK, &ss, 0);
}

void
sig_unblock(int sig)
{
	sigset_t ss;

	sigemptyset(&ss);
	sigaddset(&ss, sig);
	sigprocmask(SIG_UNBLOCK, &ss, 0);
}

void
sig_catch(int sig, void (*func)())
{
	struct sigaction sa;

	sa.sa_handler = func;
	sa.sa_flags = 0;
	sigemptyset(&sa.sa_mask);
	sigaction(sig, &sa, 0);
}
