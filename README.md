A Policy-Based Routing (PBR) Router based on Distance-Vector Algorithm

Objective
Design a DV-based router which simulates a RIP router on the Internet. The router can determine the shortest route based on the policy and transport packets.

Supported commands
The program should dispose the following commands:

N ---- Print activity’s adjacent list.

T ---- Display routing table.

D n ---- send a data packet the destination that the number n represents.

P K n1 n2 … nk ---- Specified priority route
	K: the count of nodes in the route.
	n1 n2 … nk : IDs of all K nodes and nk is the ID of destination node.
	Replace possible shortest route with possible priority route after the node receives the command. In routing update, replace original route with the shortest route and the new route is in accordance with the priority route.
	
R n ---- Refused to pass the node n.After the node receives the command，the node ignores all of the updates that contains node n in routing update.

