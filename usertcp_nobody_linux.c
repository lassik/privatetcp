#include <sys/types.h>

#include <arpa/inet.h>
#include <netinet/in.h>

#include <errno.h>
#include <stdio.h>

#include "usertcp.h"
#include "usertcp_config.h"

extern FILE *usertcp_helper_proc_net_tcp;

void
usertcp_nobody_helper_client(unsigned int sport, struct usertcp_client *client)
{
	static char buf[128];
	long local_addr, remote_addr, uid, x;
	unsigned int local_port, remote_port;
	int firstline, n;

	firstline = 1;
	while (fgets(buf, sizeof(buf) - 1, usertcp_helper_proc_net_tcp)) {
		if (firstline) {
			firstline = 0;
			continue;
		}
		n = sscanf(buf,
		    "%ld: %lX:%X %lX:%X %lX %lX:%lX %lX:%lX %lX %ld %ld %ld",
		    &x, &local_addr, &local_port, &remote_addr, &remote_port,
		    &x, &x, &x, &x, &x, &x, &uid, &x, &x);
		if (n < 12) {
			continue;
		}
		if (ntohl(local_addr) != INADDR_LOOPBACK) {
			continue;
		}
		if (ntohl(remote_addr) != INADDR_LOOPBACK) {
			continue;
		}
		if (local_port != client->port) {
			continue;
		}
		if (remote_port != sport) {
			continue;
		}
		client->uid = uid;
		break;
	}
	fseek(usertcp_helper_proc_net_tcp, 0, SEEK_SET);
}
