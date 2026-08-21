// x3daudio.h - portable declaration-only stand-in for the X3DAudio header.
#pragma once
#include "Windows.h"

// X3DAUDIO_HANDLE is an opaque byte blob in the SDK; the size is what matters
// for members declared by value.
#define X3DAUDIO_HANDLE_BYTESIZE 20
typedef BYTE X3DAUDIO_HANDLE[X3DAUDIO_HANDLE_BYTESIZE];

typedef struct X3DAUDIO_VECTOR { float x, y, z; } X3DAUDIO_VECTOR;

struct X3DAUDIO_LISTENER;
struct X3DAUDIO_EMITTER;
struct X3DAUDIO_DSP_SETTINGS;
