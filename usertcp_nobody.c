#include <sys/types.h>
#include <sys/uio.h>

#include <errno.h>
#include <pwd.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "usertcp.h"
#include "usertcp_config.h"

void
usertcp_nobody_helper(void)
{
	static struct usertcp_client client;
	struct passwd *pw;
	ssize_t nbyte;

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
		usertcp_nobody_helper_client(&client);
		errno = 0;
		pw = (client.uid == (uid_t)-1) ? 0 : getpwuid(client.uid);
		if (pw) {
			client.gid = pw->pw_gid;
		} else {
			client.uid = -1;
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
