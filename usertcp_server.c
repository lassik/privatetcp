#include <sys/socket.h>
#include <sys/wait.h>

#include <arpa/inet.h>
#include <netinet/in.h>

#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <signal.h>
#include <string.h>
#include <time.h>
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

int
main(int argc, char *argv[])
{
	static struct usertcp_client client;
	static struct timespec tv;
	static struct sockaddr_in saddr;
	static struct sockaddr_in caddr;
	static struct servent *sserv;
	uint64_t ctime;
	ssize_t nbyte;
	uid_t cuid;
	gid_t cgid;
	int devnull;
	int tohelper[2];
	int fromhelper[2];
	int sockopt, ssock, csock;
	unsigned int sport, cport;
	pid_t cchild;
	socklen_t clen;

	if (chdir(UNPRIVILEGED_DIR) == -1) {
		diesys("cannot change to unprivileged directory");
	}
	if (argc < 3) {
		usage("port command [arg ...]");
	}
	if (!(sserv = getservbyname(argv[1], "tcp"))) {
		die("TCP port not found");
	}
	sport = ntohs(sserv->s_port);
	endservent();
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
	if ((ssock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		diesys("cannot create TCP server (socket())");
	}
	sockopt = 1;
	setsockopt(ssock, SOL_SOCKET, SO_REUSEADDR, &sockopt, sizeof(sockopt));
	saddr.sin_family = AF_INET;
	saddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	saddr.sin_port = htons(sport);
	if (bind(ssock, (struct sockaddr *)&saddr, sizeof(saddr)) == -1) {
		diesys("cannot create TCP server (socket())");
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
		usertcp_helper(sport);
		_exit(126);
	}
	close(tohelper[0]);
	close(fromhelper[1]);
	if (listen(ssock, MAX_BACKLOG) == -1) {
		diesys("cannot create TCP server (listen())");
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
		memset(&tv, 0, sizeof(tv));
		clock_gettime(CLOCK_MONOTONIC, &tv);
		ctime = tv.tv_sec * 1000000000 + tv.tv_nsec;
		cport = ntohs(caddr.sin_port);
		memset(&client, 0, sizeof(client));
		client.time = ctime;
		client.port = cport;
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
		if (client.time != ctime) {
			die("bad time from helper");
		}
		if (client.port != cport) {
			die("bad port from helper");
		}
		cuid = client.uid;
		cgid = client.gid;
		memset(&client, 0, sizeof(client));
		if (!cuid) {
			warn("client user not found");
			continue;
		}
		if (cuid < MIN_CLIENT_UID) {
			warn("client user id below allowed range");
			continue;
		}
		if (cuid > MAX_CLIENT_UID) {
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
			if (setregid(cgid, cgid) == -1) {
				diesys("client: cannot change to client group");
			}
			if (setreuid(cuid, cuid) == -1) {
				diesys("client: cannot change to client user");
			}
			usertcp_client(sport, cport, (char *const *)&argv[2]);
			_exit(126);
		}
	}
	return 0;
}
