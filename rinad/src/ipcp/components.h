/*
 * Common interfaces and constants of the IPC Process components
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

#ifndef IPCP_COMPONENTS_HH
#define IPCP_COMPONENTS_HH

#ifdef __cplusplus

#include <list>

#include <librina/cdap.h>
#include <librina/ipc-process.h>
#include "events.h"

namespace rinad {

enum IPCProcessOperationalState {
	NOT_INITIALIZED,
	INITIALIZED,
	ASSIGN_TO_DIF_IN_PROCESS,
	ASSIGNED_TO_DIF
};

class IPCProcess;

/// IPC process component interface
class IPCProcessComponent {
public:
	virtual void set_ipc_process(IPCProcess * ipc_process) {
		ipc_process_ = ipc_process;
	}
	virtual ~IPCProcessComponent(){};

protected:
		IPCProcess *ipc_process_;
};

/// Interface
/// Delimits and undelimits SDUs, to allow multiple SDUs to be concatenated in the same PDU
class IDelimiter
{
public:
  virtual ~IDelimiter() {};

  /// Takes a single rawSdu and produces a single delimited byte array, consisting in
  /// [length][sdu]
  /// @param rawSdus
  /// @return
  virtual char* getDelimitedSdu(char rawSdu[]) = 0;

  /// Takes a list of raw sdus and produces a single delimited byte array, consisting in
  /// the concatenation of the sdus followed by their encoded length: [length][sdu][length][sdu] ...
  /// @param rawSdus
  /// @return
  virtual char* getDelimitedSdus(const std::list<char*>& rawSdus) = 0;

  /// Assumes that the first length bytes of the "byteArray" are an encoded varint (encoding an integer of 32 bytes max), and returns the value
  /// of this varint. If the byteArray is still not a complete varint, doesn't start with a varint or it is encoding an
  /// integer of more than 4 bytes the function will return -1.
  ///  @param byteArray
  /// @return the value of the integer encoded as a varint, or -1 if there is not a valid encoded varint32, or -2 if
  ///this may be a complete varint32 but still more bytes are needed
  virtual int readVarint32(char byteArray[], int length) = 0;

  /// Takes a delimited byte array ([length][sdu][length][sdu] ..) and extracts the sdus
  /// @param delimitedSdus
  /// @return
  virtual std::list<char*>& getRawSdus(char delimitedSdus[]) = 0;
};

/// Interface that Encodes and Decodes an object to/from bytes
class IEncoder {
	public:
		virtual char* encode(void* object) throw (Exception) = 0;
		virtual void* decode(const char serializedObject[]) throw (Exception) = 0;
		virtual ~IEncoder(){};
};

/// Contains the objects needed to request the Enrollment
class EnrollmentRequest
{
public:
	EnrollmentRequest(const rina::Neighbor &neighbor, const rina::EnrollToDIFRequestEvent &event);
	const rina::Neighbor& get_neighbor() const;
	void set_neighbor(const rina::Neighbor &neighbor);
	const rina::EnrollToDIFRequestEvent& get_event() const;
	void set_event(const rina::EnrollToDIFRequestEvent &event);

private:
		rina::Neighbor neighbor_;
		rina::EnrollToDIFRequestEvent event_;
};

/// Interface that must be implementing by classes that provide
/// the behavior of an enrollment task
class IEnrollmentTask : public IPCProcessComponent {
public:
	virtual ~IEnrollmentTask(){};
	virtual const std::list<rina::Neighbor>& get_neighbors() const = 0;
	virtual const std::list<std::string>& get_enrolled_ipc_process_names() const = 0;

	/// A remote IPC process Connect request has been received
	/// @param cdapMessage
	/// @param cdapSessionDescriptor
	virtual void connect(const rina::CDAPMessage& cdapMessage,
			const rina::CDAPSessionDescriptor& cdapSessionDescriptor) = 0;

	/// A remote IPC process Connect response has been received
	/// @param cdapMessage
	/// @param cdapSessionDescriptor
	virtual void connectResponse(const rina::CDAPMessage& cdapMessage,
			const rina::CDAPSessionDescriptor& cdapSessionDescriptor) = 0;

	/// A remote IPC process Release request has been received
	/// @param cdapMessage
	/// @param cdapSessionDescripton
	virtual void release(const rina::CDAPMessage& cdapMessage,
			const rina::CDAPSessionDescriptor& cdapSessionDescriptor) = 0;

	/// A remote IPC process Release response has been received
	/// @param cdapMessage
	/// @param cdapSessionDescriptor
	virtual void releaseResponse(const rina::CDAPMessage& cdapMessage,
			const rina::CDAPSessionDescriptor& cdapSessionDescriptor) = 0;

	/// Process a request to initiate enrollment with a new Neighbor, triggered by the IPC Manager
	/// @param event
	/// @param flowInformation
	virtual void processEnrollmentRequestEvent(const rina::EnrollToDIFRequestEvent& event,
			const rina::DIFInformation& difInformation) = 0;

	/// Starts the enrollment program
	/// @param cdapMessage
	/// @param cdapSessionDescriptor
	virtual void initiateEnrollment(EnrollmentRequest request) = 0;

	/// Called by the enrollment state machine when the enrollment request has been completed,
	/// either successfully or unsuccessfully
	/// @param candidate the IPC process we were trying to enroll to
	/// @param enrollee true if this IPC process is the one that initiated the
	/// enrollment sequence (i.e. it is the application process that wants to
	/// join the DIF)
	virtual void enrollmentCompleted(const rina::Neighbor& candidate, bool enrollee) = 0;

	/// Called by the enrollment state machine when the enrollment sequence fails
	/// @param remotePeer
	/// @param portId
	/// @param enrollee
	/// @param sendMessage
	/// @param reason
	virtual void enrollmentFailed(const rina::ApplicationProcessNamingInformation& remotePeerNamingInfo,
			int portId, const std::string& reason, bool enrolle, bool sendReleaseMessage) = 0;

	/// Finds out if the ICP process is already enrolled to the IPC process identified by
	/// the provided apNamingInfo
	/// @param apNamingInfo
	/// @return
	virtual bool isEnrolledTo(const std::string& applicationProcessName) const = 0;
};

/// Interface that must be implementing by classes that provide
/// the behavior of a Flow Allocator task
class IFlowAllocator : public IPCProcessComponent {
public:
	virtual ~IFlowAllocator(){};

	/// The Flow Allocator is invoked when an Allocate_Request.submit is received.  The source Flow
	/// Allocator determines if the request is well formed.  If not well-formed, an Allocate_Response.deliver
	/// is invoked with the appropriate error code.  If the request is well-formed, a new instance of an
	/// FlowAllocator is created and passed the parameters of this Allocate_Request to handle the allocation.
	/// It is a matter of DIF policy (AllocateNoificationPolicy) whether an Allocate_Request.deliver is invoked
	/// with a status of pending, or whether a response is withheld until an Allocate_Response can be delivered
	/// with a status of success or failure.
	/// @param allocateRequest the characteristics of the flow to be allocated.
	/// to honour the request
	virtual void submitAllocateRequest(const rina::FlowRequestEvent& flowRequestEvent) = 0;

	virtual void processCreateConnectionResponseEvent(const rina::CreateConnectionResponseEvent& event) = 0;

	/// Forward the allocate response to the Flow Allocator Instance.
	/// @param portId the portId associated to the allocate response
	/// @param AllocateFlowResponseEvent - the response from the application
	virtual void submitAllocateResponse(const rina::AllocateFlowResponseEvent& event) = 0;

	virtual void processCreateConnectionResultEvent(const rina::CreateConnectionResultEvent& event) = 0;

	virtual void processUpdateConnectionResponseEvent(const rina::UpdateConnectionResponseEvent& event) = 0;

	/// Forward the deallocate request to the Flow Allocator Instance.
	/// @param the flow deallocate request event
	/// @throws IPCException
	virtual void submitDeallocate(const rina::FlowDeallocateRequestEvent& event) = 0;

	/// When an Flow Allocator receives a Create_Request PDU for a Flow object, it consults its local Directory to see if it has an entry.
	/// If there is an entry and the address is this IPC Process, it creates an FAI and passes the Create_request to it.If there is an
	/// entry and the address is not this IPC Process, it forwards the Create_Request to the IPC Process designated by the address.
	/// @param cdapMessage
	/// @param underlyingPortId
	virtual void createFlowRequestMessageReceived(const rina::CDAPMessage& cdapMessage, int underlyingPortId) = 0;

	/// Called by the flow allocator instance when it finishes to cleanup the state.
	/// @param portId
	virtual void removeFlowAllocatorInstance(int portId) = 0;
};

/// Namespace Manager Interface
class INamespaceManager : public IPCProcessComponent {
public:
	virtual ~INamespaceManager(){};

	/// Returns the address of the IPC process where the application process is, or
	/// null otherwise
	/// @param apNamingInfo
	/// @return
	virtual unsigned int getDFTNextHop(const rina::ApplicationProcessNamingInformation& apNamingInfo) = 0;

	/// Returns the IPC Process id (0 if not an IPC Process) of the registered
	/// application, or -1 if the app is not registered
	/// @param apNamingInfo
	/// @return
	virtual int getRegIPCProcessId(const rina::ApplicationProcessNamingInformation& apNamingInfo) = 0;

	/// Add an entry to the directory forwarding table
	/// @param entry
	virtual void addDFTEntry(const rina::DirectoryForwardingTableEntry& entry) = 0;

	/// Get an entry from the application name
	/// @param apNamingInfo
	/// @return
	virtual rina::DirectoryForwardingTableEntry getDFTEntry(
			const rina::ApplicationProcessNamingInformation& apNamingInfo) = 0;

	/// Remove an entry from the directory forwarding table
	/// @param apNamingInfo
	virtual void removeDFTEntry(const rina::ApplicationProcessNamingInformation& apNamingInfo) = 0;

	/// Process an application registration request
	/// @param event
	virtual void processApplicationRegistrationRequestEvent(
			const rina::ApplicationRegistrationRequestEvent& event) = 0;

	/// Process an application unregistration request
	/// @param event
	virtual void processApplicationUnregistrationRequestEvent(
			const rina::ApplicationUnregistrationRequestEvent& event) = 0;
};

///N-1 Flow Manager interface
class INMinusOneFlowManager {
public:
	virtual ~INMinusOneFlowManager(){};

	/// Allocate an N-1 Flow with the requested QoS to the destination
	/// IPC Process
	/// @param flowInformation contains the destination IPC Process and requested
    /// QoS information
	/// @return handle to the flow request
	virtual unsigned int allocateNMinus1Flow(const rina::FlowInformation& flowInformation) throw (Exception) = 0;

	/// Process the result of an allocate request event
	/// @param event
	/// @throws IPCException
	virtual void allocateRequestResult(const rina::AllocateFlowRequestResultEvent& event) throw (Exception) = 0;

	/// Process a flow allocation request
	/// @param event
	/// @throws IPCException if something goes wrong
	virtual void flowAllocationRequested(const rina::FlowRequestEvent& event) throw (Exception) = 0;

	/// Deallocate the N-1 Flow identified by portId
	/// @param portId
	/// @throws IPCException if no N-1 Flow identified by portId exists
	virtual void deallocateNMinus1Flow(int portId) throw(Exception) = 0;

	/// Process the response of a flow deallocation request
	/// @throws IPCException
	virtual void deallocateFlowResponse(const rina::DeallocateFlowResponseEvent& event) throw (Exception) = 0;

	/// A flow has been deallocated remotely, process
	/// @param portId
	virtual void flowDeallocatedRemotely(const rina::FlowDeallocatedEvent& event) throw(Exception) = 0;

	/// Return the N-1 Flow descriptor associated to the flow identified by portId
	/// @param portId
	/// @return the N-1 Flow information
    /// @throws IPCException if no N-1 Flow identified by portId exists
	virtual const rina::FlowInformation& getNMinus1FlowInformation(int portId) const throw (Exception) = 0;

	/// Return the information of all the N-1 flows
	/// @return
	/// @throws IPCException
	virtual const std::list<rina::FlowInformation>& getAllNMinus1FlowsInformation() = 0;

	/// The IPC Process has been unregistered from or registered to an N-1 DIF
	/// @param evet
	/// @throws IPCException
	virtual void processRegistrationNotification(const rina::IPCProcessDIFRegistrationEvent& event) throw (Exception) = 0;

	/// True if the DIF name is a supoprting DIF, false otherwise
	/// @param difName
	/// @return
	virtual bool isSupportingDIF(const rina::ApplicationProcessNamingInformation& difName) = 0;
};

/// Interface PDU Forwarding Table Generator Policy
class IPDUFTGeneratorPolicy {
public:
	virtual ~IPDUFTGeneratorPolicy(){};
	virtual void set_ipc_process(const IPCProcess& ipc_process) = 0;
	virtual void set_dif_configuration(const rina::DIFConfiguration& dif_configuration) = 0;
	virtual void enrollmentToNeighbor(unsigned int address, bool newMember, unsigned int portId) = 0;
	virtual void flowAllocated(unsigned int address, unsigned int portId,
			unsigned int neighborAddress, unsigned int neighborPortId) = 0;
	virtual bool flowDeallocated(unsigned int portId) = 0;
};

/// Interface PDU Forwarding Table Generator
class IPDUForwardingTableGenerator {
public:
	virtual ~IPDUForwardingTableGenerator(){};
	virtual void set_ipc_process(const IPCProcess& ipc_process) = 0;
	virtual void set_dif_configuration(const rina::DIFConfiguration& dif_configuration) = 0;
	virtual const IPDUFTGeneratorPolicy& get_pdu_ft_generator_policy() const = 0;
};

/// Resource Allocator Interface
/// The Resource Allocator (RA) is the core of management in the IPC Process.
/// The degree of decentralization depends on the policies and how it is used. The RA has a set of meters
/// and dials that it can manipulate. The meters fall in 3 categories:
/// 	Traffic characteristics from the user of the DIF
/// 	Traffic characteristics of incoming and outgoing flows
/// 	Information from other members of the DIF
/// The Dials:
///     Creation/Deletion of QoS Classes
///     Data Transfer QoS Sets
///     Modifying Data Transfer Policy Parameters
///     Creation/Deletion of RMT Queues
///     Modify RMT Queue Servicing
///     Creation/Deletion of (N-1)-flows
///     Assignment of RMT Queues to (N-1)-flows
///     Forwarding Table Generator Output
class IResourceAllocator: public IPCProcessComponent {
public:
	virtual ~IResourceAllocator(){};
	virtual const INMinusOneFlowManager& get_n_minus_one_flow_manager() const = 0;
	virtual const IPDUForwardingTableGenerator& get_pdu_forwarding_table_generator() const = 0;
};

/// RIB Object Interface. API for the create/delete/read/write/start/stop RIB
/// functionality for certain objects (identified by objectNames)
class IRIBObject {
public:
	virtual ~IRIBObject(){};
	virtual const rina::RIBObjectData& get_data() const = 0;
	virtual const std::string& get_name() const = 0;
	virtual const std::string& get_class() const = 0;
	virtual long get_instance() const = 0;
	virtual void* get_value() const = 0;

	/// Parent-child management operations
	virtual const IRIBObject& get_parent() const = 0;
	virtual void set_parent(const IRIBObject& parent) = 0;
	virtual const std::list<IRIBObject>& get_children() const = 0;
	virtual void set_children(const std::list<IRIBObject>& children) const = 0;
	virtual unsigned int get_number_of_children() const = 0;
	virtual void add_child(const IRIBObject& child) throw (Exception) = 0;
	virtual void remove_child(const std::string& objectName) throw (Exception) = 0;

	/// Remote invocations via CDAP messages
	virtual void createObject(const rina::CDAPMessage& cdapMessage,
			const rina::CDAPSessionDescriptor& cdapSessionDescriptor) throw (Exception) = 0;
	virtual void deleteObject(const rina::CDAPMessage& cdapMessage,
			const rina::CDAPSessionDescriptor& cdapSessionDescriptor) throw (Exception) = 0;
	virtual void readObject(const rina::CDAPMessage& cdapMessage,
			const rina::CDAPSessionDescriptor& cdapSessionDescriptor) throw (Exception) = 0;
	virtual void cancelReadObject(const rina::CDAPMessage& cdapMessage,
			const rina::CDAPSessionDescriptor& cdapSessionDescriptor) throw (Exception) = 0;
	virtual void writeObject(const rina::CDAPMessage& cdapMessage,
			const rina::CDAPSessionDescriptor& cdapSessionDescriptor) throw (Exception) = 0;
	virtual void startObject(const rina::CDAPMessage& cdapMessage,
			const rina::CDAPSessionDescriptor& cdapSessionDescriptor) throw (Exception) = 0;
	virtual void stop(const rina::CDAPMessage& cdapMessage,
			const rina::CDAPSessionDescriptor& cdapSessionDescriptor) throw (Exception) = 0;
};

/// Common interface for update strategies implementations. Can be on demand, scheduled, periodic
class IUpdateStrategy {
public:
	virtual ~IUpdateStrategy(){};
};

/// Interface of classes that handle CDAP response message
class ICDAPResponseMessageHandler {
public:
	virtual ~ICDAPResponseMessageHandler(){};
	virtual void createResponse(const rina::CDAPMessage& cdapMessage,
			const rina::CDAPSessionDescriptor& cdapSessionDescriptor) = 0;
	virtual void deleteResponse(const rina::CDAPMessage& cdapMessage,
			const rina::CDAPSessionDescriptor& cdapSessionDescriptor) = 0;
	virtual void readResponse(const rina::CDAPMessage& cdapMessage,
			const rina::CDAPSessionDescriptor& cdapSessionDescriptor) = 0;
	virtual void cancelReadResponse(const rina::CDAPMessage & cdapMessage,
			const rina::CDAPSessionDescriptor& cdapSessionDescriptor) = 0;
	virtual void writeResponse(const rina::CDAPMessage& cdapMessage,
			const rina::CDAPSessionDescriptor & cdapSessionDescriptor) = 0;
	virtual void startResponse(const rina::CDAPMessage& cdapMessage,
			const rina::CDAPSessionDescriptor& cdapSessionDescriptor) = 0;
	virtual void stopResponse(const rina::CDAPMessage& cdapMessage,
			const rina::CDAPSessionDescriptor& cdapSessionDescriptor) = 0;
};

/// Part of the RIB Daemon API to control if the changes have to be notified
class NotificationPolicy {
public:
	NotificationPolicy(const std::list<unsigned int>& cdap_session_ids);
	const std::list<unsigned int>& get_cdap_session_ids() const;

private:
	std::list<unsigned int> cdap_session_ids_;
};

/// Interface that provides de RIB Daemon API
class IRIBDaemon : public IPCProcessComponent, public EventManager {
public:
	virtual ~IRIBDaemon(){};

	/// Add an object to the RIB
	/// @param ribHandler
	/// @param objectName
	/// @throws Exception
	virtual void addRIBObject(const IRIBObject& ribObject) throw (Exception) = 0;

	/// Remove an object from the RIB
	/// @param ribObject
	/// @throws Exception
	virtual void removeRIBObject(const IRIBObject& ribObject) throw (Exception) = 0;

	/// Remove an object from the RIB by objectname
	/// @param objectName
	/// @throws Exception
	virtual void removeRIBObject(const std::string objectName) throw (Exception) = 0;

	/// Send an information update, consisting on a set of CDAP messages, using the updateStrategy update strategy
	/// (on demand, scheduled)
	/// @param cdapMessages
	/// @param updateStrategy
	virtual void sendMessages(const std::list<rina::CDAPMessage>& cdapMessages,
			const IUpdateStrategy& updateStrategy) = 0;

	/// Causes a CDAP message to be sent
	/// @param cdapMessage the message to be sent
	/// @param sessionId the CDAP session id
	/// @param cdapMessageHandler the class to be called when the response message is received (if required)
	/// @throws Exception
	virtual void sendMessage(const rina::CDAPMessage& cdapMessage, int sessionId,
			const ICDAPResponseMessageHandler& cdapMessageHandler) throw (Exception) = 0;

	/// Causes a CDAP message to be sent
	/// @param cdapMessage the message to be sent
	/// @param sessionId the CDAP session id
	/// @param address the address of the IPC Process to send the Message To
	/// @param cdapMessageHandler the class to be called when the response message is received (if required)
	/// @throws Exception
	virtual void sendMessageToAddress(const rina::CDAPMessage& cdapMessage, int sessionId, long address,
			const ICDAPResponseMessageHandler& cdapMessageHandler) throw (Exception) = 0;

	/// Reads/writes/created/deletes/starts/stops one or more objects at the RIB, matching the
	/// information specified by objectId + objectClass or objectInstance.At least objectName or
	/// objectInstance have to be not null. This operation is invoked because the RIB Daemon has
	/// received a CDAP message from another IPC process
	/// @param cdapMessage The CDAP message received
	/// @param cdapSessionDescriptor Describes the CDAP session to where the CDAP message belongs
	/// @throws Exception on a number of circumstances
	virtual void processOperation(const rina::CDAPMessage& cdapMessage,
			const rina::CDAPSessionDescriptor& cdapSessionDescriptor) throw (Exception) = 0;

	/// Create or update an object in the RIB
	/// @param objectClass the class of the object
	/// @param objectName the name of the object
	/// @param objectInstance the instance of the object
	/// @param objectValue the value of the object
	/// @param notify if not null notify some of the neighbors about the change
	/// @throws Exception
	virtual void createObject(const std::string& objectClass, long objectInstance,
			const std::string& objectName, void* objectValue, NotificationPolicy notificationPolicy) throw (Exception) = 0;
	virtual void createObject(const std::string& objectClass, const std::string& objectName,
			void* objectValue, const NotificationPolicy& notificationPolicy) throw (Exception) = 0;
	virtual void createObject(long objectInstance, void* objectValue,
			const NotificationPolicy& notificationPolicy) throw (Exception) = 0;
	virtual void createObject(const std::string& objectClass, const std::string& objectName,
			void* objectValue) throw (Exception) = 0;
	virtual void createObject(long objectInstance, void* objectValue) throw (Exception) = 0;

	/// Delete an object from the RIB
	/// @param objectClass the class of the object
	/// @param objectName the name of the object
	/// @param objectInstance the instance of the object
	/// @param object the value of the object
	/// @param notify if not null notify some of the neighbors about the change
	/// @throws Exception
	virtual void deleteObject(const std::string& objectClass, long objectInstance, const std::string& objectName,
			void* objectValue, NotificationPolicy notify) throw (Exception) = 0;
	virtual void deleteObject(const std::string& objectClass, const std::string&  objectName,
			void* objectValue, NotificationPolicy notificationPolicy) throw (Exception) = 0;
	virtual void deleteObject(long objectInstance, void* objectValue, NotificationPolicy notificationPolicy) throw (Exception) = 0;
	virtual void deleteObject(const std::string& objectClass, const std::string& objectName,
			void* objectValue) throw (Exception) = 0;;
	virtual void deleteObject(long objectInstance, void* objectValue) throw (Exception) = 0;
	virtual void deleteObject(const std::string& objectClass, long objectInstance,
			const std::string&  objectName, NotificationPolicy notificationPolicy) throw (Exception) = 0;
	virtual void deleteObject(const std::string& objectClass, long objectInstance,
			const std::string&  objectName) throw (Exception) = 0;
	virtual void deleteObject(const std::string& objectClass, const std::string& objectName) throw (Exception) = 0;
	virtual void deleteObject(long objectInstance) throw (Exception) = 0;

	/// Read an object from the RIB
	/// @param objectClass the class of the object
	/// @param objectName the name of the object
	/// @param objectInstance the instance of the object
	/// @return a RIB object
	/// @throws Exception
	virtual const IRIBObject& readObject(const std::string& objectClass, long objectInstance,
			const std::string& objectName) const throw (Exception) = 0;
	virtual const IRIBObject& readObject(const std::string& objectClass, const std::string& objectName) const throw (Exception) = 0;
	virtual const IRIBObject& readObject(long objectInstance) const throw (Exception) = 0;

	/// Update the value of an object in the RIB
    /// @param objectClass the class of the object
	/// @param objectName the name of the object
	/// @param objectInstance the instance of the object
	/// @param objectValue the new value of the object
	/// @param notify if not null notify some of the neighbors about the change
	/// @throws Exception
	virtual void writeObject(const std::string& objectClass, long objectInstance,
			const std::string& objectName, void* objectValue, NotificationPolicy notify) throw (Exception) = 0;
	virtual void writeObject(const std::string& objectClass, const std::string& objectName,
			void* objectValue, NotificationPolicy notificationPolicy) throw (Exception) = 0;
	virtual void writeObject(long objectInstance, void* objectValue,
			NotificationPolicy notificationPolicy) throw (Exception) = 0;
	virtual void writeObject(const std::string& objectClass, const std::string& objectName,
			void* objectValue) throw (Exception) = 0;
	virtual void writeObject(long objectInstance, void* objectValue) throw (Exception) = 0;

	/// Start an object at the RIB
	/// @param objectClass the class of the object
	/// @param objectName the name of the object
	/// @param objectInstance the instance of the object
	/// @param objectValue the new value of the object
	/// @throws Exception
	virtual void starObject(const std::string& objectClass, long objectInstance, const std::string& objectName,
			void* objectValue) throw (Exception) = 0;
	virtual void starObject(const std::string& objectClass, const std::string& objectName,
			void* objectValue) throw (Exception) = 0;
	virtual void starObject(long objectInstance, void* objectValue) throw (Exception) = 0;
	virtual void starObject(const std::string& objectClass, const std::string& objectName) throw (Exception) = 0;
	virtual void starObject(long objectInstance) throw (Exception) = 0;

	/// Stop an object at the RIB
	/// @param objectClass the class of the object
	/// @param objectName the name of the object
	/// @param objectInstance the instance of the object
	/// @param objectValue the new value of the object
	/// @throws Exception
	virtual void stopObject(const std::string& objectClass, long objectInstance,
			const std::string& objectName, void* objectValue) throw (Exception) = 0;
	virtual void stopObject(const std::string& objectClass, const std::string& objectName, void* objectValue) throw (Exception) = 0;
	virtual void stopObject(long objectInstance, void* objectValue) throw (Exception) = 0;

	/// Process a Query RIB Request from the IPC Manager
	/// @param event
	virtual void processQueryRIBRequestEvent(const rina::QueryRIBRequestEvent& event) = 0;

	virtual const std::list<IRIBObject>& getRIBObjects() const = 0;
};

/// IPC Process interface
class IPCProcess {
public:
	virtual ~IPCProcess(){};
	virtual const IDelimiter& get_delimiter() const = 0;
	virtual const IEncoder& get_encoder() const = 0;
	virtual const rina::CDAPSessionManagerInterface& get_cdap_session_manager() const = 0;
	virtual const IEnrollmentTask& get_enrollment_task() const = 0;
	virtual const IFlowAllocator& get_flow_allocator() const = 0;
	virtual const INamespaceManager& get_namespace_manager() const = 0;
	virtual const IResourceAllocator& get_resource_allocator() const = 0;
	virtual const IRIBDaemon& get_rib_daemon() const = 0;
	virtual unsigned int get_address() = 0;
	virtual void set_address(unsigned int address) = 0;
	virtual const IPCProcessOperationalState& get_operational_state() const = 0;
	virtual void set_operational_state(const IPCProcessOperationalState& operational_state) = 0;
	virtual const rina::DIFInformation& get_dif_information() const = 0;
	virtual void set_dif_information(const rina::DIFInformation& dif_information) = 0;
	virtual const std::list<rina::Neighbor>& get_neighbors() const = 0;
	virtual unsigned int getAdressByname(const rina::ApplicationProcessNamingInformation& name) = 0;
};

/// Contains the object names of the objects in the RIB
class RIBObjectNames
{
public:
	/// Partial names
	static const std::string ADDRESS;
	static const std::string APNAME;
	static const std::string CONSTANTS;
	static const std::string DATA_TRANSFER;
	static const std::string DAF;
	static const std::string DIF;
	static const std::string DIF_REGISTRATIONS;
	static const std::string DIRECTORY_FORWARDING_TABLE_ENTRIES;
	static const std::string ENROLLMENT;
	static const std::string FLOWS;
	static const std::string FLOW_ALLOCATOR;
	static const std::string IPC;
	static const std::string MANAGEMENT;
	static const std::string NEIGHBORS;
	static const std::string NAMING;
	static const std::string NMINUSONEFLOWMANAGER;
	static const std::string NMINUSEONEFLOWS;
	static const std::string OPERATIONAL_STATUS;
	static const std::string PDU_FORWARDING_TABLE;
	static const std::string QOS_CUBES;
	static const std::string RESOURCE_ALLOCATION;
	static const std::string ROOT;
	static const std::string SEPARATOR;
	static const std::string SYNONYMS;
	static const std::string WHATEVERCAST_NAMES;
	static const std::string ROUTING;
	static const std::string FLOWSTATEOBJECTGROUP;

	/* Full names */
	static const std::string OPERATIONAL_STATUS_RIB_OBJECT_NAME;
	static const std::string OPERATIONAL_STATUS_RIB_OBJECT_CLASS;
	static const std::string PDU_FORWARDING_TABLE_RIB_OBJECT_CLASS;
	static const std::string PDU_FORWARDING_TABLE_RIB_OBJECT_NAME;

	virtual ~RIBObjectNames(){};
};

}

#endif

#endif
