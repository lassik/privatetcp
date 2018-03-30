#include <sys/types.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "usertcp.h"
#include "usertcp_config.h"

const char progname[] = PROGNAME;

void
usage(const char *args)
{
	fprintf(stderr, "usage: %s %s\n", progname, args);
	exit(1);
}

void
warn(const char *msg)
{
	fprintf(stderr, "%s: %s\n", progname, msg);
}

void
warnsys(const char *msg)
{
	if (msg) {
		fprintf(stderr, "%s: %s: %s\n", progname, msg, strerror(errno));
	} else {
		warn(strerror(errno));
	}
}

void
die(const char *msg)
{
	warn(msg);
	exit(1);
}

void
diesys(const char *msg)
{
	warnsys(msg);
	exit(1);
}
