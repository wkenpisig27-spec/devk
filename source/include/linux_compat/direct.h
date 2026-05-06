// direct.h - Linux compatibility shim for MSVC <direct.h>
// Provides _mkdir, _rmdir, _getcwd, _chdir mappings to POSIX equivalents
#pragma once
#ifndef _DIRECT_H_LINUX_COMPAT
#define _DIRECT_H_LINUX_COMPAT

#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#ifndef _mkdir
#define _mkdir(dir) mkdir(dir, 0755)
#endif
#ifndef _rmdir
#define _rmdir rmdir
#endif
#ifndef _getcwd
#define _getcwd getcwd
#endif
#ifndef _chdir
#define _chdir chdir
#endif

#endif // _DIRECT_H_LINUX_COMPAT
