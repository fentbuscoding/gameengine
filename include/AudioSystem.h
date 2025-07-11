#pragma once

#include "Platform.h"
#include <vector>
#include <memory>
#include <map>
#include <string>
#include <functional>
#include <queue>
#include <thread>
#include <mutex>
#include <atomic>

#ifdef _WIN32
    #include <dsound.h>
    #include <xaudio2.h>
    #include <x3daudio.h>
    #include <xapofx.h>
#endif

// Use DirectX 11 by default
#ifndef NEXUS_USE_DIRECTX11
#define NEXUS_USE_DIRECTX11
#endif

#ifdef NEXUS_USE_DIRECTX11
    #include <DirectXMath.h>
    using namespace DirectX;
#else
    #include <d3dx9.h>
#endif

namespace Nexus {

class Camera;

/**
 * Advanced audio system with 3D spatial audio, effects, and streaming
 */
class AudioSystem {
public:
    enum class AudioFormat {
        PCM_8,
        PCM_16,
        PCM_24,
        PCM_32,
        Float32,
        Compressed_MP3,
        Compressed_OGG,
        Compressed_AAC
    };

    enum class AudioChannelLayout {
        Mono,
        Stereo,
        Surround_5_1,
        Surround_7_1,
        Surround_7_1_4  // Dolby Atmos
    };

    enum class AudioEffectType {
        Reverb,
        Echo,
        Distortion,
        Compression,
        EQ,
        Filter,
        Chorus,
        Flanger,
        Phaser,
        Delay,
        Limiter,
        Normalize,
        Custom
    };

    enum class AudioSourceType {
        Static,      // Loaded entirely in memory
        Streaming,   // Streamed from disk
        Generated    // Procedurally generated
    };

    enum class AudioPriority {
        Low,
        Medium,
        High,
        Critical
    };

    struct AudioBuffer {
        std::string name;
        AudioFormat format;
        int sampleRate;
        int channels;
        int bitsPerSample;
        size_t dataSize;
        std::vector<uint8_t> data;
        float duration;
        
        // Compression info
        bool isCompressed;
        size_t originalSize;
        float compressionRatio;
        
        // Loop points
        bool hasLoopPoints;
        size_t loopStart;
        size_t loopEnd;
        
        // Metadata
        std::map<std::string, std::string> metadata;
    };

    struct AudioSource {
        std::string name;
        std::shared_ptr<AudioBuffer> buffer;
        AudioSourceType type;
        AudioPriority priority;
        
        // Playback properties
        float volume;
        float pitch;
        float pan;
        bool isLooping;
        bool isPlaying;
        bool isPaused;
        bool is3D;
        
        // 3D properties
        XMFLOAT3 position;
        XMFLOAT3 velocity;
        XMFLOAT3 direction;
        float minDistance;
        float maxDistance;
        float rolloffFactor;
        float dopplerFactor;
        float coneInnerAngle;
        float coneOuterAngle;
        float coneOuterGain;
        
        // Streaming properties
        size_t streamPosition;
        size_t streamBufferSize;
        std::vector<uint8_t> streamBuffer;
        std::ifstream streamFile;
        
        // Effects chain
        std::vector<std::shared_ptr<class AudioEffect>> effects;
        
        // Callbacks
        std::function<void()> onPlaybackComplete;
        std::function<void(float)> onPositionChanged;
        std::function<void()> onLoopPoint;
        
        // Runtime data
        IXAudio2SourceVoice* sourceVoice;
        size_t playbackPosition;
        float fadeVolume;
        float fadeSpeed;
        bool isFading;
        
        // Statistics
        int playCount;
        float totalPlayTime;
        float lastPlayTime;
    };

    struct AudioListener {
        XMFLOAT3 position;
        XMFLOAT3 velocity;
        XMFLOAT3 forward;
        XMFLOAT3 up;
        float masterVolume;
        
        // Environment properties
        float environmentSize;
        float environmentDiffusion;
        float environmentReflection;
        
        // Occlusion/Obstruction
        float occlusionFactor;
        float obstructionFactor;
    };

    struct AudioEffect {
        AudioEffectType type;
        std::string name;
        bool isEnabled;
        
        // Common parameters
        float intensity;
        float wetDryMix;
        
        // Effect-specific parameters
        std::map<std::string, float> parameters;
        
