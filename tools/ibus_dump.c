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
#include <sys/time.h>

#include <signal.h>
#include "linux/ikbus.h"

#define SIZE_BUFF 37
#define AF_IBUS 41
#define PF_IBUS AF_IBUS

/* background colors */
#define BGBLACK   "\33[40m"
#define BGRED     "\33[41m"
#define BGGREEN   "\33[42m"
#define BGYELLOW  "\33[43m"
#define BGBLUE    "\33[44m"
#define BGMAGENTA "\33[45m"
#define BGCYAN    "\33[46m"
#define BGWHITE   "\33[47m"

/* foreground colors */
#define FGBLACK   "\33[30m"
#define FGRED     "\33[31m"
#define FGGREEN   "\33[32m"
#define FGYELLOW  "\33[33m"
#define FGBLUE    "\33[34m"
#define FGMAGENTA "\33[35m"
#define FGCYAN    "\33[36m"
#define FGWHITE   "\33[37m"
/* bold */
#define ATTBOLD      "\33[1m"
/* reset to default */
#define ATTRESET  "\33[0m"

#define BOLD    ATTBOLD
#define RED     ATTBOLD FGRED
#define GREEN   ATTBOLD FGGREEN
#define YELLOW  ATTBOLD FGYELLOW
#define BLUE    ATTBOLD FGBLUE
#define MAGENTA ATTBOLD FGMAGENTA
#define CYAN    ATTBOLD FGCYAN


int running = 1;

struct _frame {
	unsigned char *tx_id;
	unsigned char *rx_id;
	unsigned char *len;
	unsigned char *cmd;
	unsigned char *data;
};

/*
void printmsg(unsigned char *buff, int len)
{
	int i;
	char *pr, str[64];
	pr = str;
	for (i = 0; i < len; i++) {
		sprintf(pr + 3*i," %02X", buff[i]);
	}
	printf("received: %s\n", str);
}
*/

void handle_SIGINT(int sig_num)
{
	running = 0;
}

int main(int argc, char** argv)
{
	int s, i;
	fd_set rdfs;
	struct _frame rx_frame;
	int nbytes;
	struct sigaction sa;
	struct sockaddr_ikbus addr;
	unsigned char rx_buff[SIZE_BUFF];
	struct ifreq ifr;
	const int timestamp_on = 1;
	struct timeval tv;
	struct cmsghdr *cmsg;
	struct msghdr msg;
	struct iovec iov;
	char ctrlmsg[CMSG_SPACE(sizeof(struct timeval)) + CMSG_SPACE(sizeof(uint32_t))];

	char *ifname = "ibus0";

	rx_frame.tx_id = &rx_buff[0];
	rx_frame.len   = &rx_buff[1];
	rx_frame.rx_id = &rx_buff[2];
	rx_frame.cmd   = &rx_buff[3];
	rx_frame.data  = &rx_buff[4];
	
	if((s = socket(PF_IBUS, SOCK_RAW, 0)) < 0) {
		perror("Error while opening socket");
		return -1;
	}

	if (setsockopt(s, SOL_SOCKET, SO_TIMESTAMP,
		       &timestamp_on, sizeof(timestamp_on)) < 0) {
		perror("setsockopt SO_TIMESTAMP");
		return 1;
	}

	strcpy(ifr.ifr_name, ifname);
	ioctl(s, SIOCGIFINDEX, &ifr);

	addr.ikbus_family  = AF_IBUS;
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

	iov.iov_base = rx_buff;
	msg.msg_name = &addr;
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;
	msg.msg_control = &ctrlmsg;
	
	while (running) {
		FD_ZERO(&rdfs);
		FD_SET(s, &rdfs);

		if (select(s + 1, &rdfs, NULL, NULL, NULL) <= 0) {
			//perror("select");
			running = 0;
			continue;
		}

		/* these settings may be modified by recvmsg() */
		iov.iov_len = sizeof(rx_buff);
		msg.msg_namelen = sizeof(addr);
		msg.msg_controllen = sizeof(ctrlmsg);  
		msg.msg_flags = 0;

		nbytes = recvmsg(s, &msg, 0);
		if (nbytes < 0) {
			perror("read");
			return 1;
		}
		
		if (running && (*rx_frame.len > 2)) {
			// time
			for (cmsg = CMSG_FIRSTHDR(&msg);
			     cmsg && (cmsg->cmsg_level == SOL_SOCKET);
			     cmsg = CMSG_NXTHDR(&msg,cmsg)) {
				if (cmsg->cmsg_type == SO_TIMESTAMP)
					tv = *(struct timeval *)CMSG_DATA(cmsg);
			}
			/* absolute with timestamp */
			printf("%s %010ld.%06ld ", "", tv.tv_sec, tv.tv_usec);
			// head
			printf(" %s[%02X > %02X]", "", *rx_frame.tx_id, *rx_frame.rx_id);
			printf(" %s%02X:", "", *rx_frame.cmd);
			// data
			printf("%s", "");
			for (i = 0; i < (int)*rx_frame.len -3; i++)
				printf(" %02X", rx_frame.data[i]);
/*
			 check sum
			printf(" %s%02X", "", rx_frame.data[nbytes - 4]);
*/
			printf("\n");
		}
	}
	printf("\r%sDone!\n", ATTRESET);
	close(s);
	return(EXIT_SUCCESS);
}
