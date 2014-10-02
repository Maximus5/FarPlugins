
#pragma once

#ifdef _WIN32_WINNT
#undef _WIN32_WINNT
#endif
#define _WIN32_WINNT 0x0500

#include <windows.h>
#include <stdlib.h>
#include <tchar.h>

#define _ASSERTE(x)
//#include "../../common/plugin.h"
#include "plugin.hpp"
