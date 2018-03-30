#include <sys/types.h>

#include <sys/socket.h>
#include <sys/sysctl.h>
#include <sys/ucred.h>

#include <netinet/in.h>

#include <string.h>

#include "usertcp.h"
#include "usertcp_config.h"

void
usertcp_root_helper_init(void)
{
}

void
usertcp_root_server_client(struct usertcp_client *client)
{
	static const char mib[] = "net.inet.tcp.getcred";
	static struct sockaddr_in ss[2];
	struct sockaddr_in *ssin = (struct sockaddr_in *)&ss[1];
	struct sockaddr_in *csin = (struct sockaddr_in *)&ss[0];
	static struct ucred cr;
	size_t crlen;

	crlen = sizeof(cr);
	memset(&cr, 0, sizeof(cr));
	memset(ss, 0, sizeof(ss));
	ssin->sin_len = csin->sin_len = sizeof(struct sockaddr_in);
	ssin->sin_family = csin->sin_family = AF_INET;
	ssin->sin_addr.s_addr = csin->sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	ssin->sin_port = htons(client->sport);
	csin->sin_port = htons(client->cport);
	if (sysctlbyname(mib, &cr, &crlen, ss, sizeof(ss)) == -1) {
		warnsys("sysctl");
		return;
	}
	client->uid = cr.cr_uid;
}
