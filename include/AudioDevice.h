#pragma once

#include <memory>
#include <string>

namespace Nexus {

/**
 * Audio system for playing sounds and music
 */
class AudioDevice {
public:
    AudioDevice();
    ~AudioDevice();

    // Initialization
    bool Initialize();
    void Shutdown();
    void Update();

    // Audio control
    void SetMasterVolume(float volume);
    float GetMasterVolume() const { return masterVolume_; }

    // Sound effects
    bool LoadSound(const std::string& name, const std::string& filename);
    void PlaySound(const std::string& name, float volume = 1.0f);
    void StopSound(const std::string& name);

    // Music
    bool LoadMusic(const std::string& filename);
    void PlayMusic(bool loop = true);
    void StopMusic();
    void SetMusicVolume(float volume);

private:
    bool initialized_;
    float masterVolume_;
    float musicVolume_;
};

} // namespace Nexus
