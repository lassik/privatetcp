struct privatetcp_client {
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

void privatetcp_client(struct privatetcp_client *client, const char *service);

void privatetcp_nobody_helper(void);
void privatetcp_nobody_helper_client(struct privatetcp_client *client);

void privatetcp_root_helper_init(void);
void privatetcp_root_server_client(struct privatetcp_client *client);
