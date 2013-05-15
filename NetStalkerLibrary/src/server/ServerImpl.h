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
#include "ProtocolParser.h"
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
			ProtocolParser protocolParser;
			Server* userObject;
			bool opened;
			double lastUpdateTime;
			std::map<unsigned int, Peer*> connectedPeers;
			std::map<unsigned int, BitStreamWriter*> unproccessedCreationCustomMessages;
			std::set<NetworkObject*> currentScope;
			bool currentScopeAccessible;
		public:
			ServerImpl(Server* userObject, unsigned int applicationId);
			~ServerImpl(void);

			// opens connection on specified port, must be done before any communication
			void open(const char* port = NULL);

			// close all opened connections and all network activity
			void close(void);

			// register new object type
			void registerObjectClass(ObjectClassDefinition* objectClass);

			// create networked object of given type
			NetworkObject* createObject(unsigned int classId);

			// create networked object of given type and with additional creation metadata
			NetworkObject* createObject(unsigned int classId, BitStreamWriter*& creationMetaData);

			// send custom message to specified adress
			BitStreamWriter* createCustomMessage(nsl::Peer* peer, bool reliable);

			void updateNetwork(double time = 0);

			/// add object to current scope
			/// if current scope is null, exception is thrown
			void addToScope(ServerObject* object);

			// send updates to all connected clients
			void flushNetwork(void);
		};
	};
};
