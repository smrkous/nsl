/* 
	nslClient.h
	Classes and constructs for building the client side of the connection.
*/

#pragma once

#include "nsl.h"
#include "nslReflexion.h"
#include "nslBitStream.h"

namespace nsl {

	namespace client {

		/// Forward declaration of inner library class
		class ClientImpl;

		/// Forward declaration of inner library class
		class NetworkObject;
	};


	/// Client side of shared object.
	/// Interface provides data reading operations.
	class NSL_IMPORT_EXPORT ClientObject 
	{
	private:
		friend class client::NetworkObject;

		client::NetworkObject* networkObject;

		ClientObject(client::NetworkObject*);
		~ClientObject(void);
	public:
		/// Get object instance unique (across network) identificator.
		int getId(void);

		/// Get object class unique identificator.
		int getObjectClassId(void);

		/// Get actual data of specific atribute.
		/// Valid attribute id and respective data type must be provided, 
		/// otherwise exception might be thrown to prevent memmory leak or data values might be wrong.
		template<class T> T::Type get(int attrId);
	
	};


	/// Class representing client. Manages connection and all communication. 
	/// Must be overriden by the application to define mandatory callbacks.
	class NSL_IMPORT_EXPORT Client
	{
	public:
		/// Define client side port and unique application identifier
		Client(short applicationId, short clientPort);

		/// Delete client and stop all of its work. All related network objects and streams will be deleted as well.
		~Client(void);

		/// Add custom defined object class. 
		/// Only defined classes can be received from server, otherwise exception is thrown.
		void registerObjectClass(ObjectClass* objectClass);

		/// Open the connection.
		/// This operation is non-blocking, so the connection won't open immediately.
		/// Try to updateNetwork() until true is received or until you decide to close().
		void open(int port, const char* address) throw (Exception);

		/// Close the connection.
		void close(void);

		/// Process any data, which were received from network.
		/// If true is returned, there is working connection and the data are actually updated.
		/// If false is returned, connection is not responding, but trying to connect again.
		bool updateNetwork(void);
		
		/// Send all recent client messages to server.
		/// This should be called as often as updateNetwork() for better connection performance.
		void flushNetwork(void);

		/// Callback on new incomming object.
		virtual void onCreate(ClientObject* object) = 0;

		/// Callback on object deletion.
		virtual void onDestroy(ClientObject* object) = 0;

		/// Callback on new incomming custom message.
		/// Default - no action
		virtual void onMessageAccept(BitStreamReader* stream);
		
		/// Write the data into provided stream.
		/// The data are sent to server after flushNetwork() is called.
		BitStreamWriter* createCustomMessage(void);

	private:
		client::ClientImpl *i;
	};
};