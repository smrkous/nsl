/*
 * Copyright (C) 2013 Petr Smrcek
 * This file is originally a part of the Net Stalker Library
 * For conditions of distribution and use, see copyright notice in nsl.h
 */

#pragma once

#include "../include/nsl.h"
#include "../include/nslBitStream.h"

#if defined NSL_PLATFORM_WINDOWS
	#include <SDKDDKVer.h>
	#define WIN32_LEAN_AND_MEAN
	#include <windows.h>
#else
	#include <stdlib.h>
	#include <string.h>
#endif

namespace nsl {

	/* server configuration */

	#define NSL_PACKET_BUFFER_SIZE_SERVER 50

	/* client configuration */

	#define NSL_PACKET_BUFFER_SIZE 50
	#define NSL_CUSTOM_MESSAGE_BUFFER_SIZE 150
	#define NSL_INTERPOLATION_MINIMAL_DATA_COUNT 10		// number of indexes in buffer behind application time, which cannot be rewritten
	#define NSL_MINIMAL_PACKET_COUNT 4					// minimal number of valid updates received to start the application ( >= 2 )
	#define NSL_INTERPOLATION_LATENCY_PACKET_COUNT 2.1	// number of snapshots, that is the application behind network updates (can be floating point number)
	#define NSL_MAXIMAL_SPEEDUP 1.1						// maximal multiplicator of time (speed or slow) which can be used to reach optimal application time
	#define NSL_TIME_INTERVAL_AVERAGE_COUNT 5			// number of intervals used to count average tick time (less = faster reaction but more frequent speedups/slowdowns)

	/* common configuration */

	#define NSL_COMPRESS	// should sockets be compressed?
	//#define NSL_COMPRESSION_HIGH	// define this for higher level of compression at the cost of performance
									// packet sizes with standard compression: ~85%, high compression: ~65% 
									// the bigger the raw packet is, the better the compression rates are
	//#define NSL_IPV6

	#define NSL_SEQ_MODULO 65535
	typedef unsigned short seqNumber;

	#define NSL_MAX_UDP_PACKET_SIZE 64000	// size of buffers for packet receiving and sending, do not set bigger then 64000
	#define NSL_INITIAL_MAX_PACKET_SIZE 2000	// initial size of buffers for packet parsing, this will dynamically increase as needed


	/* inner constans (do not change) */

	#define NSL_UNDEFINED_BUFFER_INDEX -1

	#define NSL_CONNECTION_FLAG_DISCONNECT 1
	#define NSL_CONNECTION_FLAG_HANDSHAKE 2
	#define NSL_CONNECTION_FLAG_UPDATE 3
	#define NSL_CONNECTION_FLAG_COMPRESSED_UPDATE 4

	#define NSL_TIMEOUT_CLIENT_CONNECTION_REQUEST 0.5
	#define NSL_TIMEOUT_CLIENT_HANDSHAKE 0.5
	#define NSL_TIMEOUT_SERVER_HANDSHAKE_RESEND 0.5
	#define NSL_TIMEOUT_SERVER_HANDSHAKE_KILL 5
	#define NSL_TIMEOUT_SERVER_CONNECTED_KILL 5

	#define NSL_OBJECT_FLAG_ACTION_DIFF 0
	#define NSL_OBJECT_FLAG_ACTION_END_OF_SECTION 1
	#define NSL_OBJECT_FLAG_ACTION_SNAPSHOT 2
	#define NSL_OBJECT_FLAG_ACTION_DELETE 3
	#define NSL_OBJECT_FLAG_ACTION_CREATE 4
	#define NSL_OBJECT_FLAG_ACTION_CREATE_AND_DELETE 5

	#define NSL_OBJECT_FLAG_SC_BIRTH 0
	#define NSL_OBJECT_FLAG_SC_SHOW 1

	#define NSL_OBJECT_FLAG_SD_DEATH 0
	#define NSL_OBJECT_FLAG_SD_HIDE 1

	#define NSL_OBJECT_FLAG_CM_EMPTY 0
	#define NSL_OBJECT_FLAG_CM_PRESENT 1

	struct ObjectFlags {
		byte action					: 3;
		byte scopeCreate			: 1;
		byte scopeDestroy			: 1;
		byte creationCustomMessage	: 1;
		byte						: 2;
		ObjectFlags(void) {*(byte*)this = 0;}
	};

	template <>
	inline nsl::BitStreamReader & operator>>(nsl::BitStreamReader& r, ObjectFlags & flags) {
		byte b = r.readByte();
		flags = *(ObjectFlags*)&b;
		return r;
	}
	
	template <>
	inline nsl::BitStreamWriter & operator<<(nsl::BitStreamWriter& w, const ObjectFlags & flags) {
		w.writeByte(*(byte*)&flags);
		return w;
	}
	
	/* debug configuration */

	//#define NSL_LOG_PACKETS
	#define NSL_PACKET_LOG_FILENAME "packet_log.csv"

	//#define NSL_PACKET_LOSS 0.1	// define to simulate loss of packets (you can specify probability, that packet will be lost)
	//#define NSL_PACKET_LOSS_SEED 9	// seed for the same results across many simulations

	#ifdef NSL_LOG_PACKETS
		void logBytes(byte* data, unsigned int size);
		void logString(const char*);
	#endif

	double getTime();

	// TODO: do dokumentace napsat, ze vsechny objekty, ktery knihovna predava, tak taky sama dealokuje
	// TODO: pridat const kde muze byt

	// TODO: zbavit se objektu na klientu po ukonceni spojeni
};
