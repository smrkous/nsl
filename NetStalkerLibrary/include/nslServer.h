/* 
	Server.h
	Classes and constructs for building the server side of the connection.
*/

#pragma once

#include "nsl.h"
#include "nslReflexion.h"
#include "nslBitStream.h"

namespace nsl {

	namespace server {

		/// Forward declaration of inner library class
		class ServerImpl;

		/// Forward declaration of inner library class
		class NetworkObject; 

		/// Forward declaration of inner library class
		class Peer;
	}

	/// class representing connected client
	class Peer
	{
	public:
		void* customPointer;

		NSL_IMPORT_EXPORT
		unsigned int getId(void);

		NSL_IMPORT_EXPORT
		const char* getIp(void);
	private:
		friend class server::Peer;
		friend class server::ServerImpl;

		Peer(server::Peer*);
		~Peer(void);
		server::Peer* peer;
	};		

	/// Server side of shared object.
	/// Interface provides data writing operations.
	class ServerObject
	{
	public:

		/// Get object instance unique (across network) identificator.
		NSL_IMPORT_EXPORT
		unsigned int getId(void);

		/// Get object class unique identificator.
		NSL_IMPORT_EXPORT
		unsigned short getObjectClassId(void);

		/// Set actual data of specific atribute.
		/// Valid attribute id and respective data type must be provided, 
		/// otherwise exception might be thrown to prevent memmory leak or data values might be wrong.
		template<class T>
		void set(unsigned int attrId, typename T::Type value) {set(attrId, T::getByteSize(), (byte*)&value);}

		/// Delete this object from network space.
		NSL_IMPORT_EXPORT
		void destroy(void);

	private:
		friend class server::NetworkObject;
		friend class server::ServerImpl;
		server::NetworkObject* networkObject;

		ServerObject(server::NetworkObject*);
		~ServerObject(void);

		NSL_IMPORT_EXPORT
		void set(unsigned int attrId, unsigned int byteSize, byte* value);
	};


	/// Class representing server. Manages connection and all communication. 
	class Server
	{
	public:

		NSL_IMPORT_EXPORT
		Server(unsigned int applicationId, unsigned short port);

		NSL_IMPORT_EXPORT
		~Server(void);

		/// Open connection and start listening.
		NSL_IMPORT_EXPORT
		void open(void);

		/// Close all opened connections and all network activity.
		NSL_IMPORT_EXPORT
		void close(void);

		/// Add custom defined object class. 
		/// Only defined classes can be created and shared with clients.
		NSL_IMPORT_EXPORT
		void registerObjectClass(ObjectClass* objectClass);

		/// Create new networked object of given type
		NSL_IMPORT_EXPORT
		ServerObject* createObject(unsigned short classId);

		/// Data written into provided stream will be sent to chosen peer after flushNetwork() is called.
		/// Max size of the message is NSL_MAX_CUSTOM_MESSAGE_SIZE
		/// The stream is removed from memmory after flushNetwork().
		/// If reliable is set to true, every sent message will be delivered to server
		/// If the message should be send just within this update (f.e. in the next packet new fresh 
		/// info will be sent and this message will be no longer relevant), set reliable to false
		NSL_IMPORT_EXPORT
		BitStreamWriter* createCustomMessage(Peer* peer, bool reliable = true);
		
		/// Callback on new client connection
		/// Default - add object receiver
		/// Returning false is considered as an authentication fail and connection is closed.
		NSL_IMPORT_EXPORT
		virtual bool onClientConnect(Peer* peer);	
		
		/// Callback on disconnect by client
		/// Default - remove object receiver
		NSL_IMPORT_EXPORT
		virtual void onClientDisconnect(Peer* peer);

		/// Callback on custom message arrival from client
		/// Default - no action
		NSL_IMPORT_EXPORT
		virtual void onMessageAccept(Peer* peer, BitStreamReader* stream);

		/// Callback for defining scope for one peer
		/// If false is returned, scope is ignored and peer receives all objects (default)
		/// If true is returned, peer will receive only objects from scope - add object to peer's scope by calling addToScope during this callback
		NSL_IMPORT_EXPORT
		virtual bool getScope(Peer* peer);
				
		/// Add object to peer's scope.
		/// Callable only in body of getScope(...), otherwise exception is thrown.
		NSL_IMPORT_EXPORT
		void addToScope(ServerObject*);

		/// Send object updates to all connected clients
		NSL_IMPORT_EXPORT
		void flushNetwork(void);

		/// Process updates from clients
		NSL_IMPORT_EXPORT
		void updateNetwork(void);

	private:
		server::ServerImpl* i;
	};
};
