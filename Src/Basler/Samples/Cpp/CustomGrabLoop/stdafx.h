// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#if defined (_MSC_VER) && defined (_WIN32)
#   define WIN32_LEAN_AND_MEAN        // Exclude rarely-used stuff from Windows headers
#   define _CRT_SECURE_NO_WARNINGS
#   include <windows.h>
#elif defined(__GNUC__) && defined(__linux__)

#else
#   error Unsupported platform
#endif

#include <ConsumerImplHelper/ToFCamera.h>
#include <iostream>
#include <iomanip>

