#include <sys/param.h>
#include <sys/socket.h>
#include <sys/sysctl.h>

#include <netinet/in.h>
#include <netinet/ip_var.h>
#include <netinet/tcp.h>
#include <netinet/tcp_timer.h>
#include <netinet/tcp_var.h>

#include <string.h>

#include "usertcp.h"
#include "usertcp_config.h"

void
usertcp_root_helper_init(void)
{
}

void
usertcp_root_server_client(unsigned int sport, struct usertcp_client *client)
{
	static const int mibvar[4] = {
	    CTL_NET, PF_INET, IPPROTO_TCP, TCPCTL_IDENT};
	static struct sockaddr_storage ss[2];
	static struct sockaddr_in *sin[2];
	uid_t uid;
	size_t uidlen;
	socklen_t sslen;

	memset(ss, 0, sizeof(ss));
	sslen = sizeof(ss);
	sin[0] = (struct sockaddr_in *)&ss[0];
	sin[1] = (struct sockaddr_in *)&ss[1];
	sin[0]->sin_len = sizeof(struct sockaddr_in);
	sin[1]->sin_len = sizeof(struct sockaddr_in);
	sin[0]->sin_family = AF_INET;
	sin[1]->sin_family = AF_INET;
	sin[0]->sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	sin[1]->sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	sin[0]->sin_port = htons(sport);
	sin[1]->sin_port = htons(client->port);
	uidlen = sizeof(uid);
	if (sysctl(mibvar, sizeof(mibvar) / sizeof(*mibvar), &uid, &uidlen, &ss,
	        sslen) == -1) {
		warnsys("net.inet.tcp.ident");
		return;
	}
	client->uid = uid;
}
