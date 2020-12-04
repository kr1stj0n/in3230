#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ifaddrs.h>		/* getifaddrs */
#include <linux/if_packet.h>    /* AF_PACKET  */
#include <arpa/inet.h>          /* htons      */
#include <stdint.h>

#include "common.h"

/*
 * This function gets a pointer to a struct sockaddr_ll
 * and fills it with necessary address info from the interface device.
 */
void get_mac_from_interface(struct sockaddr_ll *so_name)
{
	struct ifaddrs *ifaces, *ifp;
	/* Enumerate interfaces: */
	/* Note in man getifaddrs that this function dynamically allocates
	   memory. It becomes our responsability to free it! */
	if (getifaddrs(&ifaces)) {
		perror("getifaddrs");
		exit(-1);
	}

	/* Walk the list looking for ifaces interesting to us */
	//printf("Interface list:\n");
	for (ifp = ifaces; ifp != NULL; ifp = ifp->ifa_next) {
		/* We make certain that the ifa_addr member is actually set: */
		if (ifp->ifa_addr != NULL &&
				ifp->ifa_addr->sa_family == AF_PACKET &&
				(strcmp("lo", ifp->ifa_name)))
			/* Copy the address info into our temp. variable */
			memcpy(so_name, (struct sockaddr_ll*)ifp->ifa_addr, sizeof(struct sockaddr_ll));
	}
	/* After the loop, the address info of the last interface
	   enumerated is stored in so_name. */

	/* Free the interface list */
	freeifaddrs(ifaces);
}

void print_mac_addr(uint8_t *addr, size_t len) {
      size_t i;

      for (i = 0; i < len - 1; i++) {
          printf("%d:", addr[i]);
      }
      printf("%d\n", addr[i]);
}

void config_if(struct if_data *iface, int fd)
{	
	get_mac_from_interface(&iface->local_addr);
	get_mac_from_interface(&iface->remote_addr);
	iface->raw_sock_fd = fd;
}

int create_raw_socket(void)
{
	int sd;
	short unsigned int protocol = 0xFFFF;

	/* Set up a raw AF_PACKET socket without ethertype filtering */
	sd = socket(AF_PACKET, SOCK_RAW, htons(protocol));
	if (sd == -1) {
		perror("socket");
		exit(EXIT_FAILURE);	
	}

	return sd;
}

int send_arp_request(struct if_data *iface)
{
	struct ether_frame frame_hdr;
	struct msghdr      *msg;
	struct iovec       msgvec[1];
	int    rc;

	/* Fill in Ethernet header */
	uint8_t dst_addr[] = ETH_BROADCAST;
	memcpy(frame_hdr.dst_addr, dst_addr, 6);
	memcpy(frame_hdr.src_addr, iface->local_addr.sll_addr, 6);
	/* Match the ethertype in packet_socket.c: */
	frame_hdr.eth_proto[0] = frame_hdr.eth_proto[1] = 0xFF;

	/* Point to frame header */
	msgvec[0].iov_base = &frame_hdr;
	msgvec[0].iov_len  = sizeof(struct ether_frame);

	/* Allocate a zeroed-out message info struct */
	msg = (struct msghdr *)calloc(1, sizeof(struct msghdr));

	/* Fill out message metadata struct */
	memcpy(iface->remote_addr.sll_addr, dst_addr, 6);
	msg->msg_name    = &(iface->remote_addr);
	msg->msg_namelen = sizeof(struct sockaddr_ll);
	msg->msg_iovlen  = 1;
	msg->msg_iov     = msgvec;

	/* Construct and send message */
	rc = sendmsg(iface->raw_sock_fd, msg, 0);
	if (rc == -1) {
		perror("sendmsg");
		free(msg);
		return -1;
	}

	/* Remember that we allocated this on the heap; free it */
	free(msg);

	return rc;
}

int handle_arp_packet(struct if_data *iface)
{
	struct ether_frame  frame_hdr;
	struct msghdr       msg = {0};
	struct iovec        msgvec[1];
	int 		    rc;

	/* Point to frame header */
	msgvec[0].iov_base = &frame_hdr;
	msgvec[0].iov_len  = sizeof(struct ether_frame);

	/* Fill out message metadata struct */
	/* msg.msg_name    = &so_name; */
	/* msg.msg_namelen = sizeof(struct sockaddr_ll); */
	msg.msg_iovlen  = 1;
	msg.msg_iov     = msgvec;

	rc = recvmsg(iface->raw_sock_fd, &msg, 0);
	if (rc == -1) {
		perror("sendmsg");
		return -1;
	}

	memcpy(iface->remote_addr.sll_addr, frame_hdr.src_addr, 6);

	return rc;
}

int send_arp_response(struct if_data *iface)
{
	struct ether_frame frame_hdr;
	struct msghdr      *msg;
	struct iovec       msgvec[1];
	int    rc;

	/* Fill in Ethernet header */
	memcpy(frame_hdr.dst_addr, iface->remote_addr.sll_addr, 6);
	memcpy(frame_hdr.src_addr, iface->local_addr.sll_addr, 6);
	/* Match the ethertype in packet_socket.c: */
	frame_hdr.eth_proto[0] = frame_hdr.eth_proto[1] = 0xFF;

	/* Point to frame header */
	msgvec[0].iov_base = &frame_hdr;
	msgvec[0].iov_len  = sizeof(struct ether_frame);

	/* Allocate a zeroed-out message info struct */
	msg = (struct msghdr *)calloc(1, sizeof(struct msghdr));

	/* Fill out message metadata struct */
	msg->msg_name    = &(iface->remote_addr);
	msg->msg_namelen = sizeof(struct sockaddr_ll);
	msg->msg_iovlen  = 1;
	msg->msg_iov     = msgvec;

	/* Construct and send message */
	rc = sendmsg(iface->raw_sock_fd, msg, 0);
	if (rc == -1) {
		perror("sendmsg");
		free(msg);
		return -1;
	}

	/* Remember that we allocated this on the heap; free it */
	free(msg);

	return rc;
}
