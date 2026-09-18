#pragma once

// Thin IUnknown resource interfaces for the DX11-only play path.
// Named lwDx11I* so they do not collide with MindPower::lwIVertexBuffer /
// lwIIndexBuffer (engine stream objects that inherit lwInterface).
// Dual-build Debug still inherits the real D3D9 interfaces.

#include "MindPowerRenderConfig.h"

#if !MINDPOWER_USE_D3D9_DEVICE

#include <unknwn.h>

struct IDirect3DDevice9;

struct lwDx11IResource : public IUnknown
{
    STDMETHOD(GetDevice)(IDirect3DDevice9** ppDevice) PURE;
    STDMETHOD_(D3DRESOURCETYPE, GetType)() PURE;
};

struct lwDx11IBaseTexture : public lwDx11IResource
{
    STDMETHOD_(DWORD, SetLOD)(DWORD LODNew) PURE;
    STDMETHOD_(DWORD, GetLOD)() PURE;
    STDMETHOD_(DWORD, GetLevelCount)() PURE;
};

struct lwDx11ISurface : public lwDx11IResource
{
    STDMETHOD(GetDesc)(D3DSURFACE_DESC* pDesc) PURE;
    STDMETHOD(LockRect)(D3DLOCKED_RECT* pLockedRect, const RECT* pRect, DWORD Flags) PURE;
    STDMETHOD(UnlockRect)() PURE;
};

struct lwDx11ITexture : public lwDx11IBaseTexture
{
    STDMETHOD(GetLevelDesc)(UINT Level, D3DSURFACE_DESC* pDesc) PURE;
    STDMETHOD(GetSurfaceLevel)(UINT Level, lwDx11ISurface** ppSurfaceLevel) PURE;
    STDMETHOD(LockRect)(UINT Level, D3DLOCKED_RECT* pLockedRect, const RECT* pRect, DWORD Flags) PURE;
    STDMETHOD(UnlockRect)(UINT Level) PURE;
    STDMETHOD(AddDirtyRect)(const RECT* pDirtyRect) PURE;
};

struct lwDx11IVertexBuffer : public lwDx11IResource
{
    STDMETHOD(Lock)(UINT OffsetToLock, UINT SizeToLock, void** ppbData, DWORD Flags) PURE;
    STDMETHOD(Unlock)() PURE;
    STDMETHOD(GetDesc)(D3DVERTEXBUFFER_DESC* pDesc) PURE;
};

struct lwDx11IIndexBuffer : public lwDx11IResource
{
    STDMETHOD(Lock)(UINT OffsetToLock, UINT SizeToLock, void** ppbData, DWORD Flags) PURE;
    STDMETHOD(Unlock)() PURE;
    STDMETHOD(GetDesc)(D3DINDEXBUFFER_DESC* pDesc) PURE;
};

struct lwDx11IVertexShader : public IUnknown
{
    STDMETHOD(GetDevice)(IDirect3DDevice9** ppDevice) PURE;
    STDMETHOD(GetFunction)(void* pData, UINT* pSizeOfData) PURE;
};

struct lwDx11IVertexDecl : public IUnknown
{
    STDMETHOD(GetDevice)(IDirect3DDevice9** ppDevice) PURE;
    STDMETHOD(GetDeclaration)(D3DVERTEXELEMENT9* pData, UINT* pNumElements) PURE;
};

#endif
