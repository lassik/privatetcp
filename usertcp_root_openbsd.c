#include <sys/types.h>

#include <sys/socket.h>
#include <sys/socketvar.h>
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
	static const int mib[] = {CTL_NET, PF_INET, IPPROTO_TCP, TCPCTL_IDENT};
	static struct tcp_ident_mapping tim;
	struct sockaddr_in *ssin = (struct sockaddr_in *)&tim.faddr;
	struct sockaddr_in *csin = (struct sockaddr_in *)&tim.laddr;
	size_t timlen;

	timlen = sizeof(tim);
	memset(&tim, 0, sizeof(tim));
	ssin->sin_len = csin->sin_len = sizeof(struct sockaddr_in);
	ssin->sin_family = csin->sin_family = AF_INET;
	ssin->sin_addr.s_addr = csin->sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	ssin->sin_port = htons(sport);
	csin->sin_port = htons(client->cport);
	if (sysctl(mib, sizeof(mib) / sizeof(*mib), &tim, &timlen, 0, 0) ==
	    -1) {
		warnsys("sysctl");
		return;
	}
	if (tim.ruid == -1) {
		return;
	}
	client->uid = tim.ruid;
}
