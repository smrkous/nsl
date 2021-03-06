/* nslClient.h - classes and constructs for building the client side of the connection
 * Copyright (C) 2013 Petr Smrcek
 * This file is originally a part of the Net Stalker Library
 * For conditions of distribution and use, see copyright notice in nsl.h
 */

#pragma once

#include "nsl.h"
#include "nslReflection.h"
#include "nslBitStream.h"

namespace nsl {

	namespace client {

		/// Forward declaration of inner library class
		class ClientImpl;

		/// Forward declaration of inner library class
		class NetworkObject;
	};

	/// State of client
	enum ClientState {
		/// server has not responded yet or it already closed the connection
		NSL_CS_CLOSED,

		/// server responded and the connection is being estabilished
		NSL_CS_HANDSHAKING,

		/// connection estabilished, but application updates wait for buffer to fill to their minimum
		NSL_CS_BUFFERING,

		/// connection fully estabilished, updates are being received and propagated to the application
		NSL_CS_OPENED
	};

	/// Client side of shared object.
	/// Interface provides data reading operations.
	class ClientObject 
	{
	public:
		/// Get object instance unique (across network) identificator.
		NSL_IMPORT_EXPORT
		unsigned int getId(void);

		/// Get object class unique identificator.
		NSL_IMPORT_EXPORT
		unsigned short getObjectClassId(void);

		/// Get actual data of specific atribute.
		/// If the attribute was defined as different type, the result is undefined and an exception might be thrown as well.
		template<class T> 
		typename T::Type get(unsigned int attrId);

		/// Pointer to custom data
		void* customData;

	private:
		friend class client::NetworkObject;

		client::NetworkObject* networkObject;
		bool locked;

		ClientObject(client::NetworkObject*);
		~ClientObject(void);

		/// if the value is not interpolated, false is returned and the single data piece is stored in data[0]
		NSL_IMPORT_EXPORT
		bool get(
			unsigned int attrId, 
			unsigned int byteSize, 
			unsigned int& pointCount,
			byte**& data,
			double*& times,
			double& targetTime,
			abstractInterpolationFunction& interpolationFunction
		);
	};


	/// Class representing client. Manages connection and all communication. 
	/// Must be overriden by the application to define mandatory callbacks.
	class Client
	{
	public:
		/// Define client side port and unique application identifier
		NSL_IMPORT_EXPORT
		Client(unsigned short applicationId);

		/// Delete client and stop all of its work. All related network objects and streams will be deleted as well.
		NSL_IMPORT_EXPORT
		~Client(void);

		/// Add custom defined object class. 
		/// Only defined classes can be received from server, otherwise exception is thrown.
		NSL_IMPORT_EXPORT
		void registerObjectClass(ObjectClass& objectClass);

		/// Open the connection.
		/// This operation is non-blocking, so the connection won't open immediately.
		/// Try to updateNetwork() until true is received or until you decide to close().
		NSL_IMPORT_EXPORT
		void open(const char* address, const char* port, const char* clientPort = NULL);

		/// Close the connection.
		NSL_IMPORT_EXPORT
		void close(void);

		/// Process any data, which were received from network.
		/// Return current connection state
		/// Library manages time on its own, hovewer, it is possible to specify the time by application (only positive numbers are accepted).
		/// Library works with seconds (in double format), if you pass time manually in server and client both, you can choose another precission.
		NSL_IMPORT_EXPORT
		ClientState updateNetwork(double time = 0);
		
		/// Send all recent client messages to server.
		/// This should be called as often as updateNetwork() for better connection performance.
		NSL_IMPORT_EXPORT
		void flushNetwork(void);

		/// Callback on new incomming object.
		/// objectBirth = false -> object existed before, now it just got into scope
		NSL_IMPORT_EXPORT
		virtual void onCreate(ClientObject* object, bool objectBirth, BitStreamReader* creationMetaData) = 0;

		/// Callback on object deletion.
		/// objectDeath = false -> object is not really destoryed, it just left scope
		NSL_IMPORT_EXPORT
		virtual void onDestroy(ClientObject* object, bool objectDeath) = 0;

		/// Callback on new incomming custom message.
		/// Default - no action
		NSL_IMPORT_EXPORT
		virtual void onMessageAccept(BitStreamReader* stream);
		
		/// Data written into the stream will be sent to server as cutom message.
		/// Max size of the message is NSL_MAX_CUSTOM_MESSAGE_SIZE
		/// The stream is removed from memmory after flushNetwork().
		/// If reliable is set to true, every sent message will be delivered to server
		/// If the message should be send just within this update (f.e. in the next packet new fresh 
		/// info will be sent and this message will be no longer relevant), set reliable to false
		NSL_IMPORT_EXPORT
		BitStreamWriter* createCustomMessage(bool reliable = true);

	private:
		client::ClientImpl *i;
	};



	template<class T>
	typename T::Type ClientObject::get(unsigned int attrId) 
	{
		typename T::Type result;
		unsigned int pointCount;
		byte** data;
		double* times;
		double targetTime;
		abstractInterpolationFunction interpolationFunction;

		if (!get(
			attrId, 
			T::getByteSize(),
			pointCount,
			data,
			times,
			targetTime,
			interpolationFunction
		)) {
			result = *((typename T::Type*)data[0]);
		} else {
			((typename T::interpolationFunction)interpolationFunction)(pointCount, (typename T::Type**)data, times, targetTime, &result);
		};

		return result;
	}
};
