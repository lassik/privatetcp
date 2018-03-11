void sig_pause(void);
void sig_block(int sig);
void sig_unblock(int sig);
void sig_catch(int sig, void (*func)());
