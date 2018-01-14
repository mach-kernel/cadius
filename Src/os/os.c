/**
 * OS specific calls for file manipulations. Generic functionality
 * where API between POSIX and Win32 APIs overlap is to be placed here,
 * otherwise the correct support code will be loaded at build time.
 *
 * Authors:
 * Olivier Zardini, Brutal Deluxe Software, Mar. 2012
 * David Stancu, @mach-kernel, January 2018
 *
 */

#if IS_WINDOWS
#define BUILD_WIN32 1
#include "win32.c"
#else
#define BUILD_POSIX 1
#include "posix.c"
#endif
