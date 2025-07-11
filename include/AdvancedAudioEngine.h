#pragma once

#include <xaudio2.h>
#include <x3daudio.h>
#include <xapofx.h>
#include <DirectXMath.h>
#include <memory>
#include <vector>
#include <map>
#include <string>
#include <queue>
#include <functional>
#include <atomic>
#include <thread>
#include <mutex>

using namespace DirectX;

namespace Nexus {

class Camera;

/**
 * Advanced Audio Engine with 3D spatial audio, real-time effects, and console support
 * Supports multiple audio formats, streaming, procedural audio, and HRTF
 */
class AdvancedAudioEngine {
public:
    enum class AudioFormat {
        PCM_16,
        PCM_24,
        PCM_32,
        IEEE_FLOAT,
        ADPCM,
        XMA2,      // Xbox
        AT9,       // PlayStation
        OPUS,      // General compression
        OGG_VORBIS,
        MP3,
        AAC
    };

    enum class AudioQuality {
        Low = 22050,
        Medium = 44100,
        High = 48000,
        Ultra = 96000,
        Extreme = 192000
    };

    enum class ReverbType {
        None,
        Room,
        Hall,
        Cathedral,
        Cave,
        Forest,
        Underwater,
        Space,
        Custom
    };

    enum class DistanceModel {
        Linear,
        Exponential,
        InverseSquare,
        Custom
    };

    enum class HRTFQuality {
        Low,
        Medium,
        High,
        Personalized
    };

    struct AudioSettings {
        AudioQuality sampleRate = AudioQuality::High;
        int channels = 8; // Support up to 7.1 surround
        int bitDepth = 32;
        int bufferSize = 1024;
        
        bool enableSpatialAudio = true;
        bool enableHRTF = true;
        bool enableReverb = true;
        bool enableOcclusion = true;
        bool enableDoppler = true;
        bool enableCompression = true;
        
        HRTFQuality hrtfQuality = HRTFQuality::High;
        float masterVolume = 1.0f;
        float musicVolume = 0.8f;
        float sfxVolume = 1.0f;
        float voiceVolume = 1.0f;
        float ambientVolume = 0.6f;
        
        // Platform-specific
        bool enableXboxAudioOffload = true;
        bool enablePlayStationAudio3D = true;
        bool enableNintendoSwitchAudio = true;
        
        // Advanced features
        bool enableRealTimeConvolution = false;
        bool enableMIDISupport = false;
        bool enableProceduralAudio = false;
        bool enableVoiceProcessing = false;
        bool enableAudioML = false; // Machine learning audio enhancement
    };

    struct AudioClip {
        std::string name;
        AudioFormat format;
        int sampleRate;
        int channels;
        int bitDepth;
        float duration;
        bool looping;
        bool streaming;
        
        std::vector<uint8_t> audioData;
        size_t dataSize;
        
        // Compression
        bool compressed;
        float compressionRatio;
        
        // 3D properties
        float minDistance;
        float maxDistance;
        DistanceModel distanceModel;
        float rolloffFactor;
        
        // Effects
        bool hasLowPassFilter;
        bool hasHighPassFilter;
        float filterCutoff;
        
        // Metadata
        std::map<std::string, std::string> metadata;
    };

    struct AudioSource {
        int sourceId;
        std::string clipName;
        XMFLOAT3 position;
        XMFLOAT3 velocity;
        XMFLOAT3 direction;
        
        float volume;
        float pitch;
        float pan;
        bool muted;
        bool paused;
        bool is3D;
        
        // 3D audio properties
        float innerConeAngle;
        float outerConeAngle;
        float outerConeVolume;
        float dopplerFactor;
        
        // Distance attenuation
        float minDistance;
        float maxDistance;
        DistanceModel distanceModel;
        float rolloffFactor;
        
        // Occlusion/Obstruction
        float occlusionFactor;
        float obstructionFactor;
        
        // Effects chain
        std::vector<int> effectIds;
        
        // Streaming
        bool isStreaming;
        size_t streamPosition;
        std::queue<std::vector<uint8_t>> streamBuffers;
        
        // Voice processing
        IXAudio2SourceVoice* sourceVoice;
        bool voiceActive;
    };

    struct AudioListener {
        XMFLOAT3 position;
        XMFLOAT3 velocity;
        XMFLOAT3 forward;
        XMFLOAT3 up;
        
        // HRTF personalization
        float headRadius;
        float interauralDelay;
        std::vector<float> hrtfProfile;
    };

    struct AudioEffect {
        enum class Type {
            Reverb,
            Echo,
            Delay,
            Chorus,
            Flanger,
            Distortion,
            Compression,
            EQ,
            LowPass,
            HighPass,
            BandPass,
            Convolution,
            Custom
        };
        
        Type type;
        bool enabled;
        float wetDryMix;
        std::map<std::string, float> parameters;
        
        // XAPO effect
        IUnknown* xapoEffect;
        XAUDIO2_EFFECT_DESCRIPTOR descriptor;
    };

