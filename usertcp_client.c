#include <errno.h>
#include <pwd.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "usertcp.h"
#include "usertcp_config.h"

extern char **environ;

void
usertcp_client(unsigned int sport, unsigned int cport, char *const *argv)
{
	static const char client_path[] = CLIENT_PATH;
	static const char localip[] = "127.0.0.1";
	static const char localhost[] = "localhost";
	static char sportstr[8];
	static char cportstr[8];
	struct passwd *pw;

	errno = 0;
	pw = getpwuid(getuid());
	if (!pw && errno) {
		diesys("client: cannot get user info");
	}
	if (!pw) {
		die("client: user not found");
	}
	if (strstr(pw->pw_shell, "/nologin") ||
	    strstr(pw->pw_shell, "/false")) {
		die("client: user is not allowed to login");
	}
	if (chdir(pw->pw_dir) == -1) {
		diesys("client: cannot change to user's home directory");
	}
	snprintf(sportstr, sizeof(sportstr), "%u", sport);
	snprintf(cportstr, sizeof(cportstr), "%u", cport);
	if (!(environ = calloc(1, sizeof(*environ)))) {
		diesys("client: cannot clear environment");
	}
	if ((setenv("LOGNAME", pw->pw_name, 1) == -1) ||
	    (setenv("USER", pw->pw_name, 1) == -1) ||
	    (setenv("HOME", pw->pw_dir, 1) == -1) ||
	    (setenv("SHELL", pw->pw_shell, 1) == -1) ||
	    (setenv("PATH", client_path, 1) == -1) ||
	    (setenv("PROTO", "TCP", 1) == -1) ||
	    (setenv("TCPLOCALIP", localip, 1) == -1) ||
	    (setenv("TCPLOCALPORT", sportstr, 1) == -1) ||
	    (setenv("TCPREMOTEIP", localip, 1) == -1) ||
	    (setenv("TCPREMOTEPORT", cportstr, 1) == -1) ||
	    (setenv("TCPLOCALHOST", localhost, 1) == -1) ||
	    (setenv("TCPREMOTEHOST", localhost, 1) == -1) ||
	    (setenv("TCPREMOTEINFO", pw->pw_name, 1) == -1)) {
		diesys("client: cannot set environment");
	}
	fprintf(stderr, "%s: client: running as %s(%lu): %s\n", progname,
	    pw->pw_name, (unsigned long)getuid(), argv[0]);
	execv(argv[0], argv);
	diesys("client: exec");
}
