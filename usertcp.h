struct usertcp_client {
	uint64_t time;
	uint64_t port;
	uint64_t uid;
	uint64_t gid;
};

extern const char progname[];

void usage(const char *args);
void warn(const char *msg);
void warnsys(const char *msg);
void die(const char *msg);
void diesys(const char *msg);

void usertcp_client(unsigned int sport, unsigned int cport,
	char *const *argv);
void usertcp_helper(unsigned int sport);
uid_t find_uid(unsigned int sport, unsigned int cport);
