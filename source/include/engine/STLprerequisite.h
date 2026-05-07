#pragma once

#include <list>
#include <string>
#include <limits>
#include <map>
#include <cassert>
#include <fstream>
#include <exception>
#include <sstream>

// When not building as a DLL, export/import decorators are not needed.
#ifdef MINDPOWER_USE_DLL
#   ifdef CLIENT_BUILD
#       define _FONT_Export __declspec( dllimport )
#   else
#       define _FONT_Export __declspec( dllexport )
#   endif
#else
#   define _FONT_Export
#endif


// disable: "<type> needs to have dll-interface to be used by clients'
// Happens on STL member variables which are not public therefore is ok
#   pragma warning (disable : 4251)

// disable: "no suitable definition provided for explicit template
// instantiation request" Occurs in VC7 for no justifiable reason on all
// #includes of Singleton
#   pragma warning( disable: 4661)
