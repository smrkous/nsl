// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#if defined( __WIN32__ ) || defined( WIN32 ) || defined( _WIN32 )

#include "targetver.h"

#include <stdio.h>
#include <tchar.h>

#else

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#endif


// TODO: reference additional headers your program requires here
