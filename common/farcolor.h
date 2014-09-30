
#pragma once

#undef FAR_LUA

#if defined(_UNICODE) && (FAR_UNICODE>=2800)
	#include "far3l/farcolor.hpp"
	#define FAR_LUA
#elif defined(_UNICODE) && (FAR_UNICODE>=1988)
	#include "far3/farcolor.hpp"
#elif defined(_UNICODE)
	#include "unicode/farcolor.hpp"
	#undef FAR_UNICODE
#else
	#include "ascii/farcolor.hpp"
	#undef FAR_UNICODE
#endif
