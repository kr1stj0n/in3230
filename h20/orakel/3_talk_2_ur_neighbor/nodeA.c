#include <stdlib.h>		/* free */
#include <stdio.h> 		/* printf */
#include <unistd.h>             /* fgets */
#include <string.h>		/* memset */
#include <sys/socket.h>		/* socket */
#include <fcntl.h>
#include <sys/epoll.h>          /* epoll */
#include <linux/if_packet.h>	/* AF_PACKET */
#include <net/ethernet.h>	/* ETH_* */
#include <arpa/inet.h>		/* htons */

#include "common.h"

int main(int argc, char *argv[])
{
	struct if_data local_if;
	int     raw_sock, rc;

	struct epoll_event ev, events[MAX_EVENTS];
	int epollfd;

	/* Set up a raw AF_PACKET socket without ethertype filtering */
	raw_sock = create_raw_socket();

	config_if(&local_if, raw_sock);


	epollfd = epoll_create1(0);
	if (epollfd == -1) {
		perror("epoll_create1");
		exit(EXIT_FAILURE);
	}

	ev.events = EPOLLIN;
	ev.data.fd = raw_sock;
	if (epoll_ctl(epollfd, EPOLL_CTL_ADD, raw_sock, &ev) == -1) {
		perror("epoll_ctl: raw_sock");
		exit(EXIT_FAILURE);
	}

	printf("\n<nodeA> Hi! I am ");
	print_mac_addr(local_if.local_addr.sll_addr, 6);

	send_arp_request(&local_if);

	while(1) {
		rc = epoll_wait(epollfd, events, MAX_EVENTS, -1);
		if (rc == -1) {
			perror("epoll_wait");
			exit(EXIT_FAILURE);
		} else if (events->data.fd == raw_sock) {
			printf("<info> The neighbor is responding the handshake\n");
			rc = handle_arp_packet(&local_if);
			if (rc < 1) {
				perror("recv");
				exit(EXIT_FAILURE);
			}
			
			printf("<nodeA> Nice to meet you ");
			print_mac_addr(local_if.remote_addr.sll_addr, 6);
		}
		break;
	}

	close(raw_sock);

	return 0;
}
