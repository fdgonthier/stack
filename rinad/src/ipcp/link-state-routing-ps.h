/*
 * Link-state routing policy
 *
 *    Bernat Gaston <bernat.gaston@i2cat.net>
 *    Eduard Grasa <eduard.grasa@i2cat.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef IPCP_LINK_STATE_ROUTING_HH
#define IPCP_LINK_STATE_ROUTING_HH

#ifdef __cplusplus

#include <set>
#include <librina/timer.h>

#include "ipcp/components.h"
#include "common/encoders/FlowStateMessage.pb.h"

namespace rinad {

class LinkStateRoutingPolicy;

class LinkStateRoutingPs: public IRoutingPs {
public:
		static std::string LINK_STATE_POLICY;
		LinkStateRoutingPs(IRoutingComponent * rc);
		void set_dif_configuration(const rina::DIFConfiguration& dif_configuration);
		int set_policy_set_param(const std::string& name,
			const std::string& value);
		virtual ~LinkStateRoutingPs() {}

private:
        // Data model of the routing component.
        IRoutingComponent * rc;
        LinkStateRoutingPolicy * lsr_policy;
};

/// The object exchanged between IPC Processes to disseminate the state of
/// one N-1 flow supporting the IPC Processes in the DIF. This is the RIB
/// target object when the PDU Forwarding Table Generator wants to send
/// information about a single N-1 flow.The flow state object id (fso_id)
/// is generated by concatenating the address and port-id fields.
class FlowStateObject {
public:
	FlowStateObject(unsigned int address, unsigned int neighbor_address,
			unsigned int cost, bool state, int sequence_number, unsigned int age);
	const std::string toString();

	// The address of the IPC Process
	unsigned int address_;

	 // The address of the neighbor IPC Process
	unsigned int neighbor_address_;

	// The port_id assigned by the neighbor IPC Process to the N-1 flow
	unsigned int cost_;

	// Flow up (true) or down (false)
	bool up_;

	// A sequence number to be able to discard old information
	int sequence_number_;

	// Age of this FSO (in seconds)
	unsigned int age_;

	// The object has been marked for propagation
	bool modified_;

	// Avoid port in the next propagation
	int avoid_port_;

	// The object is being erased
	bool being_erased_;

	// The name of the object in the RIB
	std::string object_name_;
};

class Edge {
public:
	Edge(unsigned int address1, unsigned int address2, int weight);
	bool isVertexIn(unsigned int address) const;
	unsigned int getOtherEndpoint(unsigned int address);
	std::list<unsigned int> getEndpoints();
	bool operator==(const Edge & other) const;
	bool operator!=(const Edge & other) const;
	const std::string toString() const;

	unsigned int address1_;
	unsigned int address2_;
	int weight_;
};

class Graph {
public:
	Graph(const std::list<FlowStateObject *>& flow_state_objects);
	~Graph();

	std::list<Edge *> edges_;
	std::list<unsigned int> vertices_;

private:
	struct CheckedVertex {
		unsigned int address_;
		int port_id_;
		std::list<unsigned int> connections;

		CheckedVertex(unsigned int address) {
			address_ = address;
			port_id_ = 0;
		}

		bool connection_contains_address(unsigned int address) {
			std::list<unsigned int>::iterator it;
			for(it = connections.begin(); it != connections.end(); ++it) {
				if ((*it) == address) {
					return true;
				}
			}

			return false;
		}
	};

	std::list<FlowStateObject *> flow_state_objects_;
	std::list<CheckedVertex *> checked_vertices_;

	bool contains_vertex(unsigned int address) const;
	void init_vertices();
	CheckedVertex * get_checked_vertex(unsigned int address) const;
	void init_edges();
};

class IRoutingAlgorithm {
public:
	virtual ~IRoutingAlgorithm(){};

	//Compute the next hop to other addresses. Ownership of
	//PDUForwardingTableEntries in the list is passed to the
	//caller
	virtual std::list<rina::RoutingTableEntry *> computeRoutingTable(
			const Graph& graph,
			const std::list<FlowStateObject *>& fsoList,
			unsigned int source_address) = 0;
};

/// Contains the information of a predecessor, needed by the Dijkstra Algorithm
class PredecessorInfo {
public:
	PredecessorInfo(unsigned int nPredecessor);

	unsigned int predecessor_;
};

/// The routing algorithm used to compute the PDU forwarding table is a Shortest
/// Path First (SPF) algorithm. Instances of the algorithm are run independently
/// and concurrently by all IPC processes in their forwarding table generator
/// component, upon detection of an N-1 flow allocation/deallocation/state change.
class DijkstraAlgorithm : public IRoutingAlgorithm {
public:
	DijkstraAlgorithm();
	std::list<rina::RoutingTableEntry *> computeRoutingTable(
			const Graph& graph,
			const std::list<FlowStateObject *>& fsoList,
			unsigned int source_address);
private:
	std::set<unsigned int> settled_nodes_;
	std::set<unsigned int> unsettled_nodes_;
	std::map<unsigned int, PredecessorInfo *> predecessors_;
	std::map<unsigned int, int> distances_;

	void execute(const Graph& graph, unsigned int source);
	unsigned int getMinimum() const;
	void findMinimalDistances (const Graph& graph, unsigned int node);
	int getShortestDistance(unsigned int destination) const;
	bool isNeighbor(Edge * edge, unsigned int node) const;
	bool isSettled(unsigned int node) const;
	unsigned int getNextHop(unsigned int address, unsigned int sourceAddress);
};

/// A group of flow state objects. This is the RIB target object
/// when routing wants to send
/// information about more than one N-1 flow
class FlowStateRIBObjectGroup: public rina::BaseRIBObject {
public:
	FlowStateRIBObjectGroup(IPCProcess * ipc_process,
			LinkStateRoutingPolicy * lsr_policy);
	const void* get_value() const;
	void remoteWriteObject(void * object_value, int invoke_id,
			rina::CDAPSessionDescriptor * cdapSessionDescriptor);
	void createObject(const std::string& objectClass, const std::string& objectName,
			const void* objectValue);

private:
	LinkStateRoutingPolicy * lsr_policy_;
	IPCProcess * ipc_process_;
	IPCPRIBDaemon * rib_daemon_;
};

/// A single flow state object
class FlowStateRIBObject: public rina::BaseRIBObject {
public:
	FlowStateRIBObject(IPCProcess * ipc_process,
			const std::string& objectClass,
			const std::string& objectName, const void* objectValue);
	const void* get_value() const;
	void writeObject(const void* object);
	void createObject(const std::string& objectClass,
			const std::string& objectName,
			const void* objectValue);
	void deleteObject(const void* objectValue);

private:
	IPCProcess * ipc_process_;
	IPCPRIBDaemon * rib_daemon_;
	const void* object_value_;
};

/// The subset of the RIB that contains all the Flow State objects known by the IPC Process.
/// It exists only in the PDU forwarding table generator. It is used as an input to calculate
/// the routing and forwarding tables. The FSDB is generated by the operations on FSOs received
/// through CDAP messages or created by the resource allocator. The FSOs in the FSDB contain
/// the same information as the one formerly described in the FSO subsection, plus a list of
/// port-ids of N-1 management flows the FSOs should be sent to. Periodically the FSDB is checked
/// to look for FSOs to be propagated. Once FSOs have been written to the corresponding N-1 flows,
/// the list of port-ids is emptied. The list is updated when events that required the propagation
/// of FSO occur (such as local events notifying changes on N-1 flows or remote operations on the
/// FSDB through CDAP).
class FlowStateDatabase {
public:
	static const int NO_AVOID_PORT;
	static const long WAIT_UNTIL_REMOVE_OBJECT;

	FlowStateDatabase(rina::IMasterEncoder * encoder, FlowStateRIBObjectGroup *
			flow_state_rib_object_group, rina::Timer * timer, IPCPRIBDaemon *rib_daemon, unsigned int *maximum_age);
	bool isEmpty() const;
	void setAvoidPort(int avoidPort);
	void addObjectToGroup(unsigned int address, unsigned int neighborAddress,
			unsigned int cost, int avoid_port);
	void deprecateObject(unsigned int address);
  std::map <int, std::list<FlowStateObject*> > prepareForPropagation(const std::list<rina::FlowInformation>& flows);
	void incrementAge();
	void updateObjects(const std::list<FlowStateObject*>& newObjects, int avoidPort, unsigned int address);
	std::list<FlowStateObject*> getModifiedFSOs();
	void getAllFSOs(std::list<FlowStateObject*> &flow_state_objects);
	unsigned int get_maximum_age() const;

	//Signals a modification in the FlowStateDB
	bool modified_;
	std::list<FlowStateObject *> flow_state_objects_;

private:
	rina::IMasterEncoder * encoder_;
	FlowStateRIBObjectGroup * flow_state_rib_object_group_;
	rina::Timer * timer_;
	IPCPRIBDaemon *rib_daemon_;
	unsigned int *maximum_age_;

	FlowStateObject * getByAddress(unsigned int address);
};

class LinkStateRoutingCDAPMessageHandler: public rina::BaseCDAPResponseMessageHandler {
public:
	LinkStateRoutingCDAPMessageHandler(LinkStateRoutingPolicy * lsr_policy);
	void readResponse(int result, const std::string& result_reason,
			void * object_value, const std::string& object_name,
			rina::CDAPSessionDescriptor * session_descriptor);

private:
	LinkStateRoutingPolicy * lsr_policy_;
};

class ComputeRoutingTimerTask : public rina::TimerTask {
public:
	ComputeRoutingTimerTask(LinkStateRoutingPolicy * lsr_policy,
			long delay);
	~ComputeRoutingTimerTask() throw(){};
	void run();

private:
	LinkStateRoutingPolicy * lsr_policy_;
	long delay_;
};

class KillFlowStateObjectTimerTask : public rina::TimerTask {
public:
	KillFlowStateObjectTimerTask(IPCPRIBDaemon * rib_daemon,
			FlowStateObject * fso, FlowStateDatabase * fs_db);
	~KillFlowStateObjectTimerTask() throw(){};
	void run();

private:
	IPCPRIBDaemon * rib_daemon_;
	FlowStateObject * fso_;
	FlowStateDatabase * fs_db_;
};

class PropagateFSODBTimerTask : public rina::TimerTask {
public:
	PropagateFSODBTimerTask(LinkStateRoutingPolicy * lsr_policy,
			long delay);
	~PropagateFSODBTimerTask() throw(){};
	void run();

private:
	LinkStateRoutingPolicy * lsr_policy_;
	long delay_;
};

class UpdateAgeTimerTask : public rina::TimerTask {
public:
	UpdateAgeTimerTask(LinkStateRoutingPolicy * lsr_policy,
			long delay);
	~UpdateAgeTimerTask() throw(){};
	void run();

private:
	LinkStateRoutingPolicy * lsr_policy_;
	long delay_;
};

/// This routing policy uses a Flow State Database
/// (FSDB) populated with Flow State Objects (FSOs) to compute the PDU
/// forwarding table. This database is updated based on local events notified
/// by the Resource Allocator, or on remote operations on the Flow State Objects
/// notified by the RIB Daemon. Each IPC Process maintains a view of the
/// data-transfer connectivity graph in the DIF. To do so, information about
/// the state of N-1 flows is disseminated through the DIF periodically. The PDU
/// Forwarding Table generator applies an algorithm to compute the shortest
/// route from this IPC Process to all the other IPC Processes in the DIF. If
/// there is a Flow State Object from both IPC processes that share the flow,
/// the flow is used in the computation. This is to avoid misbehaving IPC
/// processes that disturb the routing in the DIF. The flow has to be announced
/// by both ends. The next hop of the routes to all the other IPC Processes is
/// stored as the routing table. Finally, for each entry in the routing table,
/// the PDU Forwarding Table generator selects the most appropriate N-1 flow that
/// leads to the next hop. This selection of the most appropriate N-1 flow can be
/// performed more frequently in order to perform load-balancing or to quickly route
/// around failed N-1 flows
class LinkStateRoutingPolicy: public EventListener {
public:
	LinkStateRoutingPolicy(IPCProcess * ipcp);
	~LinkStateRoutingPolicy();
	void set_ipc_process(IPCProcess * ipc_process);
	void set_dif_configuration(const rina::DIFConfiguration& dif_configuration);
	const std::list<rina::FlowInformation>& get_allocated_flows() const;

	/// N-1 Flow allocated, N-1 Flow deallocated or enrollment to neighbor completed
	void eventHappened(Event * event);

	/// Invoked periodically by a timer. Every FSO that needs to be propagated is retrieved,
	/// together with the list of port-ids it should be sent on. For every port-id, an
	/// M_WRITE CDAP message targeting the /dif/management/routing/flowstateobjectgroup/
	/// object is created, containing all the FSOs that need to be propagated on this N-1
	/// management flow. In the case that a single CDAP message would be too large (i.e.,
	/// the encoded message is larger than the Maximum SDU size of the N-1 management flow),
	/// the set of FSOs to be propagated can be spread into multiple CDAP messages. All the
	/// CDAP messages are written to the corresponding port-ids, and the list of port-ids of
	/// all FSOs is cleared.
	void propagateFSDB() const;

	/// Invoked periodically by a timer. The age of every Flow State Object is incremented
	/// with one. If an FSO reaches the maximum age - which is set by DIF policy -, it is
	/// removed from the FSDB. The FSDB is marked as ÒmodifiedÓ if it wasnÕt already and one
	/// or more FSOs are removed from the FSDB during the processing of this event.
	void updateAge();

	/// Invoked periodically by a timer. If the FSDB is marked as ÒmodifiedÓ, the PDU
	/// Forwarding Table Generator marks the FSDB as Ònot modifiedÓ and the PDU forwarding
	/// table computation procedure is initiated . An N-1 flow is only used in the calculation
	/// of the forwarding table if there are two FSOs in the FSDB related to the flow (one
	/// advertised by each IPC Process the N-1 flow connects together), and the state of the
	/// flow is ÒupÓ. The forwarding table is computed according to the algorithm explained in
	/// the Òalgorithm to compute the forwarding tableÓ section. If the FSDB is not marked as
	/// ÒmodifiedÓ nothing happens.
	void routingTableUpdate();

	/// All FSOs in the M_WRITE message are processed sequentially. If an FSO that matches
	/// with the IPC processes addresses and port-ids contained in the message can be retrieved
	/// from the FSDB, and the sequence number of the received FSO is greater than the sequence
	/// number of the retrieved FSO, the FSO is updated with the new state and sequence number,
	/// and marked to be propagated through all the N-1 management flows except the one through
	/// which the FSO was received. If the sequence number is not greater than the sequence
	/// number of the FSO in the FSDB, the FSO is discarded. If the FSO object cannot be retrieved,
	/// it is added to the FSDB and marked to be propagated through all the N-1 management flows
	/// except the one through which the FSO was received. If, while processing the event,
	/// existing FSOs in the FSDB were modified, or new ones were created, the FSDB is marked as
	/// modified - if it wasn't already -.
	/// @param objects the flow_state_objects received from a neighbor IPC Process
	/// @param portId the identifier of the N-1 flow through which the CDAP message was received
	void writeMessageReceived(const std::list<FlowStateObject *> & flow_state_objects, int portId);

	/// If the IPC Process just enrolled to the neighbor IPC Process that has sent the
	/// CDAP message, and the PDU Forwarding Table Generator still has not sent a CDAP
	/// M_WRITE message with the contents of the FSDB, then the PDU Forwarding table generator
	/// will reply with one or more M_READ_R messages containing the FSOs in its FSDB.
	/// Otherwise it will ignore the request.
	void readMessageRecieved(int invoke_id, int srcPort) const;

	bool test_;
	FlowStateDatabase * db_;
	rina::Timer * timer_;

private:
	static const int MAXIMUM_BUFFER_SIZE;
	IPCProcess * ipc_process_;
	IPCPRIBDaemon * rib_daemon_;
	rina::IMasterEncoder * encoder_;
	rina::CDAPSessionManagerInterface * cdap_session_manager_;
	FlowStateRIBObjectGroup * fs_rib_group_;
	rina::PDUFTableGeneratorConfiguration pduft_generator_config_;
	IRoutingAlgorithm * routing_algorithm_;
	unsigned int source_vertex_;
	unsigned int maximum_age_;

	/// If a flow allocation is launched before the enrollment is finished, the flow
	/// allocation procedure of the PDU Forwarding table must wait. Otherwise it will
	/// not have the necessary information about the neighbour IPC process. This pending
	/// flow allocation has to be stored in a list, until the enrollment is finished or
	/// until a flow deallocation over the same flow is called.
	std::list<rina::FlowInformation> allocated_flows_;
	rina::Lockable * lock_;

	void populateRIB();
	void subscribeToEvents();

	/// The Resource Allocator has deallocated an existing N-1 flow dedicated to data
	/// transfer. If there is a pending flow allocation over this N-1 flow, it has to
	/// be erased from the list of pending flow allocations. Otherwise, the Flow State
	/// Object corresponding to this flow is retrieved. The state is set false, the age
	/// is set to the maximum age and the sequence number pertaining to this FSO is
	/// incremented by 1. The FSO is marked for propagation (ÔpropagateÕ is set to true)
	/// and the list of port-ids associated with this FSO is filled with the port-ids of
	/// all N-1 management flows. The FSDB is marked as ÒmodifiedÓ if it wasnÕt already.
	void processFlowDeallocatedEvent(NMinusOneFlowDeallocatedEvent * event);

	/// The Resource Allocator has allocated a new N-1 flow dedicated to data transfer.
	/// If no neighbour is found, an entry has to be created and added into the list
	/// of pending flow allocations. Otherwise, a Flow State Object is created, containing
	/// the address and port-id of the IPC process and the address and port-id of the
	/// neighbour IPC process a flow is allocated to. The state is set, the age is set to
	/// 0, and the sequence number is set to 1. The FSO is added to the FSDB, marked for
	/// propagation, and associated with a new list of port-ids, which is filled with the
	/// port-ids of all N-1 management flows. The FSDB is marked as ÒmodifiedÓ if it wasnÕt
	/// already.
	void processFlowAllocatedEvent(NMinusOneFlowAllocatedEvent * event);

	/// The Enrollment Task has completed the enrollment procedure with a new neighbor IPC
	/// Process. If there are pending flow allocations over the enrolled neighbour, they have
	/// to be launched following the procedure descrived in the previous subsection called
	/// Local data transfer N-1 flow allocated. Then, the full FSDB is sent to the IPC process
	/// that a flow has been allocated to, in one or more CDAP M_WRITE messages targeting the
	/// /dif/management/routing/flowstateobjectgroup/ object.
	void processNeighborAddedEvent(NeighborAddedEvent * event);

	void processNeighborLostEvent(ConnectiviyToNeighborLostEvent * event);
};

/// Encoder of Flow State object
class FlowStateObjectEncoder: public rina::EncoderInterface {
public:
	const rina::SerializedObject* encode(const void* object);
	void* decode(const rina::ObjectValueInterface * object_value) const;
	static void convertModelToGPB(rina::messages::flowStateObject_t * gpb_fso,
			FlowStateObject * fso);
	static FlowStateObject * convertGPBToModel(
			const rina::messages::flowStateObject_t & gpb_fso);
};

class FlowStateObjectListEncoder: public rina::EncoderInterface {
public:
	const rina::SerializedObject* encode(const void* object);
	void* decode(const rina::ObjectValueInterface * object_value) const;
};

}

#endif

#endif
