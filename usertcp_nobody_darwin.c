#include <sys/types.h>

#include <sys/socketvar.h>
#include <sys/sysctl.h>

#include <arpa/inet.h>
#include <netinet/tcp_var.h>

#include <stdlib.h>

#include "usertcp.h"
#include "usertcp_config.h"

uid_t
find_uid(unsigned int sport, unsigned int cport)
{
	static const char mibvar[] = "net.inet.tcp.pcblist";
	static struct xinpgen *xigs;
	struct xinpgen *xig;
	struct xtcpcb *xt;
	size_t len;

	if (sysctlbyname(mibvar, 0, &len, 0, 0) == -1) {
		warnsys("helper: sysctl");
		return 0;
	}
	if (!(xigs = realloc(xigs, len))) {
		warnsys("helper");
		return 0;
	}
	if (sysctlbyname(mibvar, xigs, &len, 0, 0) == -1) {
		warnsys("helper: sysctl");
		return 0;
	}
	xig = xigs;
	for (;;) {
		xig = (struct xinpgen *)((char *)xig + xig->xig_len);
		if (xig->xig_len <= sizeof(struct xinpgen)) {
			return 0;
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
		if (ntohs(xt->xt_inp.inp_lport) != cport) {
			continue;
		}
		return xt->xt_socket.so_uid;
	}
}
