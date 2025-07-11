#pragma once

#include <DirectXMath.h>
#include <memory>
#include <vector>
#include <string>
#include <unordered_map>
#include <functional>

#ifdef NEXUS_FMOD_ENABLED
#include <fmod.hpp>
#include <fmod_studio.hpp>
#endif

#ifdef NEXUS_WWISE_ENABLED
#include <AK/SoundEngine/Common/AkSoundEngine.h>
#endif

using namespace DirectX;

namespace Nexus {

/**
 * Next-Generation Audio Engine with 3D spatial audio, HDR audio, and real-time effects
 */
class NextGenAudioEngine {
public:
    enum class AudioBackend {
        DirectSound,
        XAudio2,
        FMOD,
        Wwise,
        Custom
    };

    enum class AudioFormat {
        PCM_16,
        PCM_24,
        PCM_32,
        Float32,
        Compressed_OGG,
        Compressed_MP3,
        Compressed_FLAC,
        Procedural
    };

    enum class SpatialAudioTechnique {
        Stereo,
        Surround5_1,
        Surround7_1,
        Binaural,
        Ambisonics,
        ObjectBased,
        WaveFieldSynthesis
    };

    struct AudioSettings {
        AudioBackend backend = AudioBackend::XAudio2;
        int sampleRate = 48000;
        int bufferSize = 512;
        int channels = 8;
        AudioFormat format = AudioFormat::Float32;
        bool enableHDR = true;
        bool enable3DSpatial = true;
        SpatialAudioTechnique spatialTechnique = SpatialAudioTechnique::Ambisonics;
        bool enableConvolutionReverb = true;
        bool enableRealTimeEffects = true;
        bool enableVoiceChat = false;
        bool enableCompressionStreaming = true;
        float masterVolume = 1.0f;
        int maxVoices = 256;
        bool enableMultithreading = true;
        int workerThreads = 2;
    };

    struct AudioSource {
        std::string name;
        XMFLOAT3 position = XMFLOAT3(0, 0, 0);
        XMFLOAT3 velocity = XMFLOAT3(0, 0, 0);
        XMFLOAT3 direction = XMFLOAT3(0, 0, 1);
        float volume = 1.0f;
        float pitch = 1.0f;
        float minDistance = 1.0f;
        float maxDistance = 100.0f;
        float dopplerFactor = 1.0f;
        bool is3D = true;
        bool looping = false;
        bool streaming = false;
        uint32_t audioClipId = 0;
        std::vector<uint32_t> effectIds;
    };

    struct AudioListener {
        XMFLOAT3 position = XMFLOAT3(0, 0, 0);
        XMFLOAT3 velocity = XMFLOAT3(0, 0, 0);
        XMFLOAT3 forward = XMFLOAT3(0, 0, 1);
        XMFLOAT3 up = XMFLOAT3(0, 1, 0);
        float volume = 1.0f;
    };

    struct AudioEffect {
        enum Type {
            Reverb,
            Echo,
            Distortion,
            Chorus,
            Flanger,
            LowPass,
            HighPass,
            BandPass,
            Equalizer,
            Compressor,
            Limiter,
            ConvolutionReverb,
            Spatialization,
            VoiceProcessing
        };
        
        Type type;
        std::string name;
        std::unordered_map<std::string, float> parameters;
        bool enabled = true;
    };

    struct AudioBus {
        std::string name;
        float volume = 1.0f;
        bool muted = false;
        std::vector<uint32_t> effectIds;
        std::vector<uint32_t> childBuses;
        uint32_t parentBus = 0;
    };

public:
    NextGenAudioEngine();
    ~NextGenAudioEngine();

    // Initialization
    bool Initialize(const AudioSettings& settings = AudioSettings{});
    void Shutdown();

    // Settings
    void SetSettings(const AudioSettings& settings);
    const AudioSettings& GetSettings() const { return settings_; }

    // Audio Clips
    uint32_t LoadAudioClip(const std::string& filePath);
    uint32_t CreateProceduralClip(std::function<float(float)> generator, float duration);
    void UnloadAudioClip(uint32_t clipId);

    // Audio Sources
    uint32_t CreateAudioSource(const AudioSource& source);
    void DestroyAudioSource(uint32_t sourceId);
    void PlayAudioSource(uint32_t sourceId);
    void StopAudioSource(uint32_t sourceId);
    void PauseAudioSource(uint32_t sourceId);
    void SetSourcePosition(uint32_t sourceId, const XMFLOAT3& position);
    void SetSourceVelocity(uint32_t sourceId, const XMFLOAT3& velocity);
    void SetSourceVolume(uint32_t sourceId, float volume);
    void SetSourcePitch(uint32_t sourceId, float pitch);

    // Listener
    void SetListener(const AudioListener& listener);
    AudioListener GetListener() const { return listener_; }

    // 3D Spatial Audio
    void EnableHRTF(bool enable);
    void SetRoomImpulseResponse(const std::string& impulseResponseFile);
    void UpdateAcousticEnvironment(float reverbTime, float absorption, float scattering);
    void SetAmbisonicsOrder(int order);
    
    // Effects
    uint32_t CreateAudioEffect(const AudioEffect& effect);
    void DestroyAudioEffect(uint32_t effectId);
    void AttachEffectToSource(uint32_t sourceId, uint32_t effectId);
    void DetachEffectFromSource(uint32_t sourceId, uint32_t effectId);
    void SetEffectParameter(uint32_t effectId, const std::string& parameter, float value);

    // Audio Buses
    uint32_t CreateAudioBus(const AudioBus& bus);
    void DestroyAudioBus(uint32_t busId);
    void SetBusVolume(uint32_t busId, float volume);
    void MuteBus(uint32_t busId, bool mute);
    void AttachSourceToBus(uint32_t sourceId, uint32_t busId);

    // Streaming
    void EnableStreaming(uint32_t sourceId, bool enable);
    void SetStreamingBufferSize(int bufferSize);
    void PreloadStreamingData(uint32_t sourceId);

    // Voice Chat
    void EnableVoiceChat(bool enable);
    void SetVoiceChatCodec(const std::string& codec);
    void StartVoiceRecording();
    void StopVoiceRecording();
    void TransmitVoiceData(const std::vector<uint8_t>& data);

    // HDR Audio
    void EnableHDRAudio(bool enable);
    void SetDynamicRange(float range);
    void SetLoudnessNormalization(bool enable, float targetLUFS = -23.0f);

    // Real-time Processing
    void SetRealtimeCallback(std::function<void(float*, int, int)> callback);
    void ProcessAudioBuffer(float* buffer, int frames, int channels);

    // Environmental Audio
    void SetWeatherAudio(float windStrength, float rainIntensity, float thunderProbability);
    void SetEnvironmentalAmbient(const std::string& environmentType);
    void UpdateTimeOfDayAudio(float timeOfDay);

    // Music System
    void PlayMusic(const std::string& trackName, float fadeInTime = 1.0f);
    void StopMusic(float fadeOutTime = 1.0f);
    void SetMusicVolume(float volume);
    void QueueMusicTrack(const std::string& trackName);
    void EnableAdaptiveMusic(bool enable);
    void SetMusicIntensity(float intensity);

    // Update
    void Update(float deltaTime);

    // Debug and Analysis
    void EnableDebugVisualization(bool enable) { debugVisualizationEnabled_ = enable; }
    AudioStats GetAudioStats() const { return stats_; }
    std::vector<float> GetSpectrumData(int numBands = 64);
    float GetRMSLevel();
    float GetPeakLevel();

    // Console Support
    void InitializeConsoleAudio(); // PlayStation, Xbox specific initialization
    void SetSurroundSoundMode(SpatialAudioTechnique mode);
    void EnableHapticFeedback(bool enable);

private:
    // Backend-specific initialization
    bool InitializeDirectSound();
    bool InitializeXAudio2();
    bool InitializeFMOD();
    bool InitializeWwise();

    // 3D Audio processing
    void UpdateSpatialAudio();
    void CalculateHRTF(const XMFLOAT3& sourcePos, const XMFLOAT3& listenerPos);
    void ProcessAmbisonics();
    void UpdateConvolutionReverb();

    // Effect processing
    void ProcessRealtimeEffects();
    void ApplyEffect(uint32_t effectId, float* buffer, int frames);

    // Streaming management
    void UpdateStreamingSources();
    void LoadStreamingBuffer(uint32_t sourceId);

    AudioSettings settings_;
    AudioListener listener_;
    
    // Audio data
    std::unordered_map<uint32_t, void*> audioClips_;
    std::unordered_map<uint32_t, AudioSource> audioSources_;
    std::unordered_map<uint32_t, AudioEffect> audioEffects_;
    std::unordered_map<uint32_t, AudioBus> audioBuses_;
    
    uint32_t nextId_;
    
    // Backend-specific data
#ifdef NEXUS_FMOD_ENABLED
    FMOD::Studio::System* fmodStudioSystem_;
    FMOD::System* fmodSystem_;
#endif

#ifdef NEXUS_WWISE_ENABLED
    // Wwise integration data
#endif

    void* xaudio2Engine_;
    void* directSoundDevice_;
    
    // Performance tracking
    AudioStats stats_;
    bool debugVisualizationEnabled_;
    
    // Threading
    bool multithreadingEnabled_;
    std::vector<std::thread> workerThreads_;
    
    // Realtime processing
    std::function<void(float*, int, int)> realtimeCallback_;
    std::function<float(float)> proceduralGenerator_;
    
    // Music system
    std::string currentTrack_;
    std::queue<std::string> musicQueue_;
    float musicIntensity_;
    bool adaptiveMusicEnabled_;
};

} // namespace Nexus