        // XAudio2 effect
        IUnknown* xaudioEffect;
        
        virtual void Apply(float* samples, int sampleCount, int channels) = 0;
        virtual void SetParameter(const std::string& name, float value) = 0;
        virtual float GetParameter(const std::string& name) const = 0;
    };

    struct ReverbEffect : public AudioEffect {
        float roomSize;
        float damping;
        float earlyReflections;
        float lateReflections;
        float diffusion;
        float density;
        
        // Implementation state
        std::vector<float> delayBuffer;
        size_t delayIndex = 0;
        
        void Apply(float* samples, int sampleCount, int channels) override;
        void SetParameter(const std::string& name, float value) override;
        float GetParameter(const std::string& name) const override;
    };

    struct EQEffect : public AudioEffect {
        struct Band {
            float frequency;
            float gain;
            float Q;
        };
        
        std::vector<Band> bands;
        
        // Implementation state
        std::vector<float> filterStates;
        
        void Apply(float* samples, int sampleCount, int channels) override;
        void SetParameter(const std::string& name, float value) override;
        float GetParameter(const std::string& name) const override;
    };

    struct CompressorEffect : public AudioEffect {
        float threshold;
        float ratio;
        float attack;
        float release;
        float makeupGain;
        
        // Implementation state
        float envelope = 0.0f;
        
        void Apply(float* samples, int sampleCount, int channels) override;
        void SetParameter(const std::string& name, float value) override;
        float GetParameter(const std::string& name) const override;
    };

    struct AudioGroup {
        std::string name;
        float volume;
        float pitch;
        bool isMuted;
        bool isSolo;
        
        // Child groups (for hierarchical mixing)
        std::vector<std::shared_ptr<AudioGroup>> childGroups;
        std::shared_ptr<AudioGroup> parentGroup;
        
        // Sources in this group
        std::vector<std::shared_ptr<AudioSource>> sources;
        
        // Group effects
        std::vector<std::shared_ptr<AudioEffect>> effects;
        
        // Ducking support
        bool isDucking;
        float duckingThreshold;
        float duckingRatio;
        float duckingAttack;
        float duckingRelease;
        
        float GetFinalVolume() const;
        void AddSource(std::shared_ptr<AudioSource> source);
        void RemoveSource(std::shared_ptr<AudioSource> source);
    };

    struct AudioEnvironment {
        std::string name;
        
        // Room parameters
        float roomSize;
        float roomHF;
        float roomRolloffFactor;
        float decayTime;
        float decayHFRatio;
        float reflections;
        float reflectionsDelay;
        float reverb;
        float reverbDelay;
        float diffusion;
        float density;
        float hfReference;
        
        // Environment effects
        std::vector<std::shared_ptr<AudioEffect>> globalEffects;
        
        // Occlusion/Obstruction materials
        std::map<std::string, float> materialProperties;
    };

    struct AudioOcclusion {
        // Raycast-based occlusion
        bool enableRaycastOcclusion;
        int raycastSamples;
        float raycastMaxDistance;
        
        // Material-based occlusion
        std::map<std::string, float> materialAbsorption;
        std::map<std::string, float> materialTransmission;
        
        // Compute occlusion factor
        float ComputeOcclusion(const XMFLOAT3& listenerPos, 
                              const XMFLOAT3& sourcePos,
                              const std::vector<XMFLOAT3>& obstacles);
    };

    struct AudioStreaming {
        // Streaming settings
        size_t bufferSize;
        int numBuffers;
        int prebufferCount;
        
        // Streaming thread
        std::thread streamingThread;
        std::mutex streamingMutex;
        std::condition_variable streamingCondition;
        std::atomic<bool> isStreamingActive;
        
        // Buffer management
        std::queue<std::vector<uint8_t>> freeBuffers;
        std::queue<std::vector<uint8_t>> filledBuffers;
        
        void StreamingThreadFunc();
        void FillBuffer(std::shared_ptr<AudioSource> source);
    };

    struct AudioMixer {
        // Mixing parameters
        int sampleRate;
        int channels;
        int bufferSize;
        AudioChannelLayout channelLayout;
        
        // Master bus
        float masterVolume;
        float masterPitch;
        bool masterMute;
        
        // Submixes
        std::map<std::string, std::shared_ptr<AudioGroup>> groups;
        
