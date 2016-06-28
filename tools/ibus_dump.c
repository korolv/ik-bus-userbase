/* 
 * File:   ibus_dump.c
 * Author: V.Korol
 *
 * Created on February 12, 2015, 8:24 PM
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

#include <signal.h>
#include "linux/ikbus.h"

int running = 1;

void handle_SIGINT(int sig_num)
{
	running = 0;
}

int main(int argc, char** argv)
{
	int s;
	int nbytes;
	struct sigaction sa;
	struct sockaddr_ikbus addr;
	//unsigned char rx_buff[SIZE_BUFF];
	struct ifreq ifr;
	struct cmsghdr *cmsg;
	struct msghdr msg;
	char ctrlmsg[CMSG_SPACE(sizeof(struct timeval)) + CMSG_SPACE(sizeof(uint32_t))];
	struct timeval tv;
	struct ikbus_filter rfilter;

	char *ifname = "ibus0";
	
	msg.msg_namelen = sizeof(addr);
	msg.msg_controllen = sizeof(ctrlmsg);  
	msg.msg_flags = 0;
	
	if((s = socket(PF_IKBUS, SOCK_RAW, 0)) < 0) {
		perror("Error while opening socket");
		return -1;
	}

	rfilter.id_rx = IKBUS_ID_ALL;
	rfilter.id_tx = IKBUS_ID_ALL;
	setsockopt(s, SOL_IKBUS, IKBUS_FILTER, &rfilter, sizeof(rfilter));

	strcpy(ifr.ifr_name, ifname);
	ioctl(s, SIOCGIFINDEX, &ifr);

	addr.ikbus_family  = AF_IKBUS;
	addr.ifindex = ifr.ifr_ifindex; 

	printf("%s at index %d\n", ifname, ifr.ifr_ifindex);

	if(bind(s, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		perror("Error in socket bindd");
		return -2;
	}
	
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = handle_SIGINT;
	if (sigaction(SIGINT, &sa, NULL)) {
		perror("Error in sigaction");
		return -3;
	}

	while (running) {
		nbytes = recvmsg(s, &msg, 0);
		if (nbytes < 0) {
			perror("read");
			continue;
		}
		for (cmsg = CMSG_FIRSTHDR(&msg);
				    cmsg && (cmsg->cmsg_level == SOL_SOCKET);
				    cmsg = CMSG_NXTHDR(&msg,cmsg)) {
			if (cmsg->cmsg_type == SO_TIMESTAMP)
				tv = *(struct timeval *)CMSG_DATA(cmsg);
			printf("(%010ld.%06ld) ", tv.tv_sec, tv.tv_usec);
		}
	}

	close(s);
	printf("\rDone!\n");
	return(EXIT_SUCCESS);
}
