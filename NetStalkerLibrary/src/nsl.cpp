// Defines the exported functions for the DLL application.

#include "../stdafx.h"

#define NSL_BUILD_DLL

#include "../include/nsl.h"
#include "common.h"

#ifdef NSL_LOG_PACKETS
	#include <iostream>
	#include <fstream>
	std::ofstream myfile;
namespace nsl {
	void logBytes(byte* data, unsigned int size) {
		myfile.open (NSL_PACKET_LOG_FILENAME, std::ios::app);
		for (unsigned int i = 0; i < size; i++) {
			myfile << (int)data[i] << ' ';
		}
		myfile << "\n";
		myfile.close();
	}	
};
#endif
