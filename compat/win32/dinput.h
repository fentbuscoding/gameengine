// dinput.h - portable declaration-only stand-in. See d3d11.h for rationale.
#pragma once
#include "Windows.h"
struct IDirectInput8;
struct IDirectInputDevice8;
typedef IDirectInput8* LPDIRECTINPUT8;
typedef IDirectInputDevice8* LPDIRECTINPUTDEVICE8;

// DirectInput mouse state, stored by value in InputManager. Layout matches the
// SDK so the member accesses in shared code compile identically.
typedef struct _DIMOUSESTATE {
    LONG lX;
    LONG lY;
    LONG lZ;
    BYTE rgbButtons[4];
} DIMOUSESTATE;

typedef struct _DIMOUSESTATE2 {
    LONG lX;
    LONG lY;
    LONG lZ;
    BYTE rgbButtons[8];
} DIMOUSESTATE2;
