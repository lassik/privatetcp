struct usertcp_client {
	unsigned int sport;
	unsigned int cport;
	uid_t uid;
	gid_t gid;
};

extern const char progname[];

void usage(const char *args);
void warn(const char *msg);
void warnsys(const char *msg);
void die(const char *msg);
void diesys(const char *msg);

void usertcp_client(struct usertcp_client *client, const char *service);

void usertcp_nobody_helper(void);
void usertcp_nobody_helper_client(struct usertcp_client *client);

void usertcp_root_helper_init(void);
void usertcp_root_server_client(struct usertcp_client *client);
