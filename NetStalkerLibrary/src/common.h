#pragma once

#include "../stdafx.h"
#include "../include/nsl.h"

namespace nsl {

	/* Platform recognition */

	#if defined(_WIN32)
	#define PLATFORM_WINDOWS
	#elif defined(__APPLE__)
	#define PLATFORM_MAC
	#else
	#define PLATFORM_UNIX
	#endif

	/* Endianity recognition */

	#if !defined(NSL_BIG_ENDIAN) && !defined(NSL_LITTLE_ENDIAN)
		#if defined(_M_IX86) || defined(i386)
			#define NSL_LITTLE_ENDIAN
		#elif defined(__ppc__) || defined(__powerpc__) || defined (PPC)
			#define NSL_BIG_ENDIAN
		#else
			#error "NSL: Endianity not recognized, must be specified manualy by macros NSL_BIG_ENDIAN or NSL_LITTLE_ENDIAN"
		#endif
	#endif

	/* server configuration */

	#define NSL_PACKET_BUFFER_SIZE_SERVER 50

	/* client configuration */

	#define NSL_PACKET_BUFFER_SIZE 50
	#define NSL_CUSTOM_MESSAGE_BUFFER_SIZE 50
	#define NSL_UNDEFINED_BUFFER_INDEX -1
	#define NSL_INTERPOLATION_MINIMAL_DATA_COUNT 10		// number of indexes in buffer behind application time, which cannot be rewritten
	#define NSL_MINIMAL_PACKET_COUNT 3					// minimal number of valid updates received to start the application ( >= 2 )
	#define NSL_INTERPOLATION_LATENCY_PACKET_COUNT 1.1	// number of snapshots, that is the application behind network updates (can be floating point number)
	#define NSL_MAXIMAL_SPEEDUP 1.1						// maximal multiplicator of time (speed or slow) which can be used to reach optimal application time
	#define NSL_TIME_INTERVAL_AVERAGE_COUNT 5			// number of intervals used to count average tick time (less = faster reaction but more frequent speedups/slowdowns)

	/* common configuration */

	#define NSL_SEQ_MODULO 65535
	typedef unsigned short seqNumber;

	#define MAX_PACKET_SIZE 1000

	#define NSL_CONNECTION_FLAG_UPDATE 1
	#define NSL_CONNECTION_FLAG_DISCONNECT 2
	#define NSL_CONNECTION_FLAG_HANDSHAKE 3

	#define NSL_TIMEOUT_CLIENT_CONNECTION_REQUEST 0.5
	#define NSL_TIMEOUT_CLIENT_HANDSHAKE 0.5
	#define NSL_TIMEOUT_SERVER_HANDSHAKE_RESEND 0.5
	#define NSL_TIMEOUT_SERVER_HANDSHAKE_KILL 5
	#define NSL_TIMEOUT_SERVER_CONNECTED_KILL 5

	#define NSL_DUMP_FILENAME_SERVER "server_dump_log.csv"
	#define NSL_DUMP_FILENAME_CLIENT "client_dump_log.csv"
	#define NSL_PACKET_LOG_FILENAME_SERVER "server_packet_log.csv"
	#define NSL_PACKET_LOG_FILENAME__CLIENT "client_packet_log.csv"

	//#define PACKET_LOSS_RATE 30				// percentage of lost packets (simulated for debug purposes)

	#define NSL_OBJECT_FLAG_END_OF_SECTION 0

	#define NSL_OBJECT_FLAG_DIFF 1
	#define NSL_OBJECT_FLAG_SNAPSHOT 2
	#define NSL_OBJECT_FLAG_DELETE 3
	#define NSL_OBJECT_FLAG_OUT_OF_SCOPE 4

	#define NSL_OBJECT_FLAG_CREATE 5
	#define NSL_OBJECT_FLAG_IN_SCOPE 6
	#define NSL_OBJECT_FLAG_CREATE_AND_DELETE 7
	#define NSL_OBJECT_FLAG_IN_SCOPE_AND_DELETE 8
	#define NSL_OBJECT_FLAG_IN_SCOPE_AND_OUT_OF_SCOPE 9
	#define NSL_OBJECT_FLAG_CREATE_AND_OUT_OF_SCOPE 10
};
