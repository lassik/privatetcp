#include <sys/types.h>

#include <sys/socket.h>
#include <sys/sysctl.h>
#include <sys/ucred.h>

#include <netinet/in.h>

#include <stdint.h>
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
	static const char mibvar[] = "net.inet.tcp.getcred";
	static struct sockaddr_in sin[2];
	static struct ucred uc;
	size_t ucsize;

	ucsize = sizeof(uc);
	memset(&uc, 0, sizeof(uc));
	memset(sin, 0, sizeof(sin));
	sin[0].sin_len = sin[1].sin_len = sizeof(struct sockaddr_in);
	sin[0].sin_family = sin[1].sin_family = AF_INET;
	sin[0].sin_addr.s_addr = sin[1].sin_addr.s_addr =
	    htonl(INADDR_LOOPBACK);
	sin[0].sin_port = htons(client->port);
	sin[1].sin_port = htons(sport);
	if (sysctlbyname(mibvar, &uc, &ucsize, sin, sizeof(sin)) == -1) {
		warnsys(mibvar);
		return;
	}
	client->uid = uc.cr_uid;
}
