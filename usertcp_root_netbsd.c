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
	static struct sockaddr_storage ss[2];
	struct sockaddr_in *ssin = (struct sockaddr_in *)&ss[0];
	struct sockaddr_in *csin = (struct sockaddr_in *)&ss[1];
	size_t uidlen;
	uid_t uid;

	uidlen = sizeof(uid);
	uid = 0;
	memset(ss, 0, sizeof(ss));
	ssin->sin_len = csin->sin_len = sizeof(struct sockaddr_in);
	ssin->sin_family = csin->sin_family = AF_INET;
	ssin->sin_addr.s_addr = csin->sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	ssin->sin_port = htons(sport);
	csin->sin_port = htons(client->cport);
	if (sysctl(mib, sizeof(mib) / sizeof(*mib), &uid, &uidlen, ss,
	        sizeof(ss)) == -1) {
		warnsys("sysctl");
		return;
	}
	client->uid = uid;
}
