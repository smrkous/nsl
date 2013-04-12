// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#if defined( __WIN32__ ) || defined( WIN32 ) || defined( _WIN32 )

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files:
#include <windows.h>

#else

#include <stdlib.h>
#include <string.h>

#endif

