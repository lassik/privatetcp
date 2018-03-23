#include <sys/types.h>

#include <sys/socketvar.h>
#include <sys/sysctl.h>

#include <arpa/inet.h>
#include <netinet/tcp_var.h>

#include <stdlib.h>
#include <string.h>

#include "usertcp.h"
#include "usertcp_config.h"

void
usertcp_nobody_helper_client(unsigned int sport, struct usertcp_client *client)
{
	static const char mibvar[] = "net.inet.tcp.pcblist";
	static struct xinpgen *xigs;
	struct xinpgen *xig;
	struct xtcpcb *xt;
	size_t len;

	if (sysctlbyname(mibvar, 0, &len, 0, 0) == -1) {
		warnsys("helper: sysctl");
		return;
	}
	if (!(xigs = realloc(xigs, len))) {
		warnsys("helper");
		return;
	}
	memset(xigs, 0, len);
	if (sysctlbyname(mibvar, xigs, &len, 0, 0) == -1) {
		warnsys("helper: sysctl");
		return;
	}
	xig = xigs;
	for (;;) {
		xig = (struct xinpgen *)((char *)xig + xig->xig_len);
		if (xig->xig_len <= sizeof(struct xinpgen)) {
			break;
		}
		xt = (struct xtcpcb *)xig;
		if (xt->xt_inp.inp_gencnt > xigs->xig_gen) {
			continue;
		}
		if (ntohl(xt->xt_inp.inp_faddr.s_addr) != INADDR_LOOPBACK) {
			continue;
		}
		if (ntohl(xt->xt_inp.inp_laddr.s_addr) != INADDR_LOOPBACK) {
			continue;
		}
		if (ntohs(xt->xt_inp.inp_fport) != sport) {
			continue;
		}
		if (ntohs(xt->xt_inp.inp_lport) != client->port) {
			continue;
		}
		client->uid = xt->xt_socket.so_uid;
		break;
	}
}
