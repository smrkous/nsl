// Defines the exported functions for the DLL application.

#include <time.h>
#include "configuration.h"

#ifdef NSL_LOG_PACKETS
	#include <iostream>
	#include <fstream>
	std::ofstream myfile;
namespace nsl {
	void logBytes(byte* data, unsigned int size) {
		if (size > 20) {

			myfile.open (NSL_PACKET_LOG_FILENAME, std::ios::app);
			myfile << size;				// this line was added and the condition as well
			/*myfile << "[ " << size << " ] ";
			for (unsigned int i = 0; i < size; i++) {
				myfile << (int)data[i] << ' ';
			}*/
			myfile << std::endl;
			myfile.close();
		}
	}	

	void logString(const char* data)
	{
		myfile.open (NSL_PACKET_LOG_FILENAME, std::ios::app);
		myfile << data;
		myfile.close();
	}
};
#endif

namespace nsl {
	bool isLittleEndian(void) 
	{
#ifdef NSL_LITTLE_ENDIAN 
		return true;
#else
		return false;
#endif
	}

	double getTime()
	{
#ifdef NSL_PLATFORM_WINDOWS
		return ((double)clock())/CLOCKS_PER_SEC;
#else
		struct timespec ts;
		clock_gettime(CLOCK_MONOTONIC, &ts);
		return double(ts.tv_sec) + double(ts.tv_nsec) * 1e-9;
#endif
	}
};
