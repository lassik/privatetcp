#include <sys/types.h>

#include <sys/socket.h>
#include <sys/wait.h>

#include <arpa/inet.h>
#include <netinet/in.h>

#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "sig.h"
#include "usertcp.h"
#include "usertcp_config.h"

static volatile pid_t helper;
static volatile unsigned long nclient;

static void
sigterm(void)
{
	_exit(0);
}

static void
sigchld(void)
{
	pid_t child;

	while ((child = waitpid(-1, 0, WNOHANG)) > 0) {
		if (child == helper) {
			die("exiting because helper process died");
		} else if (nclient > 0) {
			nclient--;
		}
	}
}

static unsigned int
parse_tcp_port(const char *str)
{
	struct servent *s;
	int port, count, len;

	count = sscanf(str, "%d%n", &port, &len);
	if ((count != 1) || ((size_t)len != strlen(str))) {
		if (!(s = getservbyname(str, "tcp"))) {
			die("TCP port number not found for that service");
		}
		port = ntohs(s->s_port);
		endservent();
	}
	if ((port < 1) || (port > 65535)) {
		die("invalid TCP port number");
	}
	return port;
}

int
main(int argc, char *argv[])
{
	static struct usertcp_client client;
	static struct sockaddr_in saddr;
	static struct sockaddr_in caddr;
	ssize_t nbyte;
	int devnull;
	int tohelper[2];
	int fromhelper[2];
	int sockopt, ssock, csock;
	unsigned int sport, cport;
	pid_t cchild;
	socklen_t clen;

	if (geteuid()) {
		die("must be started as root");
	}
	if (chdir(NOBODY_DIR) == -1) {
		diesys("cannot change to unprivileged directory");
	}
	if (argc < 3) {
		usage("port command [arg ...]");
	}
	if (argv[2][0] != '/') {
		die("command must have an absolute path (i.e. start with /)");
	}
	sport = parse_tcp_port(argv[1]);
	sig_block(SIGCHLD);
	sig_catch(SIGCHLD, sigchld);
	sig_catch(SIGTERM, sigterm);
	sig_catch(SIGPIPE, SIG_IGN);
	close(0);
	close(1);
	if ((devnull = open("/dev/null", O_RDWR)) == -1) {
		diesys("cannot open /dev/null");
	}
	dup2(devnull, 0);
	dup2(devnull, 1);
	if ((ssock = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
		diesys("cannot create TCP socket");
	}
	sockopt = 1;
	setsockopt(ssock, SOL_SOCKET, SO_REUSEADDR, &sockopt, sizeof(sockopt));
	saddr.sin_family = AF_INET;
	saddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	saddr.sin_port = htons(sport);
	if (bind(ssock, (struct sockaddr *)&saddr, sizeof(saddr)) == -1) {
		diesys("cannot bind TCP socket");
	}
	if ((pipe(tohelper) == -1) || (pipe(fromhelper) == -1)) {
		diesys("cannot create pipe");
	}
	if ((helper = fork()) == -1) {
		diesys("cannot fork");
	}
	if (!helper) {
		dup2(tohelper[0], 0);
		dup2(fromhelper[1], 1);
		close(tohelper[0]);
		close(tohelper[1]);
		close(fromhelper[0]);
		close(fromhelper[1]);
		close(ssock);
		sig_catch(SIGCHLD, SIG_DFL);
		sig_unblock(SIGCHLD);
		sig_catch(SIGTERM, SIG_DFL);
		sig_catch(SIGPIPE, SIG_DFL);
		usertcp_root_helper_init();
		if (setregid(NOBODY_GID, NOBODY_GID) == -1) {
			diesys("helper: cannot change to unprivileged group");
		}
		if (setreuid(NOBODY_UID, NOBODY_UID) == -1) {
			diesys("helper: cannot change to unprivileged user");
		}
		usertcp_nobody_helper(sport);
		_exit(126);
	}
	close(tohelper[0]);
	close(fromhelper[1]);
	if (listen(ssock, MAX_BACKLOG) == -1) {
		diesys("cannot listen on TCP socket");
	}
	sockopt = fcntl(ssock, F_GETFL, 0);
	sockopt &= ~O_NONBLOCK;
	fcntl(ssock, F_SETFL, sockopt);
	csock = -1;
	for (;;) {
		if (csock != -1) {
			close(csock);
		}
		csock = -1;
		while (nclient >= MAX_CLIENTS) {
			sig_pause();
		}
		sig_unblock(SIGCHLD);
		do {
			clen = sizeof(caddr);
			csock = accept(ssock, (struct sockaddr *)&caddr, &clen);
		} while ((csock == -1) && (errno == EINTR));
		sig_block(SIGCHLD);
		if (csock == -1) {
			warnsys("cannot accept TCP client");
			continue;
		}
		cport = ntohs(caddr.sin_port);
		memset(&client, 0, sizeof(client));
                client.uid = -1;
                client.gid = -1;
		client.port = cport;
		usertcp_root_server_client(sport, &client);
		do {
			nbyte = write(tohelper[1], &client, sizeof(client));
		} while ((nbyte == -1) && (errno == EINTR));
		if (nbyte == -1) {
			warnsys("cannot write to helper");
			continue;
		}
		if (nbyte != sizeof(client)) {
			die("short write to helper");
		}
		do {
			nbyte = read(fromhelper[0], &client, sizeof(client));
		} while ((nbyte == -1) && (errno == EINTR));
		if (nbyte == -1) {
			diesys("cannot read from helper");
		}
		if (nbyte != sizeof(client)) {
			die("short read from helper");
		}
		if (client.port != cport) {
			die("bad port from helper");
		}
		if (client.uid == (uid_t)-1) {
			warn("client user not found");
			continue;
		}
		if (client.uid < MIN_CLIENT_UID) {
			warn("client user id below allowed range");
			continue;
		}
		if (client.uid > MAX_CLIENT_UID) {
			warn("client user id above allowed range");
			continue;
		}
		if ((cchild = fork()) == -1) {
			warnsys("cannot fork");
			continue;
		}
		nclient++;
		if (!cchild) {
			dup2(csock, 0);
			dup2(csock, 1);
			close(csock);
			close(ssock);
			close(tohelper[1]);
			close(fromhelper[0]);
			sig_catch(SIGCHLD, SIG_DFL);
			sig_unblock(SIGCHLD);
			sig_catch(SIGTERM, SIG_DFL);
			sig_catch(SIGPIPE, SIG_DFL);
			if (setregid(client.gid, client.gid) == -1) {
				diesys("client: cannot change to client group");
			}
			if (setreuid(client.uid, client.uid) == -1) {
				diesys("client: cannot change to client user");
			}
			usertcp_client(sport, cport, (char *const *)&argv[2]);
			_exit(126);
		}
	}
	return 0;
}
