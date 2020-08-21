#include <stdlib.h>		/* free */
#include <stdio.h> 		/* printf */
#include <unistd.h>   /* fgets */
#include <string.h>		/* memset */
#include <sys/socket.h>		/* socket */
#include <fcntl.h>
#include <sys/select.h> /* select */
#include <linux/if_packet.h>	/* AF_PACKET */
#include <net/ethernet.h>	/* ETH_* */
#include <arpa/inet.h>		/* htons */
#include <ifaddrs.h>		/* getifaddrs */

#define BUF_SIZE 1450
#define DST_MAC_ADDR {0x00, 0x00, 0x00, 0x00, 0x00, 0x02}

struct ether_frame {
    uint8_t dst_addr[6];
    uint8_t src_addr[6];
    uint8_t eth_proto[2];
    uint8_t contents[0];
} __attribute__((packed));



int send_raw_packet(int sd, struct sockaddr_ll *so_name, uint8_t *buf, size_t len)
{
	int    rc;
	struct ether_frame frame_hdr;
	struct msghdr      *msg;
	struct iovec       msgvec[2];

	/* Fill in Ethernet header */
  uint8_t dst_addr[] = DST_MAC_ADDR;
  memcpy(frame_hdr.dst_addr, dst_addr, 6);
  memcpy(frame_hdr.src_addr, so_name->sll_addr, 6);
  /* Match the ethertype in packet_socket.c: */
  frame_hdr.eth_proto[0] = frame_hdr.eth_proto[1] = 0xFF;

  /* Point to frame header */
  msgvec[0].iov_base = &frame_hdr;
  msgvec[0].iov_len  = sizeof(struct ether_frame);
  /* Point to frame payload */
  msgvec[1].iov_base = buf;
  msgvec[1].iov_len  = len;

  /* Allocate a zeroed-out message info struct */
  msg = (struct msghdr *)calloc(1, sizeof(struct msghdr));

  /* Fill out message metadata struct */
  memcpy(so_name->sll_addr, dst_addr, 6);
  msg->msg_name    = so_name;
  msg->msg_namelen = sizeof(struct sockaddr_ll);
  msg->msg_iovlen  = 2;
  msg->msg_iov     = msgvec;

  /* Construct and send message */
  rc = sendmsg(sd, msg, 0);
  if (rc == -1) {
    perror("sendmsg");
    free(msg);
    return 1;
  }

  /* Remember that we allocated this on the heap; free it */
  free(msg);

  return rc;
}

int recv_raw_packet(int sd, uint8_t *buf, size_t len)
{
  struct sockaddr_ll  so_name;
	struct ether_frame  frame_hdr;
	struct msghdr       msg;
	struct iovec        msgvec[2];
	int 								rc;
	
  /* Point to frame header */
  msgvec[0].iov_base = &frame_hdr;
  msgvec[0].iov_len  = sizeof(struct ether_frame);
  /* Point to frame payload */
  msgvec[1].iov_base = buf;
  msgvec[1].iov_len  = len;


  /* Fill out message metadata struct */
  //memcpy(so_name->sll_addr, dst_addr, 6);
  msg.msg_name    = &so_name;
  msg.msg_namelen = sizeof(struct sockaddr_ll);
  msg.msg_iovlen  = 2;
  msg.msg_iov     = msgvec;

	rc = recvmsg(sd, &msg, 0);
	if (rc == -1) {
    perror("sendmsg");
    return -1;
  }

	return rc;
}


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
    if (ifp->ifa_addr != NULL && ifp->ifa_addr->sa_family == AF_PACKET && (strcmp("lo", ifp->ifa_name)))
      /* Copy the address info into our temp. variable */
      memcpy(so_name, (struct sockaddr_ll*)ifp->ifa_addr, sizeof(struct sockaddr_ll));
  }
  /* After the loop, the address info of the last interface
     enumerated is stored in so_name. */

  /* Free the interface list */
  freeifaddrs(ifaces);

	return;
}


int main(int argc, char *argv[])
{
  int     raw_sock, rc;
  struct  sockaddr_ll so_name;
  uint8_t buf[BUF_SIZE];
  char    username[8];
  
	fd_set read_fds;
  struct timeval timeout;

	short unsigned int protocol;
	protocol = 0xFFFF;
  
  /* Set up a raw AF_PACKET socket without ethertype filtering */
  raw_sock = socket(AF_PACKET, SOCK_RAW, htons(protocol));
  if (raw_sock == -1) {
    perror("socket");
    return -1;
  }

  get_mac_from_interface(&so_name);

  printf("\n Enter your username: ");
  fgets(username, sizeof(username), stdin);

	send_raw_packet(raw_sock, &so_name, username, strlen(username) - 1);

	fflush(stdin);

	/*************************************************************/
	/* Initialize the timeval struct to 3 minutes.  If no        */
	/* activity after 3 minutes this program will end.           */
	/*************************************************************/
	timeout.tv_sec  = 3 * 60;
	timeout.tv_usec = 0;

  while(1) {
    memset(buf, 0, BUF_SIZE);
		FD_ZERO(&read_fds);
		FD_SET(raw_sock, &read_fds);
		FD_SET(0, &read_fds);

		rc = select(raw_sock + 1, &read_fds, NULL, NULL, &timeout);
		if (rc < 0) {
			perror("select() failed");
      break;
		}

		if (rc == 0) {
			printf("  select() timed out. Program will end.");
      break;
    }

		if (FD_ISSET(0, &read_fds)) {
			printf("Someone knocked on stdin...\n");
			fgets((char *)buf, sizeof(buf), stdin);
			if (strstr((char *)buf, "ADIOS") != NULL) {
				printf("ADIOS\n");
				break;
			}
			send_raw_packet(raw_sock, &so_name, buf, strlen((char *)buf));
		}

		else if (FD_ISSET(raw_sock, &read_fds)) {
			printf("Someone knocked on the socket...\n");
			rc = recv_raw_packet(raw_sock, buf, BUF_SIZE);
    	if (rc < 1) {
      	perror("recv");
      	return 1;
			}

			printf("\n<server>: %s\n", buf);
			fflush(stdin);
  	}
	}

	close(raw_sock);

  return 0;
}

