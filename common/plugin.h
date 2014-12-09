
#pragma once

#undef FAR_LUA

#if defined(_UNICODE) && (FAR_UNICODE>=2800)
	#include "far3l/pluginW3l.hpp"
	#define FAR_LUA
#elif defined(_UNICODE) && (FAR_UNICODE>=1988)
	#include "far3/pluginW3.hpp"
#elif defined(_UNICODE)
	#include "unicode/pluginW.hpp"
	#undef FAR_UNICODE
#else
	#include "ascii/pluginA.hpp"
	#undef FAR_UNICODE
#endif

#define MAKEFARVERSION2(major,minor,build) ( ((major)<<8) | (minor) | ((build)<<16))
