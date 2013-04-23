#pragma once

namespace nsl {
	class Server;
	class ServerObject;
	class ObjectClassDefinition;
	class BitStreamWriter;
	class Peer;

	namespace server {
		class NetworkObject;
		class Peer;
	};
};
		
#include "Connection.h"
#include "ObjectManager.h"
#include "HistoryBuffer.h"
#include <map>
#include <set>

namespace nsl {
	namespace server {

		class ServerImpl
		{
		private:
			Connection connection;
			ObjectManager objectManager;
			HistoryBuffer historyBuffer;
			Server* userObject;
			bool opened;
			std::map<unsigned int, Peer*> connectedPeers;
			std::set<NetworkObject*> currentScope;
			bool currentScopeAccessible;

			/// Append all custom messages from given index
			void pushBufferedMessagesByIndex(BitStreamWriter* stream, int bufferIndex, Peer* peer);
			void sendUpdateToPeer(Peer* peer, std::set<NetworkObject*>& scope, int ackIndex);
			void writeObjectData(ObjectClassDefinition* objectClass, BitStreamWriter* stream, byte* data);
		public:
			ServerImpl(Server* userObject, unsigned int applicationId, unsigned short port);
			~ServerImpl(void);

			// opens connection on specified port, must be done before any communication
			void open(void);

			// close all opened connections and all network activity
			void close(void);

			// register new object type
			void registerObjectClass(ObjectClassDefinition* objectClass);

			// create networked object of given type
			NetworkObject* createObject(unsigned int classId);

			// send custom message to specified adress
			BitStreamWriter* createCustomMessage(nsl::Peer* peer, bool reliable);

			void updateNetwork(void);

			/// add object to current scope
			/// if current scope is null, exception is thrown
			void addToScope(ServerObject* object);

			// send updates to all connected clients
			void flushNetwork(void);
		};
	};
};
