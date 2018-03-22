#include <stdint.h>
#include <stdio.h>

#include "usertcp.h"
#include "usertcp_config.h"

FILE *usertcp_helper_proc_net_tcp;

void
usertcp_root_helper_init(void)
{
	if (!(usertcp_helper_proc_net_tcp = fopen("/proc/net/tcp", "rb"))) {
		diesys("cannot open /proc/net/tcp");
	}
}

void
usertcp_root_server_client(unsigned int sport, struct usertcp_client *client)
{
	(void)sport;
	(void)client;
}
