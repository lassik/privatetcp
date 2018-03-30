#include <sys/types.h>

#include <sys/socket.h>
#include <sys/wait.h>

#include <arpa/inet.h>
#include <netinet/in.h>

#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "sig.h"
#include "usertcp.h"
#include "usertcp_config.h"

static volatile pid_t helper;
static volatile unsigned long nclient;
static unsigned int *ports;
static char **services;

static void
sigterm(void)
{
	if (helper) {
		kill(helper, SIGTERM);
		waitpid(helper, 0, WNOHANG);
	}
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

static int
sort_uniq(int n, unsigned int *vals)
{
	int newlen, tail, mini, m;
	unsigned int newval;

	/* Selection sort */
	for (newlen = tail = 0; tail < n; tail++) {
		for (mini = m = tail; m < n; m++) {
			if (vals[mini] > vals[m]) {
				mini = m;
			}
		}
		newval = vals[mini];
		vals[mini] = vals[tail];
		vals[tail] = newval;
		if ((newlen > 0) && (vals[newlen - 1] == newval)) {
			/* Remove duplicate */
			continue;
		}
		vals[newlen++] = newval;
	}
	return newlen;
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
	}
	if ((port < 1) || (port > 65535)) {
		die("invalid TCP port number");
	}
	return port;
}

static int
parse_tcp_ports_and_services(int n, char **strs)
{
	struct servent *s;

	if (!(ports = calloc(n, sizeof(*ports)))) {
		diesys(0);
	}
	for (int i = 0; i < n; i++) {
		ports[i] = parse_tcp_port(strs[i]);
	}
	n = sort_uniq(n, ports);
	if (!(services = calloc(n, sizeof(*services)))) {
		diesys(0);
	}
	for (int i = 0; i < n; i++) {
		if (!(s = getservbyport(htons(ports[i]), "tcp"))) {
			services[i] = "";
		} else if (!(services[i] = strdup(s->s_name))) {
			diesys(0);
		}
	}
	endservent();
	return n;
}

int
main(int argc, char *argv[])
{
	static struct usertcp_client client;
	static struct sockaddr_in sin;
	static struct pollfd *fds;
	ssize_t nbyte;
	int devnull;
	int tohelper[2];
	int fromhelper[2];
	int csock;
	unsigned int cport;
	pid_t cchild;

	if (geteuid()) {
		die("must be started as root");
	}
	if (chdir(NOBODY_DIR) == -1) {
		diesys("cannot change to unprivileged directory");
	}
	if (argc < 2) {
		usage("port [port ...]");
	}
	argv++;
	argc--;
	argc = parse_tcp_ports_and_services(argc, argv);
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
	if (fcntl(2, F_SETFD, FD_CLOEXEC) == -1) {
		diesys("cannot close standard error on exec");
	}
	if (!(fds = calloc(argc, sizeof(*fds)))) {
		diesys(0);
	}
	for (int i = 0; i < argc; i++) {
		int opt;
		fprintf(stderr, "serving on port %u\n", ports[i]);
		if ((fds[i].fd = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
			diesys("cannot create TCP socket");
		}
		opt = 1;
		setsockopt(
		    fds[i].fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
		sin.sin_family = AF_INET;
		sin.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
		sin.sin_port = htons(ports[i]);
		if (bind(fds[i].fd, (struct sockaddr *)&sin, sizeof(sin)) ==
		    -1) {
			diesys("cannot bind TCP socket");
		}
		if (listen(fds[i].fd, MAX_BACKLOG) == -1) {
			diesys("cannot listen on TCP socket");
		}
		opt = fcntl(fds[i].fd, F_GETFL, 0);
		opt &= ~O_NONBLOCK;
		fcntl(fds[i].fd, F_SETFL, opt);
		fds[i].events = POLLIN;
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
		for (int k = 0; k < argc; k++) {
			close(fds[k].fd);
		}
		sig_catch(SIGCHLD, SIG_DFL);
		sig_unblock(SIGCHLD);
		sig_catch(SIGTERM, SIG_DFL);
		sig_catch(SIGPIPE, SIG_DFL);
		usertcp_root_helper_init();
		if (setregid(NOBODY_GID, NOBODY_GID) == -1) {
			diesys("helper: cannot change to "
			       "unprivileged group");
		}
		if (setreuid(NOBODY_UID, NOBODY_UID) == -1) {
			diesys("helper: cannot change to "
			       "unprivileged user");
		}
		usertcp_nobody_helper();
		_exit(126);
	}
	close(tohelper[0]);
	close(fromhelper[1]);
	csock = -1;
	for (;;) {
		if (csock != -1) {
			close(csock);
		}
		csock = -1;
		if (poll(fds, argc, -1) == -1) {
			if ((errno == EINTR) || (errno == EAGAIN)) {
				continue;
			}
			diesys("cannot poll");
		}
		for (int i = 0; i < argc; i++) {
			if (csock != -1) {
				close(csock);
			}
			csock = -1;
			if (!(fds[i].revents & POLLIN)) {
				continue;
			}
			while (nclient >= MAX_CLIENTS) {
				sig_pause();
			}
			sig_unblock(SIGCHLD);
			do {
				socklen_t clen = sizeof(sin);
				csock = accept(
				    fds[i].fd, (struct sockaddr *)&sin, &clen);
			} while ((csock == -1) && (errno == EINTR));
			sig_block(SIGCHLD);
			if (csock == -1) {
				warnsys("cannot accept TCP client");
				continue;
			}
			cport = ntohs(sin.sin_port);
			memset(&client, 0, sizeof(client));
			client.sport = ports[i];
			client.cport = cport;
			client.uid = -1;
			client.gid = -1;
			usertcp_root_server_client(&client);
			do {
				nbyte =
				    write(tohelper[1], &client, sizeof(client));
			} while ((nbyte == -1) && (errno == EINTR));
			if (nbyte == -1) {
				warnsys("cannot write to helper");
				continue;
			}
			if (nbyte != sizeof(client)) {
				die("short write to helper");
			}
			do {
				nbyte = read(
				    fromhelper[0], &client, sizeof(client));
			} while ((nbyte == -1) && (errno == EINTR));
			if (nbyte == -1) {
				diesys("cannot read from helper");
			}
			if (nbyte != sizeof(client)) {
				die("short read from helper");
			}
			if ((client.sport != ports[i]) ||
			    (client.cport != cport)) {
				die("bad port from helper");
			}
			if (client.uid == (uid_t)-1) {
				warn("client user not found");
				continue;
			}
			if (client.uid < MIN_CLIENT_UID) {
				warn("client user id below allowed "
				     "range");
				continue;
			}
			if (client.uid > MAX_CLIENT_UID) {
				warn("client user id above allowed "
				     "range");
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
				close(tohelper[1]);
				close(fromhelper[0]);
				for (int k = 0; k < argc; k++) {
					close(fds[k].fd);
				}
				sig_catch(SIGCHLD, SIG_DFL);
				sig_unblock(SIGCHLD);
				sig_catch(SIGTERM, SIG_DFL);
				sig_catch(SIGPIPE, SIG_DFL);
				if (setregid(client.gid, client.gid) == -1) {
					diesys("client: cannot change "
					       "to client group");
				}
				if (setreuid(client.uid, client.uid) == -1) {
					diesys("client: cannot change "
					       "to client user");
				}
				usertcp_client(&client, services[i]);
				_exit(126);
			}
		}
	}
	return 0;
}
