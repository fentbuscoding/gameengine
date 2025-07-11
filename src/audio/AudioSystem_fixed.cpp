#include "AudioSystem.h"
#include "Logger.h"
#include <algorithm>

namespace Nexus {

// Simple sound instance struct for compatibility
struct SoundInstance {
    SoundInstanceID id;
    SoundID soundId;
    IXAudio2SourceVoice* sourceVoice;
    float volume;
    float pitch;
    bool looping;
    bool isPlaying;
    bool is3D;
    DirectX::XMFLOAT3 position;
    DirectX::XMFLOAT3 velocity;
};

AudioSystem::AudioSystem() 
    : xaudio2_(nullptr), xAudio2_(nullptr), masteringVoice_(nullptr), masterVoice_(nullptr),
      nextSoundId_(1), nextSoundInstanceId_(1), masterVolume_(1.0f),
      listenerPosition_(0.0f, 0.0f, 0.0f), listenerVelocity_(0.0f, 0.0f, 0.0f),
      listenerOrientation_(0.0f, 0.0f, 1.0f), listenerUpVector_(0.0f, 1.0f, 0.0f),
      dopplerEnabled_(true), reverbEnabled_(false) {
}

AudioSystem::~AudioSystem() {
    Shutdown();
}

bool AudioSystem::Initialize() {
    // Initialize XAudio2
    HRESULT hr = XAudio2Create(&xaudio2_, 0, XAUDIO2_DEFAULT_PROCESSOR);
    if (FAILED(hr)) {
        Logger::Error("Failed to initialize XAudio2");
        return false;
    }
    
    // Set compatibility alias
    xAudio2_ = xaudio2_;
    
    // Create master voice
    hr = xaudio2_->CreateMasteringVoice(&masteringVoice_);
    if (FAILED(hr)) {
        Logger::Error("Failed to create mastering voice");
        return false;
    }
    
    // Set compatibility alias
    masterVoice_ = masteringVoice_;
    
    // Initialize 3D audio
    if (!Initialize3DAudio()) {
        Logger::Warning("Failed to initialize 3D audio - 3D features will be disabled");
    }
    
    Logger::Info("Audio system initialized successfully");
    return true;
}

bool AudioSystem::Initialize3DAudio() {
    // Initialize X3DAudio
    DWORD channelMask;
    XAUDIO2_DEVICE_DETAILS deviceDetails;
    
    HRESULT hr = xaudio2_->GetDeviceDetails(0, &deviceDetails);
    if (FAILED(hr)) {
        return false;
    }
    
    channelMask = deviceDetails.OutputFormat.dwChannelMask;
    
    // Initialize X3DAudio
    X3DAudioInitialize(channelMask, X3DAUDIO_SPEED_OF_SOUND, x3dAudioHandle_);
    return true;
}

void AudioSystem::Shutdown() {
    // Clean up sound instances
    for (auto& [id, instance] : soundInstances_) {
        if (instance.find("sourceVoice") != instance.npos) {
            // This is a placeholder - in a real implementation, we'd properly clean up
        }
    }
    soundInstances_.clear();
    
    // Clean up sounds
    sounds_.clear();
    
    // Clean up XAudio2
    if (masteringVoice_) {
        masteringVoice_->DestroyVoice();
        masteringVoice_ = nullptr;
        masterVoice_ = nullptr;
    }
    
    if (xaudio2_) {
        xaudio2_->Release();
        xaudio2_ = nullptr;
        xAudio2_ = nullptr;
    }
    
    Logger::Info("Audio system shut down");
}

void AudioSystem::Update(float deltaTime) {
    // Update 3D audio calculations
    Update3DAudio();
    
    // Update streaming sounds
    UpdateStreamingSounds();
    
    // Clean up finished sounds
    CleanupFinishedSounds();
}

void AudioSystem::Update3DAudio() {
    // Update 3D audio calculations
    // This is a placeholder implementation
}

void AudioSystem::UpdateStreamingSounds() {
    // Update streaming sound buffers
    // This is a placeholder implementation
}

void AudioSystem::CleanupFinishedSounds() {
    // Clean up finished sound instances
    // This is a placeholder implementation
}

SoundID AudioSystem::LoadSound(const std::string& filename) {
    // Check if already loaded
    for (const auto& [id, soundPath] : sounds_) {
        if (soundPath == filename) {
            return id;
        }
    }
    
    // Create new sound ID
    SoundID soundId = nextSoundId_++;
    sounds_[soundId] = filename;
    
    // TODO: Load actual audio data
    Logger::Info("Loaded sound: " + filename);
    return soundId;
}

void AudioSystem::UnloadSound(SoundID soundId) {
    auto it = sounds_.find(soundId);
    if (it != sounds_.end()) {
        Logger::Info("Unloaded sound: " + it->second);
        sounds_.erase(it);
    }
}

SoundInstanceID AudioSystem::PlaySound(SoundID soundId, float volume, float pitch, bool looping) {
    auto it = sounds_.find(soundId);
    if (it == sounds_.end()) {
        Logger::Error("Sound not found: " + std::to_string(soundId));
        return 0;
    }
    
    // Create sound instance
    SoundInstanceID instanceId = nextSoundInstanceId_++;
    soundInstances_[instanceId] = "playing:" + it->second;
    
    // TODO: Create actual XAudio2 source voice and play
    
    return instanceId;
}

SoundInstanceID AudioSystem::PlaySound3D(SoundID soundId, const DirectX::XMFLOAT3& position, 
                                        float volume, float pitch, bool looping) {
    auto it = sounds_.find(soundId);
    if (it == sounds_.end()) {
        Logger::Error("Sound not found: " + std::to_string(soundId));
        return 0;
    }
    
    // Create 3D sound instance
    SoundInstanceID instanceId = nextSoundInstanceId_++;
    soundInstances_[instanceId] = "playing3d:" + it->second;
    
    // TODO: Create actual XAudio2 source voice with 3D positioning
    
    return instanceId;
}

void AudioSystem::StopSound(SoundInstanceID instanceId) {
    auto it = soundInstances_.find(instanceId);
    if (it != soundInstances_.end()) {
        // TODO: Stop the actual sound
        soundInstances_.erase(it);
    }
}

void AudioSystem::StopAllSounds() {
    // TODO: Stop all actual sounds
    soundInstances_.clear();
}

void AudioSystem::PauseSound(SoundInstanceID instanceId) {
    auto it = soundInstances_.find(instanceId);
    if (it != soundInstances_.end()) {
        // TODO: Pause the actual sound
    }
}

void AudioSystem::ResumeSound(SoundInstanceID instanceId) {
    auto it = soundInstances_.find(instanceId);
    if (it != soundInstances_.end()) {
        // TODO: Resume the actual sound
    }
}

void AudioSystem::SetSoundVolume(SoundInstanceID instanceId, float volume) {
    auto it = soundInstances_.find(instanceId);
    if (it != soundInstances_.end()) {
        // TODO: Set volume on actual sound
    }
}

void AudioSystem::SetSoundPitch(SoundInstanceID instanceId, float pitch) {
    auto it = soundInstances_.find(instanceId);
    if (it != soundInstances_.end()) {
        // TODO: Set pitch on actual sound
    }
}

void AudioSystem::SetSoundPosition(SoundInstanceID instanceId, const DirectX::XMFLOAT3& position) {
    auto it = soundInstances_.find(instanceId);
    if (it != soundInstances_.end()) {
        // TODO: Set 3D position on actual sound
    }
}

void AudioSystem::SetSoundVelocity(SoundInstanceID instanceId, const DirectX::XMFLOAT3& velocity) {
    auto it = soundInstances_.find(instanceId);
    if (it != soundInstances_.end()) {
        // TODO: Set 3D velocity on actual sound
    }
}

void AudioSystem::SetMasterVolume(float volume) {
    masterVolume_ = std::max(0.0f, std::min(1.0f, volume));
    if (masteringVoice_) {
        masteringVoice_->SetVolume(masterVolume_);
    }
}

float AudioSystem::GetMasterVolume() const {
    return masterVolume_;
}

void AudioSystem::SetListenerPosition(const DirectX::XMFLOAT3& position) {
    listenerPosition_ = position;
}

void AudioSystem::SetListenerVelocity(const DirectX::XMFLOAT3& velocity) {
    listenerVelocity_ = velocity;
}

void AudioSystem::SetListenerOrientation(const DirectX::XMFLOAT3& forward, const DirectX::XMFLOAT3& up) {
    listenerOrientation_ = forward;
    listenerUpVector_ = up;
}

void AudioSystem::EnableDoppler(bool enable) {
    dopplerEnabled_ = enable;
}

void AudioSystem::EnableReverb(bool enable) {
    reverbEnabled_ = enable;
}

void AudioSystem::ApplyReverb(SoundInstanceID instanceId, const ReverbSettings& settings) {
    auto it = soundInstances_.find(instanceId);
    if (it != soundInstances_.end()) {
        // TODO: Apply reverb effect
    }
}

void AudioSystem::ApplyEcho(SoundInstanceID instanceId, const EchoSettings& settings) {
    auto it = soundInstances_.find(instanceId);
    if (it != soundInstances_.end()) {
        // TODO: Apply echo effect
    }
}

IXAudio2SourceVoice* AudioSystem::CreateSourceVoice() {
    // TODO: Create and return a proper source voice
    return nullptr;
}

// Placeholder methods for compatibility
float AudioSystem::LinearToDecibel(float linear) const {
    return 20.0f * log10f(linear);
}

float AudioSystem::DecibelToLinear(float decibel) const {
    return powf(10.0f, decibel / 20.0f);
}

} // namespace Nexus
