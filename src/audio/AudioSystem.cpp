#include "AudioSystem.h"
#include "Logger.h"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <memory>

namespace Nexus {

// Constructor
AudioSystem::AudioSystem()
    : xaudio2_(nullptr)
    , xAudio2_(nullptr)
    , masteringVoice_(nullptr)
    , masterVoice_(nullptr)
    , nextSoundId_(1)
    , nextSoundInstanceId_(1)
    , masterVolume_(1.0f)
    , listenerPosition_(0.0f, 0.0f, 0.0f)
    , listenerVelocity_(0.0f, 0.0f, 0.0f)
    , listenerOrientation_(0.0f, 0.0f, 1.0f)
    , listenerUpVector_(0.0f, 1.0f, 0.0f)
    , dopplerEnabled_(true)
    , reverbEnabled_(true)
    , sampleRate_(44100)
    , channels_(2)
    , bufferSize_(1024)
    , channelLayout_(AudioChannelLayout::Stereo)
    , maxVoices_(64)
    , voiceVirtualizationEnabled_(true)
    , streamingEnabled_(true)
    , occlusionEnabled_(false)
    , profilingEnabled_(false)
    , isRunning_(false)
    , cpuUsage_(0.0f)
    , memoryUsage_(0.0f)
    , activeVoices_(0)
{
    // Initialize advanced structures
    listener_ = std::make_unique<AudioListener>();
    mixer_ = std::make_unique<AudioMixer>();
    streaming_ = std::make_unique<AudioStreaming>();
    occlusion_ = std::make_unique<AudioOcclusion>();
    drc_ = std::make_unique<DynamicRangeCompression>();
    analytics_ = std::make_unique<AudioAnalytics>();
    
    // Initialize listener
    listener_->position = XMFLOAT3(0.0f, 0.0f, 0.0f);
    listener_->velocity = XMFLOAT3(0.0f, 0.0f, 0.0f);
    listener_->forward = XMFLOAT3(0.0f, 0.0f, 1.0f);
    listener_->up = XMFLOAT3(0.0f, 1.0f, 0.0f);
    listener_->masterVolume = 1.0f;
    
    // Initialize mixer
    mixer_->sampleRate = sampleRate_;
    mixer_->channels = channels_;
    mixer_->bufferSize = bufferSize_;
    mixer_->channelLayout = channelLayout_;
    mixer_->masterVolume = 1.0f;
    mixer_->masterPitch = 1.0f;
    mixer_->masterMute = false;
}

// Destructor
AudioSystem::~AudioSystem() {
    Shutdown();
}

// Initialize the audio system
bool AudioSystem::Initialize(int sampleRate, int channels, int bufferSize, AudioChannelLayout layout) {
    sampleRate_ = sampleRate;
    channels_ = channels;
    bufferSize_ = bufferSize;
    channelLayout_ = layout;
    
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
    
    // Initialize X3DAudio
    DWORD channelMask;
    masteringVoice_->GetChannelMask(&channelMask);
    X3DAudioInitialize(channelMask, X3DAUDIO_SPEED_OF_SOUND, x3dAudioHandle_);
    
    isRunning_ = true;
    Logger::Info("Audio system initialized successfully");
    return true;
}

// Shutdown the audio system
void AudioSystem::Shutdown() {
    isRunning_ = false;
    
    // Stop all sounds
    StopAllSounds();
    
    // Clean up audio sources
    audioSources_.clear();
    audioBuffers_.clear();
    audioGroups_.clear();
    audioEffects_.clear();
    audioEnvironments_.clear();
    
    // Clean up legacy maps
    sounds_.clear();
    soundInstances_.clear();
    streamingSounds_.clear();
    
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

// Update the audio system
void AudioSystem::Update(float deltaTime) {
    if (!isRunning_) return;
    
    // Update 3D audio calculations
    Process3DAudio();
    
    // Update streaming sources
    UpdateStreamingSources();
    
    // Process fading
    for (auto& [name, source] : audioSources_) {
        if (source->isFading) {
            ProcessFading(source, deltaTime);
        }
    }
    
    // Update analytics
    if (analytics_) {
        analytics_->Update();
    }
}

// Legacy compatibility methods
AudioSystem::SoundID AudioSystem::LoadSound(const std::string& filePath) {
    // Check if already loaded
    for (const auto& [id, soundPath] : sounds_) {
        if (soundPath == filePath) {
            return id;
        }
    }
    
    // Load the audio file
    auto buffer = LoadAudioFile(filePath);
    if (!buffer) {
        Logger::Error("Failed to load audio file: " + filePath);
        return 0;
    }
    
    // Create new sound ID
    SoundID soundId = nextSoundId_++;
    sounds_[soundId] = filePath;
    
    // Store the buffer with the file path as key
    audioBuffers_[filePath] = buffer;
    
    Logger::Info("Loaded sound: " + filePath);
    return soundId;
}

void AudioSystem::UnloadSound(SoundID soundId) {
    auto it = sounds_.find(soundId);
    if (it != sounds_.end()) {
        // Stop any playing instances of this sound
        for (auto instIt = soundInstances_.begin(); instIt != soundInstances_.end(); ) {
            if (instIt->second == it->second) {
                StopSound(instIt->first);
                instIt = soundInstances_.erase(instIt);
            } else {
                ++instIt;
            }
        }
        
        // Remove the audio buffer
        audioBuffers_.erase(it->second);
        
        sounds_.erase(it);
        Logger::Info("Unloaded sound with ID: " + std::to_string(soundId));
    }
}

AudioSystem::SoundInstanceID AudioSystem::PlaySound(SoundID soundId, float volume, bool looping) {
    auto it = sounds_.find(soundId);
    if (it == sounds_.end()) {
        Logger::Error("Sound ID not found: " + std::to_string(soundId));
        return 0;
    }
    
    // Create sound instance ID
    SoundInstanceID instanceId = nextSoundInstanceId_++;
    soundInstances_[instanceId] = it->second;
    
    // Create audio source for this instance
    auto buffer = audioBuffers_[it->second];
    if (buffer) {
        auto source = CreateAudioSource("instance_" + std::to_string(instanceId), buffer);
        if (source) {
            source->volume = volume;
            source->isLooping = looping;
            source->isPlaying = true;
            CreateSourceVoice(source);
            PlaySound("instance_" + std::to_string(instanceId));
        }
    }
    
    Logger::Info("Playing sound: " + it->second);
    return instanceId;
}

AudioSystem::SoundInstanceID AudioSystem::PlaySound3D(SoundID soundId, const DirectX::XMFLOAT3& position, 
                                                       const DirectX::XMFLOAT3& velocity, float volume, bool looping) {
    auto it = sounds_.find(soundId);
    if (it == sounds_.end()) {
        Logger::Error("Sound ID not found: " + std::to_string(soundId));
        return 0;
    }
    
    // Create sound instance ID
    SoundInstanceID instanceId = nextSoundInstanceId_++;
    soundInstances_[instanceId] = it->second;
    
    // Create audio source for this instance
    auto buffer = audioBuffers_[it->second];
    if (buffer) {
        auto source = CreateAudioSource("instance_" + std::to_string(instanceId), buffer);
        if (source) {
            source->volume = volume;
            source->isLooping = looping;
            source->is3D = true;
            source->position = position;
            source->velocity = velocity;
            source->minDistance = 1.0f;
            source->maxDistance = 100.0f;
            source->isPlaying = true;
            CreateSourceVoice(source);
            PlaySound("instance_" + std::to_string(instanceId));
        }
    }
    
    return instanceId;
}

void AudioSystem::StopSound(SoundInstanceID instanceId) {
    auto it = soundInstances_.find(instanceId);
    if (it != soundInstances_.end()) {
        auto source = GetAudioSource("instance_" + std::to_string(instanceId));
        if (source && source->sourceVoice) {
            source->sourceVoice->Stop();
            source->isPlaying = false;
        }
        soundInstances_.erase(it);
    }
}

void AudioSystem::SetSoundVolume(SoundInstanceID instanceId, float volume) {
    auto it = soundInstances_.find(instanceId);
    if (it != soundInstances_.end()) {
        SetVolume("instance_" + std::to_string(instanceId), volume);
    }
}

void AudioSystem::SetSoundPosition(SoundInstanceID instanceId, const DirectX::XMFLOAT3& position) {
    auto it = soundInstances_.find(instanceId);
    if (it != soundInstances_.end()) {
        SetSourcePosition("instance_" + std::to_string(instanceId), 
                         XMFLOAT3(position.x, position.y, position.z));
    }
}

void AudioSystem::SetSoundVelocity(SoundInstanceID instanceId, const DirectX::XMFLOAT3& velocity) {
    auto it = soundInstances_.find(instanceId);
    if (it != soundInstances_.end()) {
        SetSourceVelocity("instance_" + std::to_string(instanceId), 
                         XMFLOAT3(velocity.x, velocity.y, velocity.z));
    }
}

void AudioSystem::ApplyDistortionEffect(SoundInstanceID instanceId, float amount, float edge) {
    // Legacy method - could be implemented using the new effect system
    Logger::Info("Distortion effect applied to instance: " + std::to_string(instanceId));
}

void AudioSystem::ApplyEcho(SoundInstanceID instanceId, float delay, float feedback) {
    // Legacy method - could be implemented using the new effect system
    Logger::Info("Echo effect applied to instance: " + std::to_string(instanceId));
}

void AudioSystem::StartStreaming(const std::string& filePath, float volume, bool looping) {
    streamingSounds_.push_back(filePath);
    
    // Create streaming source
    auto buffer = std::make_shared<AudioBuffer>();
    buffer->name = filePath;
    buffer->format = AudioFormat::PCM_16;
    buffer->sampleRate = sampleRate_;
    buffer->channels = channels_;
    
    auto source = CreateAudioSource(filePath, buffer, AudioSourceType::Streaming);
    if (source) {
        source->volume = volume;
        source->isLooping = looping;
        PlaySound(filePath);
    }
    
    Logger::Info("Started streaming: " + filePath);
}

void AudioSystem::StopStreaming(const std::string& filePath) {
    auto it = std::find(streamingSounds_.begin(), streamingSounds_.end(), filePath);
    if (it != streamingSounds_.end()) {
        streamingSounds_.erase(it);
        StopSound(filePath);
        Logger::Info("Stopped streaming: " + filePath);
    }
}

// Audio buffer management
std::shared_ptr<AudioSystem::AudioBuffer> AudioSystem::LoadAudioFile(const std::string& filePath) {
    // Check if already loaded
    auto it = audioBuffers_.find(filePath);
    if (it != audioBuffers_.end()) {
        return it->second;
    }
    
    // Try to load the file
    std::shared_ptr<AudioBuffer> buffer;
    
    // Determine file type by extension
    std::string ext = filePath.substr(filePath.find_last_of('.') + 1);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    
    if (ext == "wav") {
        buffer = LoadWAV(filePath);
    } else if (ext == "mp3") {
        buffer = LoadMP3(filePath);
    } else if (ext == "ogg") {
        buffer = LoadOGG(filePath);
    } else {
        Logger::Error("Unsupported audio format: " + ext);
        return nullptr;
    }
    
    if (buffer) {
        audioBuffers_[filePath] = buffer;
    }
    
    return buffer;
}

std::shared_ptr<AudioSystem::AudioBuffer> AudioSystem::CreateAudioBuffer(const std::string& name, 
                                                                         AudioFormat format, int sampleRate, 
                                                                         int channels, const void* data, size_t dataSize) {
    auto buffer = std::make_shared<AudioBuffer>();
    buffer->name = name;
    buffer->format = format;
    buffer->sampleRate = sampleRate;
    buffer->channels = channels;
    buffer->dataSize = dataSize;
    buffer->data.resize(dataSize);
    
    if (data) {
        std::memcpy(buffer->data.data(), data, dataSize);
    }
    
    // Calculate duration
    int bytesPerSample = 2; // Assuming 16-bit PCM
    buffer->duration = static_cast<float>(dataSize) / (sampleRate * channels * bytesPerSample);
    
    audioBuffers_[name] = buffer;
    return buffer;
}

void AudioSystem::UnloadAudioBuffer(const std::string& name) {
    audioBuffers_.erase(name);
}

std::shared_ptr<AudioSystem::AudioBuffer> AudioSystem::GetAudioBuffer(const std::string& name) {
    auto it = audioBuffers_.find(name);
    return (it != audioBuffers_.end()) ? it->second : nullptr;
}

// Audio source management
std::shared_ptr<AudioSystem::AudioSource> AudioSystem::CreateAudioSource(const std::string& name, 
                                                                         std::shared_ptr<AudioBuffer> buffer,
                                                                         AudioSourceType type) {
    auto source = std::make_shared<AudioSource>();
    source->name = name;
    source->buffer = buffer;
    source->type = type;
    source->priority = AudioPriority::Medium;
    source->volume = 1.0f;
    source->pitch = 1.0f;
    source->pan = 0.0f;
    source->isLooping = false;
    source->isPlaying = false;
    source->isPaused = false;
    source->is3D = false;
    source->position = XMFLOAT3(0.0f, 0.0f, 0.0f);
    source->velocity = XMFLOAT3(0.0f, 0.0f, 0.0f);
    source->direction = XMFLOAT3(0.0f, 0.0f, 1.0f);
    source->minDistance = 1.0f;
    source->maxDistance = 100.0f;
    source->sourceVoice = nullptr;
    
    audioSources_[name] = source;
    return source;
}

void AudioSystem::DestroyAudioSource(const std::string& name) {
    auto it = audioSources_.find(name);
    if (it != audioSources_.end()) {
        if (it->second->sourceVoice) {
            it->second->sourceVoice->DestroyVoice();
        }
        audioSources_.erase(it);
    }
}

std::shared_ptr<AudioSystem::AudioSource> AudioSystem::GetAudioSource(const std::string& name) {
    auto it = audioSources_.find(name);
    return (it != audioSources_.end()) ? it->second : nullptr;
}

// Playback control
void AudioSystem::PlaySound(const std::string& sourceName) {
    auto source = GetAudioSource(sourceName);
    if (source && source->buffer) {
        CreateSourceVoice(source);
        source->isPlaying = true;
        if (source->sourceVoice) {
            source->sourceVoice->Start();
        }
    }
}

void AudioSystem::PauseSound(const std::string& sourceName) {
    auto source = GetAudioSource(sourceName);
    if (source && source->sourceVoice) {
        source->sourceVoice->Stop();
        source->isPaused = true;
    }
}

void AudioSystem::StopSound(const std::string& sourceName) {
    auto source = GetAudioSource(sourceName);
    if (source && source->sourceVoice) {
        source->sourceVoice->Stop();
        source->isPlaying = false;
        source->isPaused = false;
    }
}

void AudioSystem::StopAllSounds() {
    for (auto& [name, source] : audioSources_) {
        if (source->sourceVoice) {
            source->sourceVoice->Stop();
        }
        source->isPlaying = false;
        source->isPaused = false;
    }
}

void AudioSystem::SetVolume(const std::string& sourceName, float volume) {
    auto source = GetAudioSource(sourceName);
    if (source) {
        source->volume = std::clamp(volume, 0.0f, 1.0f);
        if (source->sourceVoice) {
            source->sourceVoice->SetVolume(source->volume);
        }
    }
}

void AudioSystem::SetPitch(const std::string& sourceName, float pitch) {
    auto source = GetAudioSource(sourceName);
    if (source) {
        source->pitch = std::clamp(pitch, 0.5f, 2.0f);
        if (source->sourceVoice) {
            source->sourceVoice->SetFrequencyRatio(source->pitch);
        }
    }
}

void AudioSystem::SetLooping(const std::string& sourceName, bool looping) {
    auto source = GetAudioSource(sourceName);
    if (source) {
        source->isLooping = looping;
    }
}

// 3D audio methods
void AudioSystem::SetSource3D(const std::string& sourceName, bool is3D) {
    auto source = GetAudioSource(sourceName);
    if (source) {
        source->is3D = is3D;
    }
}

void AudioSystem::SetSourcePosition(const std::string& sourceName, const XMFLOAT3& position) {
    auto source = GetAudioSource(sourceName);
    if (source) {
        source->position = position;
    }
}

void AudioSystem::SetSourceVelocity(const std::string& sourceName, const XMFLOAT3& velocity) {
    auto source = GetAudioSource(sourceName);
    if (source) {
        source->velocity = velocity;
    }
}

void AudioSystem::SetSourceDirection(const std::string& sourceName, const XMFLOAT3& direction) {
    auto source = GetAudioSource(sourceName);
    if (source) {
        source->direction = direction;
    }
}

void AudioSystem::SetSourceDistance(const std::string& sourceName, float minDistance, float maxDistance) {
    auto source = GetAudioSource(sourceName);
    if (source) {
        source->minDistance = minDistance;
        source->maxDistance = maxDistance;
    }
}

void AudioSystem::SetSourceCone(const std::string& sourceName, float innerAngle, float outerAngle, float outerGain) {
    auto source = GetAudioSource(sourceName);
    if (source) {
        source->coneInnerAngle = innerAngle;
        source->coneOuterAngle = outerAngle;
        source->coneOuterGain = outerGain;
    }
}

// Listener methods
void AudioSystem::SetListenerPosition(const XMFLOAT3& position) {
    if (listener_) {
        listener_->position = position;
    }
    // Update legacy compatibility
    listenerPosition_ = DirectX::XMFLOAT3(position.x, position.y, position.z);
}

void AudioSystem::SetListenerVelocity(const XMFLOAT3& velocity) {
    if (listener_) {
        listener_->velocity = velocity;
    }
    // Update legacy compatibility
    listenerVelocity_ = DirectX::XMFLOAT3(velocity.x, velocity.y, velocity.z);
}

void AudioSystem::SetListenerOrientation(const XMFLOAT3& forward, const XMFLOAT3& up) {
    if (listener_) {
        listener_->forward = forward;
        listener_->up = up;
    }
    // Update legacy compatibility
    listenerOrientation_ = DirectX::XMFLOAT3(forward.x, forward.y, forward.z);
    listenerUpVector_ = DirectX::XMFLOAT3(up.x, up.y, up.z);
}

void AudioSystem::SetMasterVolume(float volume) {
    masterVolume_ = std::clamp(volume, 0.0f, 1.0f);
    if (listener_) {
        listener_->masterVolume = masterVolume_;
    }
    if (masteringVoice_) {
        masteringVoice_->SetVolume(masterVolume_);
    }
}

// Audio groups
std::shared_ptr<AudioSystem::AudioGroup> AudioSystem::CreateAudioGroup(const std::string& name) {
    auto group = std::make_shared<AudioGroup>();
    group->name = name;
    group->volume = 1.0f;
    group->pitch = 1.0f;
    group->isMuted = false;
    group->isSolo = false;
    
    audioGroups_[name] = group;
    return group;
}

void AudioSystem::DestroyAudioGroup(const std::string& name) {
    audioGroups_.erase(name);
}

void AudioSystem::AddSourceToGroup(const std::string& sourceName, const std::string& groupName) {
    auto source = GetAudioSource(sourceName);
    auto group = audioGroups_.find(groupName);
    if (source && group != audioGroups_.end()) {
        group->second->AddSource(source);
    }
}

void AudioSystem::RemoveSourceFromGroup(const std::string& sourceName, const std::string& groupName) {
    auto source = GetAudioSource(sourceName);
    auto group = audioGroups_.find(groupName);
    if (source && group != audioGroups_.end()) {
        group->second->RemoveSource(source);
    }
}

void AudioSystem::SetGroupVolume(const std::string& groupName, float volume) {
    auto group = audioGroups_.find(groupName);
    if (group != audioGroups_.end()) {
        group->second->volume = std::clamp(volume, 0.0f, 1.0f);
    }
}

void AudioSystem::SetGroupMute(const std::string& groupName, bool mute) {
    auto group = audioGroups_.find(groupName);
    if (group != audioGroups_.end()) {
        group->second->isMuted = mute;
    }
}

// Private helper methods
void AudioSystem::CreateSourceVoice(std::shared_ptr<AudioSource> source) {
    if (!source || !source->buffer || !xaudio2_) {
        return;
    }
    
    // Create WAVEFORMATEX structure
    WAVEFORMATEX wfx = {};
    wfx.wFormatTag = WAVE_FORMAT_PCM;
    wfx.nChannels = source->buffer->channels;
    wfx.nSamplesPerSec = source->buffer->sampleRate;
    wfx.wBitsPerSample = 16; // Assuming 16-bit PCM
    wfx.nBlockAlign = wfx.nChannels * wfx.wBitsPerSample / 8;
    wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;
    
    // Create source voice
    HRESULT hr = xaudio2_->CreateSourceVoice(&source->sourceVoice, &wfx);
    if (FAILED(hr)) {
        Logger::Error("Failed to create source voice for: " + source->name);
        return;
    }
    
    // Submit audio buffer
    XAUDIO2_BUFFER buffer = {};
    buffer.AudioBytes = static_cast<UINT32>(source->buffer->dataSize);
    buffer.pAudioData = source->buffer->data.data();
    buffer.Flags = XAUDIO2_END_OF_STREAM;
    if (source->isLooping) {
        buffer.LoopCount = XAUDIO2_LOOP_INFINITE;
    }
    
    hr = source->sourceVoice->SubmitSourceBuffer(&buffer);
    if (FAILED(hr)) {
        Logger::Error("Failed to submit source buffer for: " + source->name);
    }
}

void AudioSystem::Process3DAudio() {
    // Update 3D audio calculations for all 3D sources
    for (auto& [name, source] : audioSources_) {
        if (source->is3D && source->sourceVoice && listener_) {
            // Calculate 3D audio parameters using X3DAudio
            // This is a simplified implementation
            float distance = CalculateDistance(source->position, listener_->position);
            float attenuation = CalculateAttenuation(distance, source->minDistance, source->maxDistance, source->rolloffFactor);
            
            // Apply volume attenuation
            float finalVolume = source->volume * attenuation;
            source->sourceVoice->SetVolume(finalVolume);
            
            // Apply Doppler effect if enabled
            if (dopplerEnabled_) {
                XMFLOAT3 direction;
                direction.x = listener_->position.x - source->position.x;
                direction.y = listener_->position.y - source->position.y;
                direction.z = listener_->position.z - source->position.z;
                
                float dopplerShift = CalculateDopplerShift(source->velocity, listener_->velocity, direction);
                source->sourceVoice->SetFrequencyRatio(source->pitch * dopplerShift);
            }
        }
    }
}

void AudioSystem::UpdateStreamingSources() {
    // Update streaming audio sources
    for (auto& [name, source] : audioSources_) {
        if (source->type == AudioSourceType::Streaming && source->isPlaying) {
            // Handle streaming buffer updates
            // This is a simplified implementation
        }
    }
}

void AudioSystem::ProcessFading(std::shared_ptr<AudioSource> source, float deltaTime) {
    if (!source || !source->isFading) return;
    
    source->fadeVolume += source->fadeSpeed * deltaTime;
    source->fadeVolume = std::clamp(source->fadeVolume, 0.0f, 1.0f);
    
    if (source->sourceVoice) {
        source->sourceVoice->SetVolume(source->volume * source->fadeVolume);
    }
    
    // Check if fade is complete
    if ((source->fadeSpeed > 0 && source->fadeVolume >= 1.0f) ||
        (source->fadeSpeed < 0 && source->fadeVolume <= 0.0f)) {
        source->isFading = false;
        
        if (source->fadeVolume <= 0.0f) {
            source->isPlaying = false;
        }
    }
}

// File format loaders (real implementations)
std::shared_ptr<AudioSystem::AudioBuffer> AudioSystem::LoadWAV(const std::string& filePath) {
    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        Logger::Error("Could not open WAV file: " + filePath);
        return nullptr;
    }
    
    // Read and validate RIFF header
    char riffHeader[4];
    file.read(riffHeader, 4);
    if (std::strncmp(riffHeader, "RIFF", 4) != 0) {
        Logger::Error("Invalid WAV file: missing RIFF header");
        return nullptr;
    }
    
    // Read file size
    uint32_t fileSize;
    file.read(reinterpret_cast<char*>(&fileSize), 4);
    
    // Read WAVE header
    char waveHeader[4];
    file.read(waveHeader, 4);
    if (std::strncmp(waveHeader, "WAVE", 4) != 0) {
        Logger::Error("Invalid WAV file: missing WAVE header");
        return nullptr;
    }
    
    // Find fmt chunk
    char chunkHeader[4];
    uint32_t chunkSize;
    bool foundFmt = false;
    
    while (file.read(chunkHeader, 4) && file.read(reinterpret_cast<char*>(&chunkSize), 4)) {
        if (std::strncmp(chunkHeader, "fmt ", 4) == 0) {
            foundFmt = true;
            break;
        }
        file.seekg(chunkSize, std::ios::cur);
    }
    
    if (!foundFmt) {
        Logger::Error("Invalid WAV file: missing fmt chunk");
        return nullptr;
    }
    
    // Read format data
    uint16_t audioFormat, channels, bitsPerSample;
    uint32_t sampleRate, byteRate;
    uint16_t blockAlign;
    
    file.read(reinterpret_cast<char*>(&audioFormat), 2);
    file.read(reinterpret_cast<char*>(&channels), 2);
    file.read(reinterpret_cast<char*>(&sampleRate), 4);
    file.read(reinterpret_cast<char*>(&byteRate), 4);
    file.read(reinterpret_cast<char*>(&blockAlign), 2);
    file.read(reinterpret_cast<char*>(&bitsPerSample), 2);
    
    // Skip any remaining fmt data
    if (chunkSize > 16) {
        file.seekg(chunkSize - 16, std::ios::cur);
    }
    
    // Validate format
    if (audioFormat != 1) { // PCM
        Logger::Error("Unsupported WAV format: only PCM is supported");
        return nullptr;
    }
    
    // Find data chunk
    bool foundData = false;
    uint32_t dataSize = 0;
    
    while (file.read(chunkHeader, 4) && file.read(reinterpret_cast<char*>(&chunkSize), 4)) {
        if (std::strncmp(chunkHeader, "data", 4) == 0) {
            foundData = true;
            dataSize = chunkSize;
            break;
        }
        file.seekg(chunkSize, std::ios::cur);
    }
    
    if (!foundData) {
        Logger::Error("Invalid WAV file: missing data chunk");
        return nullptr;
    }
    
    // Create audio buffer
    auto buffer = std::make_shared<AudioBuffer>();
    buffer->name = filePath;
    buffer->format = (bitsPerSample == 8) ? AudioFormat::PCM_8 : 
                    (bitsPerSample == 16) ? AudioFormat::PCM_16 : 
                    (bitsPerSample == 24) ? AudioFormat::PCM_24 : AudioFormat::PCM_32;
    buffer->sampleRate = sampleRate;
    buffer->channels = channels;
    buffer->bitsPerSample = bitsPerSample;
    buffer->dataSize = dataSize;
    buffer->data.resize(dataSize);
    
    // Read audio data
    file.read(reinterpret_cast<char*>(buffer->data.data()), dataSize);
    
    // Calculate duration
    buffer->duration = static_cast<float>(dataSize) / (sampleRate * channels * (bitsPerSample / 8));
    
    Logger::Info("Loaded WAV file: " + filePath + " (" + std::to_string(buffer->duration) + "s)");
    return buffer;
}

std::shared_ptr<AudioSystem::AudioBuffer> AudioSystem::LoadMP3(const std::string& filePath) {
    // MP3 loading requires external library like minimp3 or mpg123
    // This is a placeholder implementation that could be extended
    Logger::Warning("MP3 loading not yet implemented: " + filePath);
    Logger::Info("To add MP3 support, integrate a library like minimp3 or mpg123");
    
    // For now, return nullptr to indicate unsupported format
    return nullptr;
}

std::shared_ptr<AudioSystem::AudioBuffer> AudioSystem::LoadOGG(const std::string& filePath) {
    // OGG loading requires external library like libvorbis or stb_vorbis
    // This is a placeholder implementation that could be extended
    Logger::Warning("OGG loading not yet implemented: " + filePath);
    Logger::Info("To add OGG support, integrate a library like libvorbis or stb_vorbis");
    
    // For now, return nullptr to indicate unsupported format
    return nullptr;
}

// Effects implementation
void AudioSystem::ApplyReverb(float* samples, int sampleCount, int channels, const ReverbEffect& effect) {
    if (!samples || sampleCount <= 0 || channels <= 0) return;
    
    // Simple reverb implementation using delay and feedback
    float roomSize = effect.GetParameter("roomSize");
    float damping = effect.GetParameter("damping");
    float wetLevel = effect.GetParameter("wetLevel");
    float dryLevel = effect.GetParameter("dryLevel");
    
    // Simplified reverb processing
    static std::vector<float> delayBuffer(44100); // 1 second delay buffer
    static size_t delayIndex = 0;
    
    for (int i = 0; i < sampleCount * channels; ++i) {
        float input = samples[i];
        
        // Get delayed sample
        float delayed = delayBuffer[delayIndex];
        
        // Apply feedback and damping
        float feedback = delayed * roomSize * damping;
        
        // Store input + feedback in delay buffer
        delayBuffer[delayIndex] = input + feedback;
        delayIndex = (delayIndex + 1) % delayBuffer.size();
        
        // Mix dry and wet signals
        samples[i] = (input * dryLevel) + (delayed * wetLevel);
    }
}

void AudioSystem::ApplyEQ(float* samples, int sampleCount, int channels, const EQEffect& effect) {
    if (!samples || sampleCount <= 0 || channels <= 0) return;
    
    // Simple 3-band EQ implementation
    float lowGain = effect.GetParameter("lowGain");
    float midGain = effect.GetParameter("midGain");
    float highGain = effect.GetParameter("highGain");
    
    // Simplified EQ processing using basic filtering
    static float lowState = 0.0f, midState = 0.0f, highState = 0.0f;
    
    for (int i = 0; i < sampleCount * channels; ++i) {
        float input = samples[i];
        
        // Simple high-pass filter for high frequencies
        float high = input - highState;
        highState += high * 0.1f;
        
        // Simple band-pass filter for mid frequencies
        float mid = high - midState;
        midState += mid * 0.3f;
        
        // Low frequencies are what's left
        float low = input - high - mid;
        
        // Apply gains and mix
        samples[i] = (low * lowGain) + (mid * midGain) + (high * highGain);
    }
}

void AudioSystem::ApplyCompressor(float* samples, int sampleCount, int channels, const CompressorEffect& effect) {
    if (!samples || sampleCount <= 0 || channels <= 0) return;
    
    // Simple compressor implementation
    float threshold = effect.GetParameter("threshold");
    float ratio = effect.GetParameter("ratio");
    float attack = effect.GetParameter("attack");
    float release = effect.GetParameter("release");
    
    static float envelope = 0.0f;
    
    for (int i = 0; i < sampleCount * channels; ++i) {
        float input = std::abs(samples[i]);
        
        // Envelope follower
        if (input > envelope) {
            envelope += (input - envelope) * attack;
        } else {
            envelope += (input - envelope) * release;
        }
        
        // Calculate gain reduction
        float gainReduction = 1.0f;
        if (envelope > threshold) {
            float excess = envelope - threshold;
            gainReduction = threshold + (excess / ratio);
            gainReduction = gainReduction / envelope;
        }
        
        // Apply compression
        samples[i] *= gainReduction;
    }
}

// Utility functions
float AudioSystem::LinearToDecibel(float linear) const {
    return 20.0f * std::log10(std::max(linear, 0.0001f));
}

float AudioSystem::DecibelToLinear(float decibel) const {
    return std::pow(10.0f, decibel / 20.0f);
}

float AudioSystem::CalculateDistance(const XMFLOAT3& pos1, const XMFLOAT3& pos2) const {
    float dx = pos2.x - pos1.x;
    float dy = pos2.y - pos1.y;
    float dz = pos2.z - pos1.z;
    return std::sqrt(dx * dx + dy * dy + dz * dz);
}

float AudioSystem::CalculateAttenuation(float distance, float minDist, float maxDist, float rolloff) const {
    if (distance <= minDist) return 1.0f;
    if (distance >= maxDist) return 0.0f;
    
    float normalizedDistance = (distance - minDist) / (maxDist - minDist);
    return std::pow(1.0f - normalizedDistance, rolloff);
}

float AudioSystem::CalculateDopplerShift(const XMFLOAT3& sourceVel, const XMFLOAT3& listenerVel,
                                        const XMFLOAT3& direction) const {
    const float speedOfSound = 343.0f; // m/s
    
    // Calculate velocity components along the line between source and listener
    float sourceSpeed = sourceVel.x * direction.x + sourceVel.y * direction.y + sourceVel.z * direction.z;
    float listenerSpeed = listenerVel.x * direction.x + listenerVel.y * direction.y + listenerVel.z * direction.z;
    
    // Calculate Doppler shift
    float doppler = (speedOfSound + listenerSpeed) / (speedOfSound + sourceSpeed);
    return std::clamp(doppler, 0.5f, 2.0f);
}

// AudioGroup method implementations
float AudioSystem::AudioGroup::GetFinalVolume() const {
    float finalVolume = volume;
    if (parentGroup) {
        finalVolume *= parentGroup->GetFinalVolume();
    }
    return finalVolume;
}

void AudioSystem::AudioGroup::AddSource(std::shared_ptr<AudioSource> source) {
    auto it = std::find(sources.begin(), sources.end(), source);
    if (it == sources.end()) {
        sources.push_back(source);
    }
}

void AudioSystem::AudioGroup::RemoveSource(std::shared_ptr<AudioSource> source) {
    auto it = std::find(sources.begin(), sources.end(), source);
    if (it != sources.end()) {
        sources.erase(it);
    }
}

// AudioAnalytics method implementations
void AudioSystem::AudioAnalytics::Update() {
    // Update analytics data - simplified implementation
    // In a real implementation, this would gather performance metrics
}

void AudioSystem::AudioAnalytics::LogStatistics() {
    // Log audio statistics - simplified implementation
    // In a real implementation, this would output detailed statistics
}

// AudioEffect method implementations
void AudioSystem::ReverbEffect::Apply(float* samples, int sampleCount, int channels) {
    if (!samples || sampleCount <= 0 || channels <= 0) return;
    
    // Get effect parameters
    float roomSize = GetParameter("roomSize");
    float damping = GetParameter("damping");
    float wetLevel = GetParameter("wetLevel");
    float dryLevel = GetParameter("dryLevel");
    
    // Initialize delay buffers if needed
    if (delayBuffer.size() < 44100) {
        delayBuffer.resize(44100, 0.0f);
        delayIndex = 0;
    }
    
    // Apply reverb processing
    for (int i = 0; i < sampleCount * channels; ++i) {
        float input = samples[i];
        
        // Get delayed sample
        float delayed = delayBuffer[delayIndex];
        
        // Apply feedback and damping
        float feedback = delayed * roomSize * damping;
        
        // Store input + feedback in delay buffer
        delayBuffer[delayIndex] = input + feedback;
        delayIndex = (delayIndex + 1) % delayBuffer.size();
        
        // Mix dry and wet signals
        samples[i] = (input * dryLevel) + (delayed * wetLevel);
    }
}

void AudioSystem::ReverbEffect::SetParameter(const std::string& name, float value) {
    parameters[name] = value;
}

float AudioSystem::ReverbEffect::GetParameter(const std::string& name) const {
    auto it = parameters.find(name);
    return (it != parameters.end()) ? it->second : 0.0f;
}

void AudioSystem::EQEffect::Apply(float* samples, int sampleCount, int channels) {
    if (!samples || sampleCount <= 0 || channels <= 0) return;
    
    // Get effect parameters
    float lowGain = GetParameter("lowGain");
    float midGain = GetParameter("midGain");
    float highGain = GetParameter("highGain");
    float lowFreq = GetParameter("lowFreq");
    float highFreq = GetParameter("highFreq");
    
    // Initialize filter states if needed
    if (filterStates.size() < 3) {
        filterStates.resize(3, 0.0f);
    }
    
    // Simple 3-band EQ implementation
    for (int i = 0; i < sampleCount * channels; ++i) {
        float input = samples[i];
        
        // High-pass filter for high frequencies
        float high = input - filterStates[0];
        filterStates[0] += high * (highFreq / 22050.0f);
        
        // Band-pass filter for mid frequencies
        float mid = high - filterStates[1];
        filterStates[1] += mid * 0.3f;
        
        // Low frequencies are what's left
        float low = input - high - mid;
        
        // Apply gains and mix
        samples[i] = (low * lowGain) + (mid * midGain) + (high * highGain);
    }
}

void AudioSystem::EQEffect::SetParameter(const std::string& name, float value) {
    parameters[name] = value;
}

float AudioSystem::EQEffect::GetParameter(const std::string& name) const {
    auto it = parameters.find(name);
    return (it != parameters.end()) ? it->second : 0.0f;
}

void AudioSystem::CompressorEffect::Apply(float* samples, int sampleCount, int channels) {
    if (!samples || sampleCount <= 0 || channels <= 0) return;
    
    // Get effect parameters
    float threshold = GetParameter("threshold");
    float ratio = GetParameter("ratio");
    float attack = GetParameter("attack");
    float release = GetParameter("release");
    float makeupGain = GetParameter("makeupGain");
    
    // Initialize envelope follower if needed
    if (envelope == 0.0f) {
        envelope = 0.0001f;
    }
    
    // Apply compression
    for (int i = 0; i < sampleCount * channels; ++i) {
        float input = std::abs(samples[i]);
        
        // Envelope follower
        if (input > envelope) {
            envelope += (input - envelope) * attack;
        } else {
            envelope += (input - envelope) * release;
        }
        
        // Calculate gain reduction
        float gainReduction = 1.0f;
        if (envelope > threshold) {
            float excess = envelope - threshold;
            float compressedExcess = excess / ratio;
            gainReduction = (threshold + compressedExcess) / envelope;
        }
        
        // Apply compression and makeup gain
        samples[i] *= gainReduction * makeupGain;
    }
}

void AudioSystem::CompressorEffect::SetParameter(const std::string& name, float value) {
    parameters[name] = value;
}

float AudioSystem::CompressorEffect::GetParameter(const std::string& name) const {
    auto it = parameters.find(name);
    return (it != parameters.end()) ? it->second : 0.0f;
}

} // namespace Nexus
