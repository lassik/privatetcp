#include <sys/types.h>

#include <stdio.h>

#include "config.h"
#include "privatetcp.h"

FILE *privatetcp_helper_proc_net_tcp;

void
privatetcp_root_helper_init(void)
{
	if (!(privatetcp_helper_proc_net_tcp = fopen("/proc/net/tcp", "rb"))) {
		diesys("cannot open /proc/net/tcp");
	}
}

void
privatetcp_root_server_client(struct privatetcp_client *client)
{
	(void)client;
}