    struct ReverbSettings {
        ReverbType type;
        float roomSize;
        float damping;
        float wetLevel;
        float dryLevel;
        float decay;
        float density;
        float diffusion;
        
        // Advanced reverb parameters
        float earlyReflections;
        float lateReverberation;
        float reverbDelay;
        XMFLOAT3 roomDimensions;
    };

    struct ProceduralAudioParams {
        enum class GeneratorType {
            Sine,
            Square,
            Sawtooth,
            Triangle,
            Noise,
            Custom
        };
        
        GeneratorType type;
        float frequency;
        float amplitude;
        float phase;
        
        // Modulation
        bool frequencyModulation;
        bool amplitudeModulation;
        float modFrequency;
        float modDepth;
        
        // Envelope
        float attack;
        float decay;
        float sustain;
        float release;
    };

    // Event system
    struct AudioEvent {
        enum class Type {
            PlaybackStarted,
            PlaybackStopped,
            PlaybackPaused,
            PlaybackResumed,
            EffectApplied,
            StreamingBufferLow,
            AudioDeviceChanged
        };
        
        Type type;
        int sourceId;
        std::string clipName;
        float timestamp;
        std::map<std::string, std::string> data;
    };

    using AudioEventCallback = std::function<void(const AudioEvent&)>;

public:
    AdvancedAudioEngine();
    ~AdvancedAudioEngine();

    // Initialization
    bool Initialize(const AudioSettings& settings = AudioSettings{});
    void Shutdown();
    void Reset();

    // Settings
    void SetAudioSettings(const AudioSettings& settings);
    const AudioSettings& GetAudioSettings() const { return settings_; }

    // Audio clip management
    bool LoadAudioClip(const std::string& filename, const std::string& clipName);
    bool LoadAudioClipFromMemory(const uint8_t* data, size_t size, const std::string& clipName, AudioFormat format);
    bool CreateProceduralClip(const std::string& clipName, const ProceduralAudioParams& params, float duration);
    void UnloadAudioClip(const std::string& clipName);
    AudioClip* GetAudioClip(const std::string& clipName);

    // Audio source management
    int CreateAudioSource(const std::string& clipName, bool is3D = false);
    void DestroyAudioSource(int sourceId);
    AudioSource* GetAudioSource(int sourceId);
    
    // Playback control
    void Play(int sourceId);
    void Stop(int sourceId);
    void Pause(int sourceId);
    void Resume(int sourceId);
    void SetLoop(int sourceId, bool loop);
    void SetVolume(int sourceId, float volume);
    void SetPitch(int sourceId, float pitch);
    void SetPan(int sourceId, float pan);
    
    // 3D audio
    void SetSourcePosition(int sourceId, const XMFLOAT3& position);
    void SetSourceVelocity(int sourceId, const XMFLOAT3& velocity);
    void SetSourceDirection(int sourceId, const XMFLOAT3& direction);
    void SetSourceCone(int sourceId, float innerAngle, float outerAngle, float outerVolume);
    void SetSourceDistanceAttenuation(int sourceId, float minDistance, float maxDistance, DistanceModel model);
    void SetSourceOcclusion(int sourceId, float occlusionFactor);
    void SetSourceObstruction(int sourceId, float obstructionFactor);
    
    // Listener
    void SetListenerPosition(const XMFLOAT3& position);
    void SetListenerVelocity(const XMFLOAT3& velocity);
    void SetListenerOrientation(const XMFLOAT3& forward, const XMFLOAT3& up);
    void SetListenerFromCamera(Camera* camera);
    void PersonalizeHRTF(const std::vector<float>& hrtfProfile);
    
    // Effects
    int CreateEffect(AudioEffect::Type type);
    void DestroyEffect(int effectId);
    void AttachEffectToSource(int sourceId, int effectId);
    void DetachEffectFromSource(int sourceId, int effectId);
    void SetEffectParameter(int effectId, const std::string& parameter, float value);
    void EnableEffect(int effectId, bool enable);
    
    // Reverb
    void SetGlobalReverb(ReverbType type);
    void SetReverbSettings(const ReverbSettings& settings);
    void SetReverbZone(const XMFLOAT3& center, float radius, ReverbType type);
    
    // Streaming
    void EnableStreaming(int sourceId, bool enable);
    void QueueStreamingBuffer(int sourceId, const std::vector<uint8_t>& buffer);
    void SetStreamingBufferSize(int sourceId, size_t bufferSize);
    bool IsStreamingBufferLow(int sourceId);
    
    // Music system
    void PlayMusic(const std::string& clipName, float fadeInTime = 0.0f);
    void StopMusic(float fadeOutTime = 0.0f);
    void CrossfadeMusic(const std::string& newClipName, float fadeTime);
    void SetMusicVolume(float volume);
    void PauseMusic();
    void ResumeMusic();
    
    // Voice processing
    void EnableVoiceChat(bool enable);
    void StartVoiceRecording();
    void StopVoiceRecording();
    void ProcessVoiceInput(const std::vector<float>& inputBuffer);
    void SetVoiceNoiseReduction(bool enable);
    void SetVoiceEchoCancellation(bool enable);
    
    // Procedural audio
    void EnableProceduralAudio(bool enable);
    void SetProceduralParameters(int sourceId, const ProceduralAudioParams& params);
    void GenerateProceduralAudio(int sourceId, float duration);
    
    // MIDI support
    bool LoadMIDIFile(const std::string& filename, const std::string& midiName);
    void PlayMIDI(const std::string& midiName);
    void StopMIDI(const std::string& midiName);
    void SetMIDIInstrument(int channel, int program);
    void SendMIDIMessage(uint8_t status, uint8_t data1, uint8_t data2);
    
    // Real-time analysis
    void EnableAudioAnalysis(bool enable);
    std::vector<float> GetFrequencySpectrum(int sourceId, int binCount = 256);
    float GetRMSLevel(int sourceId);
    float GetPeakLevel(int sourceId);
    void SetAudioVisualizationCallback(std::function<void(const std::vector<float>&)> callback);
    
    // Platform-specific features
    void InitializeXboxAudio(void* xboxContext);
    void InitializePlayStationAudio(void* psContext);
    void InitializeNintendoSwitchAudio(void* switchContext);
    void EnableSpatialAudioPlatformAPIs(bool enable);
    
    // Console certification features
    void HandleDeviceRemoval();
    void HandleDeviceInsertion();
    void SetAudioSessionCategory(const std::string& category);
    void EnableBackgroundAudio(bool enable);
    
    // Performance optimization
    void SetMaxSimultaneousSources(int maxSources);
    void EnableAudioCulling(bool enable, float cullDistance);
    void SetAudioLOD(int sourceId, float distance);
    void OptimizeMemoryUsage();
    
    // Update
    void Update(float deltaTime);
    void ProcessAudioGraph();
    
    // Event system
    void SetAudioEventCallback(AudioEventCallback callback);
    void TriggerAudioEvent(const AudioEvent& event);
    
    // Debug and visualization
    void EnableDebugVisualization(bool enable);
    void RenderAudioDebug(Camera* camera);
    void LogAudioStatistics();
    
    // Utilities
    float ConvertDecibelToLinear(float decibel);
    float ConvertLinearToDecibel(float linear);
    void NormalizeAudioData(std::vector<float>& audioData);
    
    // Machine learning enhancement
    void EnableAIAudioEnhancement(bool enable);
    void SetAINoiseSuppression(bool enable);
    void SetAIAudioUpscaling(bool enable);
    void TrainAudioModel(const std::vector<std::string>& trainingClips);

private:
    // XAudio2 core
    IXAudio2* xAudio2_;
    IXAudio2MasteringVoice* masteringVoice_;
    IXAudio2SubmixVoice* submixVoice_;
    X3DAUDIO_HANDLE x3dAudio_;
    
    // Audio data
    std::map<std::string, std::unique_ptr<AudioClip>> audioClips_;
    std::map<int, std::unique_ptr<AudioSource>> audioSources_;
    std::map<int, std::unique_ptr<AudioEffect>> audioEffects_;
    
    AudioListener listener_;
    AudioSettings settings_;
    ReverbSettings reverbSettings_;
    
    int nextSourceId_;
    int nextEffectId_;
    bool initialized_;
    
    // Threading
    std::thread audioThread_;
    std::atomic<bool> audioThreadRunning_;
    std::mutex audioMutex_;
    std::queue<std::function<void()>> audioCommands_;
    
    // Streaming
    std::thread streamingThread_;
    std::atomic<bool> streamingThreadRunning_;
    std::mutex streamingMutex_;
    
    // Platform context
    void* platformContext_;
    
    // Callbacks
    AudioEventCallback eventCallback_;
    std::function<void(const std::vector<float>&)> visualizationCallback_;
    
    // Internal methods
    bool InitializeXAudio2();
    bool InitializeX3DAudio();
    void InitializeEffects();
    void CreateSubmixVoices();
    
    void AudioThreadMain();
    void StreamingThreadMain();
    void ProcessAudioCommands();
    
    void UpdateSource3D(AudioSource* source);
    void UpdateSourceEffects(AudioSource* source);
    void CalculateDistanceAttenuation(AudioSource* source, float distance);
    void CalculateOcclusionObstruction(AudioSource* source);
    
    bool LoadWAVFile(const std::string& filename, AudioClip& clip);
    bool LoadOGGFile(const std::string& filename, AudioClip& clip);
    bool LoadMP3File(const std::string& filename, AudioClip& clip);
    bool LoadAACFile(const std::string& filename, AudioClip& clip);
    bool LoadXMA2File(const std::string& filename, AudioClip& clip);
    bool LoadAT9File(const std::string& filename, AudioClip& clip);
    bool LoadOPUSFile(const std::string& filename, AudioClip& clip);
    
    void UnloadAudioClipData(AudioClip& clip);
    void UpdateAudioSourceBuffer(AudioSource* source);
    void UpdateAudioListener();
    void UpdateSpatialAudio();
    void Update3DCalculations();
};

} // namespace Nexus