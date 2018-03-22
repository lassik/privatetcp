#include <stdint.h>

#include "usertcp.h"
#include "usertcp_config.h"

void
usertcp_root_helper_init(void)
{
}

void
usertcp_root_server_client(unsigned int sport, struct usertcp_client *client)
{
	(void)sport;
	(void)client;
}
