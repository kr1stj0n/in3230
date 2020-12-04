#ifndef _COMMON_H
#define _COMMON_H

#define MAX_EVENTS 10
#define ETH_BROADCAST {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}


struct ether_frame {
	uint8_t dst_addr[6];
	uint8_t src_addr[6];
	uint8_t eth_proto[2];
	uint8_t contents[0];
} __attribute__((packed));

struct if_data {
	int raw_sock_fd;
	struct sockaddr_ll local_addr;
	struct sockaddr_ll remote_addr;
};

void get_mac_from_interface(struct sockaddr_ll *);
void print_mac_addr(uint8_t *, size_t);
void config_if(struct if_data *, int);
int create_raw_socket(void);
int send_arp_request(struct if_data *);
int send_arp_response(struct if_data *);
int handle_arp_packet(struct if_data *);

#endif
