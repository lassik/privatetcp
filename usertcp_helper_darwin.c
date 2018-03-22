#include <sys/types.h>
#include <sys/uio.h>

#include <sys/socketvar.h>
#include <sys/sysctl.h>

#include <arpa/inet.h>
#include <netinet/tcp_var.h>

#include <errno.h>
#include <pwd.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "usertcp.h"
#include "usertcp_config.h"

static uid_t
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

void
usertcp_helper(unsigned int sport)
{
	static struct usertcp_client client;
	struct passwd *pw;
	ssize_t nbyte;

	if (setregid(UNPRIVILEGED_GID, UNPRIVILEGED_GID) == -1) {
		diesys("helper: cannot change to unprivileged group");
	}
	if (setreuid(UNPRIVILEGED_UID, UNPRIVILEGED_UID) == -1) {
		diesys("helper: cannot change to unprivileged user");
	}
	for (;;) {
		do {
			nbyte = read(0, &client, sizeof(client));
		} while ((nbyte == -1) && (errno == EINTR));
		if (nbyte == -1) {
			diesys("helper: read");
		}
		if (!nbyte) {
			die("helper: exiting because main process died");
		}
		if (nbyte != sizeof(client)) {
			die("helper: short read");
		}
		client.uid = find_uid(sport, client.port);
		errno = 0;
		pw = client.uid ? getpwuid(client.uid) : 0;
		if (pw) {
			client.gid = pw->pw_gid;
		} else {
			client.uid = 0;
			client.gid = 0;
			if (errno) {
				warnsys("helper: cannot get user info");
			}
		}
		do {
			nbyte = write(1, &client, sizeof(client));
		} while ((nbyte == -1) && (errno == EINTR));
		if (nbyte == -1) {
			diesys("helper: write");
		}
		if (nbyte != sizeof(client)) {
			die("helper: short write");
		}
	}
}
