#include <sys/types.h>

#include "usertcp.h"
#include "usertcp_config.h"

void
usertcp_nobody_helper_client(unsigned int sport, struct usertcp_client *client)
{
	(void)sport;
	(void)client;
}
