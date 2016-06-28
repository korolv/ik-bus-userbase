/* 
 * File:   ibus_send.c
 * Author: V.Korol
 *
 * Created on February 21, 2015, 8:24 PM
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <net/if.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/ikbus.h>

int main(int argc, char** argv)
{
	int i, n, count, sym, s;
	unsigned char msg[37];
	struct ifreq ifr;
	struct sockaddr_ikbus addr;
	char *ifname = "ibus0";

	if((s = socket(PF_IKBUS, SOCK_RAW, 0)) < 0) {
		perror("Error while opening socket");
		return -1;
	}

	strcpy(ifr.ifr_name, ifname);
	ioctl(s, SIOCGIFINDEX, &ifr);

	addr.ikbus_family  = AF_IKBUS;
	addr.ifindex = ifr.ifr_ifindex;

	if(bind(s, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		perror("Error in socket bindd");
		return -2;
	}

	

	/* Get message */
	count = 0;
	for (i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "CS") || !strcmp(argv[i], "cs")) {
			count++;
			break;
		}

		if (!strcmp(argv[i], "LL") || !strcmp(argv[i], "ll")) {
			count++;
			continue;
		}

		if (sscanf(argv[i], "%X", &sym)) {
			msg[count++] = (unsigned char) sym;
		}
	}
	if (count) {
		msg[1] = count - 2;
		/* Send message */
		n = write(s, msg, count);
		if (n < 1)
			printf("Error: return %d\n", n);
	}

	close(s);
	return(EXIT_SUCCESS);
}
