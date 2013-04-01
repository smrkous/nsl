#pragma once

#include "../stdafx.h"
#include "../include/NSL.h"

namespace nsl {

	#if defined(_WIN32)
	#define PLATFORM_WINDOWS
	#elif defined(__APPLE__)
	#define PLATFORM_MAC
	#else
	#define PLATFORM_UNIX
	#endif

	#define NSL_INTERPOLATION_MINIMAL_DATA_COUNT 10

	#define MAX_PACKET_SIZE 5000
	#define MAX_OBJECT_TYPES 255
	#define MAX_PEERS 20					// maximum of clients connected to one server
	#define PACKET_BUFFER_SIZE 50			// how many received values in history should be saved - depends of the speed of server sending, keeping 1-2 seconds is recommanded
											// max seq_max_future_step, older acks are not valid, so their values don't have to be stored
	#define UNDEFINED_BUFFER_INDEX -1

	#define FILENAME_DUMP_SERVER "server_dump_log.csv"
	#define FILENAME_DUMP_CLIENT "client_dump_log.csv"
	#define FILENAME_PACKET_LOG_SERVER "server_packet_log.csv"
	#define FILENAME_PACKET_LOG_CLIENT "client_packet_log.csv"
	#define LOG_PACKETS						// whether all incomming and outcomming packets should be logged (for debug purposes)

	//#define PACKET_LOSS_RATE 30				// percentage of lost packets (simulated for debug purposes)

	#define MSG_TYPE_CUSTOM_MASK 20
	#define MSG_TYPE_OBJECTS 2
	#define MSG_TYPE_ACK 1

	#define FLAG_DIFF 0
	#define FLAG_DELETE 1
	#define FLAG_SNAPSHOT 2
	#define FLAG_CREATE 3
	#define FLAG_CREATE_AND_DELETE 4


	// abstract seq and ack data type
	#define SEQ_MODULO 65535
	#define SEQ_MAX_FUTURE_STEP PACKET_BUFFER_SIZE/2 // size of awaited window
	typedef uint16 seqType


	/* constant correction (just for safety) */
	/*
	#if SEQ_MAX_FUTURE_STEP > SEQ_MODULO
	#undef SEQ_MAX_FUTURE_STEP;
	#define SEQ_MAX_FUTURE_STEP SEQ_MODULO/2
	#endif

	#if PACKET_BUFFER_SIZE > SEQ_MAX_FUTURE_STEP
	#undef PACKET_BUFFER_SIZE;
	#define PACKET_BUFFER_SIZE SEQ_MAX_FUTURE_STEP
	#endif
	*/

	
	struct Message
	{
		byte* data;
		int size;
	};

	struct Peer
	{
		ENetPeer* socket; // TODO: do class and hide socket
		unsigned char lastAck;
		//int id; // id is stored in socket->data, must be converted to int (from void*)
	};

};

#include "ObjectTypeImpl.h"