        // Final output buffer
        std::vector<float> mixBuffer;
        
        // Peak/RMS metering
        std::vector<float> peakLevels;
        std::vector<float> rmsLevels;
        
        void MixSources(const std::vector<std::shared_ptr<AudioSource>>& sources);
        void ApplyGroupEffects(std::shared_ptr<AudioGroup> group);
        void ApplyMasterEffects();
    };

    // Dynamic range compression for different platforms
    struct DynamicRangeCompression {
        bool enableDRC;
        float targetLUFS;
        float maxPeakLevel;
        float compressionRatio;
        float attack;
        float release;
        
        void ProcessAudio(float* samples, int sampleCount, int channels);
    };

    // Audio analytics and profiling
    struct AudioAnalytics {
        // Performance metrics
        float cpuUsage;
        float memoryUsage;
        int activeVoices;
        int totalVoices;
        
        // Quality metrics
        float averageLatency;
        float peakLatency;
        int underruns;
        int overruns;
        
        // Usage statistics
        std::map<std::string, int> mostPlayedSounds;
        std::map<std::string, float> soundPlayTime;
        
        void Update();
        void LogStatistics();
    };

public:
    AudioSystem();
    ~AudioSystem();

    // Initialization
    bool Initialize(int sampleRate = 44100, int channels = 2, 
                   int bufferSize = 1024, AudioChannelLayout layout = AudioChannelLayout::Stereo);
    void Shutdown();

    // Audio buffer management
    std::shared_ptr<AudioBuffer> LoadAudioFile(const std::string& filePath);
    std::shared_ptr<AudioBuffer> CreateAudioBuffer(const std::string& name, 
                                                  AudioFormat format, int sampleRate, 
                                                  int channels, const void* data, size_t dataSize);
    void UnloadAudioBuffer(const std::string& name);
    std::shared_ptr<AudioBuffer> GetAudioBuffer(const std::string& name);

    // Audio source management
    std::shared_ptr<AudioSource> CreateAudioSource(const std::string& name, 
                                                   std::shared_ptr<AudioBuffer> buffer,
                                                   AudioSourceType type = AudioSourceType::Static);
    void DestroyAudioSource(const std::string& name);
    std::shared_ptr<AudioSource> GetAudioSource(const std::string& name);

    // Playback control
    void PlaySound(const std::string& sourceName);
    void PauseSound(const std::string& sourceName);
    void StopSound(const std::string& sourceName);
    void StopAllSounds();
    void SetVolume(const std::string& sourceName, float volume);
    void SetPitch(const std::string& sourceName, float pitch);
    void SetLooping(const std::string& sourceName, bool looping);

    // 3D audio
    void SetSource3D(const std::string& sourceName, bool is3D);
    void SetSourcePosition(const std::string& sourceName, const D3DXVECTOR3& position);
    void SetSourceVelocity(const std::string& sourceName, const D3DXVECTOR3& velocity);
    void SetSourceDirection(const std::string& sourceName, const D3DXVECTOR3& direction);
    void SetSourceDistance(const std::string& sourceName, float minDistance, float maxDistance);
    void SetSourceCone(const std::string& sourceName, float innerAngle, float outerAngle, float outerGain);

    // Listener
    void SetListenerPosition(const D3DXVECTOR3& position);
    void SetListenerVelocity(const D3DXVECTOR3& velocity);
    void SetListenerOrientation(const D3DXVECTOR3& forward, const D3DXVECTOR3& up);
    void SetMasterVolume(float volume);

    // Audio groups
    std::shared_ptr<AudioGroup> CreateAudioGroup(const std::string& name);
    void DestroyAudioGroup(const std::string& name);
    void AddSourceToGroup(const std::string& sourceName, const std::string& groupName);
    void RemoveSourceFromGroup(const std::string& sourceName, const std::string& groupName);
    void SetGroupVolume(const std::string& groupName, float volume);
    void SetGroupMute(const std::string& groupName, bool mute);

    // Audio effects
    std::shared_ptr<AudioEffect> CreateEffect(AudioEffectType type, const std::string& name);
    void AddEffectToSource(const std::string& sourceName, const std::string& effectName);
    void RemoveEffectFromSource(const std::string& sourceName, const std::string& effectName);
    void AddEffectToGroup(const std::string& groupName, const std::string& effectName);
    void SetEffectParameter(const std::string& effectName, const std::string& parameter, float value);

