In this session, we're going to implement a client and server applications
that use RAW sockets to communicate to each other in a chat session.

topo_p2p.py is the python script that generates the mininet topology.

select/poll will be used to monitor the activity of stdin and raw socket descriptor




   *Client*                       *Server*
	 create raw_sock								create raw_sock
	 																call recv_raw_packet
																				.
																				.
	 send username       --------> 	Save username and cast src_mac_addr from the packet received
																	Use that mac_addr as the dst_addr to send frames to the client

	 select to monitor							select to monitor
	 stdind and raw_sock						stdin and raw_sock

	 recvmsg / sendmsg   <=======>  recvmsg / sendmsg
