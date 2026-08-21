// dsound.h - portable declaration-only stand-in. See d3d11.h for rationale.
#pragma once
#include "Windows.h"
struct IDirectSound8;
struct IDirectSoundBuffer;
struct IDirectSoundBuffer8;
struct IDirectSound3DBuffer8;
struct IDirectSound3DListener8;
typedef IDirectSound8* LPDIRECTSOUND8;
typedef IDirectSoundBuffer* LPDIRECTSOUNDBUFFER;
