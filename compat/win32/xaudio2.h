// xaudio2.h - portable declaration-only stand-in for the XAudio2 header.
// AudioSystem stores XAudio2 interface pointers as members; only the names need
// to exist for those declarations to parse. The interfaces stay incomplete so
// the real XAudio2 backend cannot be compiled off Windows by accident.
#pragma once

#if defined(_WIN32)
#error "compat/win32/xaudio2.h must not be used on Windows - include the SDK header instead."
#endif

#include "Windows.h"

// IUnknown is the root COM interface; several audio members are typed as it.
struct IUnknown;

struct IXAudio2;
struct IXAudio2Voice;
struct IXAudio2SourceVoice;
struct IXAudio2SubmixVoice;
struct IXAudio2MasteringVoice;
struct IXAudio2VoiceCallback;
struct IXAudio2EngineCallback;

typedef struct tWAVEFORMATEX {
    WORD  wFormatTag;
    WORD  nChannels;
    DWORD nSamplesPerSec;
    DWORD nAvgBytesPerSec;
    WORD  nBlockAlign;
    WORD  wBitsPerSample;
    WORD  cbSize;
} WAVEFORMATEX;

typedef struct XAUDIO2_EFFECT_DESCRIPTOR {
    IUnknown* pEffect;
    BOOL      InitialState;
    UINT32    OutputChannels;
} XAUDIO2_EFFECT_DESCRIPTOR;

typedef struct XAUDIO2_EFFECT_CHAIN {
    UINT32                     EffectCount;
    XAUDIO2_EFFECT_DESCRIPTOR* pEffectDescriptors;
} XAUDIO2_EFFECT_CHAIN;
