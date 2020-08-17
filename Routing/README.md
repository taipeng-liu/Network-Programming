# Unicast Routing

The program runs on every node in an unknown network topology to perform routing tasks to deliver the test messages intructed by the manager program. The routing algorithm is the Link State (LS) algorithm.

## Network Configurations

Everything will be contained within a single VM. The networks (i.e., nodes and network topologies) are constructed by using virtual network interfaces (created by `ifconfig`) and configuring firewall rules (configured by `iptables`). The program will act as a node in the constructed network to perform routing tasks. If there are *N* nodes in the test network, there will be *N* instances of the programs being started, each of which runs as one of the nodes with an ID that is associated to an IP addreess.

The Perl script (`make_topology.pl`) is used to establish these virtual network interfaces and firewall rules. The manager program (`manager_send`) is used to send a message to one of the VMs. 

## Usage

### Construct a network topology

To set an initial topology, run
```
perl make_topology.pl testtopo.txt
```
"testtopo.txt" is an example topology provided to you, This will set the connectivity between nodes.

### Create Node

`./ls_router <nodeID> <initialcostfile> <logfile>`

The first parameter indicates that this node should use the IP address 10.1.1.<netID>.
The second parameter points to a file containing initial link costs related to this node.
The third parameter points to a file where the program should write the required log messages to.

### Initial Link Costs File Format

The format of the initial link costs file is defined as follows:
```
<nodeID> <nodecost>
<nodeID> <nodecost>
...
```

### Log file

There are four types of log messages:
```
forward packet dest [nodeID] nexthop [nodeID] message [text text]
sending packet dest [nodeID] nesthop [nodeID] message [text text]
receive packet message [text text]
unreachable dest [nodeID]
```

### Send Message

To have the nodes send messages, or change link costs while the programs are running, use `manager_send`:
```
./manager_send 2 send 5 "Hello world!"
./manager_send 3 cost 1234
```

### Change Network Topology

E.g., to bring the link between nodes 1 and 2 up:
```
sudo iptables =I OUTPUT -s 10.1.1.1 -d 10.1.1.2 -j ACCEPT ; sudo iptables -I OUTPUT -s 10.1.1.2 -d 10.1.1.1 -j ACCEPT
```

To take that link down:
```
sudo iptables -D OUTPUT -s 10.1.1.1 -d 10.1.1.2 -j ACCEPT ; sudo iptables -D OUTPUT -s 10.1.1.2 -d 10.1.1.1 -j ACCEPT
```
