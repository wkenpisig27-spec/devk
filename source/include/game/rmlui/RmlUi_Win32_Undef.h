#pragma once

// WindowsX.h (pulled in by game Stdafx) defines window-navigation macros that collide
// with RmlUi Element method names. Undef them before including any RmlUi headers.
#ifdef GetNextSibling
#undef GetNextSibling
#endif
#ifdef GetPreviousSibling
#undef GetPreviousSibling
#endif
#ifdef GetFirstChild
#undef GetFirstChild
#endif
#ifdef GetLastChild
#undef GetLastChild
#endif
#ifdef GetNextWindow
// keep GetNextWindow — RmlUi does not use this name
#endif
#ifdef Yield
#undef Yield
#endif
#ifdef GetObject
#undef GetObject
#endif
#ifdef CreateWindow
#undef CreateWindow
#endif
#ifdef GetMessage
#undef GetMessage
#endif
#ifdef SendMessage
#undef SendMessage
#endif
#ifdef DispatchMessage
#undef DispatchMessage
#endif
#ifdef Focus
#undef Focus
#endif
#ifdef near
#undef near
#endif
#ifdef far
#undef far
#endif
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif
