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
	}

	/// Server side of shared object.
	/// Interface provides data writing operations.
	class NSL_IMPORT_EXPORT ServerObject
	{
	public:
		friend class server::NetworkObject;

		/// Get object instance unique (across network) identificator.
		int getId(void);

		/// Get object class unique identificator.
		int getObjectClassId(void);

		/// Set actual data of specific atribute.
		/// Valid attribute id and respective data type must be provided, 
		/// otherwise exception might be thrown to prevent memmory leak or data values might be wrong.
		template<class T> void set(int attrId, T::Type value);

		/// Delete this object from network space.
		void destroy(void);
	private:
		ServerObject(server::NetworkObject*);
		~ServerObject(void);
	};


	/// Class representing server. Manages connection and all communication. 
	class NSL_IMPORT_EXPORT Server
	{
	public:
		/// Open connection and start listening.
		bool open(int port);

		/// Close all opened connections and all network activity.
		void close(void);

		/// Add custom defined object class. 
		/// Only defined classes can be created and shared with clients.
		void addNetworkObjectType(ObjectClass* type);

		/// Create new networked object of given type
		ServerObject* createObject(int classId);

		/// Send custom reliable message to one client
		BitStreamWriter* sendData(int client);

		/// Callback on new client connection
		/// Default - add object receiver
		/// Returning false is considered as an authentication fail and connection is closed.
		virtual bool onClientConnect(int peer);	

		/// Callback on disconnect by client
		/// Default - remove object receiver
		virtual void onClientDisconnect(int peer);

		/// Callback on custom message arrival from client
		/// Default - no action
		virtual void onMessageAccept(int peer, BitStreamReader* stream);
	
		/// Send object updates to all connected clients
		int flushNetwork(void);

		/// Process updates from clients
		void updateNetwork(void);

		Server(void);
		~Server(void);





		
		/// Add client to object receiver list, objects will be shared with him from now
		void addObjectReceiver(int peer);

		/// Remove client from object receiver list
		void removeObjectReceiver(int peer);





	private:
		server::ServerImpl* i;
	};
};
