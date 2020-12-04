# 2nd ORAKEL (IN3230) - RAW sockets

# "Greet your neighbor!"

In this example, we're going to implement the simplest ARP protocol
which will help two neighbors to get to know each-other.

topo_p2p.py is the python script that generates the mininet topology.
The topology is the most basic one, point-to-point topology.
We try to keep things very simple in here! :-)

[NodeA]ifA------------ifB[NodeB]

NodeA and NodeB will talk to each-other via RAW sockets.
epoll() will be used to monitor the activity of the RAW socket descriptors.

While NodeB will be listening to a RAW socket, NodeA will send a broadcast frame
asking "Tell me who you are?". NodeB will reply with its MAC address and will cache
NodeA's MAC address.
After they shake their hands (Yes, no viruses in Unix :-)


Usage:

- Compile all programmes with 'make all' in the current directory
- Create the mininet topology using 'sudo mn --mac --custom topo_p2p.py --topo mytopo'
- In the mininet console, access node h1 and h2 using 'xterm h1' and 'xterm h2' or 'xterm h1 h2'
  (You should have used -Y argument in the ssh command: ssh -Y debian@ip_address_of_your_VM
- From the xterm consoles, run ./nodeB from node h2 and ./nodeA from node h1

  Practise and implement new features in the applications.

  Good luck :-)