    // Environment and occlusion
    void SetAudioEnvironment(const std::string& environmentName);
    void EnableOcclusion(bool enable);
    void SetOcclusionParameters(int raycastSamples, float maxDistance);

    // Streaming
    void EnableStreaming(bool enable);
    void SetStreamingBufferSize(size_t bufferSize);
    void SetStreamingBufferCount(int count);

    // Audio compression and limiting
    void EnableDynamicRangeCompression(bool enable);
    void SetDRCParameters(float targetLUFS, float maxPeak, float ratio);

    // Fading
    void FadeIn(const std::string& sourceName, float duration);
    void FadeOut(const std::string& sourceName, float duration);
    void CrossFade(const std::string& sourceA, const std::string& sourceB, float duration);

    // Audio analysis
    float GetSourceVolume(const std::string& sourceName) const;
    float GetSourcePitch(const std::string& sourceName) const;
    bool IsSourcePlaying(const std::string& sourceName) const;
    float GetSourcePlaybackPosition(const std::string& sourceName) const;
    void GetAudioLevels(std::vector<float>& peakLevels, std::vector<float>& rmsLevels) const;

    // Update and processing
    void Update(float deltaTime);
    void ProcessAudio(float* outputBuffer, int sampleCount);

    // Performance and optimization
    void SetMaxVoices(int maxVoices);
    void SetVoicePriority(const std::string& sourceName, AudioPriority priority);
    void EnableVoiceVirtualization(bool enable);

    // Statistics and debugging
    void GetStatistics(AudioAnalytics& analytics) const;
    void EnableProfiling(bool enable);
    void DumpAudioGraph(const std::string& filePath) const;

    // Save/Load audio configurations
    bool SaveAudioConfig(const std::string& filePath) const;
    bool LoadAudioConfig(const std::string& filePath);

    // Audio file format support
    bool RegisterAudioDecoder(const std::string& extension, 
                             std::function<std::shared_ptr<AudioBuffer>(const std::string&)> decoder);
    bool RegisterAudioEncoder(const std::string& extension,
                             std::function<bool(const std::string&, std::shared_ptr<AudioBuffer>)> encoder);

    // Legacy compatibility type definitions
    using SoundID = int;
    using SoundInstanceID = int;

    // Legacy compatibility methods (for old implementation)
    SoundID LoadSound(const std::string& filePath);
    void UnloadSound(SoundID soundId);
    SoundInstanceID PlaySound(SoundID soundId, float volume = 1.0f, bool looping = false);
    SoundInstanceID PlaySound3D(SoundID soundId, const DirectX::XMFLOAT3& position, 
                                const DirectX::XMFLOAT3& velocity = DirectX::XMFLOAT3(0,0,0), 
                                float volume = 1.0f, bool looping = false);
    void StopSound(SoundInstanceID instanceId);
    void SetSoundVolume(SoundInstanceID instanceId, float volume);
    void SetSoundPosition(SoundInstanceID instanceId, const DirectX::XMFLOAT3& position);
    void SetSoundVelocity(SoundInstanceID instanceId, const DirectX::XMFLOAT3& velocity);
    void ApplyDistortionEffect(SoundInstanceID instanceId, float amount, float edge);
    void ApplyEcho(SoundInstanceID instanceId, float delay, float feedback);
    void StartStreaming(const std::string& filePath, float volume = 1.0f, bool looping = false);
    void StopStreaming(const std::string& filePath);

private:
    // XAudio2 implementation
    void InitializeXAudio2();
    void ShutdownXAudio2();
    void CreateMasteringVoice();
    void CreateSourceVoice(std::shared_ptr<AudioSource> source);
    void DestroySourceVoice(std::shared_ptr<AudioSource> source);

    // Audio processing
    void ProcessFading(std::shared_ptr<AudioSource> source, float deltaTime);
    void Process3DAudio();
    void ProcessOcclusion();
    void ProcessEffects();

    // Streaming implementation
    void StartStreaming();
    void StopStreaming();
    void UpdateStreamingSources();

    // Voice management
    void VirtualizeVoices();
    void RealizeVoices();
    std::shared_ptr<AudioSource> FindLowestPrioritySource() const;

    // Audio format conversion
    void ConvertAudioFormat(const AudioBuffer& source, AudioBuffer& dest, AudioFormat targetFormat);
    void ResampleAudio(const AudioBuffer& source, AudioBuffer& dest, int targetSampleRate);

    // File I/O
    std::shared_ptr<AudioBuffer> LoadWAV(const std::string& filePath);
    std::shared_ptr<AudioBuffer> LoadMP3(const std::string& filePath);
    std::shared_ptr<AudioBuffer> LoadOGG(const std::string& filePath);

    // Effects implementation
    void ApplyReverb(float* samples, int sampleCount, int channels, const ReverbEffect& effect);
    void ApplyEQ(float* samples, int sampleCount, int channels, const EQEffect& effect);
    void ApplyCompressor(float* samples, int sampleCount, int channels, const CompressorEffect& effect);

    // Utility functions
    float LinearToDecibel(float linear) const;
    float DecibelToLinear(float decibel) const;
    float CalculateDistance(const XMFLOAT3& pos1, const XMFLOAT3& pos2) const;
    float CalculateAttenuation(float distance, float minDist, float maxDist, float rolloff) const;
    float CalculateDopplerShift(const XMFLOAT3& sourceVel, const XMFLOAT3& listenerVel,
                               const XMFLOAT3& direction) const;

private:
    // XAudio2 objects
    IXAudio2* xaudio2_;
    IXAudio2* xAudio2_; // Compatibility alias
    IXAudio2MasteringVoice* masteringVoice_;
    IXAudio2MasteringVoice* masterVoice_; // Compatibility alias
    X3DAUDIO_HANDLE x3dAudioHandle_;
    
    // Legacy compatibility members (from old implementation)
    int nextSoundId_;
    int nextSoundInstanceId_;
    float masterVolume_;
    XMFLOAT3 listenerPosition_;
    XMFLOAT3 listenerVelocity_;
    XMFLOAT3 listenerOrientation_;
    XMFLOAT3 listenerUpVector_;
    bool dopplerEnabled_;
    bool reverbEnabled_;
    std::map<int, std::string> sounds_; // Legacy sound storage
    std::map<int, std::string> soundInstances_; // Legacy instance storage
    std::vector<std::string> streamingSounds_; // Legacy streaming storage
    
    // Audio data
    std::map<std::string, std::shared_ptr<AudioBuffer>> audioBuffers_;
    std::map<std::string, std::shared_ptr<AudioSource>> audioSources_;
    std::map<std::string, std::shared_ptr<AudioGroup>> audioGroups_;
    std::map<std::string, std::shared_ptr<AudioEffect>> audioEffects_;
    std::map<std::string, std::shared_ptr<AudioEnvironment>> audioEnvironments_;
    
    // Listener
    std::unique_ptr<AudioListener> listener_;
    
    // Audio mixer
    std::unique_ptr<AudioMixer> mixer_;
    
    // Streaming
    std::unique_ptr<AudioStreaming> streaming_;
    
    // Occlusion
    std::unique_ptr<AudioOcclusion> occlusion_;
    
    // Dynamic range compression
    std::unique_ptr<DynamicRangeCompression> drc_;
    
    // Analytics
    std::unique_ptr<AudioAnalytics> analytics_;
    
    // Settings
    int sampleRate_;
    int channels_;
    int bufferSize_;
    AudioChannelLayout channelLayout_;
    
    // Voice management
    int maxVoices_;
    bool voiceVirtualizationEnabled_;
    
    // Performance settings
    bool streamingEnabled_;
    bool occlusionEnabled_;
    bool profilingEnabled_;
    
    // File format decoders/encoders
    std::map<std::string, std::function<std::shared_ptr<AudioBuffer>(const std::string&)>> decoders_;
    std::map<std::string, std::function<bool(const std::string&, std::shared_ptr<AudioBuffer>)>> encoders_;
    
    // Threading
    std::mutex audioMutex_;
    std::thread audioThread_;
    std::atomic<bool> isRunning_;
    
    // Current environment
    std::string currentEnvironment_;
    
    // Performance counters
    mutable float cpuUsage_;
    mutable float memoryUsage_;
    mutable int activeVoices_;
};

} // namespace Nexus